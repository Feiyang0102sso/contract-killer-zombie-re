// WebUtil.cpp - small asynchronous HTTP helper on top of CHttpTransport.
#include "WebUtil.h"
#include <wchar.h>
#include <stdlib.h>

using namespace com::glu::platform::network;

/** Error code of CHttpTransport after which the request is simply sent again. */
static const int HTTP_ERROR_RETRY = 0xf7;

WebUtil::WebUtil()
{
    m_postData = NULL;
    m_state = STATE_IDLE;
    m_url = NULL;
    m_response = NULL;
    m_multipart = false;
    memset(&m_deviceInfo, 0, sizeof(SwpDeviceInfo));
    // SwpDeviceInfo is only partly named in CHttpTransport.h: 0x14 flag, 0x20 user agent, 0x24 flag
    m_deviceInfo.m_reserved[0] = 0;
    m_deviceInfo.m_reserved[1] = 0;
    m_deviceInfo.m_reserved[2] = 0;
    m_deviceInfo.m_reserved[3] = 0;
    m_deviceInfo.m_reserved[4] = 0;
    *(unsigned char*)&m_deviceInfo.m_reserved[5] = 1;
    m_deviceInfo.m_connectTimeoutMillis = 15000;
    m_deviceInfo.m_timeoutMillis = 30000;
    m_deviceInfo.m_reserved20[0] = (unsigned int)"sioux";
    *(unsigned char*)&m_deviceInfo.m_reserved20[1] = 1;
}

WebUtil::~WebUtil()
{
    if (m_response != NULL) {
        free(m_response);
        m_response = NULL;
    }
    if (m_url != NULL) {
        free(m_url);
        m_url = NULL;
    }
    if (m_postData != NULL) {
        free(m_postData);
        m_postData = NULL;
    }
}

void WebUtil::update()
{
    if (m_state != STATE_BUSY) {
        return;
    }
    m_transport.HandleUpdate(0);
    if (!m_transport.IsBusy()) {
        handleResponse();
    }
}

void WebUtil::handleResponse()
{
    int error = m_transport.GetError();
    if (error != 0) {
        if (error == HTTP_ERROR_RETRY) {
            processRequest();
            return;
        }
        m_state = STATE_ERROR;
        return;
    }
    if (m_response != NULL) {
        free(m_response);
        m_response = NULL;
    }
    m_response = m_transport.GetResponsePtr();
    m_responseLength = m_transport.GetResponseLen();
    m_transport.DetachResponse();
    m_state = STATE_IDLE;
}

void WebUtil::prepareUrl(const XString& url)
{
    if (m_state != STATE_IDLE) {
        m_state = STATE_ERROR;
        return;
    }
    if (m_transport.IsBusy()) {
        m_state = STATE_ERROR;
        return;
    }
    if (m_url != NULL) {
        free(m_url);
        m_url = NULL;
    }
    m_url = (char*)malloc(url.Length() + 1);
    const wchar_t* text = url.c_str();
    wcstombs(m_url, text, wcslen(text) + 1);
}

void WebUtil::httpGet(const XString& url)
{
    prepareUrl(url);
    processRequest();
}

void WebUtil::processRequest()
{
    int result;
    if (m_postData != NULL) {
        // ?: as an argument: keeps the leading loads common to both branches (hoisted as in the original)
        result = m_transport.Post(m_deviceInfo, m_url, m_postData, m_postLength, 0, NULL,
            m_multipart ? "multipart/form-data; boundary=1BEF0A57BE110FD467A" : "application/x-www-form-urlencoded");
    } else {
        result = m_transport.Get(m_deviceInfo, m_url, 0, NULL);
    }
    int state = STATE_ERROR;
    if (result == 0) {
        state = STATE_BUSY;
    }
    m_state = state;
}
