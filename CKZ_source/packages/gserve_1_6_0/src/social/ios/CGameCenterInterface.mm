// CGameCenterInterface.mm - Game Center social interface: forwards to CGameCenterManager (GameKit)
// and reports the results to the local gServe user.
#include "CGameCenterInterface.h"
#import "CGameCenterManager.h"
#import "CUtil.h"
#include "CNGS.h"
#include "CNGSLocalUser.h"
#include "CNotificationHandler.h"

using com::glu::platform::components::CStrWChar;
using com::glu::platform::components::CStrChar;

/** The Game Center manager behind m_nativeManager. */
#define GC_MANAGER ((CGameCenterManager*)m_nativeManager)

CGameCenterInterface::CGameCenterInterface()
{
    m_userAuthenticated = 0;
    m_loadFriendState = 0;
    m_nativeManager = [[CGameCenterManager alloc] init];
    m_loginReadPending = 0;
    m_socialNetworkType = SOCIAL_NETWORK_GAME_CENTER;
}

CGameCenterInterface::~CGameCenterInterface()
{
    if (m_nativeManager != NULL) {
        [GC_MANAGER release];
    }
}

void CGameCenterInterface::login()
{
    CNGSLocalUser* user = CNGS::GetInstance()->GetLocalUser();
    if (!CNGS::GetInstance()->GetLocalUser()->isAuthenticated(SOCIAL_NETWORK_GAME_CENTER)) {
        // finishLogin() completes the read request.
        user->AddReadRequest();
        m_loginReadPending = 1;
        [GC_MANAGER authenticate];
    } else {
        user->CNGSHandleUserLogin(12, SOCIAL_NETWORK_GAME_CENTER, NULL);
    }
}

void CGameCenterInterface::finishLogin()
{
    CNGSLocalUser* user = CNGS::GetInstance()->GetLocalUser();
    int result;
    switch (getConnectionState()) {
    case 2: {
        CStrWChar previousId = user->GetIDForSocialNetwork(SOCIAL_NETWORK_GAME_CENTER);
        CStrWChar playerId = getPlayerID();
        user->SetIDForSocialNetwork(SOCIAL_NETWORK_GAME_CENTER, getPlayerID());
        user->SetAliasForSocialNetwork(SOCIAL_NETWORK_GAME_CENTER, getPlayerAlias());
        user->m_credentials.debugPrint();
        if (!m_userAuthenticated) {
            handlePostUserLogin(SOCIAL_NETWORK_GAME_CENTER, previousId);
        } else if (previousId.GetLength() != 0 && !(previousId == playerId)) {
            // Another Game Center player signed in.
            user->SetIDForSocialNetwork(SOCIAL_NETWORK_GAME_CENTER, getPlayerID());
            user->SetNickName(getPlayerAlias());
            user->CNGSHandlePrimaryUserChange(SOCIAL_NETWORK_GAME_CENTER, "finishLogin");
        }
        goto done;
    }
    case 3:
        user->LoadCredentials();
        CNGS::GetInstance()->SetNetworkConnectionState((eNetworkConnectionState)5, "finishLogin");
        result = 11;
        break;
    case 4:
        result = 10;
        break;
    default:
        goto done;
    }
    user->CNGSHandleUserLogin(result, SOCIAL_NETWORK_GAME_CENTER, NULL);
done:
    if (m_loginReadPending) {
        m_loginReadPending = 0;
        user->CompleteReadRequest();
    }
}

void CGameCenterInterface::logout()
{
}

void CGameCenterInterface::finishLogout()
{
    resetSaveFriendsData();
    CNGS::GetInstance()->GetLocalUser()->CNGSHandleUserLogout(1, SOCIAL_NETWORK_GAME_CENTER);
}

void CGameCenterInterface::invalidate()
{
    m_userAuthenticated = 0;
    [GC_MANAGER clearData];
}

void CGameCenterInterface::loadFriends()
{
    CNGS::GetInstance()->GetLocalUser()->AddReadRequest();
    [GC_MANAGER loadFriends];
}

void CGameCenterInterface::finishLoadFriends()
{
    if (getConnectionState() == 3) {
        m_loadFriendState = 2;
        CNGS::GetInstance()->SetNetworkConnectionState((eNetworkConnectionState)5, "finishLoadFriends");
    } else {
        m_loadFriendState = 1;
    }
    CNGS::GetInstance()->GetLocalUser()->HandleLoadFriendListFromServerResponse(SOCIAL_NETWORK_GAME_CENTER);
}

