// CNotificationHandler.cpp - the game's receiver of gServe events: login / logout results, profile
// data conflicts, gifts, awards and game specific server messages.
#include "AdBannerSizes.h"
#include "CNotificationHandler.h"
#include "ICInAppPurchase.h"
#include "CNGSOfferManager.h"
#include <stdlib.h>
#include <wchar.h>
#include "CNGS.h"
#include "CNGS_Platform.h"
#include "CNGSLoginFlow.h"
#include "CNGSLocalUser.h"
#include "CNGSRemoteUser.h"
#include "CNGSRemoteUserList.h"
#include "CNGSSocialInterface.h"
#include "CNGSFromServerMessageQ.h"
#include "CNGSFromServerMessage.h"
#include "CProfileManager.h"
#include "CPrivateData1.h"
#include "CGServeHandler.h"
#include "CBH_Player.h"
#include "CNewsFeedManager.h"
#include "CGameApp.h"
#include "App.h"

using namespace com::glu::platform::components;
using namespace com::glu::platform::core;

/** Server message types handled by the game (CNGSFromServerMessage::m_type). */
enum {
    MESSAGE_TYPE_GIFT = 0x28,
    MESSAGE_TYPE_AWARD = 0x29,
    MESSAGE_TYPE_GIFT_2 = 0x64,
    MESSAGE_TYPE_IGNORED = 300,
    MESSAGE_TYPE_GAME_SPECIFIC = 1000,          // and above: HandleGameSpecificMessage
    MESSAGE_TYPE_TARGETTED_HIGHLIGHTED = 1001,
    MESSAGE_TYPE_MISSION_INVITE = 1002,
    MESSAGE_TYPE_TARGETTED = 1004
};

/** Profile collections of the game (CProfileManager collection types). */
enum {
    COLLECTION_PROGRESS = 1000,
    COLLECTION_PRIVATE_DATA = 1001
};

unsigned char CNotificationHandler::HandleUserLogin(int userId, int result, const wchar_t* message)
{
    if (result == 2) {
        m_loginState = 0;
    }
    if (!CNGSLoginFlow::GetInstance()->IsRunning()) {
        ((App*)WindowApp::m_instance)->m_gserveHandler->CloseCurrentSocialNetworkConnectDialog();
        if (result == 2) {
            WindowApp::HandleTunnelCommand(0xb3593b79, 0, 0, 0);
        } else if (result == 1) {
            WindowApp::HandleTunnelCommand(0xbb215081, 0, 0, 0);
        }
    }
    CNGSSocialInterface* social = CNGS_Platform::GetInstance()->getSocialInterface(2);
    if (social != NULL) {
        social->getFriendsCount();
    }
    social = CNGS_Platform::GetInstance()->getSocialInterface(1);
    if (social != NULL) {
        social->getFriendsCount();
    }
    // The CNGS lookup is spelled out here: its slot lies below the inlined GetInstance() slots in the
    // original frame, i.e. it is a local of this function (b016x saw the same in CNGS::Resume).
    void* foundNgs = NULL;
    CApplet::m_pApp->m_singletons->Find(CNGS::ClassId, &foundNgs);
    CNGS* ngs = (CNGS*)foundNgs;
    if (ngs == NULL) {
        ngs = new CNGS();
    }
    ngs->GetLocalUser()->m_messageQueue->ProcessPromptMessages();
    return 1;
}

unsigned char CNotificationHandler::HandleUserLogout(unsigned char success, int network)
{
    if (network == 2) {
        WindowApp::HandleTunnelCommand(0x47f46330, 0, 0, 0);
    } else if (network == 1) {
        WindowApp::HandleTunnelCommand(0xf8a2664d, 0, 0, 0);
    }
    return 1;
}

unsigned char CNotificationHandler::HandlePrimaryUserLogout()
{
    return 1;
}

unsigned char CNotificationHandler::HandlePrimaryUserChanged(int userId)
{
    return 1;
}

