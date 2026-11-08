// CGraphics_OGLES_EAGL.cpp - OpenGL ES platform glue for iOS (see CGraphics_OGLES_EAGL.h).
#include "CGraphics_OGLES_EAGL.h"
#include "CGraphicsAbstractionManager.h"
#include "CRenderSurface_OGLES_Window_Cocoa.h"
#include "CColor.h"

using namespace com::glu::platform::components;
using namespace com::glu::platform::graphics;

void* CGraphics_OGLES_EAGL::CreateContext(void* colorFormat, const int* attributes)
{
    CGraphicsAbstractionManager* manager =
        static_cast<CGraphicsAbstractionManager*>(ICGraphicsAbstractionManager::GetInstance());

    unsigned int api;
    switch (manager->m_deviceType) {
    case CGraphicsAbstractionManager::DeviceType_OGLES1:
        api = 1;
        break;
    case CGraphicsAbstractionManager::DeviceType_OGLES2:
        api = 2;
        break;
    default:
        api = 0;
        break;
    }

    switch ((int)colorFormat) {
    case ColorFormat_R5G6B5:
        return CGraphics_OGLES_EAGL_CreateContext_R5G6B5(m_nativeWindow, api);
    case ColorFormat_A8B8G8R8:
        return CGraphics_OGLES_EAGL_CreateContext_A8B8G8R8(m_nativeWindow, api);
    default:
        return NULL;
    }
}

void CGraphics_OGLES_EAGL::Present(ICRenderSurface& surface)
{
    CGraphics_OGLES_EAGL_PresentRenderBuffer(((CRenderSurface_OGLES_Window_Cocoa&)surface).m_context);
}

void CGraphics_OGLES_EAGL::ReleaseContext(void* context)
{
    CGraphics_OGLES_EAGL_ReleaseContext(context);
}

void CGraphics_OGLES_EAGL::SetCurrentContext(void* context, void* drawSurface, void* readSurface)
{
    CGraphics_OGLES_EAGL_SetCurrentContext(context);
}

unsigned char CGraphics_OGLES_EAGL::RenderBufferStorage(void* context)
{
    return CGraphics_OGLES_EAGL_RenderBufferStorage(m_nativeWindow, context);
}
