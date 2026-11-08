// CNGSUserCredentials.cpp - identity of a gServe user and its JSON form (credentials file and
// server user objects).
// The original clears its file buffers with the C library memset, not np_memset.
#undef memset
#include <string.h>
#include "CNGSUserCredentials.h"
#include "CObjectMapObject.h"
#include "CObjectMapValue.h"
#include "CNGSJSONParser.h"
#include "CFileUtil_gServe.h"
#include "CArrayInputStream.h"
#include "CArrayOutputStream.h"
#include "ICFileMgr.h"
#include "ICStdUtil.h"
#include "CNGSConst.h"
#include "CNGSTokens.h"

using com::glu::platform::components::CStrWChar;
using com::glu::platform::components::CArrayInputStream;
using com::glu::platform::components::CArrayOutputStream;
using com::glu::platform::components::ICFileMgr;
using com::glu::platform::core::ICStdUtil;

/** Size of the credentials file buffer. */
static const unsigned int CREDENTIALS_FILE_SIZE = 0x1000;

CNGSUserCredentials::CNGSUserCredentials()
{
    alloc();
}

void CNGSUserCredentials::alloc()
{
    m_socialIds = new CStrWChar[SOCIAL_NETWORK_COUNT];
    m_socialAliases = new CStrWChar[SOCIAL_NETWORK_COUNT];
    reset();
}

CNGSUserCredentials::CNGSUserCredentials(CObjectMapObject* object, bool fromServer)
{
    alloc();
    InitFromObjectRepresentation(object, fromServer);
}

unsigned char CNGSUserCredentials::InitFromObjectRepresentation(CObjectMapObject* object, bool fromServer)
{
    unsigned char result = 0;
    if (object == NULL) {
        return result;
    }

    // JSON text of the object (only logged in debug builds)
    CStrWChar json = CNGSJSONParser::encodeValue(object);

    // credentials file keys, replaced by the server user keys below
    CStrWChar idKey("clientid");
    CStrWChar avatarIdKey("avatarid");
    CStrWChar nickNameKey("nickname");
    CStrWChar emailKey("email");
    CStrWChar networkNameKey("socialnetworkname");
    CStrWChar networkIdKey("socialnetworkid");
    if (fromServer) {
        idKey = NGS_ID_TOKEN;
        avatarIdKey = "avatarid";
        nickNameKey = "gamertag";
    }

    unsigned char validId = 0;
    CObjectMapValue* value = object->getEntry(idKey);
    if (value != NULL && value->getType() == CObjectMapValue::TYPE_INT) {
        validId = 1;
        m_userId = (int)((CObjectMapInt*)value)->m_value;
        if (m_userId < 1) {
            validId = 0;
        }
    }

    value = object->getEntry(avatarIdKey);
    if (value != NULL && value->getType() == CObjectMapValue::TYPE_INT) {
        m_avatarId = (int)((CObjectMapInt*)value)->m_value;
    }

    value = object->getEntry(nickNameKey);
    if (value != NULL && value->getType() == CObjectMapValue::TYPE_STRING) {
        m_nickName = CStrWChar(((CObjectMapString*)value)->m_value);
        result = validId;
    }

    value = object->getEntry(emailKey);
    if (value != NULL && value->getType() == CObjectMapValue::TYPE_STRING) {
        m_email = CStrWChar(((CObjectMapString*)value)->m_value);
    }

    if (fromServer) {
        // a server user carries the id of one social network: its name and the id
        CStrWChar networkName;
        CStrWChar networkId;
        value = object->getEntry(networkIdKey);
        if (value != NULL && value->getType() == CObjectMapValue::TYPE_STRING) {
            networkId = CStrWChar(((CObjectMapString*)value)->m_value);
        }
        value = object->getEntry(networkNameKey);
        if (value != NULL && value->getType() == CObjectMapValue::TYPE_STRING) {
            networkName = CStrWChar(((CObjectMapString*)value)->m_value);
            for (int network = 0; network < SOCIAL_NETWORK_COUNT; network++) {
                if (networkName == CStrWChar(SocialNetwork_NGS_ID_TOKEN[network])) {
                    m_socialIds[network] = networkId;
                    break;
                }
            }
        }
    } else {
        // the credentials file stores every network id and alias under its own key
        for (int network = 0; network < SOCIAL_NETWORK_COUNT; network++) {
            value = object->getEntry(CStrWChar(SocialNetworkIDNames[network]));
            if (value != NULL && value->getType() == CObjectMapValue::TYPE_STRING) {
                m_socialIds[network] = CStrWChar(((CObjectMapString*)value)->m_value);
            }
        }
        for (int network = 0; network < SOCIAL_NETWORK_COUNT; network++) {
            value = object->getEntry(CStrWChar(SocialNetworkAliasNames[network]));
            if (value != NULL && value->getType() == CObjectMapValue::TYPE_STRING) {
                m_socialAliases[network] = CStrWChar(((CObjectMapString*)value)->m_value);
            }
        }
    }
    return result;
}

