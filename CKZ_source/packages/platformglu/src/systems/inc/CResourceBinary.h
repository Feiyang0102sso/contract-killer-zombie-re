// CResourceBinary.h - resource wrapping a components::CBinary. (created by b011x)
#ifndef CRESOURCEBINARY_H
#define CRESOURCEBINARY_H

#include "CResource.h"
#include "CBinary.h"

namespace com { namespace glu { namespace platform { namespace systems {

class CResourceBinary : public CResource {
public:
    /** Runtime class id: CStringToKey("CResourceBinary"). */
    enum { ClassId = 0xe70dde7d };

    CResourceBinary();
    virtual ~CResourceBinary();

    virtual void Destroy();
    virtual unsigned int GetSize() const;
    virtual void* GetData() const;
    virtual unsigned int CreateInternal(unsigned int handle, const char* name, components::CInputStream* stream,
                                        unsigned int mimeType, components::CIdToObjectRouter* router,
                                        unsigned char copyName);
    virtual void ResetMembers();

    components::CBinary* m_binary; // 0x24
};

} } } }

#endif
