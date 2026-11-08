// CNGSLocalUser.cpp - the player of this device: login / validation / registration with the
// gServe server for each social network, friend lists, session continuation and the
// forwarding of all user events to the game's notification handler.
#include <stdlib.h>
#include "CNGSLocalUser.h"
#include "CNGSRemoteUser.h"
#include "CNGSRemoteUserList.h"
#include "CNGSFactory.h"
#include "CNGSFromServerMessageQ.h"
#include "CNGSSession.h"
#include "CNGSSessionConfig.h"
#include "CNGSSKUBonus.h"
#include "CNGSUtil.h"
#include "CNGSServerRequest.h"
#include "CNotificationHandler.h"
#include "CNGSLoginFlow.h"
#include "CNGSAttributeManager.h"
#include "CNGSOfferManager.h"
#include "CNGSAttribute.h"
#include "CProfileManager.h"
#include "CNGS_Platform.h"
#include "CNGSSocialInterface.h"
#include "CObjectMap.h"
#include "CObjectMapObject.h"
#include "CObjectMapArray.h"
#include "CStrChar.h"
#include "ICStdUtil.h"
#include "CNGSTokens.h"

using com::glu::platform::components::CStrWChar;
using com::glu::platform::core::CVector;
using com::glu::platform::core::ICStdUtil;

/** Collection type of the friend message timestamps on the server. */
static const int FRIEND_TIMESTAMP_COLLECTION = 0x385;
/** Friends looked up per "user search" request. */
static const int FRIEND_SEARCH_BATCH = 100;
/** Login results of CNGSHandleUserLogin. */
static const int LOGIN_RESULT_SESSION_ERROR = 1009;
static const int LOGIN_RESULT_FAILED = -1;

CNGSLocalUser::CNGSLocalUser(const CNGSUserCredentials* credentials)
    : CNGSUser(credentials)
    , m_messageQueue(NULL)
    , m_session(NULL)
    , m_flag6c(0)
{
    m_messageQueue = new CNGSFromServerMessageQ();
    m_session = new CNGSSession();
    m_remoteUserLists[0] = new CNGSRemoteUserList();
    m_remoteUserLists[1] = new CNGSRemoteUserList();
    m_remoteUserLists[2] = new CNGSRemoteUserList();
    m_skuBonus = new CNGSSKUBonus();
    m_notificationHandler = NULL;
    m_authenticatedNetworks = 0;
    m_remoteUserLists[0]->reset();
    m_remoteUserLists[0]->SetIsValid(0);
    m_remoteUserLists[1]->reset();
    m_remoteUserLists[1]->SetIsValid(0);
    m_remoteUserLists[2]->reset();
    m_remoteUserLists[2]->SetIsValid(0);
}

CNGSLocalUser::~CNGSLocalUser()
{
    if (m_messageQueue != NULL) {
        delete m_messageQueue;
        m_messageQueue = NULL;
    }
    if (m_session != NULL) {
        delete m_session;
        m_session = NULL;
    }
    if (m_remoteUserLists[0] != NULL) {
        delete m_remoteUserLists[0];
        m_remoteUserLists[0] = NULL;
    }
    if (m_remoteUserLists[2] != NULL) {
        delete m_remoteUserLists[2];
        m_remoteUserLists[2] = NULL;
    }
    if (m_remoteUserLists[1] != NULL) {
        delete m_remoteUserLists[1];
        m_remoteUserLists[1] = NULL;
    }
    if (m_skuBonus != NULL) {
        delete m_skuBonus;
        m_skuBonus = NULL;
    }
}

void CNGSLocalUser::registerNotificationHandler(CNotificationHandler* handler)
{
    m_notificationHandler = handler;
}

void CNGSLocalUser::SetIsValid(unsigned char isValid)
{
    CNGSServerObject::SetIsValid(isValid);
}

void CNGSLocalUser::CheckServerVersionCompatibility()
{
    m_session->doServerVersionCheck();
}

CObjectMapObject* CNGSLocalUser::GetObjectMapRepresentationForUpdateUserInfo()
{
    CObjectMapObject* user = new CObjectMapObject();
    user->addEntry(CStrWChar(NGS_ID_TOKEN), new CObjectMapInt(GetClientID()));
    user->addEntry(CStrWChar(NGS_GAMERTAG_TOKEN), new CObjectMapString(GetNickName()));
    user->addEntry(CStrWChar(NGS_EMAIL_TOKEN), new CObjectMapString(GetEmail()));
    user->addEntry(CStrWChar(NGS_PASSWORD_TOKEN), new CObjectMapString(CStrWChar(m_password)));
    user->addEntry(CStrWChar(NGS_FACEBOOK_ID_TOKEN), new CObjectMapString(GetIDForSocialNetwork(SOCIAL_NETWORK_FACEBOOK)));
    return user;
}

void CNGSLocalUser::LoadCredentials()
{
    CNGSUser::LoadCredentials(CREDENTIAL_FILE_NAME);
}

unsigned char CNGSLocalUser::isAuthenticated(int network)
{
    int networks = m_authenticatedNetworks;
    if (network != 0) {
        networks &= 1 << network;
    }
    return networks > 0;
}