CNGSUserCredentials::~CNGSUserCredentials()
{
    if (m_socialIds != NULL) {
        delete[] m_socialIds;
        m_socialIds = NULL;
    }
    if (m_socialAliases != NULL) {
        delete[] m_socialAliases;
        m_socialAliases = NULL;
    }
}

void CNGSUserCredentials::reset()
{
    m_userId = -1;
    m_nickName = L"";
    m_email = L"";
    m_avatarId = -1;
    for (int network = 0; network < SOCIAL_NETWORK_COUNT; network++) {
        m_socialIds[network] = L"";
        m_socialAliases[network] = L"";
    }
}

CNGSUserCredentials& CNGSUserCredentials::operator=(const CNGSUserCredentials& other)
{
    m_userId = other.m_userId;
    m_avatarId = other.m_avatarId;
    m_nickName = other.m_nickName;
    m_email = other.m_email;
    for (int network = 0; network < SOCIAL_NETWORK_COUNT; network++) {
        m_socialIds[network] = other.m_socialIds[network];
        m_socialAliases[network] = other.m_socialAliases[network];
    }
    return *this;
}

unsigned char CNGSUserCredentials::readFromFile(CStrWChar& fileName, bool useKey)
{
    unsigned char data[CREDENTIALS_FILE_SIZE];
    memset(data, 0, sizeof(data));
    CStrWChar subDirectory = getSubDirectoryPath(m_userId);
    const wchar_t* folder = NULL;
    if (useKey) {
        folder = subDirectory.GetWCharPtr();
    }

    unsigned char result = 0;
    if (CFileUtil_gServe::ReadApplicationDataFile(fileName.GetWCharPtr(), data, sizeof(data), folder)) {
        CArrayInputStream stream;
        if (stream.Open(data, sizeof(data))) {
            wchar_t text[CREDENTIALS_FILE_SIZE + 1];
            int length = stream.ReadJMUtf(text, CREDENTIALS_FILE_SIZE + 1);
            text[length] = 0;
            if (length > 0) {
                CObjectMapObject* object = (CObjectMapObject*)CNGSJSONParser::decodeValue(CStrWChar(text));
                result = InitFromObjectRepresentation(object, false);
                if (object != NULL) {
                    delete object;
                }
            }
            stream.Close();
        }
    }
    return result;
}

unsigned char CNGSUserCredentials::writeToFile(CStrWChar& fileName, bool useKey)
{
    unsigned char data[CREDENTIALS_FILE_SIZE];
    memset(data, 0, sizeof(data));
    unsigned char result = 0;
    CArrayOutputStream stream;
    if (stream.Open(data, sizeof(data))) {
        CObjectMapObject* object = CreateObjectRepresentation();
        stream.WriteJMUtf(CNGSJSONParser::encodeValue(object).GetWCharPtr());
        if (object != NULL) {
            delete object;
        }

        CStrWChar subDirectory = getSubDirectoryPath(m_userId);
        const wchar_t* folder = NULL;
        if (useKey) {
            // the per user folder has to exist before writing into it
            ICFileMgr::GetInstance()->MkDir(getFullDirectoryPath().GetWCharPtr());
            folder = subDirectory.GetWCharPtr();
        }
        result = CFileUtil_gServe::WriteApplicationDataFile(fileName.GetWCharPtr(), data, sizeof(data), folder);
        stream.Close();
    }
    return result;
}

unsigned char CNGSUserCredentials::isValid() const
{
    unsigned char valid = 1;
    if (m_userId == -1) {
        valid = m_email.GetLength() != 0;
        if (!valid) {
            for (int network = 0; network < SOCIAL_NETWORK_COUNT; network++) {
                if (m_socialIds[network].GetLength() != 0) {
                    return 1;
                }
            }
        }
    }
    return valid;
}

unsigned char CNGSUserCredentials::hasValidAvatar() const
{
    return m_avatarId > 0;
}

CStrWChar CNGSUserCredentials::getIDForSocialNetwork(eSocialNetworkType network) const
{
    return m_socialIds[network];
}

void CNGSUserCredentials::setIDForSocialNetwork(eSocialNetworkType network, const CStrWChar& id)
{
    m_socialIds[network] = id;
}

CStrWChar CNGSUserCredentials::getAliasForSocialNetwork(eSocialNetworkType network) const
{
    return m_socialAliases[network];
}

