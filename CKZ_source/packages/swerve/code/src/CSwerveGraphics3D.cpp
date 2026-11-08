// CSwerveGraphics3D.cpp - binds Glu render surfaces as targets of the Swerve renderer.
#include <string.h>
#include "CSwerveGraphics3D.h"
#include "CSwerve.h"
#include "CEventListener.h"
#include "CEvent.h"
#include "CMessage.h"
#include "CStringToKey.h"
#include "CArrayInputStream.h"
#include "ICStdUtil.h"
#include "ICGraphics.h"
#include "ICRenderSurface.h"
#include "ICResourceManager.h"
#include "CResource.h"
#include "ICCore.h"

using namespace com::glu::platform;

// Event the renderer listens for (graphics context lost / recreated).
static const unsigned int Event_GraphicsContextLost = 0xc0d486fe;
// Messages posted around a bound target.
static const unsigned int Message_TargetBound = 0x97f37ccf;
static const unsigned int Message_TargetReleased = 0xa8966e06;
static const unsigned int MessageArgument_Renderer = 0xc9403f77;
// Resource type of render surfaces created from a stream of InitParams.
static const unsigned int ResourceType_RenderSurface = 0x593f8140;

unsigned char CSwerveGraphics3D_EventCB(systems::CEvent& event, void* listener)
{
    if (event.m_eventId != Event_GraphicsContextLost) {
        return 0;
    }
    CSwerveGraphics3D* graphics3D = (CSwerveGraphics3D*)((systems::CEventListener*)listener)->m_object;
    graphics3D->m_graphics3D->m_functions->OnContextLost(graphics3D->m_graphics3D);
    return 1;
}

CSwerveGraphics3D::CSwerveGraphics3D() : com::glu::platform::core::CClass(ClassId), m_viewport()
{
    m_graphics3D = NULL;
    m_boundImage = NULL;
    m_target = NULL;
    m_previousTarget = NULL;
    m_dummySurface = NULL;
    m_listener = NULL;
    Init();
}

unsigned char CSwerveGraphics3D::Init()
{
    // a 1x1 RGBA image: makes sure the mutable surfaces work at all
    static const malij297_Image dummyImage = { 1, 1, 100, 0, 0 };

    CSwerve* swerve = CSwerve::GetInstance();
    int error = swerve->m_swerve->m_functions->CreateObject(SwerveObjectType_Graphics3D, (void**)&m_graphics3D);
    if (error == 0) {
        memset(&m_environment, 0, sizeof(m_environment));
        error = 1;
        if (malij297_GetEnvironment(&m_environment)) {
            m_dummySurface = GetMutableSurface(&dummyImage);
            if (m_dummySurface != NULL) {
                systems::CEventListener* listener = new systems::CEventListener();
                m_listener = listener;
                if (listener != NULL) {
                    unsigned int handle = core::CStringToKey("CSwerveGraphics3D", 0) | 0x80000000;
                    listener->m_object = this;
                    listener->m_handle = handle;
                    listener->m_callback = CSwerveGraphics3D_EventCB;
                    m_listener->Register(Event_GraphicsContextLost, 0x7fffffff);
                    error = 0;
                }
            }
        }
    }
    return error == 0;
}

CSwerveGraphics3D::~CSwerveGraphics3D()
{
    Free();
}

void CSwerveGraphics3D::Free()
{
    if (m_listener != NULL) {
        m_listener->UnregisterAll();
        if (m_listener != NULL) {
            delete m_listener;
            m_listener = NULL;
        }
    }

    int count = m_mutableSurfaces.m_size;
    for (int index = 0; index < count; index++) {
        MutableSurface* surface = m_mutableSurfaces.m_data[index];
        systems::ICResourceManager* resourceManager = (systems::ICResourceManager*)CApplet::m_pApp->m_resourceManager;
        resourceManager->ReleaseResource(surface->m_name);
        delete surface;
    }
    m_mutableSurfaces.Clear();
    m_mutableSurfaces.TrimToSize();
    m_dummySurface = NULL;

    if (m_graphics3D != NULL) {
        m_graphics3D->m_functions->Release(m_graphics3D);
    }
    m_graphics3D = NULL;
}

graphics::ICRenderSurface* CSwerveGraphics3D::GetMutableSurface(const malij297_Image* image)
{
    int count = m_mutableSurfaces.m_size;
    for (int index = 0; index < count; index++) {
        MutableSurface* surface = m_mutableSurfaces.m_data[index];
        unsigned int width;
        unsigned int height;
        surface->m_surface->GetWidthAndHeight(width, height);
        if (width >= image->m_width && height >= image->m_height) {
            if (surface->m_surface != NULL) {
                return surface->m_surface;
            }
            break;
        }
    }

    // a new target surface of the image size (the parameters are read from a stream)
    graphics::ICRenderSurface::InitParam params[6];
    params[0].m_key = 6;
    if (image->m_format == 100) {
        params[0].m_value = 0xf4404;
    } else {
        params[0].m_value = 0;
    }
    params[1].m_key = 7;
    params[1].m_value = 0x5a0102;
    params[2].m_key = 4;
    params[2].m_value = 0;
    params[3].m_key = 1;
    params[3].m_value = image->m_width;
    params[4].m_key = 2;
    params[4].m_value = image->m_height;
    params[5].m_key = 0;
    params[5].m_value = 0;
    components::CArrayInputStream stream;
    graphics::ICRenderSurface* created = NULL;
    if (stream.Open((const unsigned char*)params, sizeof(params))) {
        char name[0x80];
        core::ICStdUtil::SPrintF_S(name, sizeof(name), "SWV_%d_%d_%d_%08X", image->m_width, image->m_height,
                                   image->m_format, image->m_id);
        systems::ICResourceManager::CreationInfoStr info;
        info.m_mimeType = ResourceType_RenderSurface;
        info.m_stream = &stream;
        info.m_name = name;
        systems::CResource* resource;
        systems::ICResourceManager* resourceManager = (systems::ICResourceManager*)CApplet::m_pApp->m_resourceManager;
        if (resourceManager->CreateResource(info, &resource) == 3) {
            created = (graphics::ICRenderSurface*)resource->GetData();
            if (created != NULL) {
                MutableSurface* surface = new MutableSurface;
                strncpy(surface->m_name, name, sizeof(surface->m_name));
                surface->m_name[sizeof(surface->m_name) - 1] = 0;
                surface->m_surface = created;
                m_mutableSurfaces.Add(surface);
            }
        }
    }
    return created;
}