unsigned char CNGSLocalUser::friendLastMsgCheck(CNGSNotifyFunctor* notify)
{
    if (notify != NULL) {
        if (!isReady() || m_writeRequestsOutstanding != 0) {
            // busy: let the notify functor fire right away
            AddReadRequestOutstanding();
            CompleteReadRequestOutstanding();
            return 0;
        }
        m_readNotify = notify;
    }
    CAttributeManager* attributeManager = CAttributeManager::GetInstance();
    CVector<int> collections;
    int collectionType = FRIEND_TIMESTAMP_COLLECTION;
    collections.Add(collectionType);
    AddReadRequestOutstanding();
    attributeManager->getDataFromServer(collections,
                                        new CNGSLocalUserFunctor(this, 0, &CNGSLocalUser::handleFriendLastMsgCheckResponse, 0),
                                        &m_credentials);
    return 1;
}

void CNGSLocalUser::handleFriendLastMsgCheckResponse(CObjectMap* response, CNGSLocalUserFunctor* functor)
{
    CAttributeManager* attributeManager = CAttributeManager::GetInstance();
    if (WasErrorInResponse(response, functor, "handleFriendLastMsgCheckResponse") == 0) {
        CVector<CNGSAttribute*> attributes;
        attributeManager->extractAttributeValuesAsArray(response, functor, attributes);
        for (int index = 0; index < attributes.Size(); index++) {
            setFriendMsgTimestampLocally(extractClientIdFromFriendTimestamp(*attributes[index]),
                                         attributes[index]->getLastUpdatedInMS());
        }
    }
    m_remoteUserLists[1]->SetIsValid(1);
    m_remoteUserLists[0]->SetIsValid(1);
    m_remoteUserLists[2]->SetIsValid(1);
    CompleteReadRequestOutstanding();
}

int CNGSLocalUser::extractClientIdFromFriendTimestamp(CNGSAttribute& attribute)
{
    int clientId = 0;
    int separator = attribute.getName().Find(L"_", 0);
    if (separator >= 0) {
        CStrWChar idText = attribute.getName().GetSubString(separator + 1, attribute.getName().GetLength() - 1);
        clientId = atoi(CNGSUtil::WStrToCStr(idText).GetCharPtr());
    }
    return clientId;
}

void CNGSLocalUser::HandleValidateFriendListFromServerResponse(CObjectMap* response, CNGSLocalUserFunctor* functor)
{
    CNGS* ngs = CNGS::GetInstance();
    int network = functor->m_network;
    int requestedCount = functor->m_context;
    if (WasErrorInResponse(response, functor, "HandleValidateFriendListFromServerResponse") == 0) {
        CObjectMapArray* list = (CObjectMapArray*)response->getDataAt(CStrWChar(NGS_LIST_TOKEN), CStrWChar(L" "));
        int count;
        if (list != NULL && list->getType() == CObjectMapValue::TYPE_ARRAY && (count = list->m_values.Size()) > 0) {
            for (int index = 0; index < count; index++) {
                CNGSUserCredentials credentials((CObjectMapObject*)list->m_values[index], true);
                CNGSRemoteUser* user = ngs->GetFactory()->getRemoteUserByCredentials(credentials);
                if (user != NULL) {
                    if (user->isValid()) {
                        user->AddSocialNetworkType(network);
                        if (credentials.hasValidAvatar()) {
                            m_remoteUserLists[0]->addRemoteUserIfNotAlreadyInList(user);
                            user->SetIsFriendOfLocalUser(1);
                        }
                        m_remoteUserLists[1]->addRemoteUserIfNotAlreadyInList(user);
                    } else {
                        delete user;
                    }
                }
            }
        }

        // wait for the answers of all search requests of this network
        CNGSSocialInterface* social = CNGS_Platform::GetInstance()->getSocialInterface(network);
        social->m_reserved24 -= requestedCount;
        if (social->m_reserved24 != 0) {
            CompleteReadRequestOutstanding();
            return;
        }
        friendLastMsgCheck(NULL);
        m_remoteUserLists[1]->SetIsValid(1);
        m_remoteUserLists[0]->SetIsValid(1);
        // "<network friends>/<friends>/<friends with avatar>" (only logged in debug builds)
        wchar_t text[16];
        ICStdUtil::SWPrintF(text, L"%d/%d/%d", CNGS_Platform::GetInstance()->getSocialInterface(network)->getFriendsCount(),
                            m_remoteUserLists[1]->m_users.Size(), m_remoteUserLists[0]->m_users.Size());
        CNGSHandleFriendListUpdate(1, 1, network, 0);
    } else {
        CNGSHandleFriendListUpdate(0, 1, network, 0);
    }
    CompleteReadRequestOutstanding();
}

unsigned char CNGSLocalUser::HandleGameSpecificMessage(long long messageId, const CStrWChar& text, long long value)
{
    return 1;
}

unsigned char CNGSLocalUser::HandleGenericYesNoMessage(CObjectMapObject* message, int messageId)
{
    return 1;
}

unsigned char CNGSLocalUser::HandleGenericNotifyMessage(CStrWChar& text, int messageId)
{
    // button list of the notify dialog (the dialog itself is not shown in this build)
    CStrWChar okLabel("OK");
    CVector<const CStrWChar*> buttons;
    buttons.Add(&okLabel);
    return 1;
}

unsigned char CNGSLocalUser::HandleNotifyMessageWithCommand(CStrWChar& text, CNGSServerRequest* command, int messageId)
{
    CStrWChar okLabel("OK");
    CVector<const CStrWChar*> buttons;
    buttons.Add(&okLabel);
    return 1;
}

