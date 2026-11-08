// CGraphicsAbstractionManager_Cocoa.cpp - iOS factories of the graphics objects: every object is
// created for the OpenGL ES abstraction when that is the current one, and NULL otherwise.
// The C-style casts bridge b002's PROVISIONAL graphics headers, whose concrete classes are not yet
// linked to their interfaces (the casts become plain upcasts once the owners write the classes).
#include "CGraphicsAbstractionManager.h"
#include "ICDisplayProgram.h"
#include "ICFont.h"
#include "ICIndexBuffer.h"
#include "ICRasterizerState.h"
#include "ICRenderSurface.h"
#include "ICRenderText2d.h"
#include "ICShader.h"
#include "ICShaderProgram.h"
#include "ICVertexBuffer.h"
#include "CGraphics_OGLES_Cocoa.h"
#include "CGraphics2d_OGLES.h"
#include "CDisplayProgram_OGLES.h"
#include "CBitmapFont.h"
#include "CSystemFont_Cocoa.h"
#include "CIndexBuffer.h"
#include "CRasterizerState_OGLES.h"
#include "CRenderSurface_OGLES_Texture.h"
#include "CRenderSurface_OGLES_Texture_FBO.h"
#include "CRenderSurface_OGLES_Window_Cocoa.h"
#include "CRenderText2d.h"
#include "CShader_OGLES.h"
#include "CShaderProgram_OGLES.h"
#include "CVertexBuffer.h"
#include "CEvent.h"
#include "CEventListener.h"

using namespace com::glu::platform::graphics;
using namespace com::glu::platform::systems;

// ---- constants ----
static const unsigned int Event_SurfaceChanged = 0x97b4dec3;           // rebuild the window surface
static const unsigned int Event_ContextLost = 0xc0d486fe;              // save state, release the device
static const unsigned int Event_ContextRestored = 0x16625f9a;          // recreate the device
static const unsigned int Event_UseFixedFunction = 0x37f8a76d;         // fixed function pipeline
static const unsigned int Event_UseShaders = 0x7b7983ee;               // shader pipeline

/** True when objects for the OpenGL ES device have to be created. */
static inline bool IsOGLESCurrent(CGraphicsAbstractionManager* manager)
{
    return manager->m_graphics->GetAbstraction() == ICGraphicsAbstract::Abstraction_1 &&
           manager->m_deviceType == CGraphicsAbstractionManager::DeviceType_OGLES1;
}

ICDisplayProgram* ICDisplayProgram::CreateInstance()
{
    CGraphicsAbstractionManager* manager =
        static_cast<CGraphicsAbstractionManager*>(ICGraphicsAbstractionManager::GetInstance());
    if (IsOGLESCurrent(manager)) {
        return (ICDisplayProgram*)new CDisplayProgram_OGLES();
    }
    return NULL;
}

ICFont* ICFont::CreateInstance(Type type)
{
    switch (type) {
    case Type_Bitmap:
        return (ICFont*)new CBitmapFont();
    case Type_System:
        return (ICFont*)new CSystemFont_Cocoa();
    default:
        return NULL;
    }
}

ICIndexBuffer* ICIndexBuffer::CreateInstance()
{
    CGraphicsAbstractionManager* manager =
        static_cast<CGraphicsAbstractionManager*>(ICGraphicsAbstractionManager::GetInstance());
    if (IsOGLESCurrent(manager)) {
        return (ICIndexBuffer*)new CIndexBuffer();
    }
    return NULL;
}

ICRasterizerState* ICRasterizerState::CreateInstance()
{
    CGraphicsAbstractionManager* manager =
        static_cast<CGraphicsAbstractionManager*>(ICGraphicsAbstractionManager::GetInstance());
    if (IsOGLESCurrent(manager)) {
        return (ICRasterizerState*)new CRasterizerState_v1_OGLES();
    }
    return NULL;
}

