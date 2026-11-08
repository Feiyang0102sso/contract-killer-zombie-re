// CNGSTime.cpp - server clock and server version check of gServe.
#include <stdlib.h>
#include <wchar.h>
#include "CNGSTime.h"
#include "CNGS.h"
#include "CNGSTokens.h"
#include "CNGSUtil.h"
#include "CNGSLocalUser.h"
#include "CObjectMap.h"
#include "CObjectMapObject.h"

using com::glu::platform::components::CStrChar;
using com::glu::platform::components::CStrWChar;

/** Server error code of an outdated client. */
static const int SERVER_ERROR_VERSION_INVALID = 1008;
/** Size of the narrow buffer the error code text is converted into. */
static const int ERROR_CODE_TEXT_SIZE = 128;

CNGSTime::CNGSTime()
{
    m_serverTimeMs = 0;
    m_currentTimeMs = 0;
}

void CNGSTime::clearNetworkTime()
{
    m_serverTimeMs = 0;
    m_currentTimeMs = 0;
}

unsigned char CNGSTime::isValidNetworkTime()
{
    return m_currentTimeMs != 0;
}

void CNGSTime::tick(unsigned int elapsedMs)
{
    if (m_currentTimeMs == 0) {
        return;
    }
    m_currentTimeMs += elapsedMs;
}

unsigned char CNGSTime::resetNetworkTime()
{
    CObjectMapObject* message = new CObjectMapObject();
    InsertHeaderIntoMessageObject(message, "nettime", 0);
    CObjectMap* request = new CObjectMap(message);
    CNGSTimeFunctor* functor = new CNGSTimeFunctor(this, &CNGSTime::handleResponseNetworkTime);
    if (message != NULL) {
        delete message;
    }
    CStrChar url(CNGS::GetInstance()->GetServerURL());
    url += "resources/user/getnettime2";
    unsigned char sent = 0;
    CNGSServerRequest* serverRequest = new CNGSServerRequest(url.GetCharPtr(), request, functor, 0, 1);
    if (serverRequest->m_messageId != 0) {
        AddReadRequestOutstanding();
        sent = 1;
    }
    return sent;
}

void CNGSTime::handleResponseNetworkTime(CObjectMap* response, CNGSTimeFunctor* functor)
{
    if (response != NULL) {
        CStrWChar errorText = CNGSUtil::GetMessageDataString(response, CStrWChar(NGS_ERROR_CODE_TOKEN), CStrWChar(","));
        int errorCode = 0;
        if (errorText.GetLength() != 0) {
            char narrowText[ERROR_CODE_TEXT_SIZE];
            wcstombs(narrowText, errorText.GetWCharPtr(), wcslen(errorText.GetWCharPtr()) + 1);
            errorCode = atoi(narrowText);
        }
        CNGSLocalUser* user = CNGS::GetInstance()->GetLocalUser();
        if (errorCode == SERVER_ERROR_VERSION_INVALID) {
            m_serverTimeMs = 0;
            m_currentTimeMs = 0;
            user->SetIsValid(0);
            user->CNGSHandleVersionCheck(0, "handleResponseNetworkTime");
            CNGS::GetInstance()->SetNetworkConnectionState(NETWORK_VERSION_INVALID, "handleResponseNetworkTime");
        } else if (errorCode == 0 && functor->m_context == NULL) {
            m_serverTimeMs = 0;
            m_currentTimeMs = 0;
            CObjectMapValue* time = response->getDataAt(CStrWChar(NGS_LIST_TOKEN), CStrWChar(L" "));
            if (time != NULL && time->getType() == CObjectMapValue::TYPE_INT) {
                m_serverTimeMs = ((CObjectMapInt*)time)->m_value;
                m_currentTimeMs = m_serverTimeMs;
            }
            user->CNGSHandleVersionCheck(1, "handleResponseNetworkTime");
        }
    } else {
        // no reply: the connection is lost (unless the request was cancelled)
        m_serverTimeMs = 0;
        m_currentTimeMs = 0;
        CNGSLocalUser* user = CNGS::GetInstance()->GetLocalUser();
        if (functor == NULL || functor->m_status != CNGSServerRequestFunctor::STATUS_CANCELLED) {
            if (user->isValid()) {
                if (CNGS::GetInstance()->GetNetworkConnectionState() != NETWORK_CONNECTION_ERROR) {
                    user->SetIsValid(0);
                }
            }
            CNGS::GetInstance()->SetNetworkConnectionState(NETWORK_CONNECTION_ERROR, "handleResponseNetworkTime");
        }
    }
    CompleteReadRequestOutstanding();
}

unsigned char CNGSTime::doServerVersionCheck()
{
    CObjectMapObject* message = new CObjectMapObject();
    InsertHeaderIntoMessageObject(message, "nettime", 0);
    CObjectMap* request = new CObjectMap(message);
    CNGSTimeFunctor* functor = new CNGSTimeFunctor(this, &CNGSTime::handleResponseNetworkTime);
    if (message != NULL) {
        delete message;
    }
    CStrChar url(CNGS::GetInstance()->GetServerURL());
    url += "resources/user/getnettime2";
    unsigned char sent = 0;
    CNGSServerRequest* serverRequest = new CNGSServerRequest(url.GetCharPtr(), request, functor, 0, 1);
    if (serverRequest->m_messageId != 0) {
        AddReadRequestOutstanding();
        sent = 1;
    }
    return sent;
}

unsigned long long CNGSTime::getNetworkCurrentTimeMS()
{
    return m_currentTimeMs;
}

unsigned long long CNGSTime::getNetworkCurrentTimeSeconds()
{
    return m_currentTimeMs / 1000;
}

unsigned char CNGSTime::SyncAllFromServer(CNGSNotifyFunctor* notify)
{
    if (!isReady()) {
        return 0;
    }
    if (m_writeRequestsOutstanding != 0) {
        return 0;
    }
    m_readNotify = notify;
    return resetNetworkTime();
}
