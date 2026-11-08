// CResourceDataBlockFileAccess.h - singleton remembering the path of the open resource archive
// (created by b005, header-only).
#ifndef CRESOURCEDATABLOCKFILEACCESS_H
#define CRESOURCEDATABLOCKFILEACCESS_H

#include "CSingleton.h"
#include "CStrWChar.h"

namespace com { namespace glu { namespace platform { namespace components {

class CResourceDataBlockFileAccess : public CSingleton {
public:
    enum { SingletonId = 0x77817b0b };

    CResourceDataBlockFileAccess() : CSingleton(SingletonId) {}

    virtual ~CResourceDataBlockFileAccess();

    /** The application's instance, created on first use. */
    static inline CResourceDataBlockFileAccess* GetInstance()
    {
        void* found = NULL;
        CApplet::m_pApp->m_singletons->Find(SingletonId, &found);
        CResourceDataBlockFileAccess* instance = (CResourceDataBlockFileAccess*)found;
        if (instance == NULL) {
            instance = new CResourceDataBlockFileAccess();
        }
        return instance;
    }

    CStrWChar m_path;   // 0x08 archive file path
};

inline CResourceDataBlockFileAccess::~CResourceDataBlockFileAccess()
{
}

} } } }

#endif
