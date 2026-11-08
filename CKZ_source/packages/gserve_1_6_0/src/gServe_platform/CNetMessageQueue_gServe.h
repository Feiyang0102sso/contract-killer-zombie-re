// CNetMessageQueue_gServe.h - gServe message queue singleton: outgoing messages wait in a priority
// list, are sent in batches over CHttpTransports, and the decoded replies wait to be fetched by id.
// (created by b016x: class_owners.json gives the class to core/CNGSServerRequest.cpp, where the inline
//  constructor/destructor are emitted; the member functions live in CNetMessageQueue_gServe.cpp, b020.)
#ifndef CNETMESSAGEQUEUE_GSERVE_H
#define CNETMESSAGEQUEUE_GSERVE_H

#include "CSingleton.h"
#include "CStrWChar.h"
#include "CVector.h"
#include "CObjectMap.h"
#include "CNetMessageServer.h"
#include "TCList.h"

namespace com { namespace glu { namespace platform { namespace network {
class CHttpTransport;
} } } }
class CNetworkAvailability;  // added by b020

/** One queued message: the payload, its destination and the queueing options. */
class CNetMessageEnvelope {
public:
    CNetMessageEnvelope()
        : m_priority(8)
        , m_option32(0)
    {
    }

    /** The ushort argument is not stored (priority is reset to 0), as in the original. */
    CNetMessageEnvelope(const CObjectMap& message, const CNetMessageServer& server,
                        unsigned short priority, unsigned char option32, unsigned char option33)
        : m_message(message)
        , m_server(server)
        , m_priority(0)
        , m_option32(option32)
        , m_option33(option33)
    {
    }

    CObjectMap m_message;           // +0x00
    CNetMessageServer m_server;     // +0x14
    unsigned short m_priority;      // +0x30 lower values are queued first
    unsigned char m_option32;       // +0x32 (meaning unknown)
    unsigned char m_option33;       // +0x33 (meaning unknown, left uninitialized by the default constructor)
};

class CNetMessageQueue_gServe : public com::glu::platform::components::CSingleton {
public:
    /** Singleton key in CApp::m_singletons. */
    enum { ClassId = 0x69b74d03 };

    CNetMessageQueue_gServe()
        : CSingleton(ClassId)
        , m_lastMessageId(0)
        , m_state0c(0)
        , m_state10(0)
    {
    }

    virtual ~CNetMessageQueue_gServe() {}

    /** The registered instance, created on first use. */
    static inline CNetMessageQueue_gServe* GetInstance()
    {
        void* found = NULL;
        CApplet::m_pApp->m_singletons->Find(ClassId, &found);
        CNetMessageQueue_gServe* instance = (CNetMessageQueue_gServe*)found;
        if (instance == NULL) {
            instance = new CNetMessageQueue_gServe();
        }
        return instance;
    }

    /** Assign the next message id to `message` and queue it; returns the id. */
    unsigned int queueMessage(CObjectMap* message, const CNetMessageServer& server, unsigned short priority,
                              unsigned char option32, unsigned char option33);
    unsigned int queueMessage(const CNetMessageEnvelope& envelope);
    /** Hand out (and forget) the reply to message `messageId`; 0 while none arrived. */
    unsigned char fetchMessage(CObjectMap** reply, unsigned int messageId);
    /** Drop the queued message `messageId`; 1 when it was still queued. */
    unsigned char cancelMessage(unsigned int messageId);
    /** Send the queued messages up to `priority` in batches; returns how many were taken. */  // added by b020
    int flush(unsigned short priority);  // changed by b020: returns the number of messages taken (was void)
    void flushBatch(const com::glu::platform::core::CVector<CNetMessageEnvelope>& batch, const CNetMessageServer& server);
    /** Send the queued messages for `server` in one batch; returns how many were taken. */  // added by b020
    int flush(const CNetMessageServer& server);  // changed by b020: returns the number of messages taken (was void)
    void HandleUpdate(int elapsedMs);
    void atExit();
    void onStart();

    unsigned int m_lastMessageId;                                       // +0x08
    unsigned int m_state0c;                                             // +0x0c changed by b020 (was int): retry timer in ms, 0 = flush at once; compared unsigned
    CNetworkAvailability* m_state10;                                    // +0x10 changed by b020 (was int): reachability check, created by onStart()
    TCList<CNetMessageEnvelope> m_outgoing;                             // +0x14
    TCList<CNetMessageEnvelope> m_incoming;                             // +0x58
    com::glu::platform::core::CVector<com::glu::platform::network::CHttpTransport*> m_transports;  // +0x9c
    com::glu::platform::core::CVector<unsigned char*> m_requestBuffers;                            // +0xb4
    com::glu::platform::core::CVector<com::glu::platform::core::CVector<CNetMessageEnvelope> > m_batches; // +0xcc
    com::glu::platform::components::CStrWChar m_string0e4;              // +0xe4
    com::glu::platform::components::CStrWChar m_string0f4;              // +0xf4
    com::glu::platform::components::CStrWChar m_string104;              // +0x104
};

#endif
