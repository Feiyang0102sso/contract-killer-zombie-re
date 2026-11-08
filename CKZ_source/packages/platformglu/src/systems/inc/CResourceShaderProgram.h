// CResourceShaderProgram.h - resource wrapping a graphics::ICShaderProgram. (created by b011x)
#ifndef CRESOURCESHADERPROGRAM_H
#define CRESOURCESHADERPROGRAM_H

#include "CResource.h"
#include "ICShaderProgram.h"

namespace com { namespace glu { namespace platform { namespace systems {

class CResourceShaderProgram : public CResource {
public:
    /** Runtime class id: CStringToKey("CResourceShaderProgram"). */
    enum { ClassId = 0x2028c6dd };

    CResourceShaderProgram();
    virtual ~CResourceShaderProgram();

    virtual void Destroy();
    virtual unsigned int GetSize() const;
    virtual void* GetData() const;
    virtual unsigned int CreateInternal(unsigned int handle, const char* name, components::CInputStream* stream,
                                        unsigned int mimeType, components::CIdToObjectRouter* router,
                                        unsigned char copyName);
    virtual void ResetMembers();

    graphics::ICShaderProgram* m_program; // 0x24
};

} } } }

#endif
