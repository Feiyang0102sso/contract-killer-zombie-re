// CKeysetResource.h - a list of resource keys, optionally with their names resolved.
#ifndef CKEYSETRESOURCE_H
#define CKEYSETRESOURCE_H

#include "CClass.h"

namespace com { namespace glu { namespace platform { namespace components {

class CInputStream;
class CIdToObjectRouter;

class CKeysetResource : public core::CClass {
public:
    enum { ClassId = 0xf58ac174 };

    CKeysetResource()
    {
        m_classId = ClassId;
        m_count = 0;
        m_keys = NULL;
        m_names = NULL;
    }

    virtual ~CKeysetResource();

    void Destroy();
    /** uint16 count + count uint32 keys; names are looked up when `router` is given. */
    unsigned char Load(CInputStream& stream, unsigned int type, CIdToObjectRouter* router);

    unsigned int m_count;           // 0x08
    unsigned int* m_keys;           // 0x0c
    char** m_names;                 // 0x10 NULL when loaded without a router
};

inline CKeysetResource::~CKeysetResource()
{
    Destroy();
}

} } } }

#endif
