// Owner: userManagement/CNGSLocalUser.cpp (b030x; started as a b016x PROVISIONAL)
// CNGSLocalUser.h - the player of this device: login / validation against the gServe server per
// social network, the friend lists, the session, the server message queue and the notification
// handler of the game. Also the completion functors of the local user's requests.
#ifndef CNGSLOCALUSER_H
#define CNGSLOCALUSER_H

#include <stddef.h>
#include "CNGSUser.h"
#include "CNGS.h"
#include "CStrWChar.h"
#include "CNGSFunctor.h"
#include "CNGSServerRequest.h"

class CNGSRemoteUserList;
class CNGSFromServerMessageQ;
class CNGSSession;
class CNGSSKUBonus;
class CNotificationHandler;
class CNGSServerRequest;  // added by b027x
class CObjectMap;
class CNGSLocalUserFunctor;
class CNGSLocalUserNotifyFunctor;

class CNGSLocalUser : public CNGSUser {
public:
    CNGSLocalUser(const CNGSUserCredentials* credentials);
    virtual ~CNGSLocalUser();

    void registerNotificationHandler(CNotificationHandler* handler);
    virtual void SetIsValid(unsigned char isValid);
    void CheckServerVersionCompatibility();     // added by b023x
    /** {"id", "gamertag", "email", "password", "facebookid"} of this user (the caller owns it). */
    CObjectMapObject* GetObjectMapRepresentationForUpdateUserInfo();
    void LoadCredentials();  // added by b027x
    /** b145: call-free accessor (CNGS::Resume reads the session through it; shifts its inline order). */
    CNGSSession* GetSession() const { return m_session; }
    /** Logged in on `network` (0: on any network). */
    unsigned char isAuthenticated(int network);
    /** Ask for the last message times of the friends; 0 when `notify` cannot be queued now. */
    unsigned char friendLastMsgCheck(CNGSNotifyFunctor* notify);
    void handleFriendLastMsgCheckResponse(CObjectMap* response, CNGSLocalUserFunctor* functor);
    /** Client id encoded after the '_' of a friend timestamp attribute name (0 when none). */
    int extractClientIdFromFriendTimestamp(CNGSAttribute& attribute);
    void HandleValidateFriendListFromServerResponse(CObjectMap* response, CNGSLocalUserFunctor* functor);

    // vtable slots +0x2c..+0x48 in the original order  // added by b027x
    virtual unsigned char HandleGameSpecificMessage(long long messageId, const com::glu::platform::components::CStrWChar& text, long long value);  // added by b027x (b030x: returns 1)
    virtual unsigned char HandleGenericYesNoMessage(CObjectMapObject* message, int messageId);  // added by b027x (b030x: returns 1)
    virtual unsigned char HandleGenericNotifyMessage(com::glu::platform::components::CStrWChar& text, int messageId);  // added by b027x (b030x: returns 1)
    virtual unsigned char HandleNotifyMessageWithCommand(com::glu::platform::components::CStrWChar& text, CNGSServerRequest* command, int messageId);  // added by b027x (b030x: returns 1)
    virtual bool HandleBonusMessage(bool success, long long messageId, com::glu::platform::components::CStrWChar& text, com::glu::platform::core::CVector<CNGSAttribute*>& items, int amount);  // added by b027x (b030x: returns `success`)

    void CNGSHandleUserLogin(int result, int network, const wchar_t* message);  // added by b027x
    /** Add the alias of `network` to the user's attributes; returns the request id. */
    unsigned int AddUserAttribute(int network);
    void CNGSHandleUserLogout(unsigned char success, int network);  // added by b027x
    void CNGSHandlePrimaryUserLogout();
    /** The friend list of `network` was loaded (`authenticated`: the user is logged in there). */
    void CNGSHandleFriendListUpdate(unsigned char success, unsigned char authenticated, int network, int unused);
    void DebugPrintFriends();
    /** Invalidate the local copy of every registered profile collection. */
    void invalidateLocalData(unsigned char reset);
    void CNGSHandlePrimaryUserChange(int network, const char* caller);  // added by b027x
    /** Empty the friend lists and log out of every social network. */
    void Deauthenticate();
    unsigned char CNGSHandleSessionInvalid(const char* caller);
    void CNGSHandleGenericServerError(const char* caller);
    void CNGSHandleVersionCheck(unsigned char success, const char* caller);
    unsigned char CNGSHandleServerConnectionError(const char* caller);
    unsigned char CNGSHandleServerConnectionRestore(eNetworkConnectionState previousState);
    /** Reuse the saved session: check the network time, then poll the messages. */
    unsigned char AttemptToContinueSession(int network);
    void AttemptToContineSessionTimeCheckCompletion(CNGSLocalUserNotifyFunctor* functor);
    void HandleAttemptToContinueSession(CNGSLocalUserNotifyFunctor* functor);
    /** Send the user info (GetObjectMapRepresentationForUpdateUserInfo) to the server. */
    void UpdateUserInfo(int network);
    /** Validate the user of `network` with the server (or continue the saved session); returns the request id. */
    unsigned int ValidateUser(int network, unsigned char force);  // added by b027x (b030x: returns the request id)
    void HandleValidateUserResponse(CObjectMap* response, CNGSLocalUserFunctor* functor);
    void NotifyOfNetTimeCheckCompletion(CNGSLocalUserNotifyFunctor* functor);
    /** Error text of a failed reply into `message` (`path` is not used). */
    int ExtractErrorMessage(CObjectMap* response, com::glu::platform::components::CStrWChar path,
                            com::glu::platform::components::CStrWChar& message);
    unsigned int RegisterUser(int network);
    void HandleUpdateUserInfoResponse(CObjectMap* response, CNGSLocalUserFunctor* functor);
    void HandleRegisterUserResponse(CObjectMap* response, CNGSLocalUserFunctor* functor);
    unsigned int AssociateUser(int network);  // added by b027x (b030x: returns the request id)
    void HandleAssociateUserResponse(CObjectMap* response, CNGSLocalUserFunctor* functor);
    void HandleLoadFriendListFromServerResponse(int network);  // added by b027x
    /** Look the friends of `network` up on the server, 100 per request. */
    unsigned char ValidateFriendListFromServer(int network);
    void HandleAddUserAttributeResponse(CObjectMap* response, CNGSLocalUserFunctor* functor);
    unsigned int ModifyUserAttribute(int network);
    void HandleModifyUserAttributeResponse(CObjectMap* response, CNGSLocalUserFunctor* functor);

