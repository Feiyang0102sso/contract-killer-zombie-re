// CRenderSurface_OGLES_Window_Cocoa.cpp - the window render surface of the iOS device: an EAGL
// context whose color renderbuffer is backed by the CAEAGLLayer of the view.
#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>
#include "CRenderSurface_OGLES_Window_Cocoa.h"
#include "CGraphics_OGLES.h"
#include "CGraphics_OGLES_EAGL.h"
#include "CApplet.h"
#include "ICCore.h"
#include "CStrChar.h"
#include "CRectangle.h"

using namespace com::glu::platform;
using namespace com::glu::platform::graphics;

// color depth (bits, CGraphics 0x228) up to which the window uses 16 bit color
static const unsigned int Window_MaxHighColorBits = 0x10;

CRenderSurface_OGLES_Window_Cocoa::CRenderSurface_OGLES_Window_Cocoa()
{
    m_flags = (m_flags & ~(Flag_Texture | Flag_Targetable | Flag_Software | Flag_Writable)) | Flag_Targetable |
              Flag_Hardware | Flag_Writable;
    m_context = NULL;
    m_framebuffer = 0;
    m_colorRenderbuffer = 0;
    m_depthRenderbuffer = 0;
    m_multisampleFramebuffer = 0;
    m_multisampleColorRenderbuffer = 0;
    m_multisampleDepthRenderbuffer = 0;
    m_framebufferStatus = FramebufferStatus_None;
    m_lockCount = 0;
    m_keepContext = 0;
}

CRenderSurface_OGLES_Window_Cocoa::~CRenderSurface_OGLES_Window_Cocoa()
{
    Destroy();
}

void CRenderSurface_OGLES_Window_Cocoa::Destroy()
{
    DestroyFrameBuffer();
    CRenderSurface_OGLES_Texture::Destroy();
    if (m_context != NULL && !m_keepContext) {
        CGraphics_OGLES* graphics = (CGraphics_OGLES*)ICGraphics::GetInstance();
        graphics->m_proxy->ReleaseContext(m_context);
        m_context = NULL;
    }
    m_keepContext = 0;
}

unsigned char CRenderSurface_OGLES_Window_Cocoa::Initialize(const InitParam* params)
{
    RSBFormat formats[BufferCount];
    formats[Buffer_Color] = RSBFormat_None;
    formats[Buffer_Depth] = RSBFormat_None;
    formats[Buffer_Stencil] = RSBFormat_None;
    if (m_context != NULL) {
        // initialized again while the context lives (it is kept by the next Destroy)
        m_keepContext = 1;
        return 1;
    }
    Destroy();

    unsigned char failed = 0;
    for (const InitParam* param = params; param->m_key != InitParam_End && !failed; param++) {
        switch (param->m_key) {
        case InitParam_Window:
            if ((unsigned char)param->m_value) {
                m_flags |= Flag_Window;
            }
            break;
        case InitParam_Mipmaps:
            // the window has no mipmaps
            if ((unsigned char)param->m_value) {
                failed = 1;
            }
            break;
        case InitParam_ColorFormat:
            m_flags |= Flag_HasColorFormat;
            formats[Buffer_Color] = (RSBFormat)param->m_value;
            break;
        case InitParam_DepthFormat:
            m_flags |= Flag_HasDepthBuffer;
            break;
        case InitParam_StencilFormat:
            m_flags |= Flag_HasStencilBuffer;
            formats[Buffer_Stencil] = (RSBFormat)param->m_value;
            break;
        case InitParam_Multisample: {
            unsigned int multisample;
            switch (param->m_value) {
            case 1:
                multisample = Flag_Multisample1;
                break;
            case 2:
                multisample = Flag_Multisample2;
                break;
            case 3:
                multisample = Flag_Multisample3;
                break;
            default:
                multisample = 0;
                break;
            }
            m_flags = (m_flags & ~Flag_MultisampleMask) | multisample;
            break;
        }
        case InitParam_MagFilter:
            if (param->m_value != Filter_Linear && param->m_value != Filter_Nearest) {
                failed = 1;
            } else {
                m_magFilter = param->m_value;
            }
            break;
        case InitParam_MinFilter:
            m_minFilter = param->m_value;
            break;
        case InitParam_WrapS:
            m_wrapS = param->m_value;
            break;
        case InitParam_WrapT:
            m_wrapT = param->m_value;
            break;
        }
    }
    // the window always has a 16 bit depth buffer
    m_flags |= Flag_HasDepthBuffer;
    formats[Buffer_Depth] = RSBFormat_Depth16;

    if (!failed && (m_flags & (Flag_HasColorFormat | Flag_Window)) == (Flag_HasColorFormat | Flag_Window)) {
        // keep the bindings of the caller (the original reads the renderbuffer binding twice)
        GLint previousFramebuffer = 0;
        GLint previousRenderbuffer = 0;
        glGetIntegerv(GL_RENDERBUFFER_BINDING_OES, &previousFramebuffer);
        glGetIntegerv(GL_RENDERBUFFER_BINDING_OES, &previousRenderbuffer);
        CGraphics_OGLES* graphics = (CGraphics_OGLES*)ICGraphics::GetInstance();
        RSBFormat contextFormat = RSBFormat_A8B8G8R8;
        if (graphics->m_width <= Window_MaxHighColorBits) {
            contextFormat = RSBFormat_R5G6B5;
        }
        m_context = graphics->m_proxy->CreateContext((void*)contextFormat, NULL);
        graphics->m_proxy->SetCurrentContext(m_context, NULL, NULL);
        InitializeFrameBuffer();
        if (m_framebufferStatus == FramebufferStatus_StorageFailed ||
            m_framebufferStatus == FramebufferStatus_Incomplete) {
            Destroy();
            glBindRenderbufferOES(GL_RENDERBUFFER_OES, previousRenderbuffer);
            glBindFramebufferOES(GL_FRAMEBUFFER_OES, previousFramebuffer);
            return 0;
        }
        formats[Buffer_Color] = RSBFormat_A8B8G8R8;
        if (graphics->m_width <= Window_MaxHighColorBits) {
            formats[Buffer_Color] = RSBFormat_R5G6B5;
        }
        m_flags |= Flag_Format10 | Flag_Format20;
        formats[Buffer_Stencil] = RSBFormat_None;
        formats[Buffer_Depth] = RSBFormat_None;
        // the buffers only describe the renderbuffers (no memory)
        CreateBuffers(formats, CApplet::m_UIScreenMainScreenWidth, CApplet::m_UIScreenMainScreenHeight, NULL, NULL,
                      CRenderSurfaceBuffer::PitchCondition_Given, 1, (unsigned char*)1, 1);
        m_flags |= Flag_Initialized;
        m_lockCount = 0;
        return 1;
    }
    Destroy();
    return 0;
}

