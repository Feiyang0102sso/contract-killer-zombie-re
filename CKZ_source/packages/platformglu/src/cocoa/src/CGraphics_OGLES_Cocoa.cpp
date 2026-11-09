// CGraphics_OGLES_Cocoa.cpp - the OpenGL ES graphics device on iOS (see CGraphics_OGLES_Cocoa.h).
#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>
#include "CGraphics_OGLES_Cocoa.h"
#include "CGraphics_OGLES_EAGL.h"
#include "CRenderSurface_OGLES_Window_Cocoa.h"
#include "CApplet.h"
#include "ICCore.h"
#include "CStrChar.h"
#include "CEvent.h"
#include "ICRasterizerState.h"

using namespace com::glu::platform::components;
using namespace com::glu::platform::framework;
using namespace com::glu::platform::graphics;
using namespace com::glu::platform::systems;

// ---- constants ----
static const unsigned int Event_BeforePresent = 0xc2b455cb;    // posted before the frame is presented
static const unsigned int Event_AfterPresent = 0x4b2237ac;     // posted after the frame was presented

/** InitParam keys understood by the iOS device. */
enum {
    InitParam_Width = 1,
    InitParam_RequireSoftware = 2,      // a software renderer cannot be provided: fail
    InitParam_BackgroundColor = 3,      // CGraphics::m_backgroundColor (0x234)
    InitParam_DisplayScaleX = 4,        // CGraphics::m_displayScaleX (0x22c), 16.16
    InitParam_DisplayScaleY = 5,        // CGraphics::m_displayScaleY (0x230), 16.16
    InitParam_MultisampleSamples = 6
};

unsigned char CGraphics_OGLES_Cocoa::Initialize(const InitParam* params)
{
    Destroy();

    for (const InitParam* param = params; param->m_key != 0; param++) {
        switch (param->m_key) {
        case InitParam_Width:
            m_width = param->m_value;
            break;
        case InitParam_RequireSoftware:
            if ((unsigned char)param->m_value) {
                m_initializeFailed = 1;
                Destroy();
                return 0;
            }
            break;
        case InitParam_BackgroundColor:
            m_backgroundColor.value = param->m_value;
            break;
        case InitParam_DisplayScaleX:
            m_displayScaleX = param->m_value;
            break;
        case InitParam_DisplayScaleY:
            m_displayScaleY = param->m_value;
            break;
        case InitParam_MultisampleSamples:
            m_multisampleSamples = param->m_value;
            break;
        }
    }

    InitializeCore();

    // every cached GL value starts dirty so the first use always reaches OpenGL
    m_glState.Invalidate();

    // EAGL context glue on the view's layer
    m_proxy = new CGraphics_OGLES_EAGL();
    m_proxy->m_nativeWindow = CApplet::m_eaglLayer;
    CGraphics_OGLES_Cocoa_GetEAGLVersion(&m_proxy->m_versionMajor, &m_proxy->m_versionMinor);
    CreateAndInitializeWindowSurface(ICRenderSurface::Targetability_2);

    m_versionString = (const char*)glGetString(GL_VERSION);
    m_rendererString = (const char*)glGetString(GL_RENDERER);
    m_versionMajor = 1;
    m_versionMinor = 1;
    glGetIntegerv(GL_MAX_TEXTURE_UNITS, &m_maxTextureUnits);
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &m_maxTextureSize);
#ifdef CKZ_PC
    // PORT: 2D tile coordinates and dimensions are signed 16-bit values.
    if (m_maxTextureSize > 16384) {
        m_maxTextureSize = 16384;
    }
#endif
    m_maxTextureWidth = m_maxTextureSize;
    m_maxTextureHeight = m_maxTextureSize;
    m_maxRenderTargetSize = m_maxTextureSize;

    m_description.m_abstraction = 1;
    m_description.m_renderer = &m_versionString;
#ifdef CKZ_PC
    m_description.m_maxTextureSize = m_maxTextureSize;
