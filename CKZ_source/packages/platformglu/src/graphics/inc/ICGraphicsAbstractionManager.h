// ICGraphicsAbstractionManager.h - selects the graphics abstraction (renderer) the graphics
// objects are created for. (first drafted by b011x / b002 as PROVISIONAL; completed by b037x,
// the owner of graphics/src/CGraphicsAbstractionManager.cpp)
#ifndef ICGRAPHICSABSTRACTIONMANAGER_H
#define ICGRAPHICSABSTRACTIONMANAGER_H

#include <stddef.h>
#include "CApplet.h"
#include "CApp.h"
#include "CHash.h"

namespace com { namespace glu { namespace platform { namespace graphics {

class ICGraphicsAbstract {
public:
    /** Renderer abstraction (values from the resource system; names inferred). */
    enum Abstraction {
        Abstraction_None = 0,
        Abstraction_1 = 1,
        Abstraction_2 = 2,
        Abstraction_Hardware = 1,   // the platform device (OpenGL ES)
        Abstraction_Software = 2    // software rendering
    };
};

class ICGraphicsAbstractionManager {
public:
    /** Key in the application singleton table: CStringToKey("ICGraphicsAbstractionManager"). */
    enum { SingletonId = 0x8ec8a3aa };

    /** Platform factory. */
    static ICGraphicsAbstractionManager* CreateInstance();

    /** The application's manager: cached in CApp, created on first use. */
    static inline ICGraphicsAbstractionManager* GetInstance()
    {
        framework::CApp* app = CApplet::m_pApp;
        if (app == NULL) {
            return NULL;
        }
        if (app->m_graphicsAbstractionManager == NULL) {
            ICGraphicsAbstractionManager* found = NULL;
            app->m_singletons->Find(SingletonId, (void**)&found);
            ICGraphicsAbstractionManager* created = found;
            if (created == NULL) {
                created = CreateInstance();
            }
            app->m_graphicsAbstractionManager = created;
        }
        return (ICGraphicsAbstractionManager*)app->m_graphicsAbstractionManager;
    }

    /** Make `abstraction` the current one of `manager`; 0 when that abstraction has no device. */
    static unsigned char SetAbstractionLayer(ICGraphicsAbstractionManager& manager, ICGraphicsAbstract::Abstraction abstraction);

    virtual ~ICGraphicsAbstractionManager() {}  // added by b002
    /** Platform part: create the device graphics (OGLES) and the 2d renderer of `manager`; 1 when both exist. */  // added by b002
    static unsigned char CreateHardwareAbstractionLayer(ICGraphicsAbstractionManager& manager);  // added by b002
};

} } } }

#endif