void CRenderSurface_OGLES_Window_Cocoa::Lock(unsigned char** data, int& pitch, LockType lockType,
                                             const components::CRectangle<short>* area)
{
    if (m_flags & Flag_LockMask) {
        if (FetchFromBuffer(Buffer_Color, data, pitch, area)) {
            m_lockCount++;
            m_flags |= Flag_LockMask;
        }
    }
}

void CRenderSurface_OGLES_Window_Cocoa::Unlock()
{
    if (m_flags & Flag_LockMask) {
        m_lockCount--;
        if (m_lockCount == 0) {
            // back to a buffer without memory
            CRenderSurfaceBuffer* buffer = m_buffers[Buffer_Color];
            buffer->Initialize(buffer->m_format, buffer->m_width, buffer->m_height, 0, NULL, NULL,
                               CRenderSurfaceBuffer::PitchCondition_Given, 1, (void*)1);
            m_flags &= ~Flag_LockMask;
        }
    }
}

unsigned char CRenderSurface_OGLES_Window_Cocoa::GetWidthAndHeight(unsigned int& width, unsigned int& height)
{
    if (m_buffers[Buffer_Color] != NULL) {
        width = m_buffers[Buffer_Color]->m_width;
        height = m_buffers[Buffer_Color]->m_height;
        framework::ICCore* core = CApplet::m_pCore;
        if (core->m_deviceOrientationSet &&
            (core->m_deviceOrientation == framework::ICCore::DeviceOrientation_LandscapeRight ||
             core->m_deviceOrientation == framework::ICCore::DeviceOrientation_LandscapeLeft)) {
            // landscape: swap width and height
            height ^= width;
            width ^= height;
            height ^= width;
        }
        return 1;
    }
    width = 0;
    height = 0;
    return 0;
}

