// CSystemElement.h - element of a CSystem / CRegistry: a priority-ordered node naming an object
// by (class id, handle). (first drafted by b000x as PROVISIONAL; completed by b011x, the owner)
#ifndef CSYSTEMELEMENT_H
#define CSYSTEMELEMENT_H

#include <stddef.h>
#include "CPriorityClass.h"

namespace com { namespace glu { namespace platform { namespace systems {

class CSystemElement : public core::CPriorityClass {
public:
    CSystemElement()
    {
        m_reserved18 = 0;
        m_handle = 0;
        m_elementClassId = 0;
        m_object = NULL;
    }

    unsigned int GetHandle() const
    {
        return m_handle;
    }

    unsigned int GetElementClassId() const
    {
        return m_elementClassId;
    }

    void* GetObject() const
    {
        return m_object;
    }

    unsigned int m_handle;              // 0x0c key of the element (CHandle)
    unsigned int m_elementClassId;      // 0x10 class id the element is registered under
    void* m_object;                     // 0x14 the registered object
    unsigned int m_reserved18;          // 0x18 never used by the platform code
    unsigned char m_isSentinel;         // 0x1c set on the end marker embedded in a CRegistry
    CSystemElement* m_prev;             // 0x20 towards the registry head
    CSystemElement* m_next;             // 0x24 towards the registry tail
};

} } } }

#endif
