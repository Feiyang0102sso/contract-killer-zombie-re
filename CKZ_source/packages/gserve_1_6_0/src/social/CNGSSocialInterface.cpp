// CNGSSocialInterface.cpp - default (no network) behaviour of the social interfaces and the
// shared login / post / friend picture bookkeeping with the gServe local user.
#include "CNGSSocialInterface.h"
#include "CNGS.h"
#include "CNGSConst.h"
#include "CNGSLocalUser.h"

using com::glu::platform::components::CStrWChar;
using com::glu::platform::core::CVector;

/** Buckets and pool size of the saved friends data table. */
static const unsigned long SAVED_FRIENDS_DATA_BUCKETS = 10;
static const unsigned long SAVED_FRIENDS_DATA_POOL = 10;

CNGSSocialInterface::CNGSSocialInterface()
{
    m_userAuthenticated = 0;
    m_socialNetworkType = -1;
    m_messageHasTarget = 0;
}

CNGSSocialInterface::~CNGSSocialInterface()
{
}

void CNGSSocialInterface::setAppID(CStrWChar appId, CStrWChar appSecret)
{
}

unsigned char CNGSSocialInterface::isUserAuthenticated()
{
    return m_userAuthenticated;
}

void CNGSSocialInterface::setUserAuthenticated(unsigned char authenticated)
{
    m_userAuthenticated = authenticated;
}

const char* CNGSSocialInterface::getAttributeName(int network)
{
    return SocialNetwork_NGS_ID_TOKEN[network];
}

const char* CNGSSocialInterface::getUserAliasAttributeName(int network)
{
    return SocialNetwork_NGS_ALIAS_TOKEN[network];
}

void CNGSSocialInterface::login()
{
}

void CNGSSocialInterface::finishLogin()
{
}

void CNGSSocialInterface::logout()
{
}

void CNGSSocialInterface::finishLogout()
{
}

void CNGSSocialInterface::invalidate()
{
}

void CNGSSocialInterface::loadFriends()
{
}

void CNGSSocialInterface::finishLoadFriends()
{
}

int CNGSSocialInterface::getConnectionState()
{
    return 0;
}

int CNGSSocialInterface::getPostConnectionState()
{
    return getConnectionState();
}

CStrWChar CNGSSocialInterface::getPlayerID()
{
    return CStrWChar("");
}

CStrWChar CNGSSocialInterface::getPlayerAlias()
{
    return CStrWChar("");
}

CStrWChar CNGSSocialInterface::getPlayerEmail()
{
    return CStrWChar("");
}

int CNGSSocialInterface::getFriendsCount()
{
    return 0;
}

CStrWChar CNGSSocialInterface::getFriendIDAtIndex(int index)
{
    return CStrWChar("");
}

CStrWChar CNGSSocialInterface::getFriendIDFromSortedListAtIndex(int index)
{
    return m_friendsInfos[index].m_id;
}

CStrWChar CNGSSocialInterface::getFriendNameFromSortedListAtIndex(int index)
{
    if (index >= 0 && index < m_friendsInfos.Size()) {
        return m_friendsInfos[index].m_name;
    }
    return CStrWChar(L"unknown");
}

CStrWChar CNGSSocialInterface::getFriendNameFromSortedListWithId(const CStrWChar& id)
{
    for (int index = 0; index < m_friendsInfos.Size(); index++) {
        if (id == m_friendsInfos[index].m_id) {
            return m_friendsInfos[index].m_name;
        }
    }
    return CStrWChar(L"unknown");
}

int CNGSSocialInterface::getLoadFriendState()
{
    return m_loadFriendState;
}

void CNGSSocialInterface::resume()
{
}

void CNGSSocialInterface::reportScore(int score, const CStrWChar& category)
{
}

void CNGSSocialInterface::finishReportScore()
{
}

void CNGSSocialInterface::showLeaderboard(void* viewController)
{
}

void CNGSSocialInterface::showAchievement(void* viewController)
{
}

void CNGSSocialInterface::showFriendRequestScreen(void* viewController)
{
}

void CNGSSocialInterface::reportAchievement(int percent, const CStrWChar& identifier)
{
}

void CNGSSocialInterface::finishReportAchievement()
{
}

void CNGSSocialInterface::postMessage(CNGSSocialMessage* message)
{
}

void CNGSSocialInterface::postMessage(const CStrWChar* text, const CNGSUser* targetUser, const CStrWChar* link,
                                      const CStrWChar* name, const CStrWChar* picture)
{
}

void CNGSSocialInterface::postMessage(const CStrWChar* text, const CStrWChar* targetId, const CStrWChar* link,
                                      const CStrWChar* name, const CStrWChar* caption, const CStrWChar* description,
                                      const CStrWChar* picture, const CStrWChar* privacy)
{
}

void CNGSSocialInterface::finishPostMessage(unsigned char success)
{
    CNGS::GetInstance()->GetLocalUser()->CNGSHandlePostMessage(success, m_socialNetworkType);
}

unsigned char CNGSSocialInterface::isMessageToAll()
{
    return m_messageHasTarget == 0;
}

void CNGSSocialInterface::handlePostUserLogin(int network, CStrWChar& previousId)
{
    CNGSLocalUser* localUser = CNGS::GetInstance()->GetLocalUser();
    CStrWChar playerId = getPlayerID();

    if (localUser->isValid() && localUser->GetClientID() != -1) {
        // already known to the server: link the network account
        localUser->AssociateUser(network);
    } else {
        if (localUser->GetNickName().GetLength() == 0) {
            localUser->SetNickName(getPlayerAlias());
        }
        if (previousId.GetLength() != 0 && !(previousId == playerId)) {
            // another network account than the saved one logged in
            localUser->CNGSHandlePrimaryUserChange(network, "finishLogin");
        } else {
            localUser->ValidateUser(network, 0);
        }
    }
}

void CNGSSocialInterface::resetSaveFriendsData()
{
    m_savedFriendsData.RemoveAll();
    m_savedFriendsData.Init(SAVED_FRIENDS_DATA_BUCKETS, SAVED_FRIENDS_DATA_POOL, 1);
    m_friendsInfos.Clear();
}

CVector<FriendInfo>* CNGSSocialInterface::getSocialNetworkFriendsInfos()
{
    return &m_friendsInfos;
}

unsigned char CNGSSocialInterface::requestFriendPicWithID(const CStrWChar& id, const CStrWChar* type)
{
    return 0;
}

unsigned char CNGSSocialInterface::getFriendPicWithID(const CStrWChar& id, const void*& data, unsigned int& size)
{
    data = NULL;
    size = 0;
    return 0;
}

void CNGSSocialInterface::freeFriendPicWithID(const CStrWChar& id)
{
}

void CNGSSocialInterface::freeAllFriendPics()
{
}

void CNGSSocialInterface::finishLoadFriendPic(unsigned char success)
{
    CNGS::GetInstance()->GetLocalUser()->CNGSHandleFriendPicUpdate(success, m_socialNetworkType);
}
