// CNGSURLMgr.cpp - service URLs of gServe.
#include "CNGSURLMgr.h"

using com::glu::platform::components::CStrChar;

CNGSURLMgr::CNGSURLMgr()
    : CSingleton(ClassId_CNGSURLMgr)
{
    // gOffer base URL of every environment (all point to the staging server)
    m_gOfferBaseURLs[SERVER_ENV_0] = new CStrChar("http://sc-goffer-stage:8080/gOffer/");
    m_gOfferBaseURLs[SERVER_ENV_1] = new CStrChar("http://sc-goffer-stage:8080/gOffer/");
    m_gOfferBaseURLs[SERVER_ENV_2] = new CStrChar("http://sc-goffer-stage:8080/gOffer/");
    m_gOfferBaseURLs[SERVER_ENV_3] = new CStrChar("http://sc-goffer-stage:8080/gOffer/");
    m_gOfferBaseURLs[SERVER_ENV_4] = new CStrChar("http://sc-goffer-stage:8080/gOffer/");
    m_baseURLs[SERVER_BASE_URL_GOFFER][SERVER_ENV_0] = m_gOfferBaseURLs[SERVER_ENV_0]->GetCharPtr();
    m_baseURLs[SERVER_BASE_URL_GOFFER][SERVER_ENV_1] = m_gOfferBaseURLs[SERVER_ENV_1]->GetCharPtr();
    m_baseURLs[SERVER_BASE_URL_GOFFER][SERVER_ENV_2] = m_gOfferBaseURLs[SERVER_ENV_2]->GetCharPtr();
    m_baseURLs[SERVER_BASE_URL_GOFFER][SERVER_ENV_3] = m_gOfferBaseURLs[SERVER_ENV_3]->GetCharPtr();
    m_baseURLs[SERVER_BASE_URL_GOFFER][SERVER_ENV_4] = m_gOfferBaseURLs[SERVER_ENV_4]->GetCharPtr();
    m_serverEnvironment = SERVER_ENV_1;

    // leaderboard base URLs (the typo of environment 3 is in the original)
    m_baseURLs[SERVER_BASE_URL_LEADERBOARD][SERVER_ENV_0] = "http://ng-leaderboard.glu.com/ngleaderboard/rest/";
    m_baseURLs[SERVER_BASE_URL_LEADERBOARD][SERVER_ENV_1] = "http://ng-leaderboard.glu.com/ngleaderboard/rest/";
    m_baseURLs[SERVER_BASE_URL_LEADERBOARD][SERVER_ENV_2] = "http://ng-leaderboard.glu.com/ngleaderboard/rest/";
    m_baseURLs[SERVER_BASE_URL_LEADERBOARD][SERVER_ENV_3] = "http://ng-leaderboard.glu.com/rngleaderboard/est/";
    m_baseURLs[SERVER_BASE_URL_LEADERBOARD][SERVER_ENV_4] = "http://ng-leaderboard.glu.com/ngleaderboard/rest/";

    // gOffer service paths
    m_gOfferServicePaths[0] = new CStrChar("resources/offer/v1/sourcegameoffers");
    m_gOfferServicePaths[1] = new CStrChar("resources/offer/v1/offerselected");
    m_gOfferServicePaths[2] = new CStrChar("resources/offer/v1/offerpurchased");
    m_gOfferServicePaths[3] = new CStrChar("resources/offer/v1/offerrewarded");
    m_gOfferServicePaths[4] = new CStrChar("resources/offer/v1/incentive");
    m_gOfferServicePaths[5] = new CStrChar("resources/offer/v1/invitedfriendsforgame");
    m_gOfferServices[GOFFER_SOURCE_GAME_OFFERS] = m_gOfferServicePaths[0]->GetCharPtr();
    m_gOfferServices[GOFFER_OFFER_SELECTED] = m_gOfferServicePaths[1]->GetCharPtr();
    m_gOfferServices[GOFFER_OFFER_PURCHASED] = m_gOfferServicePaths[2]->GetCharPtr();
    m_gOfferServices[GOFFER_OFFER_REWARDED] = m_gOfferServicePaths[3]->GetCharPtr();
    m_gOfferServices[GOFFER_INCENTIVE] = m_gOfferServicePaths[4]->GetCharPtr();
    m_gOfferServices[GOFFER_OFFER_NETWORKS] = "resources/offer/v1/offernetworks";
    m_gOfferServices[GOFFER_SOURCE_GAME_OFFER_NETWORKS] = "resources/offer/v1/sourcegameoffernetworks";
    m_gOfferServices[GOFFER_INVITED_FRIENDS_FOR_GAME] = m_gOfferServicePaths[5]->GetCharPtr();

    // leaderboard service paths
    m_leaderboardServices[7] = "leaderboard/v1/lastscoreupdate";
    m_leaderboardServices[0] = "leaderboard/v1/getrank";
    m_leaderboardServices[1] = "leaderboard/v1/getfriendsranks";
    m_leaderboardServices[2] = "leaderboard/v1/updatescore";
    m_leaderboardServices[3] = "leaderboard/v1/replacescore";
    m_leaderboardServices[4] = "leaderboard/v1/listranks";
}