unsigned char CNotificationHandler::HandleFriendListUpdate(unsigned char success, int count)
{
    WindowApp::HandleTunnelCommand(0xe4fd16fb, count, 0, 0);
    return 1;
}

unsigned char CNotificationHandler::HandleFriendDeepPopulated(CNGSRemoteUser* user, unsigned char success)
{
    // l: original 17143a..171444 reads dynamic table +34; the unproven slot name is not added to the vtable of the concrete library class.
    typedef void (*OriginalDeepPopulateCallback)(CNGSRemoteUser*, unsigned char);
    OriginalDeepPopulateCallback* table = *(OriginalDeepPopulateCallback**)user;
    OriginalDeepPopulateCallback callback = table[0x34 / sizeof(OriginalDeepPopulateCallback)];
    callback(user, 1);
    return 1;
}

unsigned char CNotificationHandler::HandleGuestDeepPopulated(CNGSRemoteUser* user, unsigned char success)
{
    return 0;
}

unsigned char CNotificationHandler::HandleSessionInvalid()
{
    if (!CNGS::GetInstance()->GetLocalUser()->isAuthenticated(1)) {
        CNGS::GetInstance()->GetLocalUser()->isAuthenticated(2);
    }
    ((App*)WindowApp::m_instance)->m_field3c8 = true;
    ((App*)WindowApp::m_instance)->m_gserveHandler->ShowSocialNetworkSessionInvalidDialog();
    return 1;
}

unsigned char CNotificationHandler::HandleSessionError()
{
    return 1;
}

unsigned char CNotificationHandler::HandleGenericServerError()
{
    return 1;
}

unsigned char CNotificationHandler::HandleServerConnectionError()
{
    return 1;
}

unsigned char CNotificationHandler::HandleServerConnectionRestore()
{
    return 1;
}

unsigned char CNotificationHandler::HandleVersionInvalid()
{
    if (!CNGSLoginFlow::GetInstance()->IsRunning()) {
        WindowApp::HandleTunnelCommand(0x6458fa9c, 0, 0, 0);
    }
    return 1;
}

unsigned char CNotificationHandler::HandleVersionSuccess()
{
    return 1;
}

unsigned char CNotificationHandler::HandleProfileLoaded(unsigned char success, const CVector<int>& dataIds)
{
    return 1;
}

unsigned char CNotificationHandler::HandleProfileSaved(unsigned char success, const CVector<int>& dataIds)
{
    return 1;
}

unsigned char CNotificationHandler::HandleProfileDataConflict(const CVector<int>& dataIds)
{
    m_localDataText = L"";
    m_serverDataText = L"";
    if (dataIds.m_size > 0) {
        unsigned char conflict = 0;
        for (int index = 0; index < dataIds.m_size; index++) {
            if (dataIds.m_data[index] < COLLECTION_PROGRESS) {
                continue;
            }
            CPrivateData1* localData = (CPrivateData1*)((App*)WindowApp::m_instance)->m_profileManager  // changed by fx2
                ->getManagedObjectFromBuffer(PROFILE_DATA_SAVED_LOCAL, dataIds.m_data[index]);
            CPrivateData1* serverData = (CPrivateData1*)((App*)WindowApp::m_instance)->m_profileManager  // changed by fx2
                ->getManagedObjectFromBuffer(PROFILE_DATA_SYNCED, dataIds.m_data[index]);
            if (localData->getDataSaveStatus() == PROFILE_DATA_INVALIDATED) {
                // nothing worth keeping on the device: take the server data without asking
                if (dataIds.m_data[index] == COLLECTION_PRIVATE_DATA) {
                    if (localData->getExperience() == 0) {
                        goto useServerData;
                    }
                } else if (dataIds.m_data[index] == COLLECTION_PROGRESS) {
                    if (localData->m_progress == 0) {
                        goto useServerData;
                    }
                }
            }
            conflict = 1;
            if (dataIds.m_data[index] == COLLECTION_PRIVATE_DATA) {
                m_localDataText += localData->getDescription() + "\n";
                m_serverDataText += serverData->getDescription() + "\n";
                m_localSoftCurrency = (int)localData->getSoftCurrency();
                m_localHardCurrency = (int)localData->getHardCurrency();
                m_localExperience = (int)localData->getExperience();
                m_serverSoftCurrency = (int)serverData->getSoftCurrency();
                m_serverHardCurrency = (int)serverData->getHardCurrency();
                m_serverExperience = (int)serverData->getExperience();
            }
        }
        if (conflict) {
            ((App*)WindowApp::m_instance)->m_gserveHandler->CloseCurrentSocialNetworkConnectDialog();
            CGameApp::gserveDataConflict = true;
            WindowApp::HandleTunnelCommand(0x07e17dc8, 0, 0, 0);
        }
    }
    return 1;

useServerData:
    // the collections block comes last in the original (lowest stack slot)
    {
        CVector<int> collections;
        ((App*)WindowApp::m_instance)->m_profileManager->getAllRegisteredCollections(collections);  // changed by fx2
        ((App*)WindowApp::m_instance)->m_gserveHandler->SelectProfileServerData(collections);
    }
    return 1;
}

