// CResourceFont.h - resource wrapping a graphics::ICFont. (created by b011x)
#ifndef CRESOURCEFONT_H
#define CRESOURCEFONT_H

#include "CResource.h"
#include "ICFont.h"

namespace com { namespace glu { namespace platform { namespace systems {

class CResourceFont : public CResource {
public:
    /** Runtime class id: CStringToKey("CResourceFont"). */
    enum { ClassId = 0x24c74bcc };

    CResourceFont();
    virtual ~CResourceFont();

    virtual void Destroy();
    virtual unsigned int GetSize() const;
    virtual void* GetData() const;
    virtual unsigned int CreateInternal(unsigned int handle, const char* name, components::CInputStream* stream,
                                        unsigned int mimeType, components::CIdToObjectRouter* router,
                                        unsigned char copyName);
    virtual void ResetMembers();

    graphics::ICFont* m_font;    // 0x24
};

} } } }

#endif
