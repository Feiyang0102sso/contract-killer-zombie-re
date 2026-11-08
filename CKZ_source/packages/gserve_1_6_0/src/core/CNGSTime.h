// CNGSTime.h - server clock: asks the server for its time ("getnettime2") and keeps it running
// locally with the frame time. The same request doubles as the server version check.
#ifndef CNGSTIME_H
#define CNGSTIME_H

#include <stddef.h>
#include "CNGSServerObject.h"
#include "CNGSServerRequest.h"

class CNGSTimeFunctor;

class CNGSTime : public CNGSServerObject {
public:
    CNGSTime();

    virtual ~CNGSTime() {}

    /** Forget the server time. */
    void clearNetworkTime();
    unsigned char isValidNetworkTime();
    /** Advance the clock by `elapsedMs` (only once the server time is known). */
    void tick(unsigned int elapsedMs);
    /** Ask the server for its time; 1 when the request went out. */
    unsigned char resetNetworkTime();
    /** Reply of "getnettime2" (NULL when the request failed). */
    void handleResponseNetworkTime(CObjectMap* response, CNGSTimeFunctor* functor);
    unsigned char doServerVersionCheck();
    unsigned long long getNetworkCurrentTimeMS();
    unsigned long long getNetworkCurrentTimeSeconds();
    unsigned char SyncAllFromServer(CNGSNotifyFunctor* notify);

    unsigned long long m_serverTimeMs;     // +0x18 time sent by the server
    unsigned long long m_currentTimeMs;     // +0x20 server time advanced by tick(); 0 = unknown
};

/** Routes a "getnettime2" reply to a CNGSTime member function. */
class CNGSTimeFunctor : public CNGSServerRequestFunctor {
public:
    typedef void (CNGSTime::*Callback)(CObjectMap* response, CNGSTimeFunctor* functor);

    CNGSTimeFunctor(CNGSTime* target, Callback callback)
        : m_target(target)
        , m_callback(callback)
        , m_context(NULL)
    {
    }

    virtual ~CNGSTimeFunctor() {}

    virtual void requestCompleteCallback(CObjectMap* response)
    {
        // plain pointer-to-member test (clang compares it with != NULL differently)
        if (m_callback) {
            (m_target->*m_callback)(response, this);
        }
    }

    CNGSTime* m_target;         // +0x08
    Callback m_callback;        // +0x0c
    void* m_context;            // +0x14 NULL (read as a user pointer by handleResponseNetworkTime)
};

#endif
