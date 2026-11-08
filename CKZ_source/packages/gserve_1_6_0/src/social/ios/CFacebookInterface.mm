// CFacebookInterface.mm - Facebook social interface: forwards to CFacebookManager and reports the
// results (login, friends, posts, pictures) to the gServe local user.
#include "CFacebookInterface.h"
#include <stdlib.h>
#include <wchar.h>
#include "CFacebookManager.h"
#include "CNGS.h"
#include "CNGSLocalUser.h"
#include "CNGSSocialMessage.h"
#include "CStrChar.h"
#include "CUtil.h"

using com::glu::platform::components::CStrChar;
using com::glu::platform::components::CStrWChar;

/** Size of the narrow copy of the application id. */
static const int APP_ID_BUFFER_SIZE = 256;

/** CNGSLocalUser::CNGSHandleUserLogin results. */
static const int LOGIN_RESULT_CANCELLED = 10;
static const int LOGIN_RESULT_FAILED = 11;
static const int LOGIN_RESULT_ALREADY_AUTHENTICATED = 12;

/** Connection state reported to CNGS when the Facebook request failed. */
static const int SOCIAL_NETWORK_CONNECTION_ERROR = 6;

CFacebookInterface::CFacebookInterface()
{
    m_userAuthenticated = 0;
    m_loadFriendState = 0;
    m_loginReadPending = 0;
    m_nativeManager = [[CFacebookManager alloc] init];
    m_socialNetworkType = SOCIAL_NETWORK_FACEBOOK;
    m_messageHasTarget = 0;
}

CFacebookInterface::~CFacebookInterface()
{
    CFacebookManager* manager = (CFacebookManager*)m_nativeManager;
    if (manager != nil) {
        [manager release];
        m_nativeManager = nil;
    }
}

void CFacebookInterface::setAppID(CStrWChar appId, CStrWChar appSecret)
{
    char buffer[APP_ID_BUFFER_SIZE] = {0};
    wcstombs(buffer, appId.m_string, wcslen(appId.m_string) + 1);
    CStrChar narrowAppId(buffer);
    NSString* appIdString = [[[NSString alloc] initWithUTF8String:narrowAppId.GetCharPtr()] autorelease];
    [(CFacebookManager*)m_nativeManager setAppID:appIdString];
}

void CFacebookInterface::login()
{
    CNGSLocalUser* localUser = CNGS::GetInstance()->GetLocalUser();
    if (!CNGS::GetInstance()->GetLocalUser()->isAuthenticated(SOCIAL_NETWORK_FACEBOOK)) {
        // the read request stays open until finishLogin() / resume()
        m_loginReadPending = 1;
        localUser->AddReadRequest();
        [(CFacebookManager*)m_nativeManager login];
    } else {
        localUser->CNGSHandleUserLogin(LOGIN_RESULT_ALREADY_AUTHENTICATED, SOCIAL_NETWORK_FACEBOOK, NULL);
    }
}

void CFacebookInterface::finishLogin()
{
    CNGSLocalUser* localUser = CNGS::GetInstance()->GetLocalUser();
    switch (getConnectionState()) {
    case FB_STATE_DONE: {
        CStrWChar previousId = localUser->GetIDForSocialNetwork(SOCIAL_NETWORK_FACEBOOK);
        CStrWChar playerId = getPlayerID();
        if (previousId.GetLength() != 0 && !(previousId == playerId)) {
            // another Facebook account than the saved one
            localUser->SetIDForSocialNetwork(SOCIAL_NETWORK_FACEBOOK, getPlayerID());
            localUser->SetNickName(getPlayerAlias());
            localUser->CNGSHandlePrimaryUserChange(SOCIAL_NETWORK_FACEBOOK, "finishLogin");
        } else {
            localUser->SetIDForSocialNetwork(SOCIAL_NETWORK_FACEBOOK, getPlayerID());
            localUser->SetAliasForSocialNetwork(SOCIAL_NETWORK_FACEBOOK, getPlayerAlias());
            handlePostUserLogin(SOCIAL_NETWORK_FACEBOOK, previousId);
        }
        break;
    }
    case FB_STATE_CANCELLED:
        localUser->CNGSHandleUserLogin(LOGIN_RESULT_CANCELLED, SOCIAL_NETWORK_FACEBOOK, NULL);
        break;
    case FB_STATE_FAILED:
        localUser->LoadCredentials();
        CNGS::GetInstance()->SetNetworkConnectionState((eNetworkConnectionState)SOCIAL_NETWORK_CONNECTION_ERROR, "finishLogin");
        localUser->CNGSHandleUserLogin(LOGIN_RESULT_FAILED, SOCIAL_NETWORK_FACEBOOK, NULL);
        break;
    }

    if (m_loginReadPending) {
        m_loginReadPending = 0;
        localUser->CompleteReadRequest();
    }
}

