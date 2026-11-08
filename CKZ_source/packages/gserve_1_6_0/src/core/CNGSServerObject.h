// CNGSServerObject.h - base of every gServe object that is synchronized with the server.
// It counts outstanding read/write requests and fires the pending notify functor when a count
// drops to zero; it also wraps a message with the standard header and sends it.
#ifndef CNGSSERVEROBJECT_H
#define CNGSSERVEROBJECT_H

#include <stddef.h>
#include "CNGSFunctor.h"

class CObjectMap;
class CObjectMapObject;
class CNGSServerRequestFunctor;

class CNGSServerObject {
public:
    CNGSServerObject()
        : m_isValid(0)
        , m_readRequestsOutstanding(0)
        , m_writeRequestsOutstanding(0)
        , m_readNotify(NULL)
        , m_writeNotify(NULL)
    {
    }

    virtual ~CNGSServerObject();

    /** Fetch everything from the server; 1 when a request went out. */
    virtual unsigned char SyncAllFromServer(CNGSNotifyFunctor* notify)
    {
        return 0;
    }

    virtual void NotifyOfChildReadCompletion(CNGSNotifyFunctor* notify)
    {
        CompleteReadRequestOutstanding();
    }

    /** Push everything to the server. */
    virtual void SyncAllToServer(CNGSNotifyFunctor* notify)
    {
    }

    virtual void NotifyOfChildWriteCompletion(CNGSNotifyFunctor* notify)
    {
        CompleteWriteRequestOutstanding();
    }

    virtual void SetIsValid(unsigned char isValid);

    void AddReadRequestOutstanding();
    /** One read finished; the read notify functor fires (and is deleted) when none is left. */
    void CompleteReadRequestOutstanding();
    /** No read request outstanding. */
    unsigned char isReady();
    void AddWriteRequestOutstanding();
    /** One write finished; the write notify functor fires when no read or write is left. */
    void CompleteWriteRequestOutstanding();
    unsigned char isValid();

    /** Send `message` (deleted here) to the server path `service`; returns the message id (0 on failure). */
    unsigned int SendMessageObjectToServer(CObjectMapObject* message, const char* service, CNGSServerRequestFunctor* functor);
    unsigned int SendMessageToServer(CObjectMap* message, const char* service, CNGSServerRequestFunctor* functor);
    /**
     * Inspect a server reply; returns 0 when it is a success, otherwise the error code
     * (-1 for a malformed reply, -2 when there was no reply) after notifying the local user.
     */
    static int WasErrorInResponse(CObjectMap* response, CNGSServerRequestFunctor* functor, const char* caller);
    /** "errorcode" of the reply, 0 when absent, -2 when there was no reply. */
    static int GetErrorCodeInResponse(CObjectMap* response);
    /** Add the standard header to the root object of `message`. */
    void InsertHeaderIntoMessage(CObjectMap* message, const char* messageType);
    /** `noSession`: send "0" as the session id instead of the local user's session. */
    void InsertHeaderIntoMessageObject(CObjectMapObject* message, const char* messageType, unsigned char noSession);

    unsigned char m_isValid;                // +0x04
    int m_readRequestsOutstanding;          // +0x08
    int m_writeRequestsOutstanding;         // +0x0c
    CNGSNotifyFunctor* m_readNotify;        // +0x10 fired when the reads complete
    CNGSNotifyFunctor* m_writeNotify;       // +0x14 fired when the writes complete
};

#endif
