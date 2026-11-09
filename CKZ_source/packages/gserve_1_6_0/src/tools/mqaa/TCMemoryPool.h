// PROVISIONAL (created by b016x): header-only template, no owning unit (first used by core/CNGSServerRequest.cpp)
// TCMemoryPool.h - fixed-size object pool: one block of `capacity` constructed objects plus a free list.
// When a growable pool runs dry it chains a child pool of the same capacity (m_next); a child that
// becomes completely free again is released by Delete().
#ifndef TCMEMORYPOOL_H
#define TCMEMORYPOOL_H

#include <stddef.h>

template <class T>
class TCMemoryPool {
public:
    TCMemoryPool(int capacity = 16, TCMemoryPool<T>* parent = NULL)
        : m_parent(parent)
        , m_next(NULL)
        , m_elements(NULL)
        , m_freeList(NULL)
        , m_freeCount(0)
        , m_capacity(capacity)
        , m_cursor(NULL)
        , m_end(NULL)
        , m_growable(1)
    {
        Allocate();
    }

    /** Releases the chained child pools and both blocks. */
    ~TCMemoryPool();

    /** A free object: recycled first, then never used, then from a (new) child pool; NULL when exhausted. */
    T* New();
    /** Return `object` to the pool that owns it (the object is not destroyed). */
    void Delete(T* object);

    /** Create the element block and the free list for m_capacity objects. */
    void Allocate();

    TCMemoryPool<T>* m_parent;  // +0x00 pool this one was chained to (NULL for the first)
    TCMemoryPool<T>* m_next;    // +0x04 chained child pool
    T* m_elements;              // +0x08 new[]'d block
    T** m_freeList;             // +0x0c recycled objects
    int m_freeCount;            // +0x10
    int m_capacity;             // +0x14 objects per block
    T* m_cursor;                // +0x18 first never-used object
    T* m_end;                   // +0x1c m_elements + m_capacity
    unsigned char m_growable;   // +0x20 may chain child pools
};

// Out-of-class definitions without `inline` (the original keeps them as called weak functions).
template <class T>
TCMemoryPool<T>::~TCMemoryPool()
{
    if (m_next != NULL) {
        delete m_next;
    }
    if (m_elements != NULL) {
        delete[] m_elements;
    }
    delete[] m_freeList;
}

template <class T>
T* TCMemoryPool<T>::New()
{
    TCMemoryPool<T>* pool = this;
    while (pool != NULL) {
        if (pool->m_freeCount != 0) {
            pool->m_freeCount--;
            return pool->m_freeList[pool->m_freeCount];
        }
        if (pool->m_cursor < pool->m_end) {
            T* object = pool->m_cursor;
            pool->m_cursor++;
            return object;
        }
        if (pool->m_next == NULL) {
            if (!pool->m_growable) {
                return NULL;
            }
            pool->m_next = new TCMemoryPool<T>(pool->m_capacity, pool);
        }
        pool = pool->m_next;
    }
    return NULL;
}

template <class T>
void TCMemoryPool<T>::Delete(T* object)
{
    if (object == NULL) {
        return;
    }
    TCMemoryPool<T>* pool = this;
    while (object < pool->m_elements || object >= pool->m_end) {
        pool = pool->m_next;
        if (pool == NULL) {
            return;
        }
    }
    pool->m_freeList[pool->m_freeCount++] = object;
    // A chained, last child pool whose objects are all free goes away.
    // Original quirk: compares the object count with a byte count.
    if (pool->m_parent != NULL && pool->m_next == NULL
        && pool->m_cursor - pool->m_elements == pool->m_freeCount * (int)sizeof(T)) {
        pool->m_parent->m_next = NULL;
        delete pool;
    }
}

template <class T>
void TCMemoryPool<T>::Allocate()
{
    m_elements = new T[m_capacity];
    m_freeList = new T*[m_capacity];
    m_cursor = m_elements;
    m_end = m_elements + m_capacity;
}

#endif
