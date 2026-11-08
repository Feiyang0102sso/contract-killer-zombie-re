// CRegistryItr.h - forward iterator over the elements of a CRegistry (created by b000x).
#ifndef CREGISTRYITR_H
#define CREGISTRYITR_H

#include "CRegistry.h"

namespace com { namespace glu { namespace platform { namespace systems {

class CRegistryItr {
public:
    explicit CRegistryItr(CRegistry& registry)
    {
        m_registry = &registry;
        m_current = registry.m_head;
    }

    /** Past the last element. */
    bool IsDone() const
    {
        return m_current == m_registry->GetSentinel();
    }

    CSystemElement& operator*() const
    {
        return *m_current;
    }

    /** Next element (stays put on a node without successor). */
    void operator++(int)
    {
        if (m_current->m_next != NULL) {
            m_current = m_current->m_next;
        }
    }

    CRegistry* m_registry;
    CSystemElement* m_current;
};

} } } }

#endif