void CNGSUserCredentials::setAliasForSocialNetwork(eSocialNetworkType network, const CStrWChar& alias)
{
    m_socialAliases[network] = alias;
}

CObjectMapObject* CNGSUserCredentials::CreateObjectRepresentation() const
{
    CObjectMapObject* object = new CObjectMapObject();
    object->addEntry(CStrWChar("clientid"), new CObjectMapInt(m_userId));
    object->addEntry(CStrWChar("avatarid"), new CObjectMapInt(m_avatarId));
    object->addEntry(CStrWChar("nickname"), new CObjectMapString(m_nickName.GetWCharPtr()));
    object->addEntry(CStrWChar("email"), new CObjectMapString(m_email.GetWCharPtr()));
    for (int network = 0; network < SOCIAL_NETWORK_COUNT; network++) {
        object->addEntry(CStrWChar(SocialNetworkIDNames[network]),
                         new CObjectMapString(m_socialIds[network].GetWCharPtr()));
        object->addEntry(CStrWChar(SocialNetworkAliasNames[network]),
                         new CObjectMapString(m_socialAliases[network].GetWCharPtr()));
    }
    return object;
}

unsigned char CNGSUserCredentials::mergeWith(const CNGSUserCredentials& other, bool overwrite)
{
    // two different known client ids: not the same user
    if (m_userId > 0 && other.m_userId > 0 && m_userId != other.m_userId) {
        return 0;
    }

    if (other.m_userId >= 0 && (m_userId < 0 || overwrite)) {
        m_userId = other.m_userId;
    }
    if (other.m_avatarId >= 0 && (m_avatarId < 0 || overwrite)) {
        m_avatarId = other.m_avatarId;
    }
    if (other.m_nickName.GetLength() > 0 && (m_nickName.GetLength() == 0 || overwrite)) {
        m_nickName = other.m_nickName;
    }
    if (other.m_email.GetLength() > 0 && (m_email.GetLength() == 0 || overwrite)) {
        m_email = other.m_email;
    }
    for (int network = 0; network < SOCIAL_NETWORK_COUNT; network++) {
        CStrWChar otherId = other.getIDForSocialNetwork((eSocialNetworkType)network);
        if (otherId.GetLength() > 0) {
            CStrWChar ownId = getIDForSocialNetwork((eSocialNetworkType)network);
            if (ownId.GetLength() == 0 || overwrite) {
                m_socialIds[network] = otherId;
            }
        }
        CStrWChar otherAlias = other.getAliasForSocialNetwork((eSocialNetworkType)network);
        if (otherAlias.GetLength() > 0) {
            CStrWChar ownAlias = getAliasForSocialNetwork((eSocialNetworkType)network);
            if (ownAlias.GetLength() == 0 || overwrite) {
                m_socialAliases[network] = otherAlias;
            }
        }
    }
    return 1;
}

CStrWChar CNGSUserCredentials::getSubDirectoryPath() const
{
    return getSubDirectoryPath(m_userId);
}

CStrWChar CNGSUserCredentials::getFullDirectoryPath() const
{
    return getFullDirectoryPathForSubdir(getSubDirectoryPath());
}

CObjectMapObject* CNGSUserCredentials::GetUserObject() const
{
    CObjectMapObject* object = new CObjectMapObject();
    object->addEntry(CStrWChar(NGS_ID_TOKEN), new CObjectMapInt(m_userId));
    return object;
}

CObjectMapObject* CNGSUserCredentials::GetAvatarObject() const
{
    CObjectMapObject* object = NULL;
    if (m_avatarId > 0) {
        object = new CObjectMapObject();
        object->addEntry(CStrWChar(NGS_ID_TOKEN), new CObjectMapInt(m_avatarId));
    }
    return object;
}

CStrWChar CNGSUserCredentials::getSubDirectoryPath(int userId)
{
    char number[32];
    memset(number, 0, sizeof(number));
    ICStdUtil::SPrintF(number, "%i", userId);
    CStrWChar path(L"GLUID");
    path += CStrWChar(number);
    return path;
}

CStrWChar CNGSUserCredentials::getFullDirectoryPathForSubdir(const CStrWChar& subDirectory)
{
    CStrWChar path(ICFileMgr::GetInstance()->GetApplicationDataPath());
    path += ICFileMgr::GetInstance()->GetPathSeparator();
    path += subDirectory;
    return path;
}

void CNGSUserCredentials::debugPrint() const
{
    CObjectMapObject* object = CreateObjectRepresentation();
    CStrWChar json = CNGSJSONParser::encodeValue(object);
    if (object != NULL) {
        delete object;
    }
}
