// CResourceDIB.h - resource wrapping a graphics::CDIB. (created by b011x)
#ifndef CRESOURCEDIB_H
#define CRESOURCEDIB_H

#include "CResource.h"
#include "CDIB.h"

namespace com { namespace glu { namespace platform { namespace systems {

class CResourceDIB : public CResource {
public:
    /** Runtime class id: CStringToKey("CResourceDIB"). */
    enum { ClassId = 0x824d74f7 };

    CResourceDIB();
    virtual ~CResourceDIB();

    virtual void Destroy();
    virtual unsigned int GetSize() const;
    virtual void* GetData() const;
    virtual unsigned int CreateInternal(unsigned int handle, const char* name, components::CInputStream* stream,
                                        unsigned int mimeType, components::CIdToObjectRouter* router,
                                        unsigned char copyName);
    virtual void ResetMembers();

    graphics::CDIB* m_dib;       // 0x24
};

} } } }

#endif