bool CNGSLocalUser::HandleBonusMessage(bool success, long long messageId, CStrWChar& text, CVector<CNGSAttribute*>& items,
                                       int amount)
{
    return success;
}

void CNGSLocalUser::CNGSHandleUserLogin(int result, int network, const wchar_t* message)
{
    if (CNGSLoginFlow::GetInstance()->IsRunning()) {
        CNGSLoginFlow::GetInstance()->m_unused28 = network;
    }
    if (result == 0) {
        m_authenticatedNetworks |= 1 << network;
        AddUserAttribute(network);
        COfferManager::GetInstance()->setPurchasedOfferInstall(network);
        m_messageQueue->LoadMessagesToAckFromFileSystem();
        CNGSLoginFlow::GetInstance()->OnEvent(LOGIN_FLOW_EVENT_0, "CNGSHandleUserLogin");
    } else if (result == 12) {
        CNGSLoginFlow::GetInstance()->OnEvent(LOGIN_FLOW_EVENT_0, "CNGSHandleUserLogin");
    } else if (result == 1010 || result == 1011) {
        CNGSLoginFlow::GetInstance()->OnEvent(LOGIN_FLOW_EVENT_11, "CNGSHandleUserLogin");
    } else if (result == 10) {
        CNGSLoginFlow::GetInstance()->OnEvent(LOGIN_FLOW_EVENT_1, "CNGSHandleUserLogin");
    } else if (result == 11) {
        CNGSLoginFlow::GetInstance()->OnEvent(LOGIN_FLOW_EVENT_2, "CNGSHandleUserLogin");
    } else {
        CNGSLoginFlow::GetInstance()->OnEvent(LOGIN_FLOW_EVENT_9, "CNGSHandleUserLogin");
    }
    m_notificationHandler->HandleUserLogin(result, network, message);
}

unsigned int CNGSLocalUser::AddUserAttribute(int network)
{
    AddReadRequestOutstanding();
    CNGSSocialInterface* social = CNGS_Platform::GetInstance()->getSocialInterface(network);
    const char* aliasName = social->getUserAliasAttributeName(network);
    CStrWChar alias = social->getPlayerAlias();

    CObjectMapObject* message = new CObjectMapObject();
    InsertHeaderIntoMessageObject(message, "update attribute", 0);
    CObjectMapObject* user = new CObjectMapObject();
    user->addEntry(CStrWChar(NGS_ID_TOKEN), new CObjectMapInt(GetClientID()));
    message->addEntry(CStrWChar(NGS_USER_TOKEN), user);

    CObjectMapObject* attribute = new CObjectMapObject();
    attribute->addEntry(CStrWChar(NGS_TYPE_TOKEN), new CObjectMapInt(2));
    attribute->addEntry(CStrWChar(NGS_ATTRIBUTENAME_TOKEN), new CObjectMapString(CStrWChar(aliasName)));
    attribute->addEntry(CStrWChar(NGS_STRINGVALUE_TOKEN), new CObjectMapString(alias));
    CObjectMapArray* attributes = new CObjectMapArray();
    attributes->addEntry(attribute);
    message->addEntry(CStrWChar(NGS_USERATTRIBUTES_TOKEN), attributes);

    return SendMessageObjectToServer(message, "resources/user/attributes/",
                                     new CNGSLocalUserFunctor(this, network, &CNGSLocalUser::HandleAddUserAttributeResponse, 0));
}

void CNGSLocalUser::CNGSHandleUserLogout(unsigned char success, int network)
{
    if (success) {
        // drop the users that only came from this network
        if (m_remoteUserLists[0]->isValid()) {
            int index = 0;
            while (index < m_remoteUserLists[0]->m_users.Size()) {
                CNGSRemoteUser* user = m_remoteUserLists[0]->m_users[index];
                if (user->IsFromSocialNetwork(network)) {
                    user->RemoveSocialNetworkType(network);
                }
                if (user->GetSocialNetworkType() == 0) {
                    m_remoteUserLists[0]->removeRemoteUser(index);
                } else {
                    index++;
                }
            }
        }
        if (m_remoteUserLists[2]->isValid()) {
            int index = 0;
            while (index < m_remoteUserLists[2]->m_users.Size()) {
                CNGSRemoteUser* user = m_remoteUserLists[2]->m_users[index];
                if (user->IsFromSocialNetwork(network)) {
                    user->RemoveSocialNetworkType(network);
                }
                if (user->GetSocialNetworkType() == 0) {
                    m_remoteUserLists[2]->removeRemoteUser(index);
                } else {
                    index++;
                }
            }
        }
        if (m_remoteUserLists[1]->isValid()) {
            int index = 0;
            while (index < m_remoteUserLists[1]->m_users.Size()) {
                CNGSRemoteUser* user = m_remoteUserLists[1]->m_users[index];
                if (user->IsFromSocialNetwork(network)) {
                    user->RemoveSocialNetworkType(network);
                }
                if (user->GetSocialNetworkType() == 0) {
                    m_remoteUserLists[1]->removeRemoteUser(index);
                } else {
                    index++;
                }
            }
        }
        m_authenticatedNetworks &= ~(1 << network);
        if (m_authenticatedNetworks == 0) {
            CNGSHandlePrimaryUserLogout();
        }
        CNGSHandleFriendListUpdate(1, 0, network, 0);
        m_notificationHandler->HandleRandomNonFriendListUpdate(1, network);
    }
    m_notificationHandler->HandleUserLogout(success, network);
}

