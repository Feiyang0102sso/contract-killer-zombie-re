// WebUtil.h - small asynchronous HTTP helper (GET, or POST when post data is set) on top of
// CHttpTransport. (created by b047x, owner of src/facebook/WebUtil.cpp)
#ifndef WEBUTIL_H
#define WEBUTIL_H

#include <stddef.h>
#include "CHttpTransport.h"
#include "xString.h"

class WebUtil {
public:
    /** Request state (m_state). */
    enum State {
        STATE_IDLE = 0,         // nothing running / the response is ready
        STATE_BUSY = 1,
        STATE_ERROR = 2
    };

    WebUtil();
    virtual ~WebUtil();

    /** Advance the running request; the response is collected when the transport is done. */
    void update();
    /** Take over the response (or retry on error 0xf7). */
    void handleResponse();
    /** Store `url` as the char* request url; fails (STATE_ERROR) while a request is running. */
    void prepareUrl(const XString& url);
    /** Start a GET of `url`. */
    void httpGet(const XString& url);
    /** Send the prepared request: POST when m_postData is set, GET otherwise. */
    void processRequest();

    int m_state;                        // 0x04 State
    char* m_url;                        // 0x08 owned
    unsigned char* m_response;          // 0x0c owned (detached from the transport)
    int m_responseLength;               // 0x10
    unsigned char* m_postData;          // 0x14 owned
    int m_postLength;                   // 0x18
    com::glu::platform::network::CHttpTransport m_transport;   // 0x1c
    com::glu::platform::network::SwpDeviceInfo m_deviceInfo;   // 0x8bc
    bool m_multipart;                   // 0x8ec post as multipart/form-data
};

#endif
