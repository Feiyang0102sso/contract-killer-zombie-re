// CLinkList.cpp - intrusive doubly linked list of the Glu platform core.
#include "CLinkList.h"

namespace com { namespace glu { namespace platform { namespace core {

// ---- CLinkListNode ----

CLinkListNode::CLinkListNode()
{
    m_list = NULL;
    m_next = NULL;
    m_prev = NULL;
    m_data = NULL;
}

CLinkListNode::~CLinkListNode()
{
}

void CLinkListNode::RemoveFromList()
{
    if (m_list != NULL) {
        m_list->Remove(this);
    }
}

// ---- CLinkList ----

void CLinkList::Remove(CLinkListNode* node)
{
    if (node == NULL || node->m_list == NULL) {
        return;
    }

    // the node lives in another list: let that list unlink it
    if (node->m_list != this) {
        node->m_list->Remove(node);
        return;
    }

    m_count--;
    if (m_head == node) {
        m_head = node->m_next;
    }
    if (m_tail == node) {
        m_tail = node->m_prev;
    }
    if (node->m_next != NULL) {
        node->m_next->m_prev = node->m_prev;
    }
    if (node->m_prev != NULL) {
        node->m_prev->m_next = node->m_next;
    }

    node->m_list = NULL;
    node->m_next = NULL;
    node->m_prev = NULL;
}

CLinkList::CLinkList()
{
    m_head = NULL;
    m_tail = NULL;
    m_count = 0;
}

void CLinkList::Clear()
{
    m_head = NULL;
    m_tail = NULL;
    m_count = 0;
}

CLinkList::~CLinkList()
{
}

void CLinkList::Insert(CLinkListNode* node, void* data)
{
    InsertBefore(m_head, node, data);
}

void CLinkList::InsertBefore(CLinkListNode* before, CLinkListNode* node, void* data)
{
    if (node == NULL) {
        return;
    }

    node->m_list = this;
    if (data == NULL) {
        data = node;
    }
    node->m_data = data;

    if (before != NULL) {
        node->m_next = before;
        node->m_prev = before->m_prev;
    } else {
        node->m_next = m_head;
        node->m_prev = NULL;
    }

    if (node->m_next != NULL) {
        node->m_next->m_prev = node;
    }
    if (node->m_prev != NULL) {
        node->m_prev->m_next = node;
    }

    if (before == NULL || m_head == before) {
        m_head = node;
    }
    if (m_tail == NULL) {
        m_tail = node;
    }
    m_count++;
}

void CLinkList::InsertAfter(CLinkListNode* after, CLinkListNode* node, void* data)
{
    if (node == NULL) {
        return;
    }

    node->m_list = this;
    if (data == NULL) {
        data = node;
    }
    node->m_data = data;

    if (after != NULL) {
        node->m_prev = after;
        node->m_next = after->m_next;
    } else {
        node->m_prev = m_tail;
        node->m_next = NULL;
    }

    if (node->m_next != NULL) {
        node->m_next->m_prev = node;
    }
    if (node->m_prev != NULL) {
        node->m_prev->m_next = node;
    }

    if (m_head == NULL) {
        m_head = node;
    }
    if (after == NULL || m_tail == after) {
        m_tail = node;
    }
    m_count++;
}

void CLinkList::InsertAtEnd(CLinkListNode* node, void* data)
{
    InsertAfter(m_tail, node, data);
}

void CLinkList::RemoveAll()
{
    while (m_head != NULL) {
        Remove(m_head);
    }
}

CLinkListNode* CLinkList::Find(CLinkListNode* start, CLinkListMatchFunc match, void* context)
{
    if (context == NULL) {
        return NULL;
    }
    if (start == NULL) {
        start = m_head;
    }

    for (CLinkListNode* node = start; node != NULL; node = node->m_next) {
        if (match != NULL) {
            if (match(node, context)) {
                return node;
            }
        } else if (node->m_data == context) {
            return node;
        }
    }
    return NULL;
}

} } } }