/** The experience arrives in the first argument (only its low word is used). */
unsigned char CNotificationHandler::HandleAddExperienceGiftMessage(long long experience, int amount, const CStrWChar& text)
{
    CBH_Player::GetInstance()->AddXp((int)experience);
    return 1;
}

unsigned char CNotificationHandler::HandleAddItemsGiftMessage(long long messageId, long long itemId, int amount, const CStrWChar& text)
{
    return 1;
}

unsigned char CNotificationHandler::HandleAddRareCurrencyGiftMessage(long long currency, int amount, const CStrWChar& text)
{
    CBH_Player::GetInstance()->AddCredits((int)currency, L"");
    return 1;
}

unsigned char CNotificationHandler::HandleAddCommonCurrencyGiftMessage(long long currency, int amount, const CStrWChar& text)
{
    CBH_Player::GetInstance()->AddMoney((int)currency);
    return 1;
}

unsigned char CNotificationHandler::HandleGift(int giftType, const CStrWChar& text, int amount)
{
    return 1;
}

unsigned char CNotificationHandler::HandleIncentive(unsigned char success, int amount)
{
    WindowApp::HandleTunnelCommand(0x9937390f, amount, 0, 0);
    return 1;
}

unsigned char CNotificationHandler::HandleGenericNotifyMessage(int messageId, const CStrWChar& text)
{
    return 1;
}

unsigned char CNotificationHandler::HandleContentFileUploaded(unsigned char success)
{
    return 1;
}

unsigned char CNotificationHandler::HandleContentFileDownloadedSelf(unsigned char success, const unsigned char* data, int size)
{
    return 1;
}

unsigned char CNotificationHandler::HandleContentFileDownloadedFriend(unsigned char success, int userId, const unsigned char* data, int size)
{
    return 1;
}

unsigned char CNotificationHandler::HandleContentFileDeleted(unsigned char success)
{
    return 1;
}

unsigned char CNotificationHandler::HandlePostMessage(unsigned char success, int network)
{
    if (success) {
        WindowApp::HandleTunnelCommand(0xa728ca73, 0, 0, 0);
    } else {
        WindowApp::HandleTunnelCommand(0x70b135b5, 0, 0, 0);
    }
    return 1;
}

unsigned char CNotificationHandler::HandleFriendPicUpdate(unsigned char success, int userId)
{
    return 1;
}

unsigned char CNotificationHandler::HandleInitializeGluOffers(unsigned char success, int count)
{
    if (success) {
        COfferManager::GetInstance()->getUnconsumedIncentives();
    }
    return 1;
}

