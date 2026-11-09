// CHandleFactory.h - application singleton that hands out unique numeric handles.
// (header-only; first drafted by b011x, verified against the original by b000x)
// A generated handle has bit 30 set and bit 31 clear, so it never collides with a named handle
// (CHandle::FromName sets bit 31).
#ifndef CHANDLEFACTORY_H
#define CHANDLEFACTORY_H

#include <stddef.h>
#include "CSingleton.h"

namespace com { namespace glu { namespace platform { namespace core {

class CHandleFactory : public components::CSingleton {
public:
    /** Key in the application singleton table: CStringToKey("CHandleFactory"). */
    enum { SingletonId = 0x1a0a5073 };

    CHandleFactory() : components::CSingleton(SingletonId)
    {
        m_lastHandle = 0;
    }

    // `inline` spelled out: the original inlines the CSingleton destructor into both copies
    inline virtual ~CHandleFactory()
    {
        if (CApplet::m_pApp != NULL) {
            CApplet::m_pApp->m_cached2c = NULL;
        }
    }

    /** The application's factory: cached in CApp, created on first use. */
    static inline CHandleFactory* GetInstance()
    {
        framework::CApp* app = CApplet::m_pApp;
        if (app != NULL) {
            if (app->m_cached2c == NULL) {
                CHandleFactory* found = NULL;
                app->m_singletons->Find(SingletonId, (void**)&found);
                CHandleFactory* instance = found;
                if (instance == NULL) {
                    instance = new CHandleFactory();
                }
                app->m_cached2c = instance;
            }
            return (CHandleFactory*)app->m_cached2c;
        }
        return NULL;
    }

    /** A new unique handle: counter in bits 0..29, bit 30 set. */
    inline unsigned int CreateHandle()
    {
        m_lastHandle++;
        return (m_lastHandle & 0x3fffffff) | 0x40000000;
    }

    unsigned int m_lastHandle;      // 0x08 handles created so far
};

} } } }

#endif
