// CRenderSurface_OGLES_Texture_FBO.cpp - targetable texture rendered into through a framebuffer object.
#include "CRenderSurface_OGLES_Texture_FBO.h"
#include "CGraphics.h"
#include "CStringToKey.h"
#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>

namespace com { namespace glu { namespace platform { namespace graphics {

CRenderSurface_OGLES_Texture_FBO::CRenderSurface_OGLES_Texture_FBO()
{
    m_flags = (m_flags & ~(Flag_Texture | Flag_Software | Flag_Writable)) | Flag_Texture | Flag_Hardware | Flag_Writable;
    // framebuffer object targets share one kind of binding
    m_targetType = core::CStringToKey("Framebuffer", 0);
    m_framebuffer = 0;
    m_depthRenderbuffer = 0;
}

CRenderSurface_OGLES_Texture_FBO::~CRenderSurface_OGLES_Texture_FBO()
{
    Destroy();
}

void CRenderSurface_OGLES_Texture_FBO::Destroy()
{
    CRenderSurface_OGLES_Texture_FBO::DestroyFrameBuffer();
    CRenderSurface_OGLES_Targetable::Destroy();
}

unsigned char CRenderSurface_OGLES_Texture_FBO::Initialize(const InitParam* params)
{
    unsigned char result = CRenderSurface_OGLES_Texture::Initialize(params);
    m_depthFormat = 0;
    for (const InitParam* param = params; param->m_key != InitParam_End; param++) {
        if (param->m_key == InitParam_DepthFormat && param->m_value == RSBFormat_Depth16) {
            m_depthFormat = GL_DEPTH_COMPONENT16_OES;
        }
    }
    if (result == 0 || (result = InitializeFrameBuffer()) == 0) {
        Destroy();
        return 0;
    }
    return result;
}

void CRenderSurface_OGLES_Texture_FBO::DestroyFrameBuffer()
{
    if (m_depthRenderbuffer != 0) {
        glDeleteRenderbuffersOES(1, &m_depthRenderbuffer);
        m_depthRenderbuffer = 0;
    }
    if (m_framebuffer != 0) {
        glDeleteFramebuffersOES(1, &m_framebuffer);
        m_framebuffer = 0;
    }
}

void CRenderSurface_OGLES_Texture_FBO::Bind()
{
    CGraphics* graphics = (CGraphics*)ICGraphics::GetInstance();
    CRenderSurface_OGLES_Targetable* target = (CRenderSurface_OGLES_Targetable*)graphics->m_targetSurface;
    if (this == target) {
        return;
    }
    // a target of another kind has to release its binding first
    if (target != NULL && target->m_targetType != m_targetType) {
        target->RemoveSelfAsTarget();
    }
    BindFrameBuffer();
    SetAsTargetSurface();
}

unsigned char CRenderSurface_OGLES_Texture_FBO::InitializeFrameBuffer()
{
    glGenFramebuffersOES(1, &m_framebuffer);
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, m_framebuffer);
    glFramebufferTexture2DOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_TEXTURE_2D, m_texture, 0);
    if (m_depthFormat != 0) {
        CRenderSurfaceBuffer* buffer = m_buffers[Buffer_Color];
        glGenRenderbuffersOES(1, &m_depthRenderbuffer);
        glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_depthRenderbuffer);
        glRenderbufferStorageOES(GL_RENDERBUFFER_OES, m_depthFormat, buffer->m_width, buffer->m_height);
        glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_DEPTH_ATTACHMENT_OES, GL_RENDERBUFFER_OES,
                                     m_depthRenderbuffer);
    }
    unsigned char complete = glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES) == GL_FRAMEBUFFER_COMPLETE_OES;
    // restore the binding of the current target
    CGraphics* graphics = (CGraphics*)ICGraphics::GetInstance();
    if (m_targetType == ((CRenderSurface_OGLES_Texture_FBO*)graphics->m_targetSurface)->m_targetType) {
        ((CRenderSurface_OGLES_Texture_FBO*)graphics->m_targetSurface)->BindFrameBuffer();
    } else {
        glBindFramebufferOES(GL_FRAMEBUFFER_OES, 0);
    }
    return complete;
}

void CRenderSurface_OGLES_Texture_FBO::BindFrameBuffer()
{
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, m_framebuffer);
}

void CRenderSurface_OGLES_Texture_FBO::RemoveSelfAsTarget()
{
    CGraphics* graphics = (CGraphics*)ICGraphics::GetInstance();
    if (this == graphics->m_targetSurface) {
        glBindFramebufferOES(GL_FRAMEBUFFER_OES, 0);
    }
}

unsigned char CRenderSurface_OGLES_Texture_FBO::SaveStateForFutureRecreation()
{
    // keep the pixels in the color buffer while the context is gone
    int pitch;
    Lock(NULL, pitch, LockType_ReadWrite, NULL);
    DestroyFrameBuffer();
    DestroyTextureObjects();
    return 1;
}

unsigned char CRenderSurface_OGLES_Texture_FBO::Recreate()
{
    Unlock();
    InitializeFrameBuffer();
    return 1;
}

} } } }
