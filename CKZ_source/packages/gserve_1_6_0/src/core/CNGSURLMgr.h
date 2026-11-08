// CNGSURLMgr.h - base URLs of the gServe side services (gOffer, leaderboard) per server
// environment, and the service paths appended to them.
#ifndef CNGSURLMGR_H
#define CNGSURLMGR_H

#include "CSingleton.h"
#include "CStrChar.h"
#include "CStringToKey.h"

/** Singleton key, computed at static initialization in every unit that includes this header. */
static unsigned int ClassId_CNGSURLMgr = com::glu::platform::core::CStringToKey("CNGSURLMGR", 0);

/** Server environments (columns of the base URL table). */
enum eServerEnvType {
    SERVER_ENV_0 = 0,
    SERVER_ENV_1 = 1,
    SERVER_ENV_2 = 2,
    SERVER_ENV_3 = 3,
    SERVER_ENV_4 = 4,
    SERVER_ENV_COUNT = 5
};

/** Services with a base URL (rows of the base URL table). */
enum eServerBaseURLType {
    SERVER_BASE_URL_GOFFER = 0,
    SERVER_BASE_URL_1 = 1,
    SERVER_BASE_URL_LEADERBOARD = 2,
    SERVER_BASE_URL_COUNT = 3
};

/** gOffer service paths. */
enum eGOfferServiceType {
    GOFFER_SOURCE_GAME_OFFERS = 0,
    GOFFER_OFFER_SELECTED = 1,
    GOFFER_OFFER_PURCHASED = 2,
    GOFFER_OFFER_REWARDED = 3,
    GOFFER_INCENTIVE = 4,
    GOFFER_OFFER_NETWORKS = 5,
    GOFFER_SOURCE_GAME_OFFER_NETWORKS = 6,
    GOFFER_INVITED_FRIENDS_FOR_GAME = 7,
    GOFFER_SERVICE_COUNT = 8
};

class CNGSURLMgr : public com::glu::platform::components::CSingleton {
public:
    CNGSURLMgr();
    virtual ~CNGSURLMgr();

    /** The registered instance, created on first use (inlined into CNGSOfferManager.o, N_SOL CNGSURLMgr.h). */  // added by b028x
    static inline CNGSURLMgr* GetInstance()  // added by b028x
    {  // added by b028x
        unsigned int classId = ClassId_CNGSURLMgr;  // added by b028x
        void* found = NULL;  // added by b028x
        CApplet::m_pApp->m_singletons->Find(classId, &found);  // added by b028x
        CNGSURLMgr* instance = (CNGSURLMgr*)found;  // added by b028x
        if (instance == NULL) {  // added by b028x
            instance = new CNGSURLMgr();  // added by b028x
        }  // added by b028x
        return instance;  // added by b028x
    }  // added by b028x

    void setServerEnvironment(eServerEnvType environment);
    /** Base URL of `type` for the current environment (empty when not configured). */
    com::glu::platform::components::CStrChar getBaseURL(eServerBaseURLType type);
    /** gOffer base URL + the path of `service`. */
    com::glu::platform::components::CStrChar getGOfferURL(eGOfferServiceType service);

    void* m_reserved08[5];                                                  // +0x08 not used by the core
    com::glu::platform::components::CStrChar* m_gOfferBaseURLs[SERVER_ENV_COUNT];      // +0x1c owned
    com::glu::platform::components::CStrChar* m_gOfferServicePaths[6];                 // +0x30 owned
    const char* m_baseURLs[SERVER_BASE_URL_COUNT][SERVER_ENV_COUNT];       // +0x48
    const char* m_gOfferServices[GOFFER_SERVICE_COUNT];                     // +0x84
    const char* m_leaderboardServices[8];                                   // +0xa4
    eServerEnvType m_serverEnvironment;                                     // +0xc4
};

#endif
