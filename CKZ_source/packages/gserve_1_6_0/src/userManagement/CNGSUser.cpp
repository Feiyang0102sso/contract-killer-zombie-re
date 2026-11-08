// CNGSUser.cpp - common part of the local and the remote users: credentials file handling
// and accessors.
#include "CNGSUser.h"
#include "CNGS.h"
#include "CNGSFactory.h"
#include "CNGSRemoteUser.h"
#include "CFileUtil.h"
#include "ICFileMgr.h"
#include "CNGSAttributeManager.h"

using com::glu::platform::components::CStrWChar;
using com::glu::platform::components::CFileUtil;
using com::glu::platform::components::ICFileMgr;
using com::glu::platform::core::CVector;

CNGSUser::CNGSUser(const CNGSUserCredentials* credentials)
{
    if (credentials != NULL) {
        m_credentials = *credentials;
    }
}

CNGSUser::~CNGSUser()
{
}

void CNGSUser::SaveCredentials(const wchar_t* fileName)
{
    CStrWChar name(fileName);
    m_credentials.writeToFile(name, false);
    m_credentials.writeToFile(name, true);
}

void CNGSUser::LoadCredentials(const wchar_t* fileName)
{
    CStrWChar name(fileName);
    m_credentials.readFromFile(name, false);
}

void CNGSUser::ResetCredentials(const wchar_t* fileName)
{
    m_credentials.reset();
    if (fileName != NULL) {
        DeleteCredentialsFile(fileName);
    }
}

void CNGSUser::DeleteCredentialsFile(const wchar_t* fileName)
{
    CStrWChar path;
    CFileUtil::GetApplicationDataPathForFile(path, fileName);
    ICFileMgr::GetInstance()->Delete(path.GetWCharPtr());
}

int CNGSUser::GetClientID() const
{
    return m_credentials.m_userId;
}

void CNGSUser::SetNickName(const CStrWChar& nickName)
{
    m_credentials.m_nickName = nickName;
}

CStrWChar CNGSUser::GetNickName() const
{
    return m_credentials.m_nickName;
}

CStrWChar CNGSUser::GetEmail() const
{
    return m_credentials.m_email;
}

void CNGSUser::SetIDForSocialNetwork(eSocialNetworkType network, const CStrWChar& id)
{
    m_credentials.setIDForSocialNetwork(network, id);
}

CStrWChar CNGSUser::GetIDForSocialNetwork(eSocialNetworkType network) const
{
    return m_credentials.getIDForSocialNetwork(network);
}

CStrWChar CNGSUser::GetAliasForSocialNetwork(eSocialNetworkType network) const
{
    return m_credentials.getAliasForSocialNetwork(network);
}

void CNGSUser::SetAliasForSocialNetwork(eSocialNetworkType network, const CStrWChar& alias)
{
    m_credentials.setAliasForSocialNetwork(network, alias);
}

void CNGSUser::setFriendMsgTimestampLocally(int userId, unsigned long long timestamp)
{
    CNGSUserCredentials credentials;
    credentials.m_userId = userId;
    CNGSRemoteUser* user = CNGS::GetInstance()->GetFactory()->getRemoteUserByCredentials(credentials);
    if (user != NULL) {
        user->m_friendMsgTimestamp = timestamp;
    }
}

CObjectMapObject* CNGSUser::GetUserObject()
{
    return m_credentials.GetUserObject();
}

void CNGSUser::UpdateCredentials(const CNGSUserCredentials& credentials)
{
    if (m_credentials.mergeWith(credentials, true) != 1) {
        m_credentials = credentials;
    }
}

void CNGSUser::GetSKUBonusItems(CStrWChar& sku, CVector<CNGSAttribute*>& items)
{
}

CStrWChar CNGSUser::GetSKUBonusNameForItem(CNGSAttribute* item)
{
    return CStrWChar(L"??");
}

void CNGSUser::DebugPrint()
{
    m_credentials.debugPrint();
}
