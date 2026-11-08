// CResourceManager.h - common part of the resource managers: the handle -> CResource hash, the
// archive version and the debug switches. (first drafted by b000x as PROVISIONAL with the
// iterator interface, now in ICResourceManager.h; completed by b011x, the owner)
#ifndef CRESOURCEMANAGER_H
#define CRESOURCEMANAGER_H

#include <stddef.h>
#include "ICResourceManager.h"
#include "CSingleton.h"
#include "CStrWChar.h"
#include "CHash.h"

namespace com { namespace glu { namespace platform {

namespace components {
class CIdToObjectRouter;
}

namespace systems {

class CEventListener;

class CResourceManagerLegacy : public ICResourceManager, public components::CSingleton {
public:
    inline CResourceManagerLegacy() : components::CSingleton(SingletonId)
    {
        m_version = "";
        m_versionMajor = 0;
        m_versionMinor = 0;
        m_router = NULL;
        m_debugListener = NULL;
    }

    virtual ~CResourceManagerLegacy();

    virtual void Destroy();
    virtual unsigned char GetResource(unsigned int handle, CResource** resource);
    virtual const components::CStrWChar& GetVersion() const;
    virtual void GetVersion(unsigned int& major, unsigned int& minor) const;
    /** Original Legacy adds an internal creation slot at +0xb8; the secondary vtable destructor entries start at +0xc4. */
    virtual unsigned int CreateResource(CResource** resource, unsigned int handle, const char* name,
                                        unsigned int mimeType, components::CInputStream* stream) = 0;

    /** Resource debug output switches (toggled by the ResourceDebug events). */
    static unsigned char m_dbgLoggingEnabled;
    static unsigned char m_dbgVerboseLoggingEnabled;

    components::CStrWChar m_version;            // 0x0c
    unsigned int m_versionMajor;                // 0x1c
    unsigned int m_versionMinor;                // 0x20
    components::CHash m_resources;              // 0x24 handle -> first CResource of the handle
    components::CIdToObjectRouter* m_router;    // 0x38 resolves resource references while loading
    CEventListener* m_debugListener;            // 0x3c
};

} } } }

#endif