#else
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, (GLint*)&m_description.m_maxTextureSize);
#endif
    m_description.m_maxTextureWidth = m_description.m_maxTextureSize;
    m_description.m_maxTextureHeight = m_description.m_maxTextureSize;
    m_description.m_maxRenderTargetSize = m_description.m_maxTextureSize;
    m_description.m_maxRenderTargetWidth = m_description.m_maxTextureSize;
    m_description.m_maxRenderTargetHeight = m_description.m_maxTextureSize;
    m_description.m_maxViewportWidth = m_description.m_maxTextureSize;
    m_description.m_maxViewportHeight = m_description.m_maxTextureSize;
    glGetIntegerv(GL_MAX_TEXTURE_UNITS, (GLint*)&m_description.m_maxTextureUnits);
    m_description.m_maxVertexAttributes = 0x60;
    m_description.m_maxShaderParameters = 0xc0;

    // every orientation can be set by the application
    ICCore* core = CApplet::m_pCore;
    core->m_canSetDeviceOrientation[0] = 0;
    core->m_canSetDeviceOrientation[1] = 1;
    core->m_canSetDeviceOrientation[2] = 1;
    core->m_canSetDeviceOrientation[3] = 1;
    core->m_canSetDeviceOrientation[4] = 1;
    core->m_applicationCanSetOrientation = 1;
    core->m_orientationFlags1f[0] = 0;
    core->m_orientationFlags1f[1] = 0;

    CStrChar extensions((const char*)glGetString(GL_EXTENSIONS));
    m_hasTextureNpot = extensions.Find("OES_texture_npot", 0) >= 0;
    m_hasLimitedTextureNpot = extensions.Find("APPLE_texture_2D_limited_npot", 0) >= 0;
    m_hasDiscardFramebuffer = extensions.Find("GL_EXT_discard_framebuffer", 0) >= 0;
    if (m_hasTextureNpot) {
        m_npotTextures = 1;
    }
    if (m_hasLimitedTextureNpot) {
        m_npotRenderTargets = 1;
    }

    m_initialized = 1;
    if (!CGraphics_OGLES::Initialize(params)) {
        Destroy();
    }
    return m_initialized;
}

void CGraphics_OGLES_Cocoa::SwapBuffers()
{
    ICRenderSurface* target = m_targetSurface;
    // the target of a swap is the window surface (field names follow b039x, offsets unchanged)
    CRenderSurface_OGLES_Window_Cocoa* surface = (CRenderSurface_OGLES_Window_Cocoa*)target;
    if (surface == NULL || !(surface->m_flags & CRenderSurface_OGLES_Window_Cocoa::Flag_Targetable)) {
        return;
    }

    CEvent* event = new CEvent(this, Event_BeforePresent, 0, 0, CExecutable::OneShot);
    event->Run();

    if (surface->m_flags & CRenderSurface_OGLES_Window_Cocoa::Flag_MultisampleMask) {
        // resolve the multisample buffer into the window framebuffer
        m_glState.EnableScissorTest(0);
        glBindFramebufferOES(GL_READ_FRAMEBUFFER_APPLE, surface->m_multisampleFramebuffer);
        glBindFramebufferOES(GL_DRAW_FRAMEBUFFER_APPLE, surface->m_framebuffer);
        glResolveMultisampleFramebufferAPPLE();
        if (m_hasDiscardFramebuffer) {
            GLenum attachment[3] = { GL_COLOR_ATTACHMENT0_OES, GL_DEPTH_ATTACHMENT_OES, GL_STENCIL_ATTACHMENT_OES };
            glDiscardFramebufferEXT(GL_READ_FRAMEBUFFER_APPLE, 3, attachment);
        }
        glBindRenderbufferOES(GL_RENDERBUFFER_OES, surface->m_colorRenderbuffer);
        glBindFramebufferOES(GL_FRAMEBUFFER_OES, surface->m_multisampleFramebuffer);
    } else if (m_hasDiscardFramebuffer) {
        GLenum attachments[2] = { GL_DEPTH_ATTACHMENT_OES, GL_STENCIL_ATTACHMENT_OES };
        glDiscardFramebufferEXT(GL_FRAMEBUFFER_OES, 2, attachments);
    }

    m_proxy->Present(*target);
    target->DiscardContents();

    event = new CEvent(this, Event_AfterPresent, 0, 0, CExecutable::OneShot);
    event->Run();
}

unsigned char CGraphics_OGLES_Cocoa::InitializeWindowSurface(ICRenderSurface& surface)
{
    ICRenderSurface::InitParam params[] = {
        { 3, 1 },
        { 4, 1 },
        { 6, 0 },
        { 0x12, m_multisampleSamples },
        { 0, 0 }
    };

    unsigned char initialized = 0;
    if (surface.Initialize(params)) {
        initialized = 1;
        if (m_targetSurface == &surface) {
            unsigned int width;
            unsigned int height;
            m_windowSurface->GetWidthAndHeight(width, height);
            m_rasterizerState->SetScissor(0, 0, (short)width, (short)height);
            SetClearScissor(0, 0, (short)width, (short)height);
            SetViewport(0, 0, (short)width, (short)height);
        }
    }
    return initialized;
}

void CGraphics_OGLES_Cocoa::ReleaseDevice()
{
    if (m_proxy != NULL) {
        if (m_proxy->m_context != NULL) {
            m_proxy->m_context = NULL;
        }
        delete m_proxy;
        m_proxy = NULL;
    }
    m_versionMajor = 0;
    m_versionMinor = 0;
    m_versionString = "";
    m_rendererString = "";
    memset(&m_description, 0, sizeof(m_description));
}
