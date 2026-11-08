// CNGSServerRequest.cpp - requests to the gServe server and the static table of outstanding requests.
#include "CNGSServerRequest.h"
#include "CNetMessageQueue_gServe.h"
#include "CObjectMap.h"

using com::glu::platform::components::CStrChar;
using com::glu::platform::core::CLinkList;
using com::glu::platform::core::CLinkListNode;

/** A request without a reply for this long times out (ms). */
static const unsigned long long REQUEST_TIMEOUT_MS = 30000;

CLinkList CNGSServerRequest::ms_ServerRequestsTable;
long long CNGSServerRequest::ms_timer;
unsigned int CNGSServerRequest::ms_mostRecentMessage;

CNGSServerRequest::CNGSServerRequest(const char* url, CObjectMap* message, CNGSServerRequestFunctor* functor,
                                     unsigned char delaySend, unsigned char cancelOnDelete)
    : m_server(CStrChar(url))
{
    m_message = message;
    m_cancelOnDelete = cancelOnDelete;
    m_functor = functor;
    m_sent = 0;
    if (delaySend == 0) {
        Send();
    }
}

unsigned int CNGSServerRequest::Send()
{
    if (m_sent) {
        return 0;
    }
    m_server.m_value10 = 0;
    m_server.m_value14 = 0;
    m_server.m_method = CNetMessageServer::METHOD_PUT;
    m_messageId = CNetMessageQueue_gServe::GetInstance()->queueMessage(m_message, m_server, 4, 1, 0);
    ms_mostRecentMessage = m_messageId;
    CLinkListNode* node = new CLinkListNode();
    ms_ServerRequestsTable.InsertAtEnd(node, this);
    CNetMessageQueue_gServe::GetInstance()->flush(m_server);
    m_sent = 1;
    if (ms_timer < 0) {
        ms_timer = 0;
    }
    return m_messageId;
}

CNGSServerRequest::~CNGSServerRequest()
{
    if (m_cancelOnDelete) {
        if (CancelMessage() == 0) {
            OnCommandCancelled();
        }
    }
    if (m_message != NULL) {
        delete m_message;
        m_message = NULL;
    }
}

unsigned char CNGSServerRequest::CancelMessage()
{
    unsigned char cancelled = CNetMessageQueue_gServe::GetInstance()->cancelMessage(m_messageId);
    if (cancelled) {
        OnCommandCancelled();
    }
    return cancelled;
}

void CNGSServerRequest::OnCommandCancelled()
{
    if (m_functor == NULL) {
        return;
    }
    m_functor->m_status = CNGSServerRequestFunctor::STATUS_CANCELLED;
    m_functor->requestCancelledCallback(m_message);
    if (m_functor != NULL) {
        delete m_functor;
    }
    m_functor = NULL;
}

void CNGSServerRequest::OnCommandTimeout()
{
    if (m_functor == NULL) {
        return;
    }
    m_functor->m_status = CNGSServerRequestFunctor::STATUS_TIMEOUT;
    m_functor->requestTimeoutCallback(m_message);
    if (m_functor != NULL) {
        delete m_functor;
    }
    m_functor = NULL;
}

void CNGSServerRequest::OnCommandCompleted(CObjectMap* response)
{
    if (m_functor == NULL) {
        return;
    }
    m_functor->requestCompleteCallback(response);
    if (m_functor != NULL) {
        delete m_functor;
    }
    m_functor = NULL;
}

void CNGSServerRequest::HandleUpdate(int elapsedMs)
{
    CNetMessageQueue_gServe::GetInstance()->HandleUpdate(elapsedMs);

    // Deliver every reply that arrived. `request` keeps the last request visited (original behaviour:
    // the timeout below applies to it).
    CObjectMap* response = NULL;
    CNGSServerRequest* request = NULL;
    CLinkListNode* node = ms_ServerRequestsTable.m_head;
    while (node != NULL) {
        CLinkListNode* next = node->m_next;
        request = (CNGSServerRequest*)node->m_data;
        if (CNetMessageQueue_gServe::GetInstance()->fetchMessage(&response, request->m_messageId)) {
            if (response != NULL) {
                RemoveMessageIDFromQueuedList(response->m_messageId);
                request->OnCommandCompleted(response);
                if (response != NULL) {
                    delete response;
                    response = NULL;
                }
                if (request != NULL) {
                    delete request;
                    request = NULL;
                }
                if (ms_ServerRequestsTable.m_count == 0) {
                    return;
                }
            }
            ms_timer = 0;
        }
        node = next;
    }

    if (ms_ServerRequestsTable.m_count > 0) {
        ms_timer += elapsedMs;
        if (ms_timer >= REQUEST_TIMEOUT_MS) {
            if (request != NULL) {
                RemoveMessageIDFromQueuedList(request->m_messageId);
                request->OnCommandTimeout();
                delete request;
            }
            ms_timer = 0;
            if (ms_ServerRequestsTable.m_count > 0) {
                DeleteAll();
            }
        } else {
            CNetMessageQueue_gServe::GetInstance()->flush(4);
        }
    }
}

unsigned char CNGSServerRequest::RemoveMessageIDFromQueuedList(unsigned int messageId)
{
    CLinkListNode* node = ms_ServerRequestsTable.Find(ms_ServerRequestsTable.m_head, isServerRequestWeWant, (void*)messageId);
    if (node == NULL) {
        return 0;
    }
    ms_ServerRequestsTable.Remove(node);
    delete node;
    return 1;
}

unsigned char CNGSServerRequest::AreNetworkMessagesWaitingForProcessing()
{
    return ms_ServerRequestsTable.m_count > 0;
}

void CNGSServerRequest::DeleteAll()
{
    for (CLinkListNode* node = ms_ServerRequestsTable.m_head; node != NULL; node = node->m_next) {
        CNGSServerRequest* request = (CNGSServerRequest*)node->m_data;
        if (request != NULL) {
            delete request;
        }
    }
    ms_ServerRequestsTable.RemoveAll();
}

void CNGSServerRequest::ClearAllMessages()
{
    CLinkListNode* node = ms_ServerRequestsTable.m_head;
    while (node != NULL) {
        CNGSServerRequest* request = (CNGSServerRequest*)node->m_data;
        // forget the functor first so the deletion does not report a cancel
        request->m_functor = NULL;
        if (request != NULL) {
            delete request;
        }
        node = node->m_next;
    }
    ms_ServerRequestsTable.RemoveAll();
}

unsigned char isServerRequestWeWant(CLinkListNode* node, void* messageId)
{
    CNGSServerRequest* request = (CNGSServerRequest*)node->m_data;
    return request->m_messageId == (unsigned int)messageId;
}

void CNGSServerRequestFunctor::requestTimeoutCallback(CObjectMap* response)
{
    requestCompleteCallback(NULL);
}

void CNGSServerRequestFunctor::requestCancelledCallback(CObjectMap* response)
{
    requestCompleteCallback(NULL);
}