ICRenderSurface* ICRenderSurface::CreateInstance(Type type, Targetability targetability)
{
    CGraphicsAbstractionManager* manager =
        static_cast<CGraphicsAbstractionManager*>(ICGraphicsAbstractionManager::GetInstance());
    if (IsOGLESCurrent(manager)) {
        switch (type) {
        case 0:
            // a texture; render targets need a framebuffer object
            if (targetability == 0) {
                return (ICRenderSurface*)new CRenderSurface_OGLES_Texture();
            }
            return (ICRenderSurface*)new CRenderSurface_OGLES_Texture_FBO();
        case 1:     // not named in ICRenderSurface::Type yet
            // the window
            return (ICRenderSurface*)new CRenderSurface_OGLES_Window_Cocoa();
        }
    }
    return NULL;
}

ICRenderText2d* ICRenderText2d::CreateInstance()
{
    CGraphicsAbstractionManager* manager =
        static_cast<CGraphicsAbstractionManager*>(ICGraphicsAbstractionManager::GetInstance());
    if (IsOGLESCurrent(manager)) {
        return (ICRenderText2d*)new CRenderText2d();
    }
    return NULL;
}

ICShader* ICShader::CreateInstance()
{
    CGraphicsAbstractionManager* manager =
        static_cast<CGraphicsAbstractionManager*>(ICGraphicsAbstractionManager::GetInstance());
    if (IsOGLESCurrent(manager)) {
        return (ICShader*)new CShader_OGLES();
    }
    return NULL;
}

ICShaderProgram* ICShaderProgram::CreateInstance()
{
    CGraphicsAbstractionManager* manager =
        static_cast<CGraphicsAbstractionManager*>(ICGraphicsAbstractionManager::GetInstance());
    if (IsOGLESCurrent(manager)) {
        return (ICShaderProgram*)new CShaderProgram_OGLES();
    }
    return NULL;
}

ICVertexBuffer* ICVertexBuffer::CreateInstance()
{
    CGraphicsAbstractionManager* manager =
        static_cast<CGraphicsAbstractionManager*>(ICGraphicsAbstractionManager::GetInstance());
    if (IsOGLESCurrent(manager)) {
        return (ICVertexBuffer*)new CVertexBuffer();
    }
    return NULL;
}

ICGraphicsAbstractionManager* ICGraphicsAbstractionManager::CreateInstance()
{
    return new CGraphicsAbstractionManager(CGraphicsAbstractionManager::DeviceType_OGLES1, 2);
}

static unsigned char Graphics_OGLES_HandleEvent(CEvent& event, void* listener);

unsigned char ICGraphicsAbstractionManager::CreateHardwareAbstractionLayer(ICGraphicsAbstractionManager& manager)
{
    CGraphicsAbstractionManager& concrete = static_cast<CGraphicsAbstractionManager&>(manager);
    if (concrete.m_deviceType == CGraphicsAbstractionManager::DeviceType_OGLES1) {
        concrete.m_hardwareGraphics = new CGraphics_OGLES_Cocoa(Graphics_OGLES_HandleEvent);
    }
    // CGraphics2d_OGLES (graphics/2d, not written yet) implements ICGraphics2d; its header is still provisional
    concrete.m_hardwareGraphics2d = (ICGraphics2d*)new CGraphics2d_OGLES();
    return concrete.m_hardwareGraphics != NULL && concrete.m_hardwareGraphics2d != NULL;
}

/** Device events of the OpenGL ES device (the listener's object is the device). */
static unsigned char Graphics_OGLES_HandleEvent(CEvent& event, void* listener)
{
    CGraphics_OGLES_Cocoa* graphics = (CGraphics_OGLES_Cocoa*)((CEventListener*)listener)->m_object;
    switch (event.m_eventId) {
    case Event_SurfaceChanged:
        if (graphics->m_windowSurface == NULL) {
            return 0;
        }
        return graphics->InitializeWindowSurface(*graphics->m_windowSurface);
    case Event_ContextLost:
        return graphics->SaveStateAndReleaseDevice();
    case Event_ContextRestored:
        return graphics->Recreate();
    case Event_UseFixedFunction:
        graphics->InitializeIntrinsicHardwareShaderManagement(0);
        return 1;
    case Event_UseShaders:
        graphics->InitializeIntrinsicHardwareShaderManagement(1);
        return 1;
    default:
        return 0;
    }
}
