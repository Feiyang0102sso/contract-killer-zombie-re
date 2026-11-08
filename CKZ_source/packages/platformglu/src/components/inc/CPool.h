// CPool.h - fixed-size block pool for CClass objects.
// Blocks are handed out from one contiguous allocation; freed blocks go to a free list.
// A growable pool chains an overflow pool (m_next) when it runs out; an overflow pool that
// becomes completely free unlinks and deletes itself.
// (first drafted by b005 as PROVISIONAL; completed by b006, the owner)
#ifndef CPOOL_H
#define CPOOL_H

#include "CClass.h"

namespace com { namespace glu { namespace platform { namespace components {

class CPool : public core::CClass {
public:
    /** Runtime class id: CStringToKey("CPool"). */
    static const unsigned int ClassId = 0x0016699c;

    CPool();
    /** `count` blocks of `blockSize` bytes; `growable` chains another pool when full. */
    CPool(unsigned long count, unsigned long blockSize, unsigned char growable);
    virtual ~CPool();

    void Init(unsigned long count, unsigned long blockSize, unsigned char growable);
    /** Allocate the block storage and the free list for the current geometry. */
    void Allocate();

    /** A block (the size argument is only forwarded to overflow pools), NULL when exhausted. */
    virtual void* New(unsigned long size);
    /** Return a block obtained from New() (here or in an overflow pool). */
    virtual void Delete(core::CClass* object);

    CPool* m_parent;                // 0x08 pool that chained this one
    CPool* m_next;                  // 0x0c overflow pool
    unsigned char* m_blocks;        // 0x10 block storage
    void** m_freeList;              // 0x14 returned blocks
    unsigned int m_freeCount;       // 0x18
    unsigned long m_count;          // 0x1c blocks in this pool
    unsigned long m_blockSize;      // 0x20
    unsigned char* m_nextBlock;     // 0x24 next never-used block
    unsigned char* m_end;           // 0x28 end of m_blocks
    unsigned char m_growable;       // 0x2c

private:
    /** Overflow pool chained behind `parent`: same geometry, always growable. */
    explicit CPool(CPool* parent)
    {
        m_classId = ClassId;
        m_blocks = NULL;
        m_freeList = NULL;
        m_nextBlock = NULL;
        m_end = NULL;
        m_freeCount = 0;
        m_parent = parent;
        m_next = NULL;
        m_count = parent->m_count;
        m_blockSize = parent->m_blockSize;
        m_freeCount = 0;
        m_growable = 1;
        Allocate();
    }
};

} } } }

#endif
