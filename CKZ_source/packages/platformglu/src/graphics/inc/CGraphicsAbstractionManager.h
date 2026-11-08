// CGraphicsAbstractionManager.h - owns the graphics devices of each abstraction and the current one.
// (first drafted by b002 as PROVISIONAL; completed by b037x, the owner)
#ifndef CGRAPHICSABSTRACTIONMANAGER_H
#define CGRAPHICSABSTRACTIONMANAGER_H

#include "ICGraphicsAbstractionManager.h"
#include "CSingleton.h"
#include "CClass.h"

namespace com { namespace glu { namespace platform { namespace graphics {

class ICGraphics;
class ICGraphics2d;

class CGraphicsAbstractionManager : public ICGraphicsAbstractionManager, public components::CSingleton {
public:
    /** Hardware device types (m_deviceType). */
    enum DeviceType {
        DeviceType_OGLES1 = 3,
        DeviceType_OGLES2 = 4
    };

    CGraphicsAbstractionManager(unsigned int deviceType, unsigned int softwareType);
    virtual ~CGraphicsAbstractionManager();

    unsigned int m_deviceType;              // 0x0c DeviceType of the hardware layer
    unsigned int m_softwareType;            // 0x10
    ICGraphics* m_hardwareGraphics;         // 0x14 created by CreateHardwareAbstractionLayer()
    ICGraphics* m_softwareGraphics;         // 0x18
    ICGraphics* m_graphics;                 // 0x1c current abstraction's device
    ICGraphics2d* m_hardwareGraphics2d;     // 0x20
    ICGraphics2d* m_softwareGraphics2d;     // 0x24
    ICGraphics2d* m_graphics2d;             // 0x28 current
    core::CClass* m_ownedObjects[6];        // 0x2c deleted with the manager (not set by the platform layer)
};

} } } }

#endif