void CNGSLocalUser::CNGSHandlePrimaryUserLogout()
{
    if (CNGSServerRequest::AreNetworkMessagesWaitingForProcessing()) {
        CNGSServerRequest::DeleteAll();
    }
    m_messageQueue->WriteMessageIdsToAckToFS();
    m_messageQueue->Reset();
    invalidateLocalData(0);
    SetIsValid(0);
    m_session->invalidate();
    COfferManager::GetInstance()->resetOffers();
    m_notificationHandler->HandleGluOffersReset(0, 0);
    m_notificationHandler->HandlePrimaryUserLogout();
}

void CNGSLocalUser::CNGSHandleFriendListUpdate(unsigned char success, unsigned char authenticated, int network, int unused)
{
    if (success) {
        CNGS_Platform::GetInstance()->getSocialInterface(network)->setUserAuthenticated(authenticated);
        DebugPrintFriends();
        if (authenticated) {
            CNGSLoginFlow::GetInstance()->OnEvent(LOGIN_FLOW_EVENT_0, "CNGSHandleFriendListUpdate");
        }
    } else {
        CNGSLoginFlow::GetInstance()->OnEvent(LOGIN_FLOW_EVENT_9, "CNGSHandleFriendListUpdate");
    }
    m_notificationHandler->HandleFriendListUpdate(success, network);
}

void CNGSLocalUser::DebugPrintFriends()
{
    if (m_remoteUserLists[0]->isValid()) {
        int count = m_remoteUserLists[0]->m_users.Size();
        for (int index = 0; index < count; index++) {
            m_remoteUserLists[0]->m_users[index]->DebugPrint();
        }
    }
    if (m_remoteUserLists[1]->isValid()) {
        int count = m_remoteUserLists[1]->m_users.Size();
        for (int index = 0; index < count; index++) {
            m_remoteUserLists[1]->m_users[index]->DebugPrint();
        }
    }
}

void CNGSLocalUser::invalidateLocalData(unsigned char reset)
{
    CProfileManager* profileManager = CNGS::GetInstance()->GetProfileManager();
    CVector<int> collections;
    profileManager->getAllRegisteredCollections(collections);
    profileManager->invalidateLocalData(collections, reset);
}

void CNGSLocalUser::CNGSHandlePrimaryUserChange(int network, const char* caller)
{
    if (CNGSServerRequest::AreNetworkMessagesWaitingForProcessing()) {
        CNGSServerRequest::ClearAllMessages();
    }
    m_messageQueue->WriteMessageIdsToAckToFS();
    m_messageQueue->Reset();
    m_session->invalidate();
    Deauthenticate();
    invalidateLocalData(1);
    ResetCredentials(CREDENTIAL_FILE_NAME);
    CNGSLoginFlow::GetInstance()->m_unused28 = network;
    CNGSLoginFlow::GetInstance()->OnEvent(LOGIN_FLOW_EVENT_12, caller);
    m_notificationHandler->HandlePrimaryUserChanged(network);
}

void CNGSLocalUser::Deauthenticate()
{
    if (m_remoteUserLists[0]->isValid()) {
        while (m_remoteUserLists[0]->m_users.Size() != 0) {
            m_remoteUserLists[0]->removeRemoteUser(0);
        }
    }
    if (m_remoteUserLists[2]->isValid()) {
        while (m_remoteUserLists[2]->m_users.Size() != 0) {
            m_remoteUserLists[2]->removeRemoteUser(0);
        }
    }
    if (m_remoteUserLists[1]->isValid()) {
        while (m_remoteUserLists[1]->m_users.Size() != 0) {
            m_remoteUserLists[1]->removeRemoteUser(0);
        }
    }
    for (int network = 0; network < SOCIAL_NETWORK_COUNT; network++) {
        m_authenticatedNetworks &= ~(1 << network);
        CNGSSocialInterface* social = CNGS_Platform::GetInstance()->getSocialInterface(network);
        if (social != NULL) {
            social->invalidate();
        }
    }
    invalidateLocalData(0);
}

unsigned char CNGSLocalUser::CNGSHandleSessionInvalid(const char* caller)
{
    CNGSLoginFlow::GetInstance()->OnEvent(LOGIN_FLOW_EVENT_8, caller);
    unsigned char result = m_notificationHandler->HandleSessionInvalid();
    Deauthenticate();
    return result;
}

void CNGSLocalUser::CNGSHandleGenericServerError(const char* caller)
{
    CNGSLoginFlow::GetInstance()->OnEvent(LOGIN_FLOW_EVENT_9, caller);
    m_notificationHandler->HandleGenericServerError();
}

void CNGSLocalUser::CNGSHandleVersionCheck(unsigned char success, const char* caller)
{
    CNGSLoginFlow* loginFlow = CNGSLoginFlow::GetInstance();
    if (success) {
        loginFlow->OnEvent(LOGIN_FLOW_EVENT_6, caller);
        m_notificationHandler->HandleVersionSuccess();
    } else {
        loginFlow->OnEvent(LOGIN_FLOW_EVENT_7, caller);
        m_notificationHandler->HandleVersionInvalid();
    }
}

