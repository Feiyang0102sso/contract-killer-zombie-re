// CResourceMedia.h - resource wrapping a components::CMedia. (created by b011x)
#ifndef CRESOURCEMEDIA_H
#define CRESOURCEMEDIA_H

#include "CResource.h"
#include "CMedia.h"

namespace com { namespace glu { namespace platform { namespace systems {

class CResourceMedia : public CResource {
public:
    /** Runtime class id: CStringToKey("CResourceMedia"). */
    enum { ClassId = 0x4f7f1773 };

    CResourceMedia();
    virtual ~CResourceMedia();

    virtual void Destroy();
    virtual unsigned int GetSize() const;
    virtual void* GetData() const;
    virtual unsigned int CreateInternal(unsigned int handle, const char* name, components::CInputStream* stream,
                                        unsigned int mimeType, components::CIdToObjectRouter* router,
                                        unsigned char copyName);
    virtual void ResetMembers();

    components::CMedia* m_media; // 0x24
};

} } } }

#endif
