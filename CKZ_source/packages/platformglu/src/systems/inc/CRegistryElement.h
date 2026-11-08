// CRegistryElement.h - the CSystemElement a CRegistry creates for a registered object.
// (first drafted by b000x as PROVISIONAL; completed by b011x, the owner)
#ifndef CREGISTRYELEMENT_H
#define CREGISTRYELEMENT_H

#include "CSystemElement.h"

namespace com { namespace glu { namespace platform { namespace systems {

class CRegistryElement : public CSystemElement {
public:
    /** Runtime class id: CStringToKey("CRegistryElement"). */
    enum { ClassId = 0xb79257b5 };

    /** Empty element (also the end marker of a registry). */
    CRegistryElement();
    CRegistryElement(unsigned int handle, void* object, unsigned int elementClassId, int priority);

    virtual ~CRegistryElement() {}
};

} } } }

#endif
