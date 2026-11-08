// CSystem.h - an executable container of CSystemElement objects (created by b011x).
// The application keeps its systems in a root system (CApp +0x14); every system can be queried
// by element class id and / or handle. CRegistry is the concrete implementation.
#ifndef CSYSTEM_H
#define CSYSTEM_H

#include "CExecutable.h"
#include "CSystemElement.h"

namespace com { namespace glu { namespace platform { namespace systems {

class CSystem : public components::CExecutable {
public:
    /** Runtime class id: CStringToKey("CSystem"). */
    enum { ClassId = 0x364e423d };

    /** Systems run until they are removed. */
    CSystem() : components::CExecutable(InfinitelyRunning)
    {
        m_classId = ClassId;
    }

    virtual ~CSystem() {}

    /** A new element for `object`; the caller adds it with Add(). */
    virtual CSystemElement* CreateSystemElement(unsigned int handle, void* object, unsigned int elementClassId,
                                                int priority) const = 0;
    virtual CSystemElement* CreateSystemElement(unsigned int handle, int priority) const = 0;
    virtual void Add(CSystemElement& element) = 0;
    /** Unlink `element` (it is not deleted); returns 1 when it was removed. */
    virtual unsigned char Remove(CSystemElement& element) = 0;
    virtual void RemoveAll() = 0;
    /** First element matching the non-zero keys; *result is set when found. */
    virtual unsigned char Query(unsigned int elementClassId, unsigned int handle, CSystemElement** result) = 0;
};

} } } }

#endif
