// CNGSHeader.cpp - device / game / user description sent as the "header" of gServe messages.
// The constructor collects everything once (platform core, push token, bundle, local user);
// the create*Object() methods turn it into CObjectMapObject trees owned by the caller.
#include "CNGSHeader.h"
#include "CNGS.h"
#include "CNGSLocalUser.h"
#include "CObjectMap.h"
#include "CObjectMapObject.h"
#include "CObjectMapValue.h"
#include "CStrChar.h"
#include "ICCore.h"
#include "CPushNotification_Cocoa.h"
#include "CPlatformUtil.h"
#include "BundleInterface.h"

using com::glu::platform::components::CStrChar;
using com::glu::platform::components::CStrWChar;

CNGSHeader::CNGSHeader()
{
    m_carrier = L"";
    m_udid = L"";
    m_platformName = L"";
    m_platformVersion = L"";
    m_make = L"";
    m_model = L"";
    m_pushToken = L"";
    m_language = L"";
    m_country = L"";
    m_demo = L"";
    m_gameId = -1;
    m_gameVersion = L"";
    m_bundleIdentifier = L"";
    m_bundleVersion = L"";
    m_userId = -1;
    m_avatarId = -1;

    loadDeviceInfo();
    loadGameInfo();
    loadUserInfo();
}

/** Device identity from the platform core, the push token and the language / locale. */
void CNGSHeader::loadDeviceInfo()
{
    m_udid = CApplet::m_pCore->GetDeviceIdentifier();
    m_platformName = CApplet::m_pCore->GetPlatformName();
    m_platformVersion = CApplet::m_pCore->GetPlatformVersion();
    m_make = CPlatformUtil::getDeviceName();
    m_model = CApplet::m_pCore->GetPlatformIdentifier();

    CPushNotification_Cocoa* pushNotification = CPushNotification_Cocoa::GetInstance();
    if (pushNotification != NULL && pushNotification->IsEnabled()) {
        pushNotification->GetIdentity(m_pushToken);
    }

    CStrChar text;
    CApplet::m_pCore->DetectLanguage(text);
    m_language = text.GetCharPtr();
    CApplet::m_pCore->DetectLocale(text);
    m_country = text.GetCharPtr();
}

/** Game id and version from CNGS, sku and bundle version from the main bundle. */
void CNGSHeader::loadGameInfo()
{
    m_gameId = CNGS::GetInstance()->m_gameId;
    m_gameVersion = CNGS::GetInstance()->getVersionString();
    BundleInterface::BundleIdentifierString(m_bundleIdentifier);
    BundleInterface::BundleVersionString(m_bundleVersion);
}

/** Ids of the local user. */
void CNGSHeader::loadUserInfo()
{
    CNGSLocalUser* localUser = CNGS::GetInstance()->GetLocalUser();
    m_userId = localUser->m_credentials.m_userId;
    m_avatarId = localUser->m_credentials.m_avatarId;
}

CObjectMapObject* CNGSHeader::createObjectRepresentation(unsigned char includeUser) const
{
    CObjectMapObject* header = new CObjectMapObject();

    CObjectMapValue* device = createDeviceObject();
    header->addEntry(CStrWChar(L"device"), device);

    CObjectMapValue* game = createGameObject();
    header->addEntry(CStrWChar(L"game"), game);

    if (includeUser) {
        CObjectMapValue* user = createUserObject();
        header->addEntry(CStrWChar(L"user"), user);
    }
    return header;
}

CObjectMapObject* CNGSHeader::createDeviceObject() const
{
    CObjectMapObject* device = new CObjectMapObject();
    device->addEntry(CStrWChar(L"carrier"), new CObjectMapString(m_carrier));
    device->addEntry(CStrWChar(L"country"), new CObjectMapString(m_country));
    device->addEntry(CStrWChar(L"language"), new CObjectMapString(m_language));
    device->addEntry(CStrWChar(L"make"), new CObjectMapString(m_make));
    device->addEntry(CStrWChar(L"model"), new CObjectMapString(m_model));
    device->addEntry(CStrWChar(L"platformName"), new CObjectMapString(m_platformName));
    device->addEntry(CStrWChar(L"platformVersion"), new CObjectMapString(m_platformVersion));
    device->addEntry(CStrWChar(L"pushToken"), new CObjectMapString(m_pushToken));
    device->addEntry(CStrWChar(L"udid"), new CObjectMapString(m_udid));
    return device;
}

