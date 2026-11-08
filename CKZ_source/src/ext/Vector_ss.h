// PROVISIONAL (created by b057x): header-only game template, no owning unit (first user src/CGameApp.cpp)
// Vector_ss.h - growable array of the game layer (java.util.Vector style API).
// Elements live in one malloc'd block; they are copy-constructed in place and destroyed
// with explicit destructor calls. Layout from PushNotificationGift.o / App.o (b057x).
#ifndef VECTOR_SS_H
#define VECTOR_SS_H

#include <stddef.h>
#include <stdlib.h>

// Placement new comes from NPNew.h (prefix header).

template <class T>
class Vector {
public:
    Vector()
    {
        m_size = 0;
        m_capacity = 0;
        m_capacityIncrement = 4;
        m_items = NULL;
    }

    /** Copy: storage of the same capacity, elements copy-constructed (instance in SimpleDialog.o). */  // added by b058
    Vector(const Vector& other)  // added by b058
    {  // added by b058
        int capacity = other.m_capacity;  // added by b058 (read before the size, as in the original)
        m_size = other.m_size;  // added by b058
        m_capacity = capacity;  // added by b058
        m_capacityIncrement = 4;  // added by b058 (stored, then overwritten as in the original)
        m_items = NULL;  // added by b058
        m_capacityIncrement = other.m_capacityIncrement;  // added by b058
        if (m_capacity != 0) {  // added by b058
            m_items = allocate_items(m_capacity);  // added by b058
            copy_items(m_items, other.m_items, m_size);  // added by b058
        }  // added by b058
    }  // added by b058

    ~Vector()
    {
        free_all_items();
    }

    /** Copy: same capacity, elements copy-constructed (instance in CBH_PersonType.o). */  // added by b077
    Vector& operator=(const Vector& other);  // added by b077

    int size() const
    {
        return m_size;
    }

    T& elementAt(int index)
    {
        return m_items[index];
    }

    const T& elementAt(int index) const
    {
        return m_items[index];
    }

    T& operator[](int index)
    {
        return m_items[index];
    }

    const T& operator[](int index) const
    {
        return m_items[index];
    }

    /** Append a copy of `element`, growing by m_capacityIncrement when full. */
    void addElement(const T& element);
    /** Grow with default-constructed elements or shrink (destroying the tail) to `newSize`. */
    void setSize(int newSize);
    /** Destroy all elements and release the storage (the size is left unchanged). */
    void free_all_items();
    /** Destroy the elements [from, size). */
    void deallocate_items(int from);
    /** Destroy every element and release the storage. */  // added by b051x
    void removeAllElements()  // added by b051x
    {  // added by b051x
        free_all_items();  // added by b051x
        m_size = 0;  // added by b051x
        m_capacity = 0;  // added by b051x
    }  // added by b051x
    /** Shrink the storage to the current size. */  // added by b051x
    void trimToSize();  // added by b051x
    /** Remove the element at `index` (the storage is reallocated). */  // added by b051x
    void removeElementAt(int index);  // added by b051x
    /** Copy-construct `count` elements from `source` into raw storage `target`. */  // added by b051x
    void copy_items(T* target, T* source, int count) const;  // added by b051x
    /** Index of the first element equal to `element` from `start` on, -1 when absent. */  // added by b051x
    int indexOf(const T& element, int start = 0) const  // added by b051x
    {  // added by b051x
        for (int index = start; index < m_size && index >= 0; index++) {  // added by b051x
            if (m_items[index] == element) {  // added by b051x
                return index;  // added by b051x
            }  // added by b051x
        }  // added by b051x
        return -1;  // added by b051x
    }  // added by b051x
    /** Grow the storage to at least `capacity` elements (never shrinks). */  // added by b048
    void ensureCapacity(int capacity);  // added by b048
    /** Insert `element` before `index` (0 <= index < size). */  // added by b048
    void insertElementAt(const T& element, int index);  // added by b048

