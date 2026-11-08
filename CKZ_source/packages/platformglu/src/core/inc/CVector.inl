// CVector.inl - member definitions of CVector<T> (included by CVector.h; created by b006).
// No `inline` keyword here on purpose: clang only gives the raised inline-hint threshold to
// functions spelled `inline`, and the original instantiations are inlined like plain templates.

namespace com { namespace glu { namespace platform { namespace core {

template <class T>
CVector<T>::CVector(int capacity, int growBy)
    : CClass(ClassId)
{
    m_growBy = growBy;
    m_size = 0;
    m_data = NULL;
    if (capacity > 0) {
        m_data = (T*)malloc(capacity * sizeof(T));
    }
    m_capacity = capacity;
}

template <class T>
CVector<T>::CVector(const CVector<T>& other)
    : CClass(ClassId)
{
    m_growBy = other.m_growBy;
    int capacity = other.m_capacity;
    m_size = 0;
    m_data = NULL;
    if (capacity > 0) {
        m_data = (T*)malloc(capacity * sizeof(T));
    }
    m_capacity = capacity;
    AddAllNoResize(other.m_data, other.m_size);
}

template <class T>
CVector<T>::~CVector()
{
    if (m_data != NULL) {
        DestroyElements(m_data, m_size);
        free(m_data);
    }
}

template <class T>
CVector<T>& CVector<T>::operator=(const CVector<T>& other)
{
    if (this != &other) {
        CVector<T> copy(other);
        Swap(copy);
    }
    return *this;
}

template <class T>
unsigned char CVector<T>::AddAll(int index, const T* elements, int count)
{
    int oldSize = m_size;
    if (oldSize + count > m_capacity) {
        // rebuild into a bigger block: head, new elements, tail
        CVector<T> grown(GetGrownCapacity(oldSize + count), m_growBy);
        grown.AddAllNoResize(m_data, index);
        grown.AddAllNoResize(elements, count);
        grown.AddAllNoResize(m_data + index, oldSize - index);
        Swap(grown);
    } else {
        // the part landing past the old end is constructed, the rest is assigned
        int assigned = count;
        if (index + count > oldSize) {
            assigned = oldSize - index;
        }
        AddAllNoResize(elements + assigned, count - assigned);
        AddAllNoResize(m_data + index, assigned);
        for (int position = index; position < oldSize; position++) {
            Set(position, elements[position - index]);
        }
    }
    return 1;
}

template <class T>
void CVector<T>::AddAllNoResize(const T* elements, int count)
{
    for (int i = 0; i < count; i++) {
        new (&m_data[m_size]) T(elements[i]);
        m_size++;
    }
}

template <class T>
void CVector<T>::Set(int index, const T& value)
{
    m_data[index] = value;
}

template <class T>
void CVector<T>::RemoveRange(int start, int end)
{
    int removed = end - start;
    if (removed < 1) {
        return;
    }
    int size = m_size;
    int to = start;
    for (int from = end; from < size; from++) {
        Set(to, m_data[from]);
        to++;
    }
    m_size -= removed;
    DestroyElements(m_data + m_size, removed);
}

template <class T>
unsigned char CVector<T>::Remove(const T& element)
{
    int index = IndexOf(element);
    if (index < 0) {
        return 0;
    }
    RemoveRange(index, index + 1);
    return 1;
}

template <class T>
unsigned char CVector<T>::RemoveAll(const CVector<T>& other)
{
    unsigned char removedAny = 0;
    if (other.m_size == 0) {
        return removedAny;
    }
    for (int index = m_size - 1; index >= 0; index--) {
        if (other.IndexOf(m_data[index]) >= 0) {
            RemoveRange(index, index + 1);
            removedAny = 1;
        }
    }
    return removedAny;
}

template <class T>
void CVector<T>::Clear()
{
    if (m_data != NULL) {
        DestroyElements(m_data, m_size);
        m_size = 0;
    }
}

template <class T>
void CVector<T>::EnsureCapacity(int capacity)
{
    if (m_capacity < capacity) {
        CVector<T> grown(GetGrownCapacity(capacity), m_growBy);
        grown.AddAllNoResize(m_data, m_size);
        Swap(grown);
    }
}

template <class T>
void CVector<T>::SetSize(int size)
{
    int oldSize = m_size;
    if (oldSize < size) {
        EnsureCapacity(size);
        T empty = T();
        for (int added = size - oldSize; added > 0; added--) {
            AddAllNoResize(&empty, 1);
        }
    } else if (oldSize > size) {
        RemoveRange(size, oldSize);
    }
}

template <class T>
void CVector<T>::TrimToSize()
{
    if (m_size < m_capacity) {
        CVector<T> trimmed(m_size, m_growBy);
        trimmed.AddAllNoResize(m_data, m_size);
        Swap(trimmed);
    }
}

template <class T>
void CVector<T>::DestroyElements(const T* elements, int count)
{
    for (int index = count - 1; index >= 0; index--) {
        elements[index].~T();
    }
}

} } } }
