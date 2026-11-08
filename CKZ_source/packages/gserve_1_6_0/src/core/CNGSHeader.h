// CNGSHeader.h - device / game / user description sent as the "header" of every gServe message.
// Collected once at construction from the platform core, the bundle and the local user.
#ifndef CNGSHEADER_H
#define CNGSHEADER_H

#include "CStrWChar.h"

class CObjectMap;
class CObjectMapObject;

class CNGSHeader {
public:
    CNGSHeader();

    void loadDeviceInfo();
    void loadGameInfo();
    void loadUserInfo();

    /** {"device":..., "game":..., "user":...} ("user" only with `includeUser`); the caller owns it. */
    CObjectMapObject* createObjectRepresentation(unsigned char includeUser) const;
    CObjectMapObject* createDeviceObject() const;
    CObjectMapObject* createGameObject() const;
    CObjectMapObject* createUserObject() const;
    /** Pre-1.6 header layout. */
    CObjectMapObject* createObjectRepresentationLegacy(const com::glu::platform::components::CStrWChar& messageType) const;
    CObjectMapObject* createDeviceObjectLegacy() const;
    CObjectMapObject* createGameObjectLegacy() const;
    /** True when `response` reports success; otherwise its error message goes to `errorMessage`. */
    static unsigned char checkResponseOk(CObjectMap* response, com::glu::platform::components::CStrWChar& errorMessage);

    com::glu::platform::components::CStrWChar m_carrier;            // +0x00
    com::glu::platform::components::CStrWChar m_country;            // +0x10 ICCore::DetectLocale
    com::glu::platform::components::CStrWChar m_language;           // +0x20 ICCore::DetectLanguage
    com::glu::platform::components::CStrWChar m_platformName;       // +0x30
    com::glu::platform::components::CStrWChar m_platformVersion;    // +0x40
    com::glu::platform::components::CStrWChar m_make;               // +0x50 device name
    com::glu::platform::components::CStrWChar m_model;              // +0x60 platform identifier
    com::glu::platform::components::CStrWChar m_pushToken;          // +0x70
    com::glu::platform::components::CStrWChar m_udid;               // +0x80
    com::glu::platform::components::CStrWChar m_demo;               // +0x90
    int m_gameId;                                                   // +0xa0
    com::glu::platform::components::CStrWChar m_gameVersion;        // +0xa4
    com::glu::platform::components::CStrWChar m_bundleIdentifier;   // +0xb4 sent as "sku"
    com::glu::platform::components::CStrWChar m_bundleVersion;      // +0xc4
    int m_userId;                                                   // +0xd4
    int m_avatarId;                                                 // +0xd8
};

#endif
