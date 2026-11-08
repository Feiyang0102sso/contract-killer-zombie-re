// CNGSFriendsManager.cpp - gServe friends.
// Minimal reconstruction: the original functions were dead-stripped by the linker (see the
// header). Of the original object only weak functions and __GLOBAL__I_a remain; this file
// reproduces them. CObjectMapArray::getType (also weak in the original object) is not
// reproduced: the original reference that kept it out of line is not known.
#include "CNGSFriendsManager.h"
#include "CNGSURLMgr.h"
#include "CObjectMapObject.h"
#include "CObjectMapArray.h"

using com::glu::platform::components::CSingleton;
using com::glu::platform::components::CStrWChar;
using com::glu::platform::core::CVector;

CFriendsManager::CFriendsManager()
    : CSingleton(ClassId_CFriendsManager)
{
}

CFriendsManager::~CFriendsManager()
{
}

void CFriendsManager::setFriendIds(const CVector<CStrWChar>& friendIds)
{
    m_friendIds = friendIds;
}

void CFriendsManager::addFriend(CNGSRemoteUser* user)
{
    m_friends.Add(user);
}

CObjectMapObject* CFriendsManager::createInviteReplyObject(unsigned char accept)
{
    CObjectMapObject* reply = new CObjectMapObject();
    reply->addEntry(CStrWChar("accept"), new CObjectMapBool(accept));
    return reply;
}
