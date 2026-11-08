// CHash.h - chained hash table from 32-bit keys to pointers; nodes come from a CPool.
#ifndef CHASH_H
#define CHASH_H

#include "CClass.h"

namespace com { namespace glu { namespace platform { namespace components {

class CPool;

class CHash : public core::CClass {
public:
    /** One chain node (allocated from m_pool, never constructed). */
    struct Entry : public core::CClass {
        unsigned int m_key;         // 0x08
        void* m_value;              // 0x0c
        Entry* m_next;              // 0x10
    };

    CHash()
    {
        m_classId = 0x0017e658;
        // store order of the original: pool, buckets, bucket count (fx1)
        m_pool = NULL;
        m_buckets = NULL;
        m_bucketCount = 0;
    }

    virtual ~CHash()
    {
        Destroy();
    }

    /** Allocate at least `minBuckets` buckets (rounded up to a prime) and a node pool. */
    unsigned char Init(unsigned long minBuckets, unsigned long poolSize, unsigned char poolGrowable);
    void Destroy();
    void Insert(unsigned int key, void* value);
    void Remove(unsigned int key);
    void RemoveAll();
    /** True when `key` is present; its value goes to *outValue when outValue is not NULL. */
    unsigned char Find(unsigned int key, void** outValue) const;

    CPool* m_pool;                  // 0x08 node allocator
    int m_bucketCount;              // 0x0c
    Entry** m_buckets;              // 0x10
};

} } } }

#endif
