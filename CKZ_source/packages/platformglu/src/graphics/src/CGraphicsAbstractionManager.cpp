// CGraphicsAbstractionManager.cpp - the graphics devices of each abstraction and the current one.
#include "CGraphicsAbstractionManager.h"
#include "ICGraphics.h"
#include "ICGraphics2d.h"

namespace com { namespace glu { namespace platform { namespace graphics {

ICGraphics* ICGraphics::GetInstance()
{
    CGraphicsAbstractionManager* manager =
        static_cast<CGraphicsAbstractionManager*>(ICGraphicsAbstractionManager::GetInstance());
    return manager->m_graphics;
}

ICGraphics2d* ICGraphics2d::GetInstance()
{
    CGraphicsAbstractionManager* manager =
        static_cast<CGraphicsAbstractionManager*>(ICGraphicsAbstractionManager::GetInstance());
    return manager->m_graphics2d;
}

unsigned char ICGraphicsAbstractionManager::SetAbstractionLayer(ICGraphicsAbstractionManager& abstractionManager,
                                                                ICGraphicsAbstract::Abstraction abstraction)
{
    CGraphicsAbstractionManager& manager = static_cast<CGraphicsAbstractionManager&>(abstractionManager);
    if (abstraction == ICGraphicsAbstract::Abstraction_Hardware) {
        if (manager.m_hardwareGraphics == NULL) {
            return 0;
        }
        manager.m_graphics = manager.m_hardwareGraphics;
        manager.m_graphics2d = manager.m_hardwareGraphics2d;
    } else {
        if (manager.m_softwareGraphics == NULL) {
            return 0;
        }
        manager.m_graphics = manager.m_softwareGraphics;
        manager.m_graphics2d = manager.m_softwareGraphics2d;
    }
    return 1;
}

CGraphicsAbstractionManager::CGraphicsAbstractionManager(unsigned int deviceType, unsigned int softwareType)
    : components::CSingleton(SingletonId)
{
    m_deviceType = deviceType;
    m_softwareType = softwareType;
    m_hardwareGraphics = NULL;
    m_softwareGraphics = NULL;
    m_graphics = NULL;
    m_hardwareGraphics2d = NULL;
    m_softwareGraphics2d = NULL;
    m_graphics2d = NULL;
    m_ownedObjects[0] = NULL;
    m_ownedObjects[1] = NULL;
    m_ownedObjects[2] = NULL;
    m_ownedObjects[3] = NULL;
    m_ownedObjects[4] = NULL;
    m_ownedObjects[5] = NULL;
    CreateHardwareAbstractionLayer(*this);
    SetAbstractionLayer(*this, ICGraphicsAbstract::Abstraction_Hardware);
}

CGraphicsAbstractionManager::~CGraphicsAbstractionManager()
{
    if (m_ownedObjects[0] != NULL) {
        delete m_ownedObjects[0];
    }
    if (m_ownedObjects[1] != NULL) {
        delete m_ownedObjects[1];
    }
    if (m_ownedObjects[2] != NULL) {
        delete m_ownedObjects[2];
    }
    if (m_ownedObjects[3] != NULL) {
        delete m_ownedObjects[3];
    }
    if (m_ownedObjects[4] != NULL) {
        delete m_ownedObjects[4];
    }
    if (m_ownedObjects[5] != NULL) {
        delete m_ownedObjects[5];
    }
    framework::CApp* app = CApplet::m_pApp;
    if (app != NULL) {
        app->m_graphicsAbstractionManager = NULL;
    }
}

} } } }