unsigned char CNGSLocalUser::CNGSHandleServerConnectionError(const char* caller)
{
    // states 3..9 are reported by their own handlers
    int state = CNGS::GetInstance()->GetNetworkConnectionState();
    if (state >= NETWORK_SESSION_INVALID && state <= 9) {
        return 1;
    }
    CNGSLoginFlow::GetInstance()->OnEvent(LOGIN_FLOW_EVENT_10, caller);
    return m_notificationHandler->HandleServerConnectionError();
}

unsigned char CNGSLocalUser::CNGSHandleServerConnectionRestore(eNetworkConnectionState previousState)
{
    if (previousState >= 5 && previousState <= 9) {
        return 1;
    }
    return m_notificationHandler->HandleServerConnectionRestore();
}

unsigned char CNGSLocalUser::AttemptToContinueSession(int network)
{
    unsigned char started = 0;
    if (m_session->isValid()) {
        CNGSLocalUserNotifyFunctor* notify =
            new CNGSLocalUserNotifyFunctor(network, this, &CNGSLocalUser::AttemptToContineSessionTimeCheckCompletion);
        if (m_session->SyncAllFromServer(notify)) {
            started = 1;
            AddReadRequestOutstanding();
        }
    }
    return started;
}

void CNGSLocalUser::AttemptToContineSessionTimeCheckCompletion(CNGSLocalUserNotifyFunctor* functor)
{
    if (m_session->isValid()) {
        int network = 0;
        if (functor != NULL) {
            network = functor->m_param;
        }
        CNGSLocalUserNotifyFunctor* notify =
            new CNGSLocalUserNotifyFunctor(network, this, &CNGSLocalUser::HandleAttemptToContinueSession);
        if (m_messageQueue->SyncAllFromServer(notify)) {
            AddReadRequestOutstanding();
        }
    }
    CompleteReadRequestOutstanding();
}

void CNGSLocalUser::HandleAttemptToContinueSession(CNGSLocalUserNotifyFunctor* functor)
{
    if (m_session->isValid()) {
        int network = 0;
        if (functor != NULL) {
            network = functor->m_param;
        }
        SetIsValid(1);
        CNGSServerObjectNotifyFunctor* notify =
            new CNGSServerObjectNotifyFunctor(0, this, &CNGSServerObject::NotifyOfChildReadCompletion);
        if (m_session->SyncAllFromServer(notify)) {
            AddReadRequestOutstanding();
        }
        // the nickname changed since the credentials were saved: tell the server
        CNGSUserCredentials savedCredentials;
        CStrWChar fileName(CREDENTIAL_FILE_NAME);
        savedCredentials.readFromFile(fileName, false);
        if (!(savedCredentials.m_nickName == GetNickName())) {
            UpdateUserInfo(network);
        }
        CNGSHandleUserLogin(0, network, NULL);
    }
    CompleteReadRequestOutstanding();
}

void CNGSLocalUser::UpdateUserInfo(int network)
{
    CObjectMapObject* message = new CObjectMapObject();
    InsertHeaderIntoMessageObject(message, "update gamertag", 0);
    message->addEntry(CStrWChar(NGS_USER_TOKEN), GetObjectMapRepresentationForUpdateUserInfo());
    SendMessageObjectToServer(message, "resources/user/modifyuserinfo/",
                              new CNGSLocalUserFunctor(this, network, &CNGSLocalUser::HandleUpdateUserInfoResponse, 0));
}

unsigned int CNGSLocalUser::ValidateUser(int network, unsigned char force)
{
    unsigned int requestId;
    if (m_session != NULL && m_session->isValid() && GetClientID() != -1) {
        // a session is still valid: continue it
        AttemptToContinueSession(network);
        requestId = 0;
    } else {
        AddReadRequestOutstanding();
        CNGSSocialInterface* social = CNGS_Platform::GetInstance()->getSocialInterface(network);
        const char* idName = social->getAttributeName(network);
        CStrWChar playerId = social->getPlayerID();

        CObjectMapObject* message = new CObjectMapObject();
        InsertHeaderIntoMessageObject(message, "validate user", 0);
        CObjectMapObject* attribute = new CObjectMapObject();
        attribute->addEntry(CStrWChar(NGS_ATTRIBUTENAME_TOKEN), new CObjectMapString(CStrWChar(idName)));
        attribute->addEntry(CStrWChar(NGS_STRINGVALUE_TOKEN), new CObjectMapString(playerId));
        message->addEntry(CStrWChar(NGS_USERATTRIBUTE_TOKEN), attribute);
        requestId = SendMessageObjectToServer(message, "resources/user/validateuser/",
                                              new CNGSLocalUserFunctor(this, network, &CNGSLocalUser::HandleValidateUserResponse, force));
    }
    return requestId;
}

