// Owner: tools/mqaa/CObjectMap.cpp (b028x; started as a b016x PROVISIONAL)
// CObjectMap.h - one gServe message: a shared value tree plus the message id the queue assigned.
// Building helpers (beginObject/endObject/addEntry) walk m_current through the tree.
#ifndef COBJECTMAP_H
#define COBJECTMAP_H

#include <stddef.h>
#include "CStrWChar.h"
#include "CSharedPtr_gServe.h"
#include "CObjectMapValue.h"

class CObjectMap {
public:
    CObjectMap()
        : m_messageId(0)
        , m_state(1)
        , m_current(NULL)
    {
    }

    CObjectMap(const CObjectMap& other);
    /** Root = clone of `root` (NULL: empty map). */
    explicit CObjectMap(const CObjectMapValue* root);

    /** Share `other`'s tree (inlined into TCList::Push*; the building cursor is reset). */
    inline CObjectMap& operator=(const CObjectMap& other)  // changed by b020: explicit inline (inlined into CVector<CObjectMap>::Set, verified MATCH)
    {
        if (m_root.get() != other.m_root.get()) {  // changed by b020: compare through get() (verified MATCH)
            m_current = NULL;
            m_messageId = other.m_messageId;
            m_state = other.m_state;
            m_root = other.m_root;
        }
        return *this;
    }

    /** Open an object: the root when empty, otherwise a new element of the current array. */
    unsigned char beginObject();
    /** Open an object stored under `key` in the current object. */
    unsigned char beginObject(const com::glu::platform::components::CStrWChar& key);
    /** Close the current object (back to its parent). */
    bool endObject();
    unsigned char addEntry(const com::glu::platform::components::CStrWChar& key, const CObjectMapInt& value);
    /**
     * Walk `path` (keys / array indices separated by `separator`) from the root;
     * NULL when any step is missing.
     */
    CObjectMapValue* getDataAt(const com::glu::platform::components::CStrWChar& path,
                               const com::glu::platform::components::CStrWChar& separator) const;

    CObjectMapValue* getRoot() const
    {
        return m_root.m_ptr;
    }

    unsigned int m_messageId;                       // +0x00 id assigned by CNetMessageQueue_gServe::queueMessage
    CSharedPtr_gServe<CObjectMapValue> m_root;      // +0x04
    int m_state;                                    // +0x0c 1 after construction, copied as is (meaning unknown)
    CObjectMapValue* m_current;                     // +0x10 node being built
};

#endif
