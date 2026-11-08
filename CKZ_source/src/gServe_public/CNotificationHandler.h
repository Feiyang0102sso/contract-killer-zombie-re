// CNotificationHandler.h - game side receiver of gServe events (registered with
// CNGSLocalUser::registerNotificationHandler). (created by b016x as PROVISIONAL, completed by b047x,
// the owner of src/gServe_public/CNotificationHandler.cpp)
// Virtual slots in the original vtable order (+0x00 .. +0xe4). Every handler returns a flag
// (`movs r0, #1` / `#0` in the original): changed by b047x from void to unsigned char where needed.
#ifndef CNOTIFICATIONHANDLER_H
#define CNOTIFICATIONHANDLER_H

#include <wchar.h>
#include "CStrWChar.h"
#include "CVector.h"
#include "CSaveRestoreInterface.h"  // added by b023x

class CNGSRemoteUser;
class CNGSDirectFileDownloadRequest;
class CNGSLockMgrResponse;
class CNGSLock;
class CNGSSharedData;

class CNotificationHandler {
public:
    CNotificationHandler();  // added by b047x

    virtual unsigned char HandleUserLogin(int userId, int result, const wchar_t* message);  // changed by b047x: returns unsigned char
    virtual unsigned char HandleUserLogout(unsigned char success, int network);  // changed by b047x: returns unsigned char
    virtual unsigned char HandlePrimaryUserLogout();  // changed by b047x: returns unsigned char
    virtual unsigned char HandlePrimaryUserChanged(int userId);  // changed by b047x: returns unsigned char
    virtual unsigned char HandleFriendListUpdate(unsigned char success, int count);  // changed by b047x: returns unsigned char
    virtual unsigned char HandleFriendDeepPopulated(CNGSRemoteUser* user, unsigned char success);  // changed by b047x: returns unsigned char
    virtual unsigned char HandleGuestDeepPopulated(CNGSRemoteUser* user, unsigned char success);  // changed by b047x: returns unsigned char
    virtual unsigned char HandleRandomNonFriendListUpdate(unsigned char success, int count);  // changed by b047x: returns unsigned char
    virtual unsigned char HandleSessionInvalid();  // changed by b030x: returns unsigned char (CNGSLocalUser::CNGSHandleSessionInvalid returns the result)
    virtual unsigned char HandleSessionError();  // changed by b047x: returns unsigned char
    virtual unsigned char HandleGenericServerError();  // changed by b047x: returns unsigned char
    virtual unsigned char HandleServerConnectionError();  // changed by b030x: returns unsigned char (CNGSLocalUser::CNGSHandleServerConnectionError returns the result)
    virtual unsigned char HandleServerConnectionRestore();  // changed by b030x: returns unsigned char (CNGSLocalUser::CNGSHandleServerConnectionRestore returns the result)
    virtual unsigned char HandleVersionInvalid();  // changed by b047x: returns unsigned char
    virtual unsigned char HandleVersionSuccess();  // changed by b047x: returns unsigned char
    virtual unsigned char HandleProfileLoaded(unsigned char success, const com::glu::platform::core::CVector<int>& dataIds);  // changed by b047x: returns unsigned char
    virtual unsigned char HandleProfileDataConflict(const com::glu::platform::core::CVector<int>& dataIds);  // changed by b047x: returns unsigned char
    virtual unsigned char HandleProfileSaved(unsigned char success, const com::glu::platform::core::CVector<int>& dataIds);  // changed by b047x: returns unsigned char
    virtual unsigned char HandleAddExperienceGiftMessage(long long messageId, int amount, const com::glu::platform::components::CStrWChar& text);  // changed by b047x: returns unsigned char
    virtual unsigned char HandleAddItemsGiftMessage(long long messageId, long long itemId, int amount, const com::glu::platform::components::CStrWChar& text);  // changed by b047x: returns unsigned char
    virtual unsigned char HandleAddRareCurrencyGiftMessage(long long messageId, int amount, const com::glu::platform::components::CStrWChar& text);  // changed by b047x: returns unsigned char
    virtual unsigned char HandleAddCommonCurrencyGiftMessage(long long messageId, int amount, const com::glu::platform::components::CStrWChar& text);  // changed by b047x: returns unsigned char
    virtual unsigned char HandleGift(int giftType, const com::glu::platform::components::CStrWChar& text, int amount);  // changed by b047x: returns unsigned char
    virtual unsigned char HandleIncentive(unsigned char success, int amount);  // changed by b047x: returns unsigned char
    virtual unsigned char HandleGenericNotifyMessage(int messageId, const com::glu::platform::components::CStrWChar& text);  // changed by b047x: returns unsigned char
    virtual unsigned char HandleContentFileUploaded(unsigned char success);  // changed by b047x: returns unsigned char
    virtual unsigned char HandleContentFileDownloadedSelf(unsigned char success, const unsigned char* data, int size);  // changed by b047x: returns unsigned char
    virtual unsigned char HandleContentFileDownloadedFriend(unsigned char success, int userId, const unsigned char* data, int size);  // changed by b047x: returns unsigned char
    virtual unsigned char HandleContentFileDeleted(unsigned char success);  // changed by b047x: returns unsigned char
    virtual unsigned char HandlePostMessage(unsigned char success, int network);  // changed by b047x: returns unsigned char
    virtual unsigned char HandleFileDownloadedMessage(unsigned char success, CNGSDirectFileDownloadRequest& request);  // changed by b047x: returns unsigned char
    virtual unsigned char HandleBatchFileDownloadStartedMessage(const com::glu::platform::components::CStrWChar& batch, int count);  // changed by b047x: returns unsigned char
    virtual unsigned char HandleBatchFileDownloadCompleteMessage(const com::glu::platform::components::CStrWChar& batch, int count);  // changed by b047x: returns unsigned char
    virtual unsigned char HandleGameCenterAppLogin(unsigned char success);  // changed by b047x: returns unsigned char
    virtual unsigned char HandleLoginFlowNextStep();  // changed by b047x: returns unsigned char
    /** A saved profile file has an unknown format; return 1 with a replacement in data / size. */  // changed by b025x
    virtual unsigned char HandleProfileDataFormatIncorrect(const com::glu::platform::components::CStrWChar& name,  // changed by b025x: returns unsigned char (CProfileManager::loadFromFileSystem tests it; the default returns 0)
        const com::glu::platform::components::CStrWChar& format, unsigned char*& data, unsigned int& size);
    virtual unsigned char HandleFeaturedGameSelected(unsigned char success, int gameId);  // changed by b047x: returns unsigned char
    virtual unsigned char HandleLockMgrGetSharedData(const CNGSLockMgrResponse& response, const CNGSLock* lock);  // changed by b047x: returns unsigned char
    virtual unsigned char HandleLockMgrSaveSharedData(const CNGSLockMgrResponse& response, const CNGSLock* lock);  // changed by b047x: returns unsigned char
    virtual unsigned char HandleLockMgrQueryAllSharedData(const CNGSLockMgrResponse& response, com::glu::platform::core::CVector<CNGSSharedData*>& data);  // changed by b047x: returns unsigned char
    virtual unsigned char HandleLockMgrReleaseLock(const CNGSLockMgrResponse& response, const CNGSLock* lock);  // changed by b047x: returns unsigned char
    virtual unsigned char HandleFriendPicUpdate(unsigned char success, int userId);  // changed by b047x: returns unsigned char
    virtual unsigned char HandleInitializeGluOffers(unsigned char success, int count);  // changed by b047x: returns unsigned char
    virtual unsigned char HandleSendInvite(unsigned char success, int count);  // changed by b047x: returns unsigned char
    virtual unsigned char HandleGluOffersReset(unsigned char success, int count);  // changed by b047x: returns unsigned char
    /** The automatic server message poll started (CNGS::HandleUpdate). */
    virtual unsigned char HandleAutoPollStart();  // changed by b047x: returns unsigned char
    virtual unsigned char HandleProfileSaved(ProfileDataSaveStatus status, const com::glu::platform::core::CVector<int>& dataIds);  // added by b023x (b047x: returns unsigned char)
    /** A message poll reply arrived (CNGSFromServerMessageQ). */  // added by b023x
    virtual unsigned char HandleReceivedEnvelope();  // added by b023x (b047x: returns unsigned char)
    virtual unsigned char HandleNewMessagesReceived();  // added by b023x (b047x: returns unsigned char)
    /** Ids left unacknowledged by the last run were loaded. */  // added by b023x
    virtual unsigned char HandleOldUnackedMessages();  // added by b023x (b047x: returns unsigned char)
    virtual unsigned char HandleGameSpecificMessage(long long messageType, const com::glu::platform::components::CStrWChar& text, long long senderId);  // added by b027x (b047x: returns unsigned char; the first argument is the message type)
    /** The Facebook login web page closed; 0 = do not pass the URL on to Facebook (CFacebookManager). */  // added by b027x
    virtual unsigned char HandleFacebookLoginPageClose();  // added by b027x
    virtual unsigned char HandleGameCenterAppLogin(const com::glu::platform::components::CStrWChar& playerId);  // added by b047x (+0xd0)
    virtual unsigned char HandleGameCenterAppFriendsListUpdate(const com::glu::platform::components::CStrWChar& friends);  // added by b047x (+0xd4)
    virtual unsigned char HandleProfileContentFileDownloaded(unsigned char success, int collection);  // added by b047x (+0xd8)
    virtual unsigned char HandleProfileContentFileUploaded(unsigned char success, int collection);  // added by b047x (+0xdc)
    virtual ~CNotificationHandler();  // added by b047x (+0xe0 / +0xe4)

    // Layout from the constructor and destructor (added by b047x).
    com::glu::platform::core::CVector<long long> m_giftMessageIds;     // 0x04 gift messages kept for the gift screen
    int m_loginState;                                                   // 0x1c 3: Facebook login page open
    int m_localSoftCurrency;                                            // 0x20 data conflict: device copy
    int m_localHardCurrency;                                            // 0x24
    int m_localExperience;                                              // 0x28
    int m_serverSoftCurrency;                                           // 0x2c data conflict: server copy
    int m_serverHardCurrency;                                           // 0x30
    int m_serverExperience;                                             // 0x34
    com::glu::platform::components::CStrWChar m_localDataText;          // 0x38 data conflict: device copy summary
    com::glu::platform::components::CStrWChar m_serverDataText;         // 0x48 data conflict: server copy summary
};

#endif
