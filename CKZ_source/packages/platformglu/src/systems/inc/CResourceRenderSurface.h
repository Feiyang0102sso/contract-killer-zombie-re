// CResourceRenderSurface.h - resource wrapping a graphics::ICRenderSurface. (created by b011x)
#ifndef CRESOURCERENDERSURFACE_H
#define CRESOURCERENDERSURFACE_H

#include "CResource.h"
#include "ICRenderSurface.h"

namespace com { namespace glu { namespace platform { namespace systems {

class CResourceRenderSurface : public CResource {
public:
    /** Runtime class id: CStringToKey("CResourceRenderSurface"). */
    enum { ClassId = 0x23441af5 };

    CResourceRenderSurface();
    virtual ~CResourceRenderSurface();

    virtual void Destroy();
    virtual unsigned int GetSize() const;
    virtual void* GetData() const;
    virtual unsigned int CreateInternal(unsigned int handle, const char* name, components::CInputStream* stream,
                                        unsigned int mimeType, components::CIdToObjectRouter* router,
                                        unsigned char copyName);
    virtual void ResetMembers();

    graphics::ICRenderSurface* m_surface; // 0x24
};

} } } }

#endif
