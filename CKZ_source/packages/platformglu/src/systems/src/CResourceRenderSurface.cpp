// CResourceRenderSurface.cpp - resource wrapping a ICRenderSurface.
#include "CResourceRenderSurface.h"
#include "ICRenderSurface.h"
#include "ICGraphics.h"
#include "ICGraphicsAbstractionManager.h"

namespace com { namespace glu { namespace platform { namespace systems {

CResourceRenderSurface::CResourceRenderSurface()
{
    m_classId = ClassId;
    m_surface = NULL;
    CResource::ResetMembers();
}

CResourceRenderSurface::~CResourceRenderSurface()
{
    CResourceRenderSurface::Destroy();
}

void CResourceRenderSurface::Destroy()
{
    if (m_surface != NULL) {
        delete m_surface;
        m_surface = NULL;
    }
}

void CResourceRenderSurface::ResetMembers()
{
    CResource::ResetMembers();
}

unsigned int CResourceRenderSurface::GetSize() const
{
    if (m_surface == NULL) {
        return 0;
    }
    return 8;
}

void* CResourceRenderSurface::GetData() const
{
    return m_surface;
}

unsigned int CResourceRenderSurface::CreateInternal(unsigned int handle, const char* name, components::CInputStream* stream,
                                          unsigned int mimeType, components::CIdToObjectRouter* router,
                                          unsigned char copyName)
{
    m_handle = handle;
    graphics::ICGraphicsAbstract::Abstraction abstraction;
    graphics::ICRenderSurface::Targetability targetability;
    unsigned int surfaceType = mimeType;
    unsigned char known = 1;
    // generic types: pick the format of the running renderer
    if (mimeType == 0xfc31810d) {
        int current = (int)graphics::ICGraphics::GetInstance()->GetAbstraction();
        if (current == 1) {
            surfaceType = 0xe179f4be;
        } else if (current == 2) {
            surfaceType = 0xe179efbe;
        }
    } else if (mimeType == 0x593f8140) {
        int current = (int)graphics::ICGraphics::GetInstance()->GetAbstraction();
        if (current == 1) {
            surfaceType = 0x59f4bc59;
        } else if (current == 2) {
            surfaceType = 0x59efbc42;
        }
    }
    switch (surfaceType) {
    case 0x9f4bc472:
        abstraction = graphics::ICGraphicsAbstract::Abstraction_1;
        targetability = graphics::ICRenderSurface::Targetability_3;
        break;
    case 0xe179efbe:
        targetability = graphics::ICRenderSurface::Targetability_0;
        abstraction = graphics::ICGraphicsAbstract::Abstraction_2;
        break;
    case 0xe179f4be:
        abstraction = graphics::ICGraphicsAbstract::Abstraction_1;
        targetability = graphics::ICRenderSurface::Targetability_0;
        break;
    case 0x59efbc42:
        abstraction = graphics::ICGraphicsAbstract::Abstraction_2;
        targetability = graphics::ICRenderSurface::Targetability_1;
        break;
    case 0x59f4bc42:
        targetability = graphics::ICRenderSurface::Targetability_1;
        abstraction = graphics::ICGraphicsAbstract::Abstraction_1;
        break;
    case 0x59f4bc59:
        abstraction = graphics::ICGraphicsAbstract::Abstraction_1;
        targetability = graphics::ICRenderSurface::Targetability_2;
        break;
    default:
        known = 0;
        break;
    }
    if (!known) {
        Destroy();
        return CreateResult_Failed;
    }

    graphics::ICGraphicsAbstractionManager::SetAbstractionLayer(*graphics::ICGraphicsAbstractionManager::GetInstance(),
                                                                abstraction);
    unsigned int result = CreateResult_Failed;
    unsigned char success;
    m_surface = graphics::ICRenderSurface::CreateInstance(graphics::ICRenderSurface::Type_Default, targetability);
    if (m_surface == NULL) {
        success = 1;
    } else if (stream == NULL) {
        success = 1;
        result = CreateResult_Created;
    } else {
        success = 0;
        result = CreateResult_Failed;
        if (m_surface->Load(*stream, surfaceType, router)) {
            result = CreateResult_Loaded;
            success = 1;
        }
    }
    if (name != NULL && success) {
        SetName(name, copyName);
    }
    if (success) {
        m_isCreated = 1;
    } else {
        Destroy();
        result = CreateResult_Failed;
    }
    return result;
}

} } } }