int CGameCenterInterface::getConnectionState()
{
    return [GC_MANAGER getConnectionState];
}

CStrWChar CGameCenterInterface::getPlayerID()
{
    NSString* playerId = [GC_MANAGER getLocalUserID];
    return CStrWChar([CUtil NSStringToWchar:playerId]);
}

CStrWChar CGameCenterInterface::getPlayerAlias()
{
    NSString* alias = [GC_MANAGER getLocalUserAlias];
    return CStrWChar([CUtil NSStringToWchar:alias]);
}

int CGameCenterInterface::getFriendsCount()
{
    return [GC_MANAGER getFriendsCount];
}

CStrWChar CGameCenterInterface::getFriendIDAtIndex(int index)
{
    NSString* friendId = [[GC_MANAGER getFriends] objectAtIndex:index];
    return CStrWChar([CUtil NSStringToWchar:friendId]);
}

void CGameCenterInterface::reportScore(int score, const CStrWChar& category)
{
    char categoryText[256] = {0};
    wcstombs(categoryText, category.m_string, wcslen(category.m_string) + 1);
    CStrChar categoryName(categoryText);
    const char* categoryChars = categoryName.GetCharPtr();
    NSString* categoryString = [[[NSString alloc] initWithUTF8String:categoryChars] autorelease];
    [GC_MANAGER reportScore:score forCategory:categoryString];
}

void CGameCenterInterface::finishReportScore()
{
    getConnectionState();
}

void CGameCenterInterface::showLeaderboard(void* viewController)
{
    [GC_MANAGER showLeaderBoard:(UIViewController*)viewController];
}

void CGameCenterInterface::showAchievement(void* viewController)
{
    [GC_MANAGER showAchievement:(UIViewController*)viewController];
}

void CGameCenterInterface::showFriendRequestScreen(void* viewController)
{
    [GC_MANAGER showFriendRequestScreen:(UIViewController*)viewController];
}

void CGameCenterInterface::reportAchievement(int percent, const CStrWChar& identifier)
{
    char identifierText[256] = {0};
    wcstombs(identifierText, identifier.m_string, wcslen(identifier.m_string) + 1);
    CStrChar identifierName(identifierText);
    const char* identifierChars = identifierName.GetCharPtr();
    NSString* identifierString = [[[NSString alloc] initWithUTF8String:identifierChars] autorelease];
    [GC_MANAGER reportAchievementIdentifier:identifierString percentComplete:percent];
}

void CGameCenterInterface::finishReportAchievement()
{
    getConnectionState();
}

void CGameCenterInterface::handleNotification()
{
    CNGSLocalUser* user = CNGS::GetInstance()->GetLocalUser();
    if (isUserAuthenticated()) {
        CStrWChar previousId = user->GetIDForSocialNetwork(SOCIAL_NETWORK_GAME_CENTER);
        user->SetIDForSocialNetwork(SOCIAL_NETWORK_GAME_CENTER, getPlayerID());
        CStrWChar playerId = getPlayerID();
        if (previousId.GetLength() != 0 && !(previousId == playerId)) {
            // Another Game Center player signed in.
            user->SetIDForSocialNetwork(SOCIAL_NETWORK_GAME_CENTER, getPlayerID());
            user->SetNickName(getPlayerAlias());
            user->CNGSHandlePrimaryUserChange(SOCIAL_NETWORK_GAME_CENTER, "finishLogin");
        } else {
            user->m_notificationHandler->HandleGameCenterAppLogin([GC_MANAGER isUserAuthenticated]);
        }
    } else {
        user->m_notificationHandler->HandleGameCenterAppLogin([GC_MANAGER isUserAuthenticated]);
    }
}

void CGameCenterInterface::handleFriendsNotification()
{
}

unsigned char CGameCenterInterface::requestFriendPicWithID(const CStrWChar& id, const CStrWChar* type)
{
    return 0;
}

unsigned char CGameCenterInterface::getFriendPicWithID(const CStrWChar& id, const void*& data, unsigned int& size)
{
    data = NULL;
    size = 0;
    return 0;
}

void CGameCenterInterface::freeFriendPicWithID(const CStrWChar& id)
{
}

void CGameCenterInterface::freeAllFriendPics()
{
}
