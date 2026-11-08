// CPool.cpp - fixed-size block pool.
#include "CPool.h"

namespace com { namespace glu { namespace platform { namespace components {

CPool::CPool()
{
    m_classId = ClassId;
    m_parent = NULL;
    m_next = NULL;
    m_blocks = NULL;
    m_freeList = NULL;
    m_freeCount = 0;
    m_count = 0;
    m_blockSize = 0;
    m_nextBlock = NULL;
    m_end = NULL;
    m_growable = 0;
}

CPool::CPool(unsigned long count, unsigned long blockSize, unsigned char growable)
{
    m_classId = ClassId;
    m_blocks = NULL;
    m_freeList = NULL;
    m_nextBlock = NULL;
    m_end = NULL;
    m_parent = NULL;
    m_next = NULL;
    m_freeCount = 0;
    m_count = count;
    m_blockSize = blockSize;
    m_growable = growable;
    Allocate();
}

void CPool::Init(unsigned long count, unsigned long blockSize, unsigned char growable)
{
    m_blocks = NULL;
    m_freeList = NULL;
    m_end = NULL;
    m_freeCount = 0;
    m_parent = NULL;
    m_next = NULL;
    m_count = count;
    m_blockSize = blockSize;
    m_nextBlock = NULL;
    m_growable = growable;
    Allocate();
}

void CPool::Allocate()
{
    m_blocks = new unsigned char[m_count * m_blockSize];
    m_freeList = new void*[m_count];
    m_nextBlock = m_blocks;
    m_end = m_blocks + m_blockSize * m_count;
}

CPool::~CPool()
{
    if (m_next != NULL) {
        delete m_next;
    }
    if (m_blocks != NULL) {
        delete[] m_blocks;
    }
    if (m_freeList != NULL) {
        delete[] m_freeList;
    }
}

void* CPool::New(unsigned long size)
{
    void* block = NULL;
    if (m_freeCount == 0) {
        if (m_nextBlock < m_end) {
            block = m_nextBlock;
            m_nextBlock += m_blockSize;
        } else if (m_next != NULL) {
            block = m_next->New(size);
        } else if (m_growable) {
            m_next = new CPool(this);
            if (m_next != NULL) {
                block = m_next->New(size);
            }
        }
    } else {
        m_freeCount--;
        block = m_freeList[m_freeCount];
    }
    return block;
}

void CPool::Delete(core::CClass* object)
{
    if (object == NULL) {
        return;
    }

    if ((unsigned char*)object < m_blocks || (unsigned char*)object >= m_end) {
        // not ours: it came from an overflow pool
        if (m_next != NULL) {
            m_next->Delete(object);
        }
        return;
    }

    m_freeList[m_freeCount++] = object;

    // an overflow pool whose handed-out blocks all came back unlinks and deletes itself
    if (m_parent != NULL && m_next == NULL) {
        if (m_freeCount * m_blockSize == (unsigned long)(m_nextBlock - m_blocks)) {
            m_parent->m_next = NULL;
            delete this;
        }
    }
}

} } } }