unsigned char CRenderSurface_OGLES_Window_Cocoa::InitializeFrameBuffer()
{
    CGraphics_OGLES* graphics = (CGraphics_OGLES*)ICGraphics::GetInstance();
    glGenRenderbuffersOES(1, &m_colorRenderbuffer);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_colorRenderbuffer);
    if (!((CGraphics_OGLES_EAGL*)graphics->m_proxy)->RenderBufferStorage(m_context)) {
        m_framebufferStatus = FramebufferStatus_StorageFailed;
        return 0;
    }
    glGenFramebuffersOES(1, &m_framebuffer);
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, m_framebuffer);
    glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_RENDERBUFFER_OES, m_colorRenderbuffer);
    GLint width;
    GLint height;
    glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_WIDTH_OES, &width);
    glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_HEIGHT_OES, &height);
    glGenRenderbuffersOES(1, &m_depthRenderbuffer);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_depthRenderbuffer);
    glRenderbufferStorageOES(GL_RENDERBUFFER_OES, GL_DEPTH_COMPONENT16_OES, width, height);
    glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_DEPTH_ATTACHMENT_OES, GL_RENDERBUFFER_OES, m_depthRenderbuffer);
    if (glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES) != GL_FRAMEBUFFER_COMPLETE_OES) {
        m_framebufferStatus = FramebufferStatus_Incomplete;
        return 0;
    }

    unsigned int multisample;
    switch (m_flags & Flag_MultisampleMask) {
    case Flag_Multisample1:
        multisample = 1;
        break;
    case Flag_Multisample2:
        multisample = 2;
        break;
    case Flag_Multisample3:
        multisample = 3;
        break;
    default:
        multisample = 0;
        break;
    }
    if (multisample != 0) {
        components::CStrChar extensions((const char*)glGetString(GL_EXTENSIONS));
        if (extensions.Find("GL_APPLE_framebuffer_multisample", 0) < 0) {
            m_flags &= ~Flag_MultisampleMask;
            multisample = 0;
        }
    }

    GLsizei samples;
    switch (multisample) {
    case 0:
        m_framebufferStatus = FramebufferStatus_Complete;
        return 1;
    case 1:
        samples = 2;
        break;
    case 2:
        samples = 4;
        break;
    case 3:
        samples = 8;
        break;
    }
    // render into a multisample framebuffer that is resolved into the window framebuffer
    glGenRenderbuffersOES(1, &m_multisampleColorRenderbuffer);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_multisampleColorRenderbuffer);
    GLenum colorFormat = GL_RGBA8_OES;
    if (graphics->m_width <= Window_MaxHighColorBits) {
        colorFormat = GL_RGB5_A1_OES;
    }
    glRenderbufferStorageMultisampleAPPLE(GL_RENDERBUFFER_OES, samples, colorFormat, width, height);
    glGenFramebuffersOES(1, &m_multisampleFramebuffer);
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, m_multisampleFramebuffer);
    glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_RENDERBUFFER_OES,
                                 m_multisampleColorRenderbuffer);
    glGenRenderbuffersOES(1, &m_multisampleDepthRenderbuffer);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_multisampleDepthRenderbuffer);
    glRenderbufferStorageMultisampleAPPLE(GL_RENDERBUFFER_OES, samples, GL_DEPTH_COMPONENT16_OES, width, height);
    glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_DEPTH_ATTACHMENT_OES, GL_RENDERBUFFER_OES,
                                 m_multisampleDepthRenderbuffer);
    if (glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES) != GL_FRAMEBUFFER_COMPLETE_OES) {
        // no multisampling then
        if (m_multisampleDepthRenderbuffer != 0) {
            glDeleteRenderbuffersOES(1, &m_multisampleDepthRenderbuffer);
            m_multisampleDepthRenderbuffer = 0;
        }
        if (m_multisampleColorRenderbuffer != 0) {
            glDeleteRenderbuffersOES(1, &m_multisampleColorRenderbuffer);
            m_multisampleColorRenderbuffer = 0;
        }
        if (m_multisampleFramebuffer != 0) {
            glDeleteFramebuffersOES(1, &m_multisampleFramebuffer);
            m_multisampleFramebuffer = 0;
        }
        m_flags &= ~Flag_MultisampleMask;
    }
    m_framebufferStatus = FramebufferStatus_Complete;
    return 1;
}

void CRenderSurface_OGLES_Window_Cocoa::DestroyFrameBuffer()
{
    if (m_multisampleDepthRenderbuffer != 0) {
        glDeleteRenderbuffersOES(1, &m_multisampleDepthRenderbuffer);
        m_multisampleDepthRenderbuffer = 0;
    }
    if (m_multisampleColorRenderbuffer != 0) {
        glDeleteRenderbuffersOES(1, &m_multisampleColorRenderbuffer);
        m_multisampleColorRenderbuffer = 0;
    }
    if (m_multisampleFramebuffer != 0) {
        glDeleteFramebuffersOES(1, &m_multisampleFramebuffer);
        m_multisampleFramebuffer = 0;
    }
    if (m_depthRenderbuffer != 0) {
        glDeleteRenderbuffersOES(1, &m_depthRenderbuffer);
        m_depthRenderbuffer = 0;
    }
    if (m_colorRenderbuffer != 0) {
        glDeleteRenderbuffersOES(1, &m_colorRenderbuffer);
        m_colorRenderbuffer = 0;
    }
    if (m_framebuffer != 0) {
        glDeleteFramebuffersOES(1, &m_framebuffer);
        m_framebuffer = 0;
    }
    m_framebufferStatus = FramebufferStatus_None;
}

void CRenderSurface_OGLES_Window_Cocoa::BindFrameBuffer()
{
    if (m_multisampleFramebuffer != 0) {
        glBindFramebufferOES(GL_FRAMEBUFFER_OES, m_multisampleFramebuffer);
        glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_multisampleColorRenderbuffer);
    } else {
        glBindFramebufferOES(GL_FRAMEBUFFER_OES, m_framebuffer);
        glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_colorRenderbuffer);
    }
}

void CRenderSurface_OGLES_Window_Cocoa::RemoveSelfAsTarget()
{
    CGraphics_OGLES* graphics = (CGraphics_OGLES*)ICGraphics::GetInstance();
    if (this == graphics->m_targetSurface) {
        glBindFramebufferOES(GL_FRAMEBUFFER_OES, 0);
        glBindRenderbufferOES(GL_RENDERBUFFER_OES, 0);
    }
}

unsigned char CRenderSurface_OGLES_Window_Cocoa::SaveStateForFutureRecreation()
{
    return 1;
}

unsigned char CRenderSurface_OGLES_Window_Cocoa::Recreate()
{
    return 1;
}
