// CNGSLeaderboardManager.h - gServe leaderboards (b023x).
// Everything of the original object was dead-stripped by the linker except the weak
// CHash_gServe destructors and the static initialization of ClassId_CNGSURLMgr (which the
// stripped code read). The class below is a minimal reconstruction producing exactly those;
// its members and functions are not the original ones.
#ifndef CNGSLEADERBOARDMANAGER_H
#define CNGSLEADERBOARDMANAGER_H

#include <stddef.h>
#include "CNGSServerObject.h"
#include "CNGSURLMgr.h"

class CHash_gServe;

class CNGSLeaderboardManager : public CNGSServerObject {
public:
    CNGSLeaderboardManager();
    virtual ~CNGSLeaderboardManager();

    /** (reconstructed) leaderboard base URL of the current server environment. */
    com::glu::platform::components::CStrChar getLeaderboardBaseURL();

    CHash_gServe* m_leaderboards;   // leaderboard id -> data (reconstructed)
};

#endif
