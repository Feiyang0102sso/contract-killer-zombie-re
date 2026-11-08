// CResourceKeyset.h - resource wrapping a components::CKeysetResource. (created by b011x)
#ifndef CRESOURCEKEYSET_H
#define CRESOURCEKEYSET_H

#include "CResource.h"
#include "CKeysetResource.h"

namespace com { namespace glu { namespace platform { namespace systems {

class CResourceKeyset : public CResource {
public:
    /** Runtime class id: CStringToKey("CResourceKeyset"). */
    enum { ClassId = 0xe790bd00 };

    CResourceKeyset();
    virtual ~CResourceKeyset();

    virtual void Destroy();
    virtual unsigned int GetSize() const;
    virtual void* GetData() const;
    virtual unsigned int CreateInternal(unsigned int handle, const char* name, components::CInputStream* stream,
                                        unsigned int mimeType, components::CIdToObjectRouter* router,
                                        unsigned char copyName);
    virtual void ResetMembers();

    components::CKeysetResource* m_keyset; // 0x24
};

} } } }

#endif