void CFacebookInterface::logout()
{
    CNGS::GetInstance()->GetLocalUser()->AddReadRequest();
    [(CFacebookManager*)m_nativeManager logout];
}

void CFacebookInterface::finishLogout()
{
    resetSaveFriendsData();
    CNGSLocalUser* localUser = CNGS::GetInstance()->GetLocalUser();
    localUser->CompleteReadRequest();
    localUser->CNGSHandleUserLogout(1, SOCIAL_NETWORK_FACEBOOK);
}

void CFacebookInterface::invalidate()
{
    m_userAuthenticated = 0;
    [(CFacebookManager*)m_nativeManager clearData];
}

void CFacebookInterface::loadFriends()
{
    CNGS::GetInstance()->GetLocalUser()->AddReadRequest();
    [(CFacebookManager*)m_nativeManager loadFriends];
}

void CFacebookInterface::finishLoadFriends()
{
    if (getConnectionState() == FB_STATE_FAILED) {
        m_loadFriendState = 2;
        CNGS::GetInstance()->SetNetworkConnectionState((eNetworkConnectionState)SOCIAL_NETWORK_CONNECTION_ERROR, "finishLoadFriends");
    } else {
        m_loadFriendState = 1;
    }
    CNGS::GetInstance()->GetLocalUser()->HandleLoadFriendListFromServerResponse(SOCIAL_NETWORK_FACEBOOK);
}

int CFacebookInterface::getConnectionState()
{
    return [(CFacebookManager*)m_nativeManager getConnectionState];
}

int CFacebookInterface::getPostConnectionState()
{
    return [(CFacebookManager*)m_nativeManager getPostConnectionState];
}

CStrWChar CFacebookInterface::getPlayerID()
{
    NSString* text = [(CFacebookManager*)m_nativeManager getUserID];
    return CStrWChar([CUtil NSStringToWchar:text]);
}

CStrWChar CFacebookInterface::getPlayerAlias()
{
    NSString* text = [(CFacebookManager*)m_nativeManager getUserName];
    return CStrWChar([CUtil NSStringToWchar:text]);
}

CStrWChar CFacebookInterface::getPlayerEmail()
{
    NSString* text = [(CFacebookManager*)m_nativeManager getUserEmail];
    return CStrWChar([CUtil NSStringToWchar:text]);
}

int CFacebookInterface::getFriendsCount()
{
    return [(CFacebookManager*)m_nativeManager getFriendsCount];
}

CStrWChar CFacebookInterface::getFriendIDAtIndex(int index)
{
    NSString* friendId = [[(CFacebookManager*)m_nativeManager getFriends] objectAtIndex:index];
    return CStrWChar([CUtil NSStringToWchar:friendId]);
}

void CFacebookInterface::postMessage(CNGSSocialMessage* message)
{
    CStrWChar targetId;
    message->GetParam("target_id", targetId);
    m_messageHasTarget = targetId.GetLength() > 0;
    [(CFacebookManager*)m_nativeManager publishStream:message];
}

void CFacebookInterface::postMessage(const CStrWChar* text, const CNGSUser* targetUser, const CStrWChar* link,
                                     const CStrWChar* name, const CStrWChar* picture)
{
    CStrWChar targetId;
    if (targetUser != NULL) {
        const CNGSUserCredentials* credentials = &targetUser->m_credentials;
        targetId = credentials->getIDForSocialNetwork(SOCIAL_NETWORK_FACEBOOK);
        credentials->debugPrint();
        if (targetId.GetLength() < 1) {
            // the user has no Facebook id: nothing to post to
            return;
        }
    }
    postMessage(text, &targetId, link, name, NULL, NULL, picture, NULL);
}

