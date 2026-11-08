// CNGSFriendsManager.h - gServe friends (b023x).
// The original object was dead-stripped by the linker except for weak template / inline
// functions it was the first to emit (CVector<CNGSRemoteUser*>, CVector<CStrWChar> copy and
// assignment, CObjectMapBool) and the static initialization of ClassId_CFriendsManager.
// The class below is a minimal reconstruction producing those; its members and functions are
// not the original ones (only the class id string "CFriendsManager" is).
#ifndef CNGSFRIENDSMANAGER_H
#define CNGSFRIENDSMANAGER_H

#include <stddef.h>
#include "CSingleton.h"
#include "CStringToKey.h"
#include "CStrWChar.h"
#include "CVector.h"

class CNGSRemoteUser;
class CObjectMapObject;

/** Singleton key; every includer computes it at static initialization. */
static unsigned int ClassId_CFriendsManager = com::glu::platform::core::CStringToKey("CFriendsManager", 0);

class CFriendsManager : public com::glu::platform::components::CSingleton {
public:
    CFriendsManager();
    virtual ~CFriendsManager();

    /** (reconstructed) remember the friend ids of the local user. */
    void setFriendIds(const com::glu::platform::core::CVector<com::glu::platform::components::CStrWChar>& friendIds);
    /** (reconstructed) add a populated friend. */
    void addFriend(CNGSRemoteUser* user);
    /** (reconstructed) {"accept": accept} request body. */
    CObjectMapObject* createInviteReplyObject(unsigned char accept);

    com::glu::platform::core::CVector<CNGSRemoteUser*> m_friends;                         // reconstructed
    com::glu::platform::core::CVector<com::glu::platform::components::CStrWChar> m_friendIds; // reconstructed
};

#endif