int CSwerveGraphics3D::BindTarget(graphics::ICRenderSurface* target, const components::CRectangle<short>* viewport,
                                  int flags)
{
    (new systems::CMessage(this, Message_TargetBound, 0, 2,
                           systems::CMessage::ArgumentType_UInt32, 0, MessageArgument_Renderer,
                           systems::CMessage::ArgumentType_UInt32, 0, 0))->Run();

    if (target == NULL) {
        return 0x1000f;
    }
    if (m_boundImage != NULL) {
        return 0x1000d;
    }
    if (m_target != NULL) {
        return 0x1000d;
    }
    graphics::ICRenderSurface* previous = graphics::ICGraphics::GetInstance()->GetTargetSurface();
    graphics::ICGraphics::GetInstance()->SetTargetSurface(*target);
    unsigned int width;
    unsigned int height;
    target->GetWidthAndHeight(width, height);
    if (viewport != NULL) {
        m_viewport.m_x = viewport->m_x;
        m_viewport.m_y = viewport->m_y;
        m_viewport.m_width = viewport->m_width;
        m_viewport.m_height = viewport->m_height;
    } else {
        m_viewport.m_x = 0;
        m_viewport.m_y = 0;
        m_viewport.m_width = width;
        m_viewport.m_height = height;
    }
    graphics::ICRenderSurface::Description previousDescription;
    if (previous->GetDescription(previousDescription)) {
        if (previousDescription.m_multisample == 0) {
            flags &= ~2;
        } else {
            flags |= 2;
        }
    }
    int result = m_graphics3D->m_functions->BindTarget(m_graphics3D, width, height, m_viewport.m_x, m_viewport.m_y,
                                                        m_viewport.m_width, m_viewport.m_height, flags);
    if (result != 0) {
        return result;
    }
    m_target = target;
    m_previousTarget = previous;

    // rotate with the device when rendering to the window
    graphics::ICRenderSurface::Description targetDescription;
    target->GetDescription(targetDescription);
    if (targetDescription.m_usage == 1 && CApplet::m_pCore->m_deviceOrientationSet) {
        switch (CApplet::m_pCore->m_deviceOrientation) {
        case framework::ICCore::DeviceOrientation_Portrait:
            break;
        case framework::ICCore::DeviceOrientation_LandscapeRight:
            return m_graphics3D->m_functions->SetDisplayRotation(m_graphics3D, 270);
        case framework::ICCore::DeviceOrientation_PortraitUpsideDown:
            return m_graphics3D->m_functions->SetDisplayRotation(m_graphics3D, 180);
        case framework::ICCore::DeviceOrientation_LandscapeLeft:
            return m_graphics3D->m_functions->SetDisplayRotation(m_graphics3D, 90);
        default:
            return result;
        }
    }
    return m_graphics3D->m_functions->SetDisplayRotation(m_graphics3D, 0);
}

void CSwerveGraphics3D::BindMutableImageEnd()
{
    m_boundImage->m_functions->Release(m_boundImage);
    m_boundImage = NULL;
    if (m_environment.m_hasFramebufferObject != 0) {
        graphics::ICRenderSurface* previous = m_previousTarget;
        if (previous != NULL) {
            graphics::ICGraphics::GetInstance()->SetTargetSurface(*previous);
            m_previousTarget = NULL;
        }
    }
}

int CSwerveGraphics3D::ReleaseTarget()
{
    int result;
    unsigned int released;
    if (m_target != NULL) {
        m_target->UpdateTextureWithTarget();
        m_target = NULL;
        result = m_graphics3D->m_functions->ReleaseTarget(m_graphics3D);
        graphics::ICRenderSurface* previous = m_previousTarget;
        graphics::ICGraphics::GetInstance()->SetTargetSurface(*previous);
        m_previousTarget = NULL;
        released = MessageArgument_Renderer;
    } else {
        released = 0;
        result = 0;
        if (m_boundImage != NULL) {
            released = 0x24;
            result = m_graphics3D->m_functions->ReleaseImageTarget(m_graphics3D);
            BindMutableImageEnd();
        }
    }
    (new systems::CMessage(this, Message_TargetReleased, 0, 2,
                           systems::CMessage::ArgumentType_UInt32, 0, released,
                           systems::CMessage::ArgumentType_UInt32, 0, 0))->Run();
    return result;
}
