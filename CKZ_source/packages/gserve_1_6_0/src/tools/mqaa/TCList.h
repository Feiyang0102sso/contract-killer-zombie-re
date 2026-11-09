// PROVISIONAL (created by b016x): header-only template, no owning unit (first used by core/CNGSServerRequest.cpp)
// TCList.h - doubly linked list of values whose nodes come from a TCMemoryPool.
// Values are copied into the nodes; an empty list hands out copies of m_null.
#ifndef TCLIST_H
#define TCLIST_H

#include <stddef.h>
#include "TCMemoryPool.h"

/** One list node: the value plus the links. */
template <class T>
class TCListNode {
public:
    TCListNode()
        : m_prev(NULL)
        , m_next(NULL)
    {
    }

    TCListNode(const T& data, TCListNode<T>* prev, TCListNode<T>* next)
        : m_data(data)
        , m_prev(prev)
        , m_next(next)
    {
    }

    T m_data;
    TCListNode<T>* m_prev;      // towards the head
    TCListNode<T>* m_next;      // towards the tail
};

/** Position in a TCList (m_value is never filled by the list itself). */
template <class T>
class TCListItr {
public:
    explicit TCListItr(TCListNode<T>* node)
        : m_node(node)
    {
    }

    /** The current value (m_value past the end). */  // added by b020
    inline T& operator*()  // added by b020
    {  // added by b020
        if (m_node != NULL) {  // added by b020
            return m_node->m_data;  // added by b020
        }  // added by b020
        return m_value;  // added by b020
    }  // added by b020

    /** Step towards the tail (stays past the end). */  // added by b020
    inline TCListItr<T>& operator++()  // added by b020
    {  // added by b020
        if (m_node != NULL) {  // added by b020
            m_node = m_node->m_next;  // added by b020
        }  // added by b020
        return *this;  // added by b020
    }  // added by b020

    /** Step towards the head (stays past the end). */  // added by b020
    inline TCListItr<T>& operator--()  // added by b020
    {  // added by b020
        if (m_node != NULL) {  // added by b020
            m_node = m_node->m_prev;  // added by b020
        }  // added by b020
        return *this;  // added by b020
    }  // added by b020

    TCListNode<T>* m_node;      // +0x00 current node, NULL past the end
    T m_value;                  // +0x04
};

template <class T>
class TCList {
public:
    TCList()
        : m_head(NULL)
        , m_tail(NULL)
        , m_count(0)
    {
        m_pool = new TCMemoryPool<TCListNode<T> >();
    }

    ~TCList()
    {
        RemoveAll();
        if (m_pool != NULL) {
            delete m_pool;
        }
    }

    int GetCount() const
    {
        return m_count;
    }

    /** The last value (m_null when the list is empty). */  // added by b020
    inline T& GetLast()  // added by b020
    {  // added by b020
        if (m_tail != NULL) {  // added by b020
            return m_tail->m_data;  // added by b020
        }  // added by b020
        return m_null;  // added by b020
    }  // added by b020

    void PushFirst(const T& value);
    void PushLast(const T& value);
    /** Insert `value` before the node of `itr`. */
    void InsertAt(const T& value, TCListItr<T>& itr);
    /** Unlink the node of `itr` (itr becomes the end position). */
    void Remove(TCListItr<T>& itr);
    /** Unlink the last value and return it (a copy of m_null when the list is empty). */
    T PullLast();
    void RemoveAll();

    T m_null;                               // +0x00 returned by PullLast() on an empty list
    TCListNode<T>* m_head;
    TCListNode<T>* m_tail;
    int m_count;
    TCMemoryPool<TCListNode<T> >* m_pool;
};

// Out-of-class definitions without `inline` (the original keeps them as called weak functions).
template <class T>
void TCList<T>::PushFirst(const T& value)
{
    TCListNode<T>* node = m_pool->New();
    *node = TCListNode<T>(value, NULL, m_head);
    if (m_head != NULL) {  // changed by b020: != NULL branch first (original if-conversion order, verified MATCH)
        m_head->m_prev = node;
    } else {
        m_tail = node;
    }
    m_head = node;
    m_count++;
}

template <class T>
void TCList<T>::PushLast(const T& value)
{
    TCListNode<T>* node = m_pool->New();
    *node = TCListNode<T>(value, m_tail, NULL);
    if (m_tail != NULL) {  // changed by b020: != NULL branch first (original if-conversion order, verified MATCH)
        m_tail->m_next = node;
    } else {
        m_head = node;
    }
    m_tail = node;
    m_count++;
}

template <class T>
void TCList<T>::InsertAt(const T& value, TCListItr<T>& itr)
{
    TCListNode<T>* node = m_pool->New();
    *node = TCListNode<T>(value, itr.m_node->m_prev, itr.m_node);
    if (itr.m_node->m_prev != NULL) {  // changed by b020: != NULL branch first (original if-conversion order, verified MATCH)
        itr.m_node->m_prev->m_next = node;
    } else {
        m_head = node;
    }
    itr.m_node->m_prev = node;
    m_count++;
}

template <class T>
void TCList<T>::Remove(TCListItr<T>& itr)
{
    TCListNode<T>* node = itr.m_node;
    if (node == NULL) {
        return;
    }
    if (node->m_next != NULL) {  // changed by b020: != NULL branch first (original if-conversion order, verified MATCH)
        node->m_next->m_prev = node->m_prev;
    } else {
        m_tail = node->m_prev;
    }
    if (node->m_prev != NULL) {  // changed by b020: != NULL branch first (original if-conversion order, verified MATCH)
        node->m_prev->m_next = node->m_next;
    } else {
        m_head = node->m_next;
    }
    m_pool->Delete(node);
    itr.m_node = NULL;
    m_count--;
    if (m_count == 0) {
        m_head = NULL;
        m_tail = NULL;
    }
}

template <class T>
T TCList<T>::PullLast()
{
    if (m_tail != NULL) {
        T value(m_tail->m_data);
        TCListNode<T>* last = m_tail;
        m_tail = last->m_prev;
        if (m_tail != NULL) {
            m_tail->m_next = NULL;
        }
        m_pool->Delete(last);
        m_count--;
        if (m_count == 0) {
            m_head = NULL;
        }
        return value;
    }
    return m_null;
}

template <class T>
void TCList<T>::RemoveAll()
{
    while (m_count != 0) {
        PullLast();
    }
}

#endif
