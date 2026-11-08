// PROVISIONAL (created by b057x): header-only game template, no owning unit (first user src/FPSGameplay/AI/CGameAIMap.cpp)
// CFixedVector.h - array allocated once (reserve) and filled up to its capacity.
#ifndef CFIXEDVECTOR_H
#define CFIXEDVECTOR_H

#include <stddef.h>

template <class T>
class CFixedVector {
public:
    CFixedVector()
    {
        m_data = NULL;
        m_size = 0;
        m_capacity = 0;
    }

    ~CFixedVector()
    {
        if (m_data != NULL) {
            delete[] m_data;
            m_data = NULL;
        }
    }

    /** Allocate room for `capacity` elements (only once, while empty). */
    void reserve(int capacity);
    /** Original e3458/e65a8: overwrites with the last item, does not preserve order, does not destruct or clear the tail item. */
    void remove_at(int index); // added by b070g

    T* m_data;          // 0x00
    int m_size;         // 0x04 elements in use
    int m_capacity;     // 0x08 elements allocated
};

template <class T>
void CFixedVector<T>::reserve(int capacity)
{
    if (capacity < 1) {
        return;
    }
    if (m_data != NULL) {
        return;
    }
    m_data = new T[capacity];
    if (m_data != NULL) {
        m_capacity = capacity;
    }
}

template <class T>
void CFixedVector<T>::remove_at(int index)
{
    if (index >= 0 && index < m_size) {
        if (m_size >= 2) {
            m_data[index] = m_data[m_size - 1];
        }
        --m_size;
    }
}

#endif
