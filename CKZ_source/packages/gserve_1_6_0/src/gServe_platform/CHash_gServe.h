// CHash_gServe.h - gserve's copy of the platform chained hash table (32-bit key -> pointer).
// Unlike CHash the constructor only sets the class id; the owner calls Init() itself.
// (b020; the class is constructed in locking/CNGSLockManager.cpp)
#ifndef CHASH_GSERVE_H
#define CHASH_GSERVE_H

#include "CClass.h"

namespace com { namespace glu { namespace platform { namespace components {
class CPool;
} } } }

class CHash_gServe : public com::glu::platform::core::CClass {
public:
    /** Runtime class id (constant stored by the constructor). */
    static const unsigned int ClassId = 0xd92d5b60;

    /** One chain node (allocated from m_pool, never constructed). */
    struct Entry : public com::glu::platform::core::CClass {
        unsigned int m_key;         // 0x08
        void* m_value;              // 0x0c
        Entry* m_next;              // 0x10
        unsigned int m_reserved14;  // 0x14 never touched here; the pool block size is 0x18
    };

    CHash_gServe()
    {
        m_classId = ClassId;
    }

    virtual ~CHash_gServe()
    {
        Destroy();
    }

    /** Allocate at least `minBuckets` buckets (rounded up to a prime) and a node pool. */
    unsigned char Init(unsigned long minBuckets, unsigned long poolSize, unsigned char poolGrowable);
    void Destroy();
    /** Return every node to the pool (the stored values are not touched). */
    void RemoveAll();

    com::glu::platform::components::CPool* m_pool;  // 0x08 node allocator
    int m_bucketCount;                              // 0x0c
    Entry** m_buckets;                              // 0x10
};

#endif
