// CRenderSurface_SW.h - render surface whose pixels live in system memory.
// (created by b039x, the owner)
#ifndef CRENDERSURFACE_SW_H
#define CRENDERSURFACE_SW_H

#include "CRenderSurface.h"

namespace com { namespace glu { namespace platform { namespace graphics {

class CRenderSurface_SW : public CRenderSurface {
public:
    CRenderSurface_SW();
    virtual ~CRenderSurface_SW();

    virtual ICGraphicsAbstract::Abstraction GetAbstraction() const
    {
        return ICGraphicsAbstract::Abstraction_Software;
    }
    virtual void UpdateTextureWithTarget()
    {
    }
    virtual void Lock(unsigned char** data, int& pitch, LockType lockType, const components::CRectangle<short>* area);
    virtual void Unlock();
    virtual components::ColorFormat GetColorFormat();
    virtual unsigned char GetWidthAndHeight(unsigned int& width, unsigned int& height);
    virtual unsigned char GetDescription(Description& description);

    /** Pitch and address of `area` (NULL: the whole image) in m_buffers[buffer]. */
    unsigned char FetchFromBuffer(unsigned int buffer, unsigned char** data, int& pitch,
                                  const components::CRectangle<short>* area);
};

} } } }

#endif
