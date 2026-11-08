// CRegistry.h - priority-sorted list of CSystemElement objects, optionally accelerated by a
// query object (see CRegistryAccelerateHandleQuery). The inline members follow the original
// copies emitted in CGraphicsAbstractionManager_Cocoa.o.
// (first drafted by b000x as PROVISIONAL; completed by b011x, the owner)
#ifndef CREGISTRY_H
#define CREGISTRY_H

#include <stddef.h>
#include "CSystem.h"
#include "CRegistryElement.h"

namespace com { namespace glu { namespace platform { namespace systems {

class CRegistry;

/**
 * Interface of a registry accelerator (CRegistryAccelerateHandleQuery). The original name of
 * this base is not in the symbols. While the accelerator forwards a call to the registry,
 * m_inRegistryCall is set so that the registry does not call back into it.
 */
class ICRegistryQuery : public core::CClass {
public:
    /** The registry and recursion flag are initialized before the derived vtable. */
    ICRegistryQuery()
    {
        m_registry = NULL;
        m_inRegistryCall = 0;
    }

    virtual ~ICRegistryQuery() {}

    virtual void Add(CSystemElement& element) = 0;
    virtual unsigned char Remove(CSystemElement& element) = 0;
    virtual void RemoveAll() = 0;
    virtual unsigned char Query(unsigned int elementClassId, unsigned int handle, CSystemElement** result) = 0;
    virtual void SyncWithRegistry(CRegistry& registry) = 0;

    unsigned char m_inRegistryCall;     // 0x08
    CRegistry* m_registry;          // 0x0c
};

class CRegistry : public CSystem {
public:
    /** Runtime class id: CStringToKey("CRegistry"). */
    enum { ClassId = 0x431e438f };

    CRegistry();

    virtual ~CRegistry();

    virtual void OnExecute()
    {
    }

    virtual CSystemElement* CreateSystemElement(unsigned int handle, void* object, unsigned int elementClassId,
                                                int priority) const
    {
        return new CRegistryElement(handle, object, elementClassId, priority);
    }

    virtual CSystemElement* CreateSystemElement(unsigned int handle, int priority) const
    {
        return new CRegistryElement(handle, NULL, 0, priority);
    }

    /** Insert `element` after the last element whose priority is not lower. */
    virtual void Add(CSystemElement& element);
    virtual unsigned char Remove(CSystemElement& element);

    virtual void RemoveAll()
    {
        if (m_accelerator != NULL && !m_accelerator->m_inRegistryCall) {
            m_accelerator->RemoveAll();
            return;
        }
        m_head = GetSentinel();
        m_tail = GetSentinel();
    }

    virtual unsigned char Query(unsigned int elementClassId, unsigned int handle, CSystemElement** result)
    {
        if (m_accelerator != NULL && !m_accelerator->m_inRegistryCall) {
            return m_accelerator->Query(elementClassId, handle, result);
        }
        if (elementClassId != 0 && handle != 0) {
            return QueryClassIdAndHandle(elementClassId, handle, (CRegistryElement**)result);
        }
        if (elementClassId != 0) {
            return QueryClassId(elementClassId, (CRegistryElement**)result);
        }
        if (handle != 0) {
            return QueryHandle(handle, (CRegistryElement**)result);
        }
        return 0;
    }

    unsigned char QueryClassId(unsigned int elementClassId, CRegistryElement** result);
    unsigned char QueryHandle(unsigned int handle, CRegistryElement** result);
    unsigned char QueryClassIdAndHandle(unsigned int elementClassId, unsigned int handle, CRegistryElement** result);

    /** End marker of the element list (the embedded m_sentinel). */
    CSystemElement* GetSentinel()
    {
        return &m_sentinel;
    }

    /** True when no element is registered. */
    bool IsEmpty()
    {
        return m_head == GetSentinel();
    }

    /** Replace the accelerator (the old one is deleted) and let it index the current elements. */
    inline void SetAccelerator(ICRegistryQuery* accelerator)
    {
        if (m_accelerator != NULL) {
            delete m_accelerator;
            m_accelerator = NULL;
        }
        if (accelerator != NULL) {
            m_accelerator = accelerator;
            accelerator->SyncWithRegistry(*this);
        }
    }

    CRegistryElement m_sentinel;        // 0x1c end marker of the element list
    CSystemElement* m_head;             // 0x44 first element (sentinel when empty)
    CSystemElement* m_tail;             // 0x48 last element (sentinel when empty)
    ICRegistryQuery* m_accelerator;     // 0x4c optional, owned
};

inline CRegistry::~CRegistry()
{
    if (m_accelerator != NULL) {
        delete m_accelerator;
    }
}

} } } }

#endif
