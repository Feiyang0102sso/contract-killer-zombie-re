// CNetMessageServer.cpp - destination (URL + HTTP method) of a queued gServe message.
#include "CNetMessageServer.h"

using com::glu::platform::components::CStrChar;

CNetMessageServer::CNetMessageServer()
{
    m_value10 = 0;
    m_value14 = 0;
    m_method = METHOD_POST;
}

CNetMessageServer::CNetMessageServer(const CStrChar& url)
{
    // The original appends the URL to the empty member string (no operator= self check).
    m_url.Concatenate(url.m_string);
    m_value10 = 0;
    m_value14 = 0;
    m_method = METHOD_POST;
}

const char* CNetMessageServer::getMethod() const
{
    switch (m_method) {
    case METHOD_GET:
        return "GET ";
    case METHOD_POST:
        return "POST ";
    case METHOD_PUT:
        return "PUT ";
    }
    return NULL;
}
