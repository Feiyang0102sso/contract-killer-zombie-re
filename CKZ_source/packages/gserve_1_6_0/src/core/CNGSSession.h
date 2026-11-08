// CNGSSession.h - the local user's gServe session: the persisted session id ("Session.dat")
// plus the server clock.
#ifndef CNGSSESSION_H
#define CNGSSESSION_H

#include "CNGSServerObject.h"
#include "CNGSSessionConfig.h"
#include "CNGSTime.h"

class CNGSSession : public CNGSServerObject {
public:
    /** Loads the session id saved in "Session.dat"; valid when one was found. */
    CNGSSession();
    virtual ~CNGSSession() {}

    /** Take over `config`; returns 0. */
    int init(const CNGSSessionConfig& config);
    bool doesSessionMatch(const CNGSSessionConfig& config);
    void clearNetworkTime();
    unsigned char isValidNetworkTime();
    void tick(unsigned int elapsedMs);
    /** Drop the session id and save the empty session. */
    void invalidate();
    bool isSessionKeyValid();
    unsigned char doServerVersionCheck();
    unsigned long long getNetworkCurrentTimeMS();
    unsigned long long getNetworkCurrentTimeSeconds();
    virtual unsigned char SyncAllFromServer(CNGSNotifyFunctor* notify);

    CNGSSessionConfig m_config;     // +0x18
    CNGSTime m_time;                // +0x28
};

#endif