    // b116: HandlePostMessage is declared before AddReadRequest: the original vtable has it at +0x48
    // (slot 0x40), AddReadRequest / CompleteReadRequest follow (0x44 / 0x48). In the old order every
    // AddReadRequest() call (Facebook / Game Center login, logout, friends) went to HandlePostMessage.
    virtual unsigned char HandlePostMessage(unsigned char success, int network);  // added by b027x (b030x: returns 1)
    /** One more read request outstanding (the social login waits for its result). */  // added by b027x
    virtual void AddReadRequest();  // added by b027x
    virtual void CompleteReadRequest();  // added by b027x
    virtual void GetSKUBonusItems(com::glu::platform::components::CStrWChar& sku, com::glu::platform::core::CVector<CNGSAttribute*>& items);
    virtual com::glu::platform::components::CStrWChar GetSKUBonusNameForItem(CNGSAttribute* item);
    void CNGSHandlePostMessage(unsigned char success, int network);  // added by b027x
    void CNGSHandleFriendPicUpdate(unsigned char success, int network);  // added by b027x

    virtual unsigned char IsLocalUser()
    {
        return 1;
    }

    CNGSRemoteUserList* m_remoteUserLists[3];                   // +0x48 [0] friends with avatar, [1] friends, [2] other users
    com::glu::platform::components::CStrWChar m_password;      // +0x54
    CNGSFromServerMessageQ* m_messageQueue;                     // +0x64
    CNGSSession* m_session;                                     // +0x68
    unsigned char m_flag6c;                                     // +0x6c cleared only (meaning unknown)
    CNGSSKUBonus* m_skuBonus;                                   // +0x70
    CNotificationHandler* m_notificationHandler;                // +0x74
    unsigned int m_authenticatedNetworks;                       // +0x78 1 << network per logged in network
};

/** Calls (m_target->*m_callback)(this) when a CNGSServerObject sync completes. */
class CNGSServerObjectNotifyFunctor : public CNGSNotifyFunctor {
public:
    typedef void (CNGSServerObject::*Callback)(CNGSNotifyFunctor* functor);

    CNGSServerObjectNotifyFunctor(int param, CNGSServerObject* target, Callback callback)
        : m_param(param)
        , m_target(target)
        , m_callback(callback)
    {
    }

    virtual ~CNGSServerObjectNotifyFunctor() {}

    virtual void notify()
    {
        if (m_callback) {
            (m_target->*m_callback)(this);
        }
    }

    int m_param;                    // +0x04
    CNGSServerObject* m_target;     // +0x08
    Callback m_callback;            // +0x0c
};

/** Calls (m_target->*m_callback)(this) when a sync of the local user's session / queue completes. */
class CNGSLocalUserNotifyFunctor : public CNGSNotifyFunctor {
public:
    typedef void (CNGSLocalUser::*Callback)(CNGSLocalUserNotifyFunctor* functor);

    CNGSLocalUserNotifyFunctor(int network, CNGSLocalUser* target, Callback callback)
        : m_param(network)
        , m_target(target)
        , m_callback(callback)
    {
    }

    virtual ~CNGSLocalUserNotifyFunctor() {}

    virtual void notify()
    {
        if (m_callback) {
            (m_target->*m_callback)(this);
        }
    }

    int m_param;                    // +0x04 social network of the request
    CNGSLocalUser* m_target;        // +0x08
    Callback m_callback;            // +0x0c
};

/** Routes a server reply to a CNGSLocalUser member function. */
class CNGSLocalUserFunctor : public CNGSServerRequestFunctor {
public:
    typedef void (CNGSLocalUser::*Callback)(CObjectMap* response, CNGSLocalUserFunctor* functor);

    CNGSLocalUserFunctor(CNGSLocalUser* target, int network, Callback callback, int context)
        : m_target(target)
        , m_network(network)
        , m_callback(callback)
        , m_context(context)
    {
    }

    virtual ~CNGSLocalUserFunctor() {}

    virtual void requestCompleteCallback(CObjectMap* response)
    {
        if (m_callback && m_target != NULL) {
            (m_target->*m_callback)(response, this);
        }
    }

    CNGSLocalUser* m_target;    // +0x08
    int m_network;              // +0x0c social network of the request
    Callback m_callback;        // +0x10
    int m_context;              // +0x18 force flag (ValidateUser) / number of users asked (friend search)
};

#endif
