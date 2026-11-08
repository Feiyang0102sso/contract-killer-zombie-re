// CResourceShader.h - resource wrapping a graphics::ICShader. (created by b011x)
#ifndef CRESOURCESHADER_H
#define CRESOURCESHADER_H

#include "CResource.h"
#include "ICShader.h"

namespace com { namespace glu { namespace platform { namespace systems {

class CResourceShader : public CResource {
public:
    /** Runtime class id: CStringToKey("CResourceShader"). */
    enum { ClassId = 0xe61c2a06 };

    CResourceShader();
    virtual ~CResourceShader();

    virtual void Destroy();
    virtual unsigned int GetSize() const;
    virtual void* GetData() const;
    virtual unsigned int CreateInternal(unsigned int handle, const char* name, components::CInputStream* stream,
                                        unsigned int mimeType, components::CIdToObjectRouter* router,
                                        unsigned char copyName);
    virtual void ResetMembers();

    graphics::ICShader* m_shader; // 0x24
};

} } } }

#endif
