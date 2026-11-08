// CNGSRemoteUserList.cpp - list of remote users with lookup tables by client id and by
// social network user id.
#include "CNGSRemoteUserList.h"
#include "CNGSRemoteUser.h"
#include "CNGS.h"
#include "CNGSFactory.h"
#include "CStringToKey.h"

using com::glu::platform::core::CStringToKey;

/** Number of social network lookup tables. */
static const int NETWORK_TABLE_COUNT = 6;

CNGSRemoteUserList::CNGSRemoteUserList()
{
    m_usersById.Init(10, 10, 1);
    for (int network = 0; network < NETWORK_TABLE_COUNT; network++) {
        m_usersByNetworkId[network].Init(10, 10, 1);
    }
    reset();
}

void CNGSRemoteUserList::reset()
{
    m_field18 = 0;
    m_users.Clear();
    m_usersById.RemoveAll();
    for (int network = 0; network < NETWORK_TABLE_COUNT; network++) {
        m_usersByNetworkId[network].RemoveAll();
    }
}

CNGSRemoteUserList::~CNGSRemoteUserList()
{
}

void CNGSRemoteUserList::addRemoteUserIfNotAlreadyInList(CNGSRemoteUser* user)
{
    void* found = NULL;
    if (user->GetClientID() > 0) {
        m_usersById.Find(user->GetClientID(), &found);
    }
    if (found == NULL) {
        m_users.Add(user);
        m_usersById.Insert(user->GetClientID(), user);
        for (int network = 0; network < NETWORK_TABLE_COUNT; network++) {
            if (user->IsFromSocialNetwork(network)) {
                m_usersByNetworkId[network].Insert(CStringToKey(user->m_credentials.m_socialIds[network].GetWCharPtr(), 0), user);
            }
        }
    }
}

void CNGSRemoteUserList::removeRemoteUser(int index)
{
    CNGSRemoteUser* user = m_users[index];
    if (user->GetClientID() > 0) {
        m_usersById.Remove(user->GetClientID());
        for (int network = 0; network < NETWORK_TABLE_COUNT; network++) {
            if (user->IsFromSocialNetwork(network)) {
                m_usersByNetworkId[network].Remove(CStringToKey(user->m_credentials.m_socialIds[network].GetWCharPtr(), 0));
            }
        }
        CNGS::GetInstance()->GetFactory()->removeRemoteUserByID(user->GetClientID());
    }
    m_users.RemoveAt(index);
}

CNGSRemoteUser* CNGSRemoteUserList::findRemoteUserByID(int userId)
{
    void* found = NULL;
    CNGSRemoteUser* user = NULL;
    if (userId > 0) {
        m_usersById.Find(userId, &found);
        user = (CNGSRemoteUser*)found;
    }
    return user;
}
