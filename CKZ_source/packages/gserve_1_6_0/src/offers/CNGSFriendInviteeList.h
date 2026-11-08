// CNGSFriendInviteeList.h - set of friend ids of one social network, in insertion order.
// The ids live in a CLinkList (node data = heap CStrWChar*) and are indexed by
// CStringToKey(id) in a CHash (value = the list node).
#ifndef CNGSFRIENDINVITEELIST_H
#define CNGSFRIENDINVITEELIST_H

#include "CLinkList.h"
#include "CHash.h"
#include "CStrWChar.h"

class CFriendInviteeList {
public:
    CFriendInviteeList();

    /** Forget every id (the strings and nodes are not freed). */
    void Clear();
    /** Append `id` unless it is already there. */
    void Add(com::glu::platform::components::CStrWChar id);
    unsigned char Exists(const com::glu::platform::components::CStrWChar& id);
    int Size();
    /** Unlink `id` from the list and the index. */
    void RemoveElement(const com::glu::platform::components::CStrWChar& id);

    com::glu::platform::core::CLinkList m_list;     // +0x00 node data: CStrWChar*
    com::glu::platform::components::CHash* m_index; // +0x10 CStringToKey(id) -> node
};

#endif
