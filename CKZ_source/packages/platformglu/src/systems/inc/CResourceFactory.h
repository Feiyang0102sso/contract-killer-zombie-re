// CResourceFactory.h - creates the CResource subclass matching a mime type (application
// singleton). (created by b011x)
#ifndef CRESOURCEFACTORY_H
#define CRESOURCEFACTORY_H

#include <stddef.h>
#include "CSingleton.h"

namespace com { namespace glu { namespace platform { namespace systems {

class CResource;

class CResourceFactory : public components::CSingleton {
public:
    /** Key in the application singleton table. */
    enum { SingletonId = 0x7451aa08 };

    CResourceFactory() : components::CSingleton(SingletonId) {}

    virtual ~CResourceFactory() {}

    /** Resource for `mimeType` in *resource; returns 1 when the type is known. */
    virtual unsigned char CreateResource(unsigned int mimeType, CResource** resource);

    /** The application's factory, created on first use. */
    static inline CResourceFactory* GetInstance()
    {
        CResourceFactory* found = NULL;
        CApplet::m_pApp->m_singletons->Find(SingletonId, (void**)&found);
        CResourceFactory* factory = found;
        if (factory == NULL) {
            factory = new CResourceFactory();
        }
        return factory;
    }

    /** New resource object for `mimeType`, NULL when the type is unknown. */
    CResource* CreateResource(unsigned int mimeType);
};

} } } }

#endif