CNGSURLMgr::~CNGSURLMgr()
{
    if (m_gOfferBaseURLs[SERVER_ENV_0] != NULL) {
        delete m_gOfferBaseURLs[SERVER_ENV_0];
        m_gOfferBaseURLs[SERVER_ENV_0] = NULL;
    }
    if (m_gOfferBaseURLs[SERVER_ENV_1] != NULL) {
        delete m_gOfferBaseURLs[SERVER_ENV_1];
        m_gOfferBaseURLs[SERVER_ENV_1] = NULL;
    }
    if (m_gOfferBaseURLs[SERVER_ENV_2] != NULL) {
        delete m_gOfferBaseURLs[SERVER_ENV_2];
        m_gOfferBaseURLs[SERVER_ENV_2] = NULL;
    }
    if (m_gOfferBaseURLs[SERVER_ENV_3] != NULL) {
        delete m_gOfferBaseURLs[SERVER_ENV_3];
        m_gOfferBaseURLs[SERVER_ENV_3] = NULL;
    }
    if (m_gOfferBaseURLs[SERVER_ENV_4] != NULL) {
        delete m_gOfferBaseURLs[SERVER_ENV_4];
        m_gOfferBaseURLs[SERVER_ENV_4] = NULL;
    }
    if (m_gOfferServicePaths[0] != NULL) {
        delete m_gOfferServicePaths[0];
        m_gOfferServicePaths[0] = NULL;
    }
    if (m_gOfferServicePaths[1] != NULL) {
        delete m_gOfferServicePaths[1];
        m_gOfferServicePaths[1] = NULL;
    }
    if (m_gOfferServicePaths[2] != NULL) {
        delete m_gOfferServicePaths[2];
        m_gOfferServicePaths[2] = NULL;
    }
    if (m_gOfferServicePaths[3] != NULL) {
        delete m_gOfferServicePaths[3];
        m_gOfferServicePaths[3] = NULL;
    }
    if (m_gOfferServicePaths[4] != NULL) {
        delete m_gOfferServicePaths[4];
        m_gOfferServicePaths[4] = NULL;
    }
    if (m_gOfferServicePaths[5] != NULL) {
        delete m_gOfferServicePaths[5];
        m_gOfferServicePaths[5] = NULL;
    }
}

void CNGSURLMgr::setServerEnvironment(eServerEnvType environment)
{
    m_serverEnvironment = environment;
}

CStrChar CNGSURLMgr::getBaseURL(eServerBaseURLType type)
{
    CStrChar url;
    const char* baseURL = m_baseURLs[type][m_serverEnvironment];
    if (baseURL != NULL) {
        url = baseURL;
    }
    return url;
}

CStrChar CNGSURLMgr::getGOfferURL(eGOfferServiceType service)
{
    CStrChar url = getBaseURL(SERVER_BASE_URL_GOFFER);
    url += m_gOfferServices[service];
    return url;
}
