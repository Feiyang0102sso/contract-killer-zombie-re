// CNGSFriendInviteeList.cpp - friend id set used by the gOffer invite flow.
#include "CNGSFriendInviteeList.h"
#include "CStringToKey.h"

using com::glu::platform::components::CStrWChar;
using com::glu::platform::components::CHash;
using com::glu::platform::core::CLinkListNode;
using com::glu::platform::core::CStringToKey;

CFriendInviteeList::CFriendInviteeList()
{
    m_index = new CHash();
    m_index->Init(10, 10, 1);
}

void CFriendInviteeList::Clear()
{
    m_index->RemoveAll();
    m_list.Clear();
}

void CFriendInviteeList::Add(CStrWChar id)
{
    if (Exists(id)) {
        return;
    }
    CStrWChar* copy = new CStrWChar();
    copy->Concatenate(id.m_string);
    CLinkListNode* node = new CLinkListNode();
    m_list.InsertBefore(m_list.m_head, node, copy);
    m_index->Insert(CStringToKey(copy->m_string, 0), node);
}

unsigned char CFriendInviteeList::Exists(const CStrWChar& id)
{
    unsigned char found = 0;
    if (Size() > 0) {
        void* node;
        found = m_index->Find(CStringToKey(id.m_string, 0), &node);
    }
    return found;
}

int CFriendInviteeList::Size()
{
    return m_list.m_count;
}

void CFriendInviteeList::RemoveElement(const CStrWChar& id)
{
    void* found;
    if (m_index->Find(CStringToKey(id.m_string, 0), &found)) {
        CLinkListNode* node = (CLinkListNode*)found;
        m_list.Remove(node);
    }
    m_index->Remove(CStringToKey(id.m_string, 0));
}
