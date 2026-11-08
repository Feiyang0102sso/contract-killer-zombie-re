// CNGSServerRequest.h - one request to the gServe server: queues its message in the
// CNetMessageQueue_gServe, waits for the reply in the static request table and reports the
// outcome (reply, timeout, cancel) to its functor.
#ifndef CNGSSERVERREQUEST_H
#define CNGSSERVERREQUEST_H

#include <stddef.h>
#include "CLinkList.h"
#include "CNetMessageServer.h"

class CObjectMap;

/** Receiver of a request outcome; owned (and deleted) by the request. */
class CNGSServerRequestFunctor {
public:
    /** m_status values set before the callbacks. */
    enum {
        STATUS_CANCELLED = -1,
        STATUS_TIMEOUT = -2
    };

    CNGSServerRequestFunctor()
        : m_status(0)
    {
    }

    virtual ~CNGSServerRequestFunctor() {}

    /** The reply arrived (NULL after a timeout or a cancel by default). */
    virtual void requestCompleteCallback(CObjectMap* response) = 0;
    /** Default: requestCompleteCallback(NULL). */
    virtual void requestTimeoutCallback(CObjectMap* response);
    /** Default: requestCompleteCallback(NULL). */
    virtual void requestCancelledCallback(CObjectMap* response);

    int m_status;       // +0x04 0, STATUS_CANCELLED or STATUS_TIMEOUT
};

class CNGSServerRequest {
public:
    /** The request takes ownership of `message` and `functor`; it is sent now unless `delaySend`. */
    CNGSServerRequest(const char* url, CObjectMap* message, CNGSServerRequestFunctor* functor,
                      unsigned char delaySend, unsigned char cancelOnDelete);
    virtual ~CNGSServerRequest();

    /** Queue the message (once) and register the request; returns the message id (0 when already sent). */
    unsigned int Send();
    /** Cancel the queued message; 1 when it was still queued (the functor is told). */
    unsigned char CancelMessage();
    void OnCommandCancelled();
    void OnCommandTimeout();
    void OnCommandCompleted(CObjectMap* response);

    /** Pump the queue, deliver the replies and time the oldest outstanding request out. */
    static void HandleUpdate(int elapsedMs);
    /** Unlink the request waiting for `messageId`; 1 when found. */
    static unsigned char RemoveMessageIDFromQueuedList(unsigned int messageId);
    static unsigned char AreNetworkMessagesWaitingForProcessing();
    /** Delete every outstanding request. */
    static void DeleteAll();
    /** Delete every outstanding request without notifying the functors. */
    static void ClearAllMessages();

    static com::glu::platform::core::CLinkList ms_ServerRequestsTable;     // outstanding requests (node data = request)
    static long long ms_timer;                                             // ms since the last reply
    static unsigned int ms_mostRecentMessage;                              // id of the last queued message

    unsigned int m_messageId;                   // +0x04 id given by the queue
    CObjectMap* m_message;                      // +0x08 owned
    CNGSServerRequestFunctor* m_functor;        // +0x0c owned
    unsigned char m_cancelOnDelete;             // +0x10
    unsigned char m_sent;                       // +0x11
    CNetMessageServer m_server;                 // +0x14
    long long m_reserved30;                     // +0x30 never touched in the core (sizeof is 0x38)
};

/** CLinkList::Find callback: the node's request waits for message id `messageId`. */
unsigned char isServerRequestWeWant(com::glu::platform::core::CLinkListNode* node, void* messageId);

#endif
