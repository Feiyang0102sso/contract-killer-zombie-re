// CNGSFactory.h - creates the local and remote gServe users and caches the remote ones by user id.
// Games derive from it to create their own user classes.
#ifndef CNGSFACTORY_H
#define CNGSFACTORY_H

#include <stddef.h>
#include "CHash.h"

class CNGSUserCredentials;
class CNGSLocalUser;
class CNGSRemoteUser;

class CNGSFactory {
public:
    CNGSFactory();

    virtual CNGSRemoteUser* createRemoteUser(const CNGSUserCredentials& credentials);
    virtual CNGSLocalUser* createLocalUser(const CNGSUserCredentials& credentials);
    virtual ~CNGSFactory();

    /** Cached remote user with the credentials' id (merged with `credentials`), created when new; NULL for id <= 0. */
    CNGSRemoteUser* getRemoteUserByCredentials(const CNGSUserCredentials& credentials);
    void removeRemoteUserByID(int userId);
    CNGSRemoteUser* findExistingRemoteUserByID(int userId);

    com::glu::platform::components::CHash* m_remoteUsers;     // +0x04 user id -> CNGSRemoteUser*
};

#endif