void CNGSLocalUser::HandleValidateUserResponse(CObjectMap* response, CNGSLocalUserFunctor* functor)
{
    bool validated = false;
    bool sessionFailed = false;
    bool nickNameChanged = false;
    bool otherUser = false;
    if (GetErrorCodeInResponse(response) != 1001 &&
        WasErrorInResponse(response, functor, "HandleValidateUserResponse") == 0) {
        CObjectMapObject* root = (CObjectMapObject*)response->getRoot();
        CNGSSessionConfig previousConfig = m_session->m_config;
        CNGSSessionConfig config(root);
        if (m_session->init(config) < 0) {
            sessionFailed = true;
        } else {
            if (!m_session->doesSessionMatch(previousConfig)) {
                CStrWChar sessionFile(L"Session.dat");
                CStrWChar sessionKey(L"xxx");
                config.writeToFile(sessionFile, sessionKey, false);
            }
            m_session->SetIsValid(1);
            CObjectMapValue* users = root->getEntry(CStrWChar(NGS_LIST_TOKEN));
            if (users->getType() == CObjectMapValue::TYPE_ARRAY) {
                CObjectMapArray* list = (CObjectMapArray*)users;
                if (list->m_values.Size() > 0) {
                    CNGSUserCredentials credentials((CObjectMapObject*)list->m_values[list->m_values.Size() - 1], true);
                    if (GetClientID() < 1 || GetClientID() == credentials.m_userId) {
                        // keep our nickname and push it to the server when the server had another one
                        CStrWChar nickName = GetNickName();
                        UpdateCredentials(credentials);
                        if (!(GetNickName() == nickName)) {
                            nickNameChanged = true;
                            SetNickName(nickName);
                        }
                    } else {
                        otherUser = true;
                    }
                }
            } else if (users->getType() == CObjectMapValue::TYPE_OBJECT) {
                CNGSUserCredentials credentials((CObjectMapObject*)users, true);
                if (GetClientID() < 1 || GetClientID() == credentials.m_userId) {
                    CStrWChar nickName = GetNickName();
                    UpdateCredentials(credentials);
                    if (!(GetNickName() == nickName)) {
                        nickNameChanged = true;
                        SetNickName(nickName);
                    }
                } else {
                    otherUser = true;
                }
            }
            CNGSLocalUserNotifyFunctor* notify =
                new CNGSLocalUserNotifyFunctor(0, this, &CNGSLocalUser::NotifyOfNetTimeCheckCompletion);
            if (m_session->SyncAllFromServer(notify)) {
                AddReadRequestOutstanding();
            }
            validated = true;
        }
    }

    SetIsValid(0);
    int network = functor->m_network;
    if (sessionFailed) {
        m_remoteUserLists[0]->SetIsValid(0);
        m_remoteUserLists[2]->SetIsValid(0);
        CNGSUser::LoadCredentials(CREDENTIAL_FILE_NAME);
        CNGSHandleUserLogin(LOGIN_RESULT_SESSION_ERROR, network, NULL);
        m_notificationHandler->HandleSessionError();
    } else if (validated) {
        if (otherUser) {
            CNGSHandlePrimaryUserChange(network, "HandleValidateUserResponse");
        } else {
            if (nickNameChanged) {
                UpdateUserInfo(network);
            }
            SetIsValid(1);
            SaveCredentials(CREDENTIAL_FILE_NAME);
            CNGSHandleUserLogin(0, network, NULL);
        }
    } else {
        unsigned char force = (unsigned char)functor->m_context;
        if (GetClientID() == -1 || !force) {
            // unknown user: register it
            RegisterUser(network);
        } else {
            m_remoteUserLists[0]->SetIsValid(0);
            m_remoteUserLists[2]->SetIsValid(0);
            CNGSUser::LoadCredentials(CREDENTIAL_FILE_NAME);
            CStrWChar errorMessage("");
            ExtractErrorMessage(response, CStrWChar(L"/payload/0/message/message"), errorMessage);
            CNGSHandleUserLogin(LOGIN_RESULT_FAILED, network, errorMessage.GetWCharPtr());
        }
    }
    CompleteReadRequestOutstanding();
}

void CNGSLocalUser::NotifyOfNetTimeCheckCompletion(CNGSLocalUserNotifyFunctor* functor)
{
    CNGSServerObjectNotifyFunctor* notify =
        new CNGSServerObjectNotifyFunctor(0, this, &CNGSServerObject::NotifyOfChildReadCompletion);
    if (m_messageQueue->SyncAllFromServer(notify)) {
        AddReadRequestOutstanding();
    }
    CompleteReadRequestOutstanding();
}

int CNGSLocalUser::ExtractErrorMessage(CObjectMap* response, CStrWChar path, CStrWChar& message)
{
    if (response != NULL) {
        CObjectMapValue* value = response->getDataAt(CStrWChar(NGS_MESSAGE_TOKEN), CStrWChar(L" "));
        if (value->getType() == CObjectMapValue::TYPE_STRING) {
            message = CNGSUtil::GetMessageDataString(response, CStrWChar(NGS_MESSAGE_TOKEN), CStrWChar(","));
        }
    }
    return 0;
}

unsigned int CNGSLocalUser::RegisterUser(int network)
{
    AddReadRequestOutstanding();
    CNGSSocialInterface* social = CNGS_Platform::GetInstance()->getSocialInterface(network);
    const char* idName = social->getAttributeName(network);
    CStrWChar playerId = social->getPlayerID();

    CObjectMapObject* message = new CObjectMapObject();
    InsertHeaderIntoMessageObject(message, "register user", 0);
    CObjectMapObject* user = new CObjectMapObject();
    user->addEntry(CStrWChar(NGS_GAMERTAG_TOKEN), new CObjectMapString(GetNickName()));
    message->addEntry(CStrWChar(NGS_USER_TOKEN), user);

    CObjectMapObject* attribute = new CObjectMapObject();
    attribute->addEntry(CStrWChar(NGS_ATTRIBUTENAME_TOKEN), new CObjectMapString(CStrWChar(idName)));
    attribute->addEntry(CStrWChar(NGS_STRINGVALUE_TOKEN), new CObjectMapString(playerId));
    message->addEntry(CStrWChar(NGS_USERATTRIBUTE_TOKEN), attribute);

    return SendMessageObjectToServer(message, "resources/user/registeruser/",
                                     new CNGSLocalUserFunctor(this, network, &CNGSLocalUser::HandleRegisterUserResponse, 0));
}

