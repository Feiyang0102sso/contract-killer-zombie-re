// CNGSServerObject.cpp - base of the gServe objects synchronized with the server: outstanding
// request counters, sending a message (with the standard header) and decoding server errors.
#include <stdlib.h>
#include <wchar.h>
#include "CNGSServerObject.h"
#include "CNGS.h"
#include "CNGSHeader.h"
#include "CNGSLocalUser.h"
#include "CNGSServerRequest.h"
#include "CNGSSession.h"
#include "CNGSTokens.h"
#include "CNGSUtil.h"
#include "CObjectMap.h"
#include "CObjectMapObject.h"
#include "CObjectMapValue.h"
#include "CStrChar.h"

using com::glu::platform::components::CStrChar;
using com::glu::platform::components::CStrWChar;

/** "errorcode" values with a special meaning. */
static const int SERVER_ERROR_VERSION_INVALID = 1008;
static const int SERVER_ERROR_SESSION_INVALID = 1009;
/** 1010 / 1011 change nothing in the connection state (meaning unknown). */
static const int SERVER_ERROR_IGNORED_FIRST = 1010;
static const int SERVER_ERROR_IGNORED_LAST = 1011;

/** Size of the narrow buffer the error code text is converted into. */
static const int ERROR_CODE_TEXT_SIZE = 128;

CNGSServerObject::~CNGSServerObject()
{
    ;   // the body is not empty in the original (a compiled-out log): the vtable pointer is still stored
}

void CNGSServerObject::AddReadRequestOutstanding()
{
    m_readRequestsOutstanding++;
}

void CNGSServerObject::CompleteReadRequestOutstanding()
{
    int remaining = m_readRequestsOutstanding - 1;
    if (remaining < 0) {
        remaining = 0;
    }
    m_readRequestsOutstanding = remaining;

    if (m_readRequestsOutstanding == 0) {
        if (m_readNotify != NULL) {
            m_readNotify->notify();
            delete m_readNotify;
            m_readNotify = NULL;
        }
    }
}

unsigned char CNGSServerObject::isReady()
{
    return m_readRequestsOutstanding == 0;
}

void CNGSServerObject::AddWriteRequestOutstanding()
{
    m_writeRequestsOutstanding++;
}

/** Note: the original waits for the *read* count to reach zero here. */
void CNGSServerObject::CompleteWriteRequestOutstanding()
{
    m_writeRequestsOutstanding--;

    if (m_readRequestsOutstanding == 0) {
        if (m_writeNotify != NULL) {
            m_writeNotify->notify();
            delete m_writeNotify;
            m_writeNotify = NULL;
        }
    }
}

unsigned char CNGSServerObject::isValid()
{
    return m_isValid;
}

void CNGSServerObject::SetIsValid(unsigned char isValid)
{
    m_isValid = isValid;
}

int CNGSServerObject::SendMessageObjectToServer(CObjectMapObject* message, const char* service,
                                                         CNGSServerRequestFunctor* functor)
{
    // The map keeps a clone of the object, so the object itself is deleted here.
    unsigned int messageId = SendMessageToServer(new CObjectMap(message), service, functor);
    delete message;
    return messageId;
}

unsigned int CNGSServerObject::SendMessageToServer(CObjectMap* message, const char* service,
                                                   CNGSServerRequestFunctor* functor)
{
    CStrChar url(CNGS::GetInstance()->GetServerURL());
    url += service;

    // The request registers itself in the request table and owns message and functor.
    CNGSServerRequest* request = new CNGSServerRequest(url.GetCharPtr(), message, functor, 0, 1);
    return request->m_messageId;
}

