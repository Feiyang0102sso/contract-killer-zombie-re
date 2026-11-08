// CRegistryAccelerateHandleQuery.h - registry accelerator: keeps a handle -> element hash in
// step with the registry so that handle queries do not walk the list (created by b000x).
// The members are defined inline after the class: the destructor is the key function, so the
// vtable and every virtual are emitted (weak) in each file that includes this header.
#ifndef CREGISTRYACCELERATEHANDLEQUERY_H
#define CREGISTRYACCELERATEHANDLEQUERY_H

#include <stddef.h>
#include "CHash.h"
#include "CRegistry.h"
#include "CRegistryItr.h"

namespace com { namespace glu { namespace platform { namespace systems {

class CRegistryAccelerateHandleQuery : public ICRegistryQuery {
public:
    enum { ClassId = 0x21d748ff };

    /** The hash gets at least `buckets` buckets and a growable pool of `poolSize` nodes. */
    inline CRegistryAccelerateHandleQuery(unsigned long buckets, unsigned long poolSize)  // `inline` added by b011x
    {
        m_classId = ClassId;
        m_hash = NULL;
        m_hash = new components::CHash();
        m_hash->Init(buckets, poolSize, 1);
    }

    virtual ~CRegistryAccelerateHandleQuery();

    virtual void Add(CSystemElement& element);
    virtual unsigned char Remove(CSystemElement& element);
    virtual void RemoveAll();
    /** A class id alone is answered by the registry, a handle by the hash. */
    virtual unsigned char Query(unsigned int elementClassId, unsigned int handle, CSystemElement** result);
    /** Attach to `registry` and hash its current elements. */
    virtual void SyncWithRegistry(CRegistry& registry);

    components::CHash* m_hash;      // 0x10 handle -> CSystemElement*

private:
    /** Element hashed under `handle` into *result (NULL when absent); true when found. */
    unsigned char FindHandle(unsigned int handle, CSystemElement** result)
    {
        CSystemElement* element;
        unsigned char found = m_hash->Find(handle, (void**)&element);
        // ?: on purpose: the original selects the stored value
        *result = found ? element : NULL;
        return found;
    }
};

inline CRegistryAccelerateHandleQuery::~CRegistryAccelerateHandleQuery()
{
    if (m_hash != NULL) {
        delete m_hash;
    }
}

inline void CRegistryAccelerateHandleQuery::Add(CSystemElement& element)
{
    m_inRegistryCall = 1;
    m_registry->Add(element);
    m_inRegistryCall = 0;
    m_hash->Insert(element.GetHandle(), &element);
}

inline unsigned char CRegistryAccelerateHandleQuery::Remove(CSystemElement& element)
{
    m_inRegistryCall = 1;
    unsigned char removed = m_registry->Remove(element);
    m_inRegistryCall = 0;
    if (removed) {
        m_hash->Remove(element.GetHandle());
    }
    return removed;
}

inline void CRegistryAccelerateHandleQuery::RemoveAll()
{
    m_inRegistryCall = 1;
    m_registry->RemoveAll();
    m_inRegistryCall = 0;
    m_hash->RemoveAll();
}

inline unsigned char CRegistryAccelerateHandleQuery::Query(unsigned int elementClassId, unsigned int handle,
                                                           CSystemElement** result)
{
    if (handle != 0 && elementClassId != 0) {
        if (FindHandle(handle, result)) {
            // registered under another class id: not a match
            if ((*result)->GetElementClassId() != elementClassId) {
                *result = NULL;
            }
        }
        return *result != NULL;
    }
    if (handle != 0) {
        FindHandle(handle, result);
        return *result != NULL;
    }
    if (elementClassId == 0) {
        return 0;
    }
    m_inRegistryCall = 1;
    unsigned char found = m_registry->Query(elementClassId, handle, result);
    m_inRegistryCall = 0;
    return found;
}

inline void CRegistryAccelerateHandleQuery::SyncWithRegistry(CRegistry& registry)
{
    m_registry = &registry;
    for (CRegistryItr element(registry); !element.IsDone(); element++) {
        m_hash->Insert((*element).GetHandle(), &*element);
    }
}

} } } }

#endif