void CNGSLocalUser::HandleUpdateUserInfoResponse(CObjectMap* response, CNGSLocalUserFunctor* functor)
{
}

void CNGSLocalUser::HandleRegisterUserResponse(CObjectMap* response, CNGSLocalUserFunctor* functor)
{
    bool registered = false;
    if (WasErrorInResponse(response, functor, "HandleRegisterUserResponse") == 0) {
        registered = true;
        CObjectMapObject* root = (CObjectMapObject*)response->getRoot();
        if (root->getType() == CObjectMapValue::TYPE_OBJECT) {
            // the server user plus our id / alias of the network we registered with
            CNGSUserCredentials credentials(root, true);
            eSocialNetworkType network = (eSocialNetworkType)functor->m_network;
            credentials.setIDForSocialNetwork(network, GetIDForSocialNetwork(network));
            credentials.setAliasForSocialNetwork(network, GetAliasForSocialNetwork(network));
            UpdateCredentials(credentials);
        }
    }
    CompleteReadRequestOutstanding();
    SetIsValid(0);
    int network = functor->m_network;
    if (registered) {
        ValidateUser(network, 1);
    } else {
        CStrWChar errorMessage("");
        ExtractErrorMessage(response, CStrWChar(L"/payload/0/message/message"), errorMessage);
        m_remoteUserLists[0]->SetIsValid(0);
        m_remoteUserLists[2]->SetIsValid(0);
        CNGSUser::LoadCredentials(CREDENTIAL_FILE_NAME);
        CNGSHandleUserLogin(LOGIN_RESULT_FAILED, network, errorMessage.GetWCharPtr());
    }
}

unsigned int CNGSLocalUser::AssociateUser(int network)
{
    AddReadRequestOutstanding();
    CNGSSocialInterface* social = CNGS_Platform::GetInstance()->getSocialInterface(network);
    const char* idName = social->getAttributeName(network);
    CStrWChar playerId = social->getPlayerID();

    CObjectMapObject* message = new CObjectMapObject();
    InsertHeaderIntoMessageObject(message, "update attribute", 0);
    CObjectMapObject* user = new CObjectMapObject();
    user->addEntry(CStrWChar(NGS_ID_TOKEN), new CObjectMapInt(GetClientID()));
    message->addEntry(CStrWChar(NGS_USER_TOKEN), user);

    CObjectMapObject* attribute = new CObjectMapObject();
    attribute->addEntry(CStrWChar(NGS_TYPE_TOKEN), new CObjectMapInt(2));
    attribute->addEntry(CStrWChar(NGS_ATTRIBUTENAME_TOKEN), new CObjectMapString(CStrWChar(idName)));
    attribute->addEntry(CStrWChar(NGS_STRINGVALUE_TOKEN), new CObjectMapString(playerId));
    CObjectMapArray* attributes = new CObjectMapArray();
    attributes->addEntry(attribute);
    message->addEntry(CStrWChar(NGS_USERATTRIBUTES_TOKEN), attributes);

    return SendMessageObjectToServer(message, "resources/user/attributes/",
                                     new CNGSLocalUserFunctor(this, network, &CNGSLocalUser::HandleAssociateUserResponse, 0));
}

void CNGSLocalUser::HandleAssociateUserResponse(CObjectMap* response, CNGSLocalUserFunctor* functor)
{
    int network = functor->m_network;
    int error = WasErrorInResponse(response, functor, "HandleAssociateUserResponse");
    if (error == 0) {
        CObjectMapObject* root = (CObjectMapObject*)response->getRoot();
        if (root->getType() == CObjectMapValue::TYPE_OBJECT) {
            // read but not used (the original keeps the local credentials)
            CNGSUserCredentials credentials(root, true);
        }
        SaveCredentials(CREDENTIAL_FILE_NAME);
        CNGSHandleUserLogin(0, network, NULL);
    } else {
        CNGSUser::LoadCredentials(CREDENTIAL_FILE_NAME);
        CStrWChar errorMessage("");
        ExtractErrorMessage(response, CStrWChar(L"/payload/0/message/message"), errorMessage);
        CNGSHandleUserLogin(error, network, errorMessage.GetWCharPtr());
    }
    CompleteReadRequestOutstanding();
}

void CNGSLocalUser::HandleLoadFriendListFromServerResponse(int network)
{
    CNGSSocialInterface* social = CNGS_Platform::GetInstance()->getSocialInterface(network);
    if (social->getConnectionState() == 2) {
        if (social->getFriendsCount() == 0) {
            // no friends on this network: the lists are complete
            m_remoteUserLists[0]->SetIsValid(1);
            m_remoteUserLists[1]->SetIsValid(1);
            CNGSHandleFriendListUpdate(1, 1, network, 0);
        } else {
            ValidateFriendListFromServer(network);
        }
    } else {
        CNGSHandleFriendListUpdate(0, 1, network, 0);
    }
    CompleteReadRequestOutstanding();
}