    int m_size;                // 0x00 elements in use
    int m_capacity;             // 0x04 elements allocated
    int m_capacityIncrement;    // 0x08 growth step
    T* m_items;                 // 0x0c element storage

private:
    /** Raw storage for `count` elements, NULL when count is not positive. */
    static T* allocate_items(int count)
    {
        int bytes = count * (int)sizeof(T);
        if (bytes > 0) {
            return (T*)malloc(bytes);
        }
        return NULL;
    }
};

template <class T>
void Vector<T>::addElement(const T& element)
{
    if (m_capacity == m_size) {
        T* newItems = allocate_items(m_capacity + m_capacityIncrement);
        if (newItems == NULL) {
            return;
        }
        m_capacity = m_capacity + m_capacityIncrement;
        copy_items(newItems, m_items, m_size);  // changed by b051x (original calls copy_items: AnimationController instance)
        free_all_items();
        new (&newItems[m_size]) T(element);
        m_items = newItems;
    } else {
        new (&m_items[m_size]) T(element);
    }
    m_size++;
}

template <class T>
void Vector<T>::setSize(int newSize)
{
    if (newSize < 0 || m_size == newSize) {
        return;
    }
    // Each branch stores the size and returns on its own: the original lays the shrink block  // changed by fx2
    // (str size; return) right after the compare and the in-place grow jumps back to it.  // changed by fx2
    if (m_size > newSize) {
        deallocate_items(newSize);
        m_size = newSize;  // changed by fx2
        return;  // changed by fx2
    }
    if (m_capacity >= newSize) {  // changed by fx2
        T* added = m_items + m_size;  // changed by b051x (base read once, as in the original)
        for (int index = newSize - m_size; index > 0; index--) {  // changed by b051x
            new (added) T();  // changed by b051x
            added++;  // changed by b051x
        }  // changed by b051x
        m_size = newSize;  // changed by fx2
        return;  // changed by fx2
    }
    T* newItems = allocate_items(newSize);
    copy_items(newItems, m_items, m_size);  // changed by b051x
    free_all_items();
    T* added = newItems + m_size;  // changed by b051x
    for (int index = newSize - m_size; index > 0; index--) {  // changed by b051x
        new (added) T();  // changed by b051x
        added++;  // changed by b051x
    }  // changed by b051x
    m_items = newItems;
    m_size = newSize;
    m_capacity = newSize;
}

template <class T>
void Vector<T>::free_all_items()
{
    if (m_items == NULL) {
        return;
    }
    deallocate_items(0);
    // Read into a local before the test: original cmp+beq instead of cbz (DBG_VALUE). // changed by fx2
    T* items = m_items;  // changed by fx2
    if (items != NULL) {  // changed by fx2
        free(items);  // changed by fx2
    }
    m_items = NULL;
}

template <class T>
void Vector<T>::deallocate_items(int from)
{
    if (m_items == NULL) {
        return;
    }
    if (from < 0 || from >= m_size) {
        return;
    }
    for (int index = from; index < m_size; index++) {
        m_items[index].~T();
    }
}

template <class T>  // added by b051x
void Vector<T>::trimToSize()  // added by b051x
{  // added by b051x
    if (m_capacity <= 0) {  // added by b051x
        return;  // added by b051x
    }  // added by b051x
    if (m_size == 0) {  // added by b051x
        free_all_items();  // added by b051x
        m_capacity = 0;  // added by b051x
        return;  // added by b051x
    }  // added by b051x
    T* newItems = allocate_items(m_size);  // added by b051x
    copy_items(newItems, m_items, m_size);  // added by b051x
    free_all_items();  // added by b051x
    m_capacity = m_size;  // added by b051x
    m_items = newItems;  // added by b051x
}  // added by b051x

// The new block holds m_size + m_capacity - 1 elements and m_capacity is left unchanged (as in the original).  // added by b051x
template <class T>  // added by b051x
void Vector<T>::removeElementAt(int index)  // added by b051x
{  // added by b051x
    if (index < 0 || index >= m_size) {  // added by b051x
        return;  // added by b051x
    }  // added by b051x
    if (m_size == 1 && m_capacity == 1) {  // added by b051x
        removeAllElements();  // added by b051x
        return;  // added by b051x
    }  // added by b051x
    T* newItems = allocate_items(m_size + m_capacity - 1);  // added by b051x
    copy_items(newItems, m_items, index);  // added by b051x
    copy_items(newItems + index, m_items + index + 1, m_size - index - 1);  // added by b051x
    free_all_items();  // added by b051x
    m_items = newItems;  // added by b051x
    m_size--;  // added by b051x
}  // added by b051x

