// CLinkList.h - intrusive doubly linked list of the Glu platform core.
// A node is embedded in (or owned by) the object it represents and carries a user data pointer;
// the list never allocates or frees nodes.
#ifndef CLINKLIST_H
#define CLINKLIST_H

#include <stddef.h>

namespace com { namespace glu { namespace platform { namespace core {

class CLinkList;

/** One element of a CLinkList (20 bytes). */
class CLinkListNode {
public:
    CLinkListNode();
    virtual ~CLinkListNode();

    /** Unlink from whatever list this node is in (no-op when not linked). */
    void RemoveFromList();

    CLinkList* GetList() const { return m_list; }
    CLinkListNode* GetNext() const { return m_next; }
    CLinkListNode* GetPrev() const { return m_prev; }
    void* GetData() const { return m_data; }

    CLinkList* m_list;          // +0x04 owning list, NULL when unlinked
    CLinkListNode* m_next;      // +0x08 towards the tail
    CLinkListNode* m_prev;      // +0x0C towards the head
    void* m_data;               // +0x10 user data (the node itself when inserted with NULL)
};

/** Callback for CLinkList::Find: return non-zero when `node` matches `context`. */
typedef unsigned char (*CLinkListMatchFunc)(CLinkListNode* node, void* context);

/** Doubly linked list header (16 bytes). */
class CLinkList {
public:
    CLinkList();
    virtual ~CLinkList();

    /** Forget all nodes without touching them. */
    void Clear();

    /** Unlink `node`; when it belongs to another list it is removed from that one. */
    void Remove(CLinkListNode* node);
    /** Insert `node` at the head. `data` NULL means the node itself. */
    void Insert(CLinkListNode* node, void* data);
    /** Insert `node` before `before` (NULL = at the head). */
    void InsertBefore(CLinkListNode* before, CLinkListNode* node, void* data);
    /** Insert `node` after `after` (NULL = at the tail). */
    void InsertAfter(CLinkListNode* after, CLinkListNode* node, void* data);
    /** Insert `node` at the tail. */
    void InsertAtEnd(CLinkListNode* node, void* data);
    /** Unlink every node. */
    void RemoveAll();
    /**
     * Search from `start` (NULL = head) towards the tail. With a callback the first node it
     * accepts is returned, otherwise the first node whose data equals `context`.
     */
    CLinkListNode* Find(CLinkListNode* start, CLinkListMatchFunc match, void* context);

    CLinkListNode* GetHead() const { return m_head; }
    CLinkListNode* GetTail() const { return m_tail; }
    int GetCount() const { return m_count; }

    CLinkListNode* m_head;      // +0x04
    CLinkListNode* m_tail;      // +0x08
    int m_count;                // +0x0C
};

} } } }

#endif
