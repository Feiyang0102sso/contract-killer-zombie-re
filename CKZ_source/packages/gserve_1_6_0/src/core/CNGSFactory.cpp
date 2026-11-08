// CNGSFactory.cpp - default user factory of gServe.
#include "CNGSFactory.h"
#include "CNGSUserCredentials.h"
#include "CNGSUser.h"
#include "CNGSLocalUser.h"
#include "CNGSRemoteUser.h"

using com::glu::platform::components::CHash;

/** Initial bucket count and node pool size of the remote user cache. */
static const unsigned long REMOTE_USER_HASH_SIZE = 10;

CNGSFactory::CNGSFactory()
    : m_remoteUsers(NULL)
{
    m_remoteUsers = new CHash();
    m_remoteUsers->Init(REMOTE_USER_HASH_SIZE, REMOTE_USER_HASH_SIZE, 1);
}

CNGSFactory::~CNGSFactory()
{
    if (m_remoteUsers != NULL) {
        m_remoteUsers->Destroy();
        if (m_remoteUsers != NULL) {
            delete m_remoteUsers;
            m_remoteUsers = NULL;
        }
    }
}

CNGSRemoteUser* CNGSFactory::getRemoteUserByCredentials(const CNGSUserCredentials& credentials)
{
    CNGSRemoteUser* user = NULL;
    if (credentials.m_userId > 0) {
        user = findExistingRemoteUserByID(credentials.m_userId);
        if (user == NULL) {
            user = createRemoteUser(credentials);
            m_remoteUsers->Insert(credentials.m_userId, user);
        } else {
            user->m_credentials.mergeWith(credentials, false);
        }
    }
    return user;
}

void CNGSFactory::removeRemoteUserByID(int userId)
{
    if (userId < 1) {
        return;
    }
    if (m_remoteUsers != NULL) {
        m_remoteUsers->Remove(userId);
    }
}

CNGSRemoteUser* CNGSFactory::findExistingRemoteUserByID(int userId)
{
    void* found = NULL;
    CNGSRemoteUser* user = NULL;
    if (userId > 0) {
        user = NULL;
        if (m_remoteUsers != NULL) {
            m_remoteUsers->Find(userId, &found);
            user = (CNGSRemoteUser*)found;
        }
    }
    return user;
}

CNGSLocalUser* CNGSFactory::createLocalUser(const CNGSUserCredentials& credentials)
{
    return new CNGSLocalUser(&credentials);
}

CNGSRemoteUser* CNGSFactory::createRemoteUser(const CNGSUserCredentials& credentials)
{
    return new CNGSRemoteUser(&credentials);
}
