// PROVISIONAL (created by b016x): header-only template, no owning unit; tools/mqaa (b029) not written yet
// CSharedPtr_gServe.h - minimal reference-counted pointer used by gServe's object maps.
// The counter is allocated lazily by incrementCounter(); reset() deletes the object with the last reference.
#ifndef CSHAREDPTR_GSERVE_H
#define CSHAREDPTR_GSERVE_H

#include <stddef.h>

template <class T>
class CSharedPtr_gServe {
public:
    CSharedPtr_gServe()
        : m_ptr(NULL)
        , m_counter(NULL)
    {
    }

    /** Take ownership of `pointer` (counter starts at 1). */
    explicit CSharedPtr_gServe(T* pointer)
        : m_ptr(pointer)
        , m_counter(NULL)
    {
        incrementCounter();
    }

    CSharedPtr_gServe(const CSharedPtr_gServe<T>& other)
        : m_ptr(other.m_ptr)
        , m_counter(other.m_counter)
    {
        incrementCounter();
    }

    ~CSharedPtr_gServe()
    {
        reset();
    }

    CSharedPtr_gServe<T>& operator=(const CSharedPtr_gServe<T>& other)
    {
        reset();
        m_ptr = other.m_ptr;
        m_counter = other.m_counter;
        incrementCounter();
        return *this;
    }

    T* get() const
    {
        return m_ptr;
    }

    /** Drop this reference; the object and the counter are deleted with the last one. */
    void reset();
    /** Add a reference (allocates the counter on first use). */
    void incrementCounter();

    T* m_ptr;           // +0x00
    int* m_counter;     // +0x04 shared reference count, NULL until the first incrementCounter()
};

// Defined out of the class body without `inline`: the original keeps both as called weak functions.
template <class T>
void CSharedPtr_gServe<T>::reset()
{
    if (m_ptr != NULL) {
        if (--(*m_counter) == 0) {
            delete m_ptr;
            delete m_counter;
        }
        m_ptr = NULL;
        m_counter = NULL;
    }
}

template <class T>
void CSharedPtr_gServe<T>::incrementCounter()
{
    if (m_ptr == NULL) {
        return;
    }
    if (m_counter != NULL) {
        (*m_counter)++;
    } else {
        m_counter = new int;
        *m_counter = 1;
    }
}

#endif