int CNGSServerObject::WasErrorInResponse(CObjectMap* response, CNGSServerRequestFunctor* functor, const char* caller)
{
    CNGSLocalUser* localUser = CNGS::GetInstance()->GetLocalUser();

    if (response == NULL) {
        // No reply: the connection is lost unless the request was cancelled or it is already known.
        if (localUser == NULL) {
            return -2;
        }
        if (functor != NULL && functor->m_status == CNGSServerRequestFunctor::STATUS_CANCELLED) {
            return -2;
        }
        if (!localUser->isValid()) {
            return -2;
        }
        if (CNGS::GetInstance()->GetNetworkConnectionState() == NETWORK_CONNECTION_ERROR) {
            return -2;
        }
        localUser->SetIsValid(0);
        CNGS::GetInstance()->SetNetworkConnectionState(NETWORK_CONNECTION_ERROR, caller);
        return -2;
    }

    CObjectMapValue* errorValue = response->getDataAt(CStrWChar(NGS_ERROR_CODE_TOKEN), CStrWChar(L" "));
    if (errorValue != NULL) {
        if (errorValue->getType() != CObjectMapValue::TYPE_STRING) {
            return -1;
        }

        int errorCode = GetErrorCodeInResponse(response);
        switch (errorCode) {
        case SERVER_ERROR_SESSION_INVALID:
            CNGS::GetInstance()->SetNetworkConnectionState(NETWORK_SESSION_INVALID, caller);
            if (localUser != NULL) {
                CNGSSession* session = localUser->m_session;
                localUser->SetIsValid(0);
                if (session != NULL) {
                    session->invalidate();
                    if (CNGSServerRequest::AreNetworkMessagesWaitingForProcessing()) {
                        CNGSServerRequest::DeleteAll();
                    }
                }
                localUser->CNGSHandleSessionInvalid(caller);
            }
            break;

        case SERVER_ERROR_VERSION_INVALID:
            if (localUser != NULL) {
                if (CNGSServerRequest::AreNetworkMessagesWaitingForProcessing()) {
                    CNGSServerRequest::DeleteAll();
                }
                if (CNGS::GetInstance()->GetNetworkConnectionState() != NETWORK_VERSION_INVALID) {
                    localUser->SetIsValid(0);
                    localUser->CNGSHandleVersionCheck(0, caller);
                    CNGS::GetInstance()->SetNetworkConnectionState(NETWORK_VERSION_INVALID, caller);
                }
            }
            CNGS::GetInstance()->SetNetworkConnectionState(NETWORK_VERSION_INVALID, caller);
            break;

        case SERVER_ERROR_IGNORED_FIRST:
        case SERVER_ERROR_IGNORED_LAST:
            break;

        default:
            if (localUser != NULL) {
                localUser->SetIsValid(0);
                localUser->CNGSHandleGenericServerError(caller);
            }
            CNGS::GetInstance()->SetNetworkConnectionState(NETWORK_SERVER_ERROR, caller);
            break;
        }

        // The server message is fetched but not used any more (its log was compiled out).
        CObjectMapValue* messageValue = response->getDataAt(CStrWChar(NGS_MESSAGE_TOKEN), CStrWChar(L" "));
        if (messageValue->getType() == CObjectMapValue::TYPE_STRING) {
            CStrWChar message;
            message = CNGSUtil::GetMessageDataString(response, CStrWChar(NGS_MESSAGE_TOKEN), CStrWChar(","));
        }
        return errorCode;
    }

    // Success: the connection works (again). The original dereferences localUser without a check.
    if (localUser != NULL) {
        if (!localUser->isValid() ||
            CNGS::GetInstance()->GetNetworkConnectionState() == NETWORK_CONNECTION_ERROR) {
            localUser->SetIsValid(1);
        }
    }
    localUser->SetIsValid(1);
    CNGS::GetInstance()->SetNetworkConnectionState(NETWORK_CONNECTED, caller);
    return 0;
}

int CNGSServerObject::GetErrorCodeInResponse(CObjectMap* response)
{
    int errorCode = -2;
    if (response != NULL) {
        errorCode = 0;
        CObjectMapValue* errorValue = response->getDataAt(CStrWChar(NGS_ERROR_CODE_TOKEN), CStrWChar(L" "));
        if (errorValue != NULL && errorValue->getType() == CObjectMapValue::TYPE_STRING) {
            char narrowText[ERROR_CODE_TEXT_SIZE];
            CStrWChar errorText;
            errorText = CNGSUtil::GetMessageDataString(response, CStrWChar(NGS_ERROR_CODE_TOKEN), CStrWChar(","));

            wcstombs(narrowText, errorText.m_string, wcslen(errorText.m_string) + 1);
            errorCode = atoi(narrowText);
        }
    }
    return errorCode;
}

void CNGSServerObject::InsertHeaderIntoMessage(CObjectMap* message, const char* messageType)
{
    if (message == NULL) {
        return;
    }
    if (message->getRoot() != NULL) {
        InsertHeaderIntoMessageObject((CObjectMapObject*)message->getRoot(), messageType, 0);
    }
}

/** Adds {"header": legacy header + sessionid + mac + type} to `message`. */
void CNGSServerObject::InsertHeaderIntoMessageObject(CObjectMapObject* message, const char* messageType,
                                                     unsigned char noSession)
{
    CNGSLocalUser* localUser = CNGS::GetInstance()->GetLocalUser();
    CNGSHeader headerInfo;
    CObjectMapObject* header = headerInfo.createObjectRepresentationLegacy(CStrWChar(messageType));

    if (noSession) {
        header->addEntry(CStrWChar("sessionid"), new CObjectMapString(CStrWChar("0")));
    } else {
        // The session id is copied into a temporary first, as in the original.
        header->addEntry(CStrWChar("sessionid"),
                         new CObjectMapString(CStrWChar(localUser->m_session->m_config.m_sessionId)));
    }
    header->addEntry(CStrWChar(NGS_MAC_TOKEN), new CObjectMapString(CStrWChar("")));
    header->addEntry(CStrWChar(NGS_TYPE_TOKEN), new CObjectMapString(CStrWChar(messageType)));

    message->addEntry(CStrWChar(NGS_HEADER_TOKEN), header);
}
