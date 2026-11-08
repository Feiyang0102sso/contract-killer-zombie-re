// Owner: userManagement/CNGSRemoteUserList.cpp (b030x; started as a b025x PROVISIONAL)
// CNGSRemoteUserList.h - a list of remote users (friends, ...) kept by the local user, indexed by
// client id and by the user id of every social network the user came from.
#ifndef CNGSREMOTEUSERLIST_H
#define CNGSREMOTEUSERLIST_H

#include "CNGSServerObject.h"
#include "CVector.h"
#include "CHash.h"

class CNGSRemoteUser;

class CNGSRemoteUserList : public CNGSServerObject {
public:
    CNGSRemoteUserList();
    /** Forget every user (the users themselves belong to the factory). */
    void reset();
    virtual ~CNGSRemoteUserList();

    void addRemoteUserIfNotAlreadyInList(CNGSRemoteUser* user);
    /** Remove the user at `index` (also from the factory when it has a client id). */
    void removeRemoteUser(int index);
    /** The listed user with id `userId`, NULL when not in the list. */
    CNGSRemoteUser* findRemoteUserByID(int userId);

    int m_field18;                                                      // +0x18 cleared by reset() (meaning unknown)
    com::glu::platform::core::CVector<CNGSRemoteUser*> m_users;        // +0x1c
    com::glu::platform::components::CHash m_usersById;                 // +0x34 client id -> user
    com::glu::platform::components::CHash m_usersByNetworkId[6];       // +0x48 key of the network user id -> user
};

#endif