unsigned char CNotificationHandler::HandleSendInvite(unsigned char success, int count)
{
    if (success) {
        WindowApp::HandleTunnelCommand(0x940a3e98, 0, 0, 0);
    } else {
        WindowApp::HandleTunnelCommand(0xcdb8030a, 0, 0, 0);
    }
    return 1;
}

unsigned char CNotificationHandler::HandleGluOffersReset(unsigned char success, int count)
{
    return 1;
}

unsigned char CNotificationHandler::HandleAutoPollStart()
{
    return 1;
}

unsigned char CNotificationHandler::HandleRandomNonFriendListUpdate(unsigned char success, int count)
{
    return 1;
}

unsigned char CNotificationHandler::HandleProfileSaved(ProfileDataSaveStatus status, const CVector<int>& dataIds)
{
    return 1;
}

unsigned char CNotificationHandler::HandleReceivedEnvelope()
{
    return 1;
}

unsigned char CNotificationHandler::HandleOldUnackedMessages()
{
    HandleNewMessagesReceived();
    return 0;
}

unsigned char CNotificationHandler::HandleNewMessagesReceived()
{
    CNGSFromServerMessageQ* queue = CNGS::GetInstance()->GetLocalUser()->m_messageQueue;
    CVector<long long> messageIds;
    queue->GetAllMessagesByUserAndType(messageIds, 0, 0, (CNGSFromServerMessage::CNGS_MSG_SUBTYPE)0);
    for (int index = 0; index < messageIds.m_size; index++) {
        CNGSFromServerMessage* message = queue->GetMessageById(messageIds.m_data[index]);
        if (message == NULL) {
            continue;
        }
        long long type = message->m_type;
        switch (type) {
        case MESSAGE_TYPE_GIFT:
        case MESSAGE_TYPE_GIFT_2: {
            // gifts stay in the queue until the player collects them
            m_giftMessageIds.Add(messageIds.m_data[index]);
            CNewsFeedManager* newsFeed = ((App*)WindowApp::m_instance)->m_newsFeedManager;
            if (newsFeed != NULL) {
                CNGSMessageGift* gift = (CNGSMessageGift*)message;
                if (gift->m_itemType > 0) {
                    newsFeed->AddGiftReceivedMessage((int)gift->m_senderId, (int)gift->m_itemType - 1, (int)gift->m_messageId);
                }
            }
            break;
        }
        case MESSAGE_TYPE_AWARD: {
            CNewsFeedManager* newsFeed = ((App*)WindowApp::m_instance)->m_newsFeedManager;
            if (newsFeed != NULL) {
                CNGSMessageAward* award = (CNGSMessageAward*)message;
                int commonCurrency = (int)award->m_currencyCommon;
                int rareCurrency = (int)award->m_currencyRare;
                if (commonCurrency > 0 || rareCurrency > 0) {
                    newsFeed->AddServerAwardMessage(commonCurrency, rareCurrency, 0, (int)award->m_messageId);
                }
            }
            break;
        }
        case MESSAGE_TYPE_IGNORED:
            break;
        default:
            if (type >= MESSAGE_TYPE_GAME_SPECIFIC) {
                if (!HandleGameSpecificMessage(type, message->GetMessageText(), message->m_senderId)) {
                    break;
                }
            }
            if (queue->MarkMessageForAck(message->m_messageId) && message != NULL) {
                delete message;
            }
            break;
        }
    }
    return 1;
}