void CFacebookInterface::postMessage(const CStrWChar* text, const CStrWChar* targetId, const CStrWChar* link,
                                     const CStrWChar* name, const CStrWChar* caption, const CStrWChar* description,
                                     const CStrWChar* picture, const CStrWChar* privacy)
{
    CNGSSocialMessage* message = CNGSSocialMessage::CreateInstance(SOCIAL_NETWORK_FACEBOOK);
    if (targetId != NULL && targetId->GetLength() > 0) {
        message->SetParam("target_id", targetId->m_string);
    }
    if (text != NULL && text->GetLength() > 0) {
        message->SetParam("message", text->m_string);
    }
    if (name != NULL && name->GetLength() > 0) {
        message->SetParam("name", name->m_string);
    }
    if (caption != NULL && caption->GetLength() > 0) {
        message->SetParam("caption", caption->m_string);
    }
    if (description != NULL && description->GetLength() > 0) {
        message->SetParam("description", description->m_string);
    }
    if (link != NULL && link->GetLength() > 0) {
        message->SetParam("link", link->m_string);
    }
    if (picture != NULL && picture->GetLength() > 0) {
        message->SetParam("picture", picture->m_string);
    }
    if (privacy != NULL && privacy->GetLength() > 0) {
        message->SetParam("privacy", privacy->m_string);
    }
    postMessage(message);
}

void CFacebookInterface::resume()
{
    // the login page was left without an answer: release the read request of login()
    if (m_loginReadPending) {
        m_loginReadPending = 0;
        CNGS::GetInstance()->GetLocalUser()->CompleteReadRequest();
    }
}

BOOL CFacebookInterface::handleOpenURL(NSURL* url)
{
    return [(CFacebookManager*)m_nativeManager handleOpenURL:url];
}

unsigned char CFacebookInterface::requestFriendPicWithID(const CStrWChar& id, const CStrWChar* type)
{
    NSString* pictureType = @"small";
    if (type != NULL) {
        pictureType = [CUtil WcharToNSString:type->m_string];
    }
    NSString* friendId = [CUtil WcharToNSString:id.m_string];
    return [(CFacebookManager*)m_nativeManager loadUserPic:friendId type:pictureType] == YES;
}

unsigned char CFacebookInterface::getFriendPicWithID(const CStrWChar& id, const void*& data, unsigned int& size)
{
    unsigned char found = 0;
    data = NULL;
    size = 0;
    CFacebookManager* manager = (CFacebookManager*)m_nativeManager;
    NSString* friendId = [CUtil WcharToNSString:id.m_string];
    NSMutableDictionary* info = [[manager getFriendsInfo] objectForKey:friendId];
    if (info != nil) {
        found = 1;
        NSData* picture = [info objectForKey:@"picture"];
        if (picture != nil) {
            found = 1;
            data = [picture bytes];
            size = [picture length];
        }
    }
    return found;
}

void CFacebookInterface::freeFriendPicWithID(const CStrWChar& id)
{
    CFacebookManager* manager = (CFacebookManager*)m_nativeManager;
    NSString* friendId = [CUtil WcharToNSString:id.m_string];
    NSMutableDictionary* info = [[manager getFriendsInfo] objectForKey:friendId];
    if (info != nil) {
        if ([info objectForKey:@"picture"] != nil) {
            [info removeObjectForKey:@"picture"];
        }
    }
}

void CFacebookInterface::freeAllFriendPics()
{
    CFacebookManager* manager = (CFacebookManager*)m_nativeManager;
    NSMutableArray* friends = [manager getFriends];
    for (unsigned int index = 0; index < [friends count]; index++) {
        CStrWChar friendId([CUtil NSStringToWchar:[friends objectAtIndex:index]]);
        if (friendId.GetLength() > 0) {
            freeFriendPicWithID(friendId);
        }
    }
    CStrWChar playerId([CUtil NSStringToWchar:[manager getUserID]]);
    if (playerId.GetLength() > 0) {
        freeFriendPicWithID(playerId);
    }
}