CObjectMapObject* CNGSHeader::createGameObject() const
{
    CObjectMapObject* game = new CObjectMapObject();
    game->addEntry(CStrWChar(L"id"), new CObjectMapInt(m_gameId));
    game->addEntry(CStrWChar(L"sku"), new CObjectMapString(m_bundleIdentifier));
    game->addEntry(CStrWChar(L"version"), new CObjectMapString(m_gameVersion));
    return game;
}

CObjectMapObject* CNGSHeader::createUserObject() const
{
    CObjectMapObject* user = new CObjectMapObject();
    user->addEntry(CStrWChar(L"id"), new CObjectMapInt(m_userId));
    user->addEntry(CStrWChar(L"avatarId"), new CObjectMapInt(m_avatarId));
    return user;
}

/** `messageType` is not used (the legacy header carries no type). */
CObjectMapObject* CNGSHeader::createObjectRepresentationLegacy(const CStrWChar& messageType) const
{
    CObjectMapObject* header = new CObjectMapObject();

    CObjectMapValue* device = createDeviceObjectLegacy();
    CObjectMapValue* game = createGameObjectLegacy();
    header->addEntry(CStrWChar(L"device"), device);
    header->addEntry(CStrWChar(L"game"), game);
    return header;
}

CObjectMapObject* CNGSHeader::createDeviceObjectLegacy() const
{
    CObjectMapObject* device = new CObjectMapObject();
    device->addEntry(CStrWChar(L"carrier"), new CObjectMapString(m_carrier));
    device->addEntry(CStrWChar(L"id"), new CObjectMapString(m_udid));
    device->addEntry(CStrWChar(L"platformCode"), new CObjectMapString(m_platformName));
    device->addEntry(CStrWChar(L"deviceName"), new CObjectMapString(m_make));
    device->addEntry(CStrWChar(L"hardwareVersion"), new CObjectMapString(m_model));
    device->addEntry(CStrWChar(L"platformVersion"), new CObjectMapString(m_platformVersion));
    device->addEntry(CStrWChar(L"devicetoken"), new CObjectMapString(m_pushToken));
    device->addEntry(CStrWChar(L"locale"), new CObjectMapString(m_language));
    device->addEntry(CStrWChar(L"countryCode"), new CObjectMapString(m_country));
    return device;
}

CObjectMapObject* CNGSHeader::createGameObjectLegacy() const
{
    CObjectMapObject* game = new CObjectMapObject();
    game->addEntry(CStrWChar(L"demo"), new CObjectMapString(m_demo));
    game->addEntry(CStrWChar(L"id"), new CObjectMapInt(m_gameId));
    game->addEntry(CStrWChar(L"version"), new CObjectMapString(m_gameVersion));
    game->addEntry(CStrWChar(L"bundleIdentifier"), new CObjectMapString(m_bundleIdentifier));
    game->addEntry(CStrWChar(L"bundleVersion"), new CObjectMapString(m_bundleVersion));
    return game;
}

/**
 * Looks at "content"/"success" of the reply. On failure with both "errorcode" and "message"
 * present, " <message>" is appended to `errorMessage`.
 */
unsigned char CNGSHeader::checkResponseOk(CObjectMap* response, CStrWChar& errorMessage)
{
    unsigned char ok = 0;
    if (response != NULL) {
        CObjectMapObject* content = (CObjectMapObject*)response->getDataAt(CStrWChar(L"content"), CStrWChar(L" "));
        if (content != NULL) {
            // "success" is assumed present (no NULL check in the original).
            CObjectMapBool* successValue = (CObjectMapBool*)content->getEntry(CStrWChar(L"success"));
            bool success = successValue->m_value != 0;
            ok = 1;
            if (!success) {
                CObjectMapValue* errorCode = content->getEntry(CStrWChar(L"errorcode"));
                CObjectMapString* message = (CObjectMapString*)content->getEntry(CStrWChar(L"message"));
                ok = 0;
                if (errorCode != NULL && message != NULL) {
                    CStrWChar text(message->m_value);
                    errorMessage += L" ";
                    errorMessage += text;
                    ok = success;   // still 0 here; the original returns the flag itself
                }
            }
        }
    }
    return ok;
}
