// CResourceStrWChar.h - resource wrapping a components::CStrWChar. (created by b011x)
#ifndef CRESOURCESTRWCHAR_H
#define CRESOURCESTRWCHAR_H

#include "CResource.h"
#include "CStrWChar.h"

namespace com { namespace glu { namespace platform { namespace systems {

class CResourceStrWChar : public CResource {
public:
    /** Runtime class id: CStringToKey("CResourceStrWChar"). */
    enum { ClassId = 0x012b7b64 };

    CResourceStrWChar();
    virtual ~CResourceStrWChar();

    virtual void Destroy();
    virtual unsigned int GetSize() const;
    virtual void* GetData() const;
    virtual unsigned int CreateInternal(unsigned int handle, const char* name, components::CInputStream* stream,
                                        unsigned int mimeType, components::CIdToObjectRouter* router,
                                        unsigned char copyName);
    virtual void ResetMembers();

    components::CStrWChar* m_string; // 0x24
};

} } } }

#endif
