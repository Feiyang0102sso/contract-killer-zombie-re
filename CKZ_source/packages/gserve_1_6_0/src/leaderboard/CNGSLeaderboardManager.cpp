// CNGSLeaderboardManager.cpp - gServe leaderboards.
// Minimal reconstruction: the original functions were dead-stripped by the linker (see the
// header); only the weak CHash_gServe destructors and the ClassId_CNGSURLMgr initialization
// of the original object are reproduced.
#include "CNGSLeaderboardManager.h"
#include "CHash_gServe.h"

using com::glu::platform::components::CStrChar;

CNGSLeaderboardManager::CNGSLeaderboardManager()
{
    m_leaderboards = new CHash_gServe();
    m_leaderboards->Init(10, 10, 1);
}

CNGSLeaderboardManager::~CNGSLeaderboardManager()
{
    if (m_leaderboards != NULL) {
        delete m_leaderboards;
        m_leaderboards = NULL;
    }
}

CStrChar CNGSLeaderboardManager::getLeaderboardBaseURL()
{
    void* found = NULL;
    CApplet::m_pApp->m_singletons->Find(ClassId_CNGSURLMgr, &found);
    CNGSURLMgr* urlMgr = (CNGSURLMgr*)found;
    if (urlMgr == NULL) {
        urlMgr = new CNGSURLMgr();
    }
    return urlMgr->getBaseURL(SERVER_BASE_URL_LEADERBOARD);
}