unsigned char CNotificationHandler::HandleGameSpecificMessage(long long messageType, const CStrWChar& text, long long senderId)
{
    switch (messageType) {
    case MESSAGE_TYPE_TARGETTED_HIGHLIGHTED:
    case MESSAGE_TYPE_TARGETTED: {
        CNewsFeedManager* newsFeed = ((App*)WindowApp::m_instance)->m_newsFeedManager;
        if (newsFeed != NULL) {
            if (messageType == MESSAGE_TYPE_TARGETTED_HIGHLIGHTED) {
                newsFeed->AddTargettedMessage((int)senderId, text, true);
            } else {
                newsFeed->AddTargettedMessage((int)senderId, text, false);
            }
        }
        break;
    }
    case MESSAGE_TYPE_MISSION_INVITE: {
        // only friends may invite: the text is the quoted mission id
        CNGSRemoteUserList* friends = CNGS::GetInstance()->GetLocalUser()->m_remoteUserLists[0];
        for (int index = 0; index < friends->m_users.m_size; index++) {
            if (friends->m_users.m_data[index]->GetClientID() == senderId) {
                CStrWChar missionText = text.GetSubString(1, text.GetLength() - 2);
                char missionBuffer[12];
                const wchar_t* missionChars = missionText.GetWCharPtr();
                wcstombs(missionBuffer, missionChars, wcslen(missionChars) + 1);
                int missionId = atoi(missionBuffer);
                ((App*)WindowApp::m_instance)->m_newsFeedManager->AddMissionInviteMessage((int)senderId, missionId);
            }
        }
        break;
    }
    }
    return 0;
}

unsigned char CNotificationHandler::HandleFacebookLoginPageClose()
{
    if (m_loginState == 3) {
        m_loginState = 1;
        return 1;
    }
    return 0;
}

CNotificationHandler::CNotificationHandler()
    : m_localSoftCurrency(0)
    , m_localHardCurrency(0)
    , m_localExperience(0)
    , m_serverSoftCurrency(0)
    , m_serverHardCurrency(0)
    , m_serverExperience(0)
{
    m_loginState = 0;
}

CNotificationHandler::~CNotificationHandler()
{
}

unsigned char CNotificationHandler::HandleProfileContentFileDownloaded(unsigned char success, int collection)
{
    WindowApp::HandleTunnelCommand(0x42650932, 0, 0, 0);
    return 1;
}

unsigned char CNotificationHandler::HandleProfileContentFileUploaded(unsigned char success, int collection)
{
    return 1;
}

unsigned char CNotificationHandler::HandleGameCenterAppLogin(const CStrWChar& playerId)
{
    return 1;
}

unsigned char CNotificationHandler::HandleGameCenterAppFriendsListUpdate(const CStrWChar& friends)
{
    return 1;
}

unsigned char CNotificationHandler::HandleGameCenterAppLogin(unsigned char success)
{
    return 1;
}

unsigned char CNotificationHandler::HandleLoginFlowNextStep()
{
    return 1;
}

unsigned char CNotificationHandler::HandleFeaturedGameSelected(unsigned char success, int gameId)
{
    return 1;
}

unsigned char CNotificationHandler::HandleLockMgrGetSharedData(const CNGSLockMgrResponse& response, const CNGSLock* lock)
{
    return 0;
}

unsigned char CNotificationHandler::HandleLockMgrSaveSharedData(const CNGSLockMgrResponse& response, const CNGSLock* lock)
{
    return 0;
}

unsigned char CNotificationHandler::HandleLockMgrQueryAllSharedData(const CNGSLockMgrResponse& response, CVector<CNGSSharedData*>& data)
{
    return 0;
}

unsigned char CNotificationHandler::HandleLockMgrReleaseLock(const CNGSLockMgrResponse& response, const CNGSLock* lock)
{
    return 0;
}

unsigned char CNotificationHandler::HandleProfileDataFormatIncorrect(const CStrWChar& name, const CStrWChar& format,
    unsigned char*& data, unsigned int& size)
{
    return 0;
}

unsigned char CNotificationHandler::HandleFileDownloadedMessage(unsigned char success, CNGSDirectFileDownloadRequest& request)
{
    return 0;
}

unsigned char CNotificationHandler::HandleBatchFileDownloadStartedMessage(const CStrWChar& batch, int count)
{
    return 0;
}

unsigned char CNotificationHandler::HandleBatchFileDownloadCompleteMessage(const CStrWChar& batch, int count)
{
    return 0;
}
