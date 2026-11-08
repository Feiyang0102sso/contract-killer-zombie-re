// CStack.h - fixed-capacity stack of values (header-only; b000x). The stack starts with one
// (default constructed) element; Push() duplicates the top so it can be modified in place.
#ifndef CSTACK_H
#define CSTACK_H

#include <stddef.h>

namespace com { namespace glu { namespace platform { namespace core {

template <class T>
class CStack {
public:
    CStack()
    {
        m_data = NULL;
        m_count = 0;
        m_capacity = 0;
    }

    // No destructor: the original CStack has none (owners call Destroy() explicitly);
    // a destructor here adds a member destruction to CDisplayProgram/CGraphics2d_OGLES D2 (b037x, b035).

    /** Allocate `capacity` default-constructed elements; the first one is the initial top. */
    void Initialize(unsigned int capacity)
    {
        m_data = new T[capacity];
        m_capacity = capacity;
        m_count = 1;
    }

    /** Free the storage. */
    void Destroy()
    {
        delete[] m_data;
        m_data = NULL;
        m_capacity = 0;
        m_count = 0;
    }

    /** Push a copy of the current top. */
    void Push()
    {
        m_data[m_count] = m_data[m_count - 1];
        m_count++;
    }

    void Pop()
    {
        m_count--;
    }

    T& Top()
    {
        return m_data[m_count - 1];
    }

    unsigned int GetCount() const
    {
        return m_count;
    }

    T* m_data;                  // 0x00
    unsigned int m_capacity;    // 0x04 elements allocated
    unsigned int m_count;       // 0x08 elements in use (top = m_count - 1)
};

} } } }

#endif
