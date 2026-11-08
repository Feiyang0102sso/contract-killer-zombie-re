// Owner: tools/mqaa/CNetMessageServer.cpp (b028x; started as a b016x PROVISIONAL)
// CNetMessageServer.h - destination of a queued gServe message: URL and HTTP method.
#ifndef CNETMESSAGESERVER_H
#define CNETMESSAGESERVER_H

#include "CStrChar.h"

class CNetMessageServer {
public:
    /** HTTP methods (m_method). */
    enum eMethod {
        METHOD_GET = 0,
        METHOD_POST = 1,
        METHOD_PUT = 2
    };

    /** Empty URL, POST. */
    CNetMessageServer();
    /** `url`, POST. */
    CNetMessageServer(const com::glu::platform::components::CStrChar& url);

    /** "GET ", "POST ", "PUT " or NULL. */
    const char* getMethod() const;

    /** Same URL (CStrChar rules) and same settings; batches only group equal servers. */  // added by b020
    inline bool operator==(const CNetMessageServer& other) const  // added by b020
    {  // added by b020
        if (m_url != other.m_url) {  // added by b020
            return false;  // added by b020
        }  // added by b020
        return m_value10 == other.m_value10 && m_value14 == other.m_value14 && m_method == other.m_method;  // added by b020
    }  // added by b020

    com::glu::platform::components::CStrChar m_url;    // +0x00
    int m_value10;                                      // +0x10 0 after construction (meaning unknown)
    int m_value14;                                      // +0x14 0 after construction (meaning unknown)
    int m_method;                                       // +0x18 eMethod
};

#endif