unsigned char CNGSLocalUser::ValidateFriendListFromServer(int network)
{
    CNGSSocialInterface* social = CNGS_Platform::GetInstance()->getSocialInterface(network);
    const char* idName = social->getAttributeName(network);
    CStrWChar playerId = social->getPlayerID();
    int friendCount = CNGS_Platform::GetInstance()->getSocialInterface(network)->getFriendsCount();
    // number of friends whose search reply is still expected
    CNGS_Platform::GetInstance()->getSocialInterface(network)->m_reserved24 = friendCount;
    for (int start = 0; start < friendCount; start += FRIEND_SEARCH_BATCH) {
        CObjectMapObject* message = new CObjectMapObject();
        InsertHeaderIntoMessageObject(message, "user search", 0);
        CObjectMapObject* user = new CObjectMapObject();
        user->addEntry(CStrWChar(NGS_ID_TOKEN), new CObjectMapInt(GetClientID()));
        message->addEntry(CStrWChar(NGS_USER_TOKEN), user);

        CObjectMapArray* attributes = new CObjectMapArray();
        int count = CNGS_Platform::GetInstance()->getSocialInterface(network)->getFriendsCount();
        int batchCount;
        for (batchCount = 0; batchCount < FRIEND_SEARCH_BATCH && start + batchCount < count; batchCount++) {
            CStrWChar friendId = CNGS_Platform::GetInstance()->getSocialInterface(network)->getFriendIDAtIndex(start + batchCount);
            CObjectMapObject* attribute = new CObjectMapObject();
            attribute->addEntry(CStrWChar(NGS_ATTRIBUTENAME_TOKEN), new CObjectMapString(CStrWChar(idName)));
            attribute->addEntry(CStrWChar(NGS_STRINGVALUE_TOKEN), new CObjectMapString(friendId));
            attributes->addEntry(attribute);
        }
        message->addEntry(CStrWChar(NGS_USERATTRIBUTES_TOKEN), attributes);

        CNGSLocalUserFunctor* functor =
            new CNGSLocalUserFunctor(this, network, &CNGSLocalUser::HandleValidateFriendListFromServerResponse, batchCount);
        if (SendMessageObjectToServer(message, "resources/user/search/", functor) > 0) {
            AddReadRequestOutstanding();
        }
    }
    return 0;
}

void CNGSLocalUser::HandleAddUserAttributeResponse(CObjectMap* response, CNGSLocalUserFunctor* functor)
{
    int network = functor->m_network;
    if (WasErrorInResponse(response, functor, "HandleAddUserAttributeResponse") != 0) {
        // the attribute exists already: modify it
        ModifyUserAttribute(network);
    }
    CompleteReadRequestOutstanding();
}

unsigned int CNGSLocalUser::ModifyUserAttribute(int network)
{
    CNGSSocialInterface* social = CNGS_Platform::GetInstance()->getSocialInterface(network);
    const char* aliasName = social->getUserAliasAttributeName(network);
    CStrWChar alias = social->getPlayerAlias();

    CObjectMapObject* message = new CObjectMapObject();
    InsertHeaderIntoMessageObject(message, "update attribute", 0);
    message->addEntry(CStrWChar(NGS_USER_TOKEN), GetObjectMapRepresentationForUpdateUserInfo());

    CObjectMapObject* attribute = new CObjectMapObject();
    attribute->addEntry(CStrWChar(NGS_TYPE_TOKEN), new CObjectMapInt(2));
    attribute->addEntry(CStrWChar(NGS_ATTRIBUTENAME_TOKEN), new CObjectMapString(CStrWChar(aliasName)));
    attribute->addEntry(CStrWChar(NGS_STRINGVALUE_TOKEN), new CObjectMapString(alias));
    CObjectMapArray* attributes = new CObjectMapArray();
    attributes->addEntry(attribute);
    message->addEntry(CStrWChar(NGS_USERATTRIBUTES_TOKEN), attributes);

    return SendMessageObjectToServer(message, "resources/user/modifyuserinfo/",
                                     new CNGSLocalUserFunctor(this, network, &CNGSLocalUser::HandleModifyUserAttributeResponse, 0));
}

void CNGSLocalUser::HandleModifyUserAttributeResponse(CObjectMap* response, CNGSLocalUserFunctor* functor)
{
    WasErrorInResponse(response, functor, "HandleModifyUserAttributeResponse");
}

void CNGSLocalUser::AddReadRequest()
{
    AddReadRequestOutstanding();
}

void CNGSLocalUser::CompleteReadRequest()
{
    CompleteReadRequestOutstanding();
}

void CNGSLocalUser::GetSKUBonusItems(CStrWChar& sku, CVector<CNGSAttribute*>& items)
{
}

CStrWChar CNGSLocalUser::GetSKUBonusNameForItem(CNGSAttribute* item)
{
    return CStrWChar(L"??");
}

void CNGSLocalUser::CNGSHandlePostMessage(unsigned char success, int network)
{
    CNGSSocialInterface* social = CNGS_Platform::GetInstance()->getSocialInterface(network);
    if (social != NULL) {
        social->isMessageToAll();
    }
    COfferManager::GetInstance()->handlePostMessage(success);
    m_notificationHandler->HandlePostMessage(success, network);
}

unsigned char CNGSLocalUser::HandlePostMessage(unsigned char success, int network)
{
    return 1;
}

void CNGSLocalUser::CNGSHandleFriendPicUpdate(unsigned char success, int network)
{
    m_notificationHandler->HandleFriendPicUpdate(success, network);
}
