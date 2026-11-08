// Owner: userManagement/CNGSRemoteUser.cpp (b030x; started as a b016x PROVISIONAL)
// CNGSRemoteUser.h - another player known to the local user (friend or guest): can be looked up
// by nickname on the server and carries the profile collections downloaded for it.
#ifndef CNGSREMOTEUSER_H
#define CNGSREMOTEUSER_H

#include <stddef.h>
#include "CNGSUser.h"
#include "CNGSServerRequest.h"
#include "CVector.h"

class CObjectMap;
class CSaveRestoreInterface;
class CNGSRemoteUserFunctor;

class CNGSRemoteUser : public CNGSUser {
public:
    /** With incomplete credentials that still name a nickname the user is looked up on the server. */
    CNGSRemoteUser(const CNGSUserCredentials* credentials);
    /** Ask the server for the user named like our nickname; returns the request id (0: nothing sent). */
    unsigned int PopulateViaNickname();
    virtual ~CNGSRemoteUser();

    virtual unsigned char IsLocalUser();
    /** No request outstanding and the content file collections (if any) can be downloaded. */
    virtual unsigned char isAvailable();
    // Slots +0x30 / +0x34: empty (imported, probably pure) in the original vtable data; the game calls
    // +0x34 with 1 from CNotificationHandler::HandleFriendDeepPopulated. Names are guesses.  // added by b047x
    // Former declaration: virtual void reservedVirtual30();  // added by b047x
    // Former declaration: virtual void SetDeepPopulated(unsigned char populated);  // added by b047x
    // l: the original concrete class can be new'd at 0x78; real vtable has valid methods up to +2c; the two b047 guesses above are no longer emitted as virtual slots.
    // The dynamic +34 dispatch at 171438 keeps the ABI view in the caller; the unknown dynamic type/method name is still tracked as open debt.

    void SetIsFriendOfLocalUser(unsigned char isFriend);
    void HandlePopulateViaNicknameResponse(CObjectMap* response, CNGSRemoteUserFunctor* functor);
    /** The user came from social network `network` (bit set of networks). */
    void AddSocialNetworkType(int network);
    void RemoveSocialNetworkType(int network);
    unsigned char IsFromSocialNetwork(int network) const;
    unsigned int GetSocialNetworkType() const;
    /** A content file of this friend arrived (forwarded to the notification handler). */  // added by b025x
    unsigned char HandleContentFileDownloadedFriend(unsigned char success, int userId, const unsigned char* data, int size);  // added by b025x (b030x: returns 1)

    unsigned char m_isFriendOfLocalUser;                                        // +0x48
    unsigned long long m_friendMsgTimestamp;                                    // +0x4c last message time (setFriendMsgTimestampLocally)
    unsigned int m_socialNetworkTypes;                                          // +0x54 1 << network per network
    com::glu::platform::core::CVector<CSaveRestoreInterface*> m_profileData;   // +0x58 downloaded collections
    int m_profileDataLimit;                                                     // +0x70 loop bound of HandleContentFileDownloadedFriend (only cleared here)
    int m_reserved74;                                                           // +0x74 never touched here (sizeof is 0x78)
};

/** Routes a server reply to a CNGSRemoteUser member function. */
class CNGSRemoteUserFunctor : public CNGSServerRequestFunctor {
public:
    typedef void (CNGSRemoteUser::*Callback)(CObjectMap* response, CNGSRemoteUserFunctor* functor);

    CNGSRemoteUserFunctor(CNGSRemoteUser* target, Callback callback)
        : m_target(target)
        , m_callback(callback)
        , m_context(NULL)
    {
    }

    virtual ~CNGSRemoteUserFunctor() {}

    virtual void requestCompleteCallback(CObjectMap* response)
    {
        if (m_callback && m_target != NULL) {
            (m_target->*m_callback)(response, this);
        }
    }

    CNGSRemoteUser* m_target;   // +0x08
    int m_param;                // +0x0c not set by this constructor
    Callback m_callback;        // +0x10
    void* m_context;            // +0x18 NULL
};

#endif
