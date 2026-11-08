// CNGS.h - gServe root singleton: server URL, game version, network connection state and the
// local user; pumps the request queue and the content manager every frame.
#ifndef CNGS_H
#define CNGS_H

#include <stddef.h>
#include "CSingleton.h"
#include "CStrWChar.h"
#include "CNGSTokens.h"

class CNGSFactory;
class CNGSLocalUser;
class CProfileManager;
class COfferManager;

/** Connection state reported by the request layer. */
enum eNetworkConnectionState {
    NETWORK_CONNECTED = 0,
    NETWORK_CONNECTION_ERROR = 1,
    NETWORK_SERVER_ERROR = 2,          // generic server error reply
    NETWORK_SESSION_INVALID = 3,       // error 1009
    NETWORK_VERSION_INVALID = 4
};

class CNGS : public com::glu::platform::components::CSingleton {
public:
    /** Singleton key in CApp::m_singletons. */
    enum { ClassId = 0x7a23 };

    CNGS();
    virtual ~CNGS();

    /** The registered instance, created on first use. */
    static inline CNGS* GetInstance()
    {
        void* found = NULL;
        CApplet::m_pApp->m_singletons->Find(ClassId, &found);
        CNGS* instance = (CNGS*)found;
        if (instance == NULL) {
            instance = new CNGS();
        }
        return instance;
    }

    /** Drop the outstanding requests and everything owned. */
    void destroy();
    /** Game version "major.minor.build". */
    void setVersion(com::glu::platform::components::CStrWChar& major,
                    com::glu::platform::components::CStrWChar& minor,
                    com::glu::platform::components::CStrWChar& build);
    com::glu::platform::components::CStrWChar getVersionString();
    void SetServerURL(const char* url);
    char* GetServerURL();
    /** Created on first use by the factory from the saved credentials. */
    CNGSLocalUser* GetLocalUser();
    CNGSFactory* GetFactory();
    CProfileManager* GetProfileManager();
    /** Offer manager singleton. Dead-stripped in the shipped binary (see CNGS.cpp). */
    COfferManager* GetOfferManager();
    void SetFactory(CNGSFactory* factory);
    /** Store `state` and tell the local user about lost / restored connections. */
    void SetNetworkConnectionState(eNetworkConnectionState state, const char* reason);
    eNetworkConnectionState GetNetworkConnectionState();
    void HandleUpdate(int elapsedMs);
    void Pause();
    void Resume();

    eNetworkConnectionState m_networkState;                 // +0x08
    CNGSFactory* m_factory;                                 // +0x0c owned
    int m_gameId;                                           // +0x10 set by the game (not initialized here)
    char* m_serverURL;                                      // +0x14 owned
    CNGSLocalUser* m_localUser;                             // +0x18 owned
    CProfileManager* m_profileManager;                      // +0x1c owned
    unsigned int m_autoPollIntervalMs;                      // +0x20 message auto-poll period (0 = off)
    com::glu::platform::components::CStrWChar m_version;    // +0x24
};

#endif
