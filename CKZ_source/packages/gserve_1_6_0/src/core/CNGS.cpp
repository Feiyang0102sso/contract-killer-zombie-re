// CNGS.cpp - gServe root singleton.
#include <string.h>
#include "CNGS.h"
#include "CNGSFactory.h"
#include "CNGSServerRequest.h"
#include "CNGSSession.h"
#include "CNGSUserCredentials.h"
#include "CNGSLocalUser.h"
#include "CNGSFromServerMessageQ.h"
#include "CNGSContentManager.h"
#include "CNGSLoginFlow.h"
#include "CNGSLockManager.h"
#include "CProfileManager.h"
#include "CNGS_Platform.h"
#include "CNGSSocialInterface.h"
#include "CNotificationHandler.h"
#include "CNGSOfferManager.h"

using com::glu::platform::components::CStrWChar;

/** Default period of the automatic server message poll (ms). */
static const unsigned int DEFAULT_AUTO_POLL_INTERVAL_MS = 120000;

/** Time since the last automatic message poll (ms). */
static unsigned int s_autoPollTimerMs = 0;

CNGS::CNGS()
    : CSingleton(ClassId)
    , m_factory(NULL)
    , m_autoPollIntervalMs(DEFAULT_AUTO_POLL_INTERVAL_MS)
{
    m_networkState = NETWORK_CONNECTED;
    m_serverURL = new char[1];
    m_serverURL[0] = '\0';
    m_localUser = NULL;
    m_profileManager = NULL;
    m_version = L"-1.-1.-1";
}

CNGS::~CNGS()
{
    destroy();
}

void CNGS::destroy()
{
    CNGSServerRequest::DeleteAll();
    if (m_serverURL != NULL) {
        delete[] m_serverURL;
        m_serverURL = NULL;
    }
    if (m_factory != NULL) {
        delete m_factory;
        m_factory = NULL;
    }
    if (m_localUser != NULL) {
        delete m_localUser;
        m_localUser = NULL;
    }
    if (m_profileManager != NULL) {
        delete m_profileManager;
        m_profileManager = NULL;
    }
}

void CNGS::setVersion(CStrWChar& major, CStrWChar& minor, CStrWChar& build)
{
    m_version = major + L"." + minor + L"." + build;
}

CStrWChar CNGS::getVersionString()
{
    CStrWChar version;
    version += m_version;
    return version;
}

void CNGS::SetServerURL(const char* url)
{
    if (m_serverURL != NULL) {
        delete[] m_serverURL;
        m_serverURL = NULL;
    }
    int length = strlen(url);
    m_serverURL = new char[length + 2];
    strcpy(m_serverURL, url);
}

char* CNGS::GetServerURL()
{
    return m_serverURL;
}

CNGSLocalUser* CNGS::GetLocalUser()
{
    if (m_localUser == NULL) {
        CNGSFactory* factory = CNGS::GetInstance()->m_factory;
        if (factory != NULL) {
            CNGSUserCredentials credentials;
            CStrWChar fileName(CREDENTIAL_FILE_NAME);
            credentials.readFromFile(fileName, false);
            m_localUser = factory->createLocalUser(credentials);
        }
    }
    return m_localUser;
}

CNGSFactory* CNGS::GetFactory()
{
    return m_factory;
}

CProfileManager* CNGS::GetProfileManager()
{
    if (m_profileManager == NULL) {
        m_profileManager = new CProfileManager(NULL);
    }
    return m_profileManager;
}

void CNGS::SetFactory(CNGSFactory* factory)
{
    m_factory = factory;
}

void CNGS::SetNetworkConnectionState(eNetworkConnectionState state, const char* reason)
{
    eNetworkConnectionState previousState = m_networkState;
    m_networkState = state;
    CNGSLocalUser* user = CNGS::GetInstance()->GetLocalUser();
    if (previousState == NETWORK_CONNECTED) {
        if (m_networkState != NETWORK_CONNECTED) {
            user->CNGSHandleServerConnectionError(reason);
        }
    } else {
        if (m_networkState == NETWORK_CONNECTION_ERROR) {
            user->CNGSHandleServerConnectionError(reason);
        }
        if (m_networkState == NETWORK_CONNECTED) {
            user->CNGSHandleServerConnectionRestore(previousState);
        }
    }
}

eNetworkConnectionState CNGS::GetNetworkConnectionState()
{
    return m_networkState;
}

void CNGS::HandleUpdate(int elapsedMs)
{
    CNGSServerRequest::HandleUpdate(elapsedMs);
    CNGSContentManager::GetInstance()->HandleUpdate(elapsedMs);

    CNGSLocalUser* user = GetLocalUser();
    if (user == NULL) {
        return;
    }
    if (user->m_session != NULL) {
        user->m_session->tick(elapsedMs);
    }
    if (!user->isAuthenticated(0)) {
        return;
    }
    if (user->m_session->isSessionKeyValid() != true) {
        return;
    }
    if (CNGSLoginFlow::GetInstance()->IsRunning()) {
        return;
    }
    // Poll the server for messages every m_autoPollIntervalMs, acknowledge the handled ones otherwise.
    if (m_autoPollIntervalMs != 0 && s_autoPollTimerMs > m_autoPollIntervalMs && user->isReady()) {
        if (user->m_messageQueue->SyncAllFromServer(NULL)) {
            s_autoPollTimerMs = 0;
            user->m_notificationHandler->HandleAutoPollStart();
        }
    } else {
        s_autoPollTimerMs += elapsedMs;
        user->m_messageQueue->AckMessages();
    }
}

void CNGS::Pause()
{
    CNGSLocalUser* user = GetLocalUser();
    if (user != NULL && user->isValid()) {
        if (user->m_session != NULL) {
            user->m_session->clearNetworkTime();
        }
    }
    CNGSLockManager::GetInstance()->suspend();
}

void CNGS::Resume()
{
    CNGSLocalUser* user = GetLocalUser();
    if (user != NULL && user->isValid()) {
        CNGSSession* session = user->m_session;
        if (session != NULL) {
            session->clearNetworkTime();
            session->SyncAllFromServer(NULL);
        }
    }
    if (user != NULL) {
        CNGS_Platform::GetInstance()->getSocialInterface(SOCIAL_INTERFACE_FACEBOOK)->resume();
        user->m_messageQueue->ProcessPromptMessages();
    }
    CNGSLockManager::GetInstance()->resume();
}

/*
 * Not present in the shipped binary (removed by the linker's dead-strip), but the original CNGS.o had a
 * function reading ClassId_COfferManager: its static-init store survives and merges with
 * s_autoPollTimerMs into __MergedGlobals (b028x experiment). The name is a guess.
 */
COfferManager* CNGS::GetOfferManager()
{
    return COfferManager::GetInstance();
}
