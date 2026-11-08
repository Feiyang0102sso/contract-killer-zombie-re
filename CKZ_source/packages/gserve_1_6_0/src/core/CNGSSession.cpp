// CNGSSession.cpp - gServe session of the local user.
#include "CNGSSession.h"

using com::glu::platform::components::CStrWChar;

CNGSSession::CNGSSession()
{
    m_time = CNGSTime();
    CNGSSessionConfig savedConfig;
    CStrWChar fileName(L"Session.dat");
    CStrWChar key(L"xxx");
    unsigned char loaded = savedConfig.readFromFile(fileName, key, false);
    m_config = savedConfig;
    SetIsValid(loaded);
}

int CNGSSession::init(const CNGSSessionConfig& config)
{
    m_config = config;
    return 0;
}

bool CNGSSession::doesSessionMatch(const CNGSSessionConfig& config)
{
    return m_config == config;
}

void CNGSSession::clearNetworkTime()
{
    m_time.clearNetworkTime();
}

unsigned char CNGSSession::isValidNetworkTime()
{
    return m_time.isValidNetworkTime();
}

void CNGSSession::tick(unsigned int elapsedMs)
{
    m_time.tick(elapsedMs);
}

void CNGSSession::invalidate()
{
    CStrWChar fileName(L"Session.dat");
    CStrWChar key(L"xxx");
    SetIsValid(0);
    m_config.reset();
    m_config.writeToFile(fileName, key, false);
}

bool CNGSSession::isSessionKeyValid()
{
    return m_config.m_sessionId.GetLength() != 0;
}

unsigned char CNGSSession::doServerVersionCheck()
{
    return m_time.doServerVersionCheck();
}

unsigned long long CNGSSession::getNetworkCurrentTimeMS()
{
    return m_time.getNetworkCurrentTimeMS();
}

unsigned long long CNGSSession::getNetworkCurrentTimeSeconds()
{
    return m_time.getNetworkCurrentTimeSeconds();
}

unsigned char CNGSSession::SyncAllFromServer(CNGSNotifyFunctor* notify)
{
    // The clock does the server round trip; the session only keeps the notify functor.
    CNGSServerObject* time = &m_time;
    if (!m_time.isReady()) {
        return 0;
    }
    if (m_time.m_writeRequestsOutstanding != 0) {
        return 0;
    }
    m_readNotify = notify;
    return time->SyncAllFromServer(notify);
}
