// ICGraphicsResource.h - base of every object a graphics device creates (created by b037x).
// Constructor and destructor live in graphics/src/CGraphics.cpp.
#ifndef ICGRAPHICSRESOURCE_H
#define ICGRAPHICSRESOURCE_H

#include "ICGraphicsAbstractionManager.h"

namespace com { namespace glu { namespace platform { namespace graphics {

/**
 * Base of the objects a device creates (surfaces, buffers, shaders, display programs). Every
 * resource registers itself in the resource registry of the current device for its lifetime, so
 * that the device can save and recreate them when the context is lost.
 */
class ICGraphicsResource {
public:
    explicit ICGraphicsResource(unsigned int resourceType);
    virtual ~ICGraphicsResource();

    /** The device abstraction the resource belongs to. */
    virtual ICGraphicsAbstract::Abstraction GetAbstraction() const = 0;

    unsigned int m_resourceType;    // 0x04 key of the resource kind (registry element class id)
};

} } } }

#endif