template <class T>  // added by b051x
void Vector<T>::copy_items(T* target, T* source, int count) const  // added by b051x
{  // added by b051x
    for (int index = 0; index < count; index++) {  // added by b051x
        new (&target[index]) T(source[index]);  // added by b051x
    }  // added by b051x
}  // added by b051x

template <class T>  // added by b048
void Vector<T>::ensureCapacity(int capacity)  // added by b048
{  // added by b048
    if (m_capacity >= capacity) {  // added by b048
        return;  // added by b048
    }  // added by b048
    T* newItems = allocate_items(capacity);  // added by b048
    if (newItems == NULL) {  // added by b048
        return;  // added by b048
    }  // added by b048
    copy_items(newItems, m_items, m_size);  // added by b048
    free_all_items();  // added by b048
    m_items = newItems;  // added by b048
    m_capacity = capacity;  // added by b048
}  // added by b048

// changed by b077: rewritten after Vector<SRank> (CCommonGameConfig.o), Vector<Window*> (CPopupController.o)
// and Vector<SDeferredPlayback>: an index out of range appends (addElement); elements are copy-constructed
// (the in-place shift also destroys the moved one). The in-place branch tests m_capacity < m_size (never
// true), so every insert reallocates (as in the original).
template <class T>  // added by b048
void Vector<T>::insertElementAt(const T& element, int index)  // added by b048
{  // added by b048
    if (index < 0 || index >= m_size) {  // added by b048
        addElement(element);  // changed by b077 (was a plain return)
        return;  // added by b048
    }  // added by b048
    if (m_capacity < m_size) {  // added by b048
        for (int position = m_size - 1; position >= index; position--) {  // added by b048
            new (&m_items[position + 1]) T(m_items[position]);  // changed by b077
            m_items[position].~T();  // changed by b077
        }  // added by b048
        new (&m_items[index]) T(element);  // changed by b077
    } else {  // added by b048
        T* newItems = allocate_items(m_capacity + m_capacityIncrement);  // added by b048
        if (newItems == NULL) {  // added by b048
            return;  // added by b048
        }  // added by b048
        m_capacity = m_capacity + m_capacityIncrement;  // added by b048
        copy_items(newItems, m_items, index);  // changed by b077
        new (&newItems[index]) T(element);  // changed by b077
        copy_items(&newItems[index + 1], &m_items[index], m_size - index);  // changed by b077
        free_all_items();  // added by b048
        m_items = newItems;  // added by b048
    }  // added by b048
    m_size++;  // added by b048
}  // added by b048

template <class T>  // added by b077
Vector<T>& Vector<T>::operator=(const Vector<T>& other)  // added by b077
{  // added by b077
    if (&other != this) {  // added by b077
        free_all_items();  // added by b077
        m_size = other.m_size;  // added by b077
        m_capacity = other.m_capacity;  // added by b077
        m_capacityIncrement = other.m_capacityIncrement;  // added by b077
        if (m_capacity != 0) {  // added by b077
            m_items = allocate_items(m_capacity);  // added by b077
            copy_items(m_items, other.m_items, m_size);  // added by b077
        }  // added by b077
    }  // added by b077
    return *this;  // added by b077
}  // added by b077

/** delete every element of `items` and empty the vector (header of the original unknown). */  // added by b077
template <class T>  // added by b077
void SAFE_DELETE_VECTOR(Vector<T*>& items)  // added by b077
{  // added by b077
    for (int index = 0; index < items.size(); index++) {  // added by b077
        if (items[index] != NULL) {  // added by b077
            delete items[index];  // added by b077
            items[index] = NULL;  // added by b077
        }  // added by b077
    }  // added by b077
    items.removeAllElements();  // added by b077
}  // added by b077

#endif
