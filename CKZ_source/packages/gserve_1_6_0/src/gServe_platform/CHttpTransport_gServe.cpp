// CHttpTransport_gServe.cpp - asynchronous HTTP/1.1 client of gserve (GET / POST, chunked responses,
// redirects, If-Modified-Since).
#include "CHttpTransport_gServe.h"

#include <stdlib.h>
#include <string.h>
#include "CHttpDataChunk_gServe.h"
#include "CWUtil.h"
#include "CGenUtil.h"
#include "ICStdUtil.h"

using namespace com::glu::platform;

static const char cszGet[] = "GET ";
static const char cszPost[] = "POST ";
static const char cszHTTP[] = "HTTP/";
static const char cszHTTPVersion[] = "1.1";
static const char cszCRLF[] = "\r\n";
static const char cszHost[] = "Host: ";
static const char cszUserAgent[] = "User-Agent: ";
static const char cszUserAgentDefault[] = "Glu Native";
static const char cszContentType[] = "Content-Type: ";
static const char cszContentTypeDefault[] = "application/octet-stream";
static const char cszContentLength[] = "Content-Length: ";
static const char cszIfModifiedSince[] = "If-Modified-Since: ";
static const char cszTransferEncoding[] = "Transfer-Encoding: ";
static const char cszChunked[] = "chunked";
static const char cszLocation[] = "Location: ";

CHttpTransport_gServe::CHttpTransport_gServe()
{
    m_socket = network::ICSocket::CreateInstance();
    m_host = (const char*)NULL;
    m_port = 0;
    m_path = (const char*)NULL;
    m_requestHeader = (const char*)NULL;
    m_timeoutMillis = 0;
    m_connectTimeoutMillis = 0;
    m_requestData = NULL;
    m_requestDataLength = 0;
    m_keepAlive = 0;
    m_response = NULL;
    m_responseLength = 0;
    m_statusCode = 0;
    m_chunked = 0;
    m_chunkedLength = 0;
    m_buffer[0] = 0;
    m_bufferLength = 0;
    m_state = State_Idle;
    m_error = 0;
    m_socketError = 0;
    m_reserved898 = 0;
    m_redirect = new RedirectInfo;
    if (m_redirect != NULL) {
        m_redirect->m_ifModifiedSince = components::CStrChar();
    }
    m_location = (const char*)NULL;
}

CHttpTransport_gServe::~CHttpTransport_gServe()
{
    m_socket->Disconnect();
    ResetForNextRequest();
    if (m_redirect != NULL) {
        delete m_redirect;
        m_redirect = NULL;
    }
    if (m_socket != NULL) {
        delete m_socket;
    }
    m_socket = NULL;
}

void CHttpTransport_gServe::Cancel()
{
    m_socket->Disconnect();
    ResetForNextRequest();
}

void CHttpTransport_gServe::ResetForNextRequest()
{
    m_host = (const char*)NULL;
    m_port = 0;
    m_path = (const char*)NULL;
    m_requestHeader = (const char*)NULL;
    m_timeoutMillis = 0;
    m_connectTimeoutMillis = 0;
    m_requestData = NULL;
    m_requestDataLength = 0;
    m_keepAlive = 0;
    if (m_response != NULL) {
        free(m_response);
        m_response = NULL;
    }
    m_responseLength = 0;
    m_statusCode = 0;
    m_location = (const char*)NULL;
    m_chunked = 0;
    m_chunkedLength = 0;
    ClearChunkList();
    m_buffer[0] = 0;
    m_bufferLength = 0;
    m_state = State_Idle;
    m_error = 0;
    m_socketError = 0;
    m_reserved898 = 0;
}

int CHttpTransport_gServe::Get(const network::SwpDeviceInfo& info, const char* url, unsigned char keepAlive,
                               const char* userAgent, const char* ifModifiedSince)
{
    return Send(info, url, NULL, 0, keepAlive, userAgent, NULL, cszGet, ifModifiedSince);
}

int CHttpTransport_gServe::Send(const network::SwpDeviceInfo& info, const char* url, unsigned char* data,
                                unsigned int dataLength, unsigned char keepAlive, const char* userAgent,
                                const char* contentType, const char* method, const char* ifModifiedSince)
{
    if (IsBusy()) {
        return -1;
    }
    if (url == NULL) {
        return 1;
    }

    ResetForNextRequest();

    // remember the request for a later redirect
    if (m_redirect != NULL) {
        m_redirect->m_timeoutMillis = info.m_timeoutMillis;
        m_redirect->m_connectTimeoutMillis = info.m_connectTimeoutMillis;
        m_redirect->m_url = url;
        m_redirect->m_keepAlive = keepAlive;
        m_redirect->m_userAgent = userAgent;
    }

    // gserve does not support https: the flag is parsed and dropped
    unsigned char secure;
    int result = network::CWUtil::ParseUrl(url, m_host, m_port, m_path, secure);
    if (result != 0) {
        return result;
    }

    m_requestData = data;
    m_requestDataLength = dataLength;
    m_timeoutMillis = info.m_timeoutMillis;
    m_connectTimeoutMillis = info.m_connectTimeoutMillis;
    m_keepAlive = keepAlive;

    char number[20];
    if (data != NULL) {
        if (method != NULL) {
            m_requestHeader = method;
        } else {
            m_requestHeader = cszPost;
        }
        if (m_redirect != NULL) {
            m_redirect->m_method = m_requestHeader;
        }
        m_requestHeader += m_path;
        m_requestHeader += " ";
        m_requestHeader += cszHTTP;
        m_requestHeader += cszHTTPVersion;
        m_requestHeader += cszCRLF;
        m_requestHeader += cszHost;
        m_requestHeader += m_host;
        m_requestHeader += ":";
        core::ICStdUtil::SPrintF(number, "%hu%s", m_port, cszCRLF);
        m_requestHeader += number;
        m_requestHeader += cszUserAgent;
        // ?: on purpose: the original selects the string and makes a single call
        m_requestHeader += (userAgent != NULL) ? userAgent : cszUserAgentDefault;
        m_requestHeader += cszCRLF;
        m_requestHeader += cszContentType;
        m_requestHeader += (contentType != NULL) ? contentType : cszContentTypeDefault;
        m_requestHeader += cszCRLF;
        m_requestHeader += cszContentLength;
        core::ICStdUtil::SPrintF(number, "%lu%s", m_requestDataLength, cszCRLF);
        m_requestHeader += number;
        m_requestHeader += cszCRLF;
    } else {
        m_requestHeader = cszGet;
        if (m_redirect != NULL) {
            m_redirect->m_method = m_requestHeader;
            if (ifModifiedSince != NULL) {
                m_redirect->m_ifModifiedSince = ifModifiedSince;
            }
        }
        m_requestHeader += m_path;
        m_requestHeader += " ";
        m_requestHeader += cszHTTP;
        m_requestHeader += cszHTTPVersion;
        m_requestHeader += cszCRLF;
        m_requestHeader += cszHost;
        m_requestHeader += m_host;
        m_requestHeader += ":";
        core::ICStdUtil::SPrintF(number, "%hu%s", m_port, cszCRLF);
        m_requestHeader += number;
        if (ifModifiedSince != NULL) {
            m_requestHeader += cszIfModifiedSince;
            m_requestHeader += ifModifiedSince;
            m_requestHeader += cszCRLF;
        }
        m_requestHeader += cszUserAgent;
        // ?: on purpose: the original selects the string and makes a single call
        m_requestHeader += (userAgent != NULL) ? userAgent : cszUserAgentDefault;
        m_requestHeader += cszCRLF;
        m_requestHeader += cszCRLF;
    }

    m_state = State_Connect;
    return result;
}

void CHttpTransport_gServe::HandleUpdate(int elapsed)
{
    m_socket->HandleUpdate(elapsed);

    switch (m_state) {
    case State_Connecting:
    case State_SendingHeader:
    case State_SendingBody:
    case State_ReceivingHeader:
    case State_ParsingHeader:
    case State_ReceivingBody:
    case State_ChunkHeader:
    case State_ChunkData:
    case State_ChunkFooter:
    case State_ChunkTrailer:
        // wait for the pending socket operation
        if (m_socket->IsBusy()) {
            break;
        }
        if (AbortOnError(m_socket->GetError())) {
            break;
        }
        switch (m_state) {
        case State_Connecting:
            SendRequestHeader();
            break;
        case State_SendingHeader:
            if (m_requestData != NULL) {
                SendRequest();
            } else {
                ReceiveResponseHeader();
            }
            break;
        case State_SendingBody:
            ReceiveResponseHeader();
            break;
        case State_ReceivingHeader:
            m_state = State_ParsingHeader;
            m_bufferLength += m_socket->GetDataLen();
            ParseResponseHeader();
            break;
        case State_ParsingHeader:
        case State_ChunkTrailer:
            m_bufferLength += m_socket->GetDataLen();
            ParseResponseHeader();
            break;
        case State_ReceivingBody:
            m_bufferLength += m_socket->GetDataLen();
            ReceiveResponse();
            break;
        case State_ChunkHeader:
            m_bufferLength += m_socket->GetDataLen();
            ParseChunkHeader();
            break;
        case State_ChunkData:
            m_bufferLength += m_socket->GetDataLen();
            ParseChunkData();
            break;
        case State_ChunkFooter:
            m_bufferLength += m_socket->GetDataLen();
            ParseChunkFooter();
            break;
        }
        break;
    case State_Connect:
        Connect();
        break;
    case State_Cleanup:
        Cleanup();
        break;
    }
}

unsigned char CHttpTransport_gServe::AbortOnError(int error)
{
    if (error == 0) {
        return 0;
    }
    if (m_state == State_Cleanup) {
        return 0;
    }
    m_error = error;
    m_socketError = m_socket->GetSocketError();
    m_socket->Disconnect();
    m_state = State_Cleanup;
    return 1;
}

void CHttpTransport_gServe::SendRequestHeader()
{
    m_state = State_SendingHeader;
    const char* header = m_requestHeader.GetCharPtr();
    AbortOnError(m_socket->Send((const unsigned char*)header, strlen(header), m_timeoutMillis));
}

void CHttpTransport_gServe::SendRequest()
{
    m_state = State_SendingBody;
    AbortOnError(m_socket->Send(m_requestData, m_requestDataLength, m_timeoutMillis));
}

void CHttpTransport_gServe::ReceiveResponseHeader()
{
    m_state = State_ReceivingHeader;
    AbortOnError(ReceiveIntoBuffer());
}

void CHttpTransport_gServe::ParseResponseHeader()
{
    int error = 0;
    unsigned char done = 0;
    int lineCount = 0;
    unsigned int parsedLength = 0;      // bytes of the lines handled so far
    char* buffer = m_buffer;

    buffer[m_bufferLength] = 0;
    char* lineStart = buffer;
    char* lineEnd = strstr(lineStart, cszCRLF);
    unsigned int lineEndOffset;

    while (lineEnd != NULL && !(done || error != 0)) {
        lineEndOffset = lineEnd - buffer + 2;
        *lineEnd = 0;
        if (lineEnd == lineStart) {
            // empty line: end of the header, the body follows
            lineCount++;
            if (m_chunked) {
                ConsumeBuffer(lineEndOffset);
                if (m_state == State_ChunkTrailer) {
                    m_state = State_Cleanup;
                } else {
                    ReceiveChunkedResponse();
                }
                done = 1;
            } else {
                done = 1;
                if (m_responseLength != 0) {
                    m_response = (unsigned char*)malloc(m_responseLength);
                    if (m_response == NULL) {
                        error = Error_OutOfMemory;
                    } else if (lineEndOffset < m_bufferLength) {
                        unsigned int available = m_bufferLength - lineEndOffset;
                        if (available > m_responseLength) {
                            available = m_responseLength;
                        }
                        memcpy(m_response, m_buffer + lineEndOffset, available);
                    }
                }
                if (error == 0) {
                    ConsumeBuffer(lineEndOffset);
                    ReceiveResponse();
                }
            }
            parsedLength = lineEndOffset;
        } else {
            lineCount++;
            if (ParseHTTPHeaderLine(lineStart)) {
                lineStart = m_buffer + lineEndOffset;
                parsedLength = lineEndOffset;
                lineEnd = strstr(lineStart, cszCRLF);
            } else {
                error = Error_BadResponse;
                parsedLength = lineEndOffset;
            }
        }
    }

    if (!done && error == 0) {
        // no complete line left: keep the unparsed rest and receive more
        if (m_bufferLength == BufferSize && lineCount == 0) {
            error = Error_BadResponse;
        } else {
            ConsumeBuffer(parsedLength);
            error = ReceiveIntoBuffer();
        }
    }
    AbortOnError(error);
}

void CHttpTransport_gServe::ReceiveResponse()
{
    int error = Error_BadStatus;
    switch (m_statusCode) {
    case 304:
        // not modified: done without a body
        m_state = State_Cleanup;
        error = 0;
        break;
    case 200:
    case 201:
    case 204:
    case 302:
    case 303:
        // these carry a usable body
        if (m_responseLength != 0 && m_bufferLength < m_responseLength) {
            m_state = State_ReceivingBody;
            error = m_socket->Receive(m_response + m_bufferLength, m_responseLength - m_bufferLength, m_timeoutMillis);
        } else {
            m_state = State_Cleanup;
            error = 0;
        }
        break;
    }
    AbortOnError(error);
}

void CHttpTransport_gServe::ParseChunkHeader()
{
    int error;
    char* buffer = m_buffer;
    buffer[m_bufferLength] = 0;
    char* lineEnd = strstr(buffer, cszCRLF);

    if (lineEnd != NULL) {
        *lineEnd = 0;
        if (lineEnd == buffer) {
            error = Error_BadResponse;
        } else {
            unsigned int lineLength = lineEnd - buffer + 2;
            unsigned int chunkSize = core::CGenUtil::HexAToI(buffer);
            if (chunkSize != 0) {
                CHttpDataChunk_gServe* chunk = new CHttpDataChunk_gServe();
                if (chunk == NULL) {
                    error = Error_OutOfMemory;
                } else {
                    m_chunks.InsertAtEnd(chunk, NULL);
                    chunk->m_data = (unsigned char*)malloc(chunkSize);
                    if (chunk->m_data == NULL) {
                        error = Error_OutOfMemory;
                    } else {
                        chunk->m_size = chunkSize;
                        chunk->m_received = 0;
                        m_chunkedLength += chunkSize;
                        ConsumeBuffer(lineLength);
                        m_state = State_ChunkData;
                        if (m_bufferLength != 0) {
                            ParseChunkData();
                            error = 0;
                        } else {
                            error = ReceiveIntoBuffer();
                        }
                    }
                }
            } else {
                // last chunk: assemble the body, trailer headers may follow
                ConsumeBuffer(lineLength);
                if (!CreateResponseFromChunkedData()) {
                    error = Error_OutOfMemory;
                } else {
                    m_state = State_ChunkTrailer;
                    if (m_bufferLength != 0) {
                        ParseResponseHeader();
                        error = 0;
                    } else {
                        error = ReceiveIntoBuffer();
                    }
                }
            }
        }
    } else {
        if (m_bufferLength == BufferSize) {
            error = Error_BadResponse;
        } else {
            error = ReceiveIntoBuffer();
        }
    }
    AbortOnError(error);
}

void CHttpTransport_gServe::ParseChunkData()
{
    int error = Error_BadResponse;
    m_buffer[m_bufferLength] = 0;
    CHttpDataChunk_gServe* chunk = (CHttpDataChunk_gServe*)m_chunks.GetTail();
    if (chunk != NULL) {
        unsigned int missing = chunk->m_size - chunk->m_received;
        unsigned int count = m_bufferLength;
        if (count > missing) {
            count = missing;
        }
        if (count != 0) {
            memcpy(chunk->m_data + chunk->m_received, m_buffer, count);
            missing -= count;
            chunk->m_received += count;
        }
        ConsumeBuffer(count);
        error = 0;
        if (chunk->m_received == chunk->m_size) {
            m_state = State_ChunkFooter;
            if (m_bufferLength != 0) {
                ParseChunkFooter();
                error = 0;
            } else {
                error = ReceiveIntoBuffer();
            }
        }
        if (error == 0 && missing != 0) {
            error = ReceiveIntoBuffer();
        }
    }
    AbortOnError(error);
}

void CHttpTransport_gServe::ParseChunkFooter()
{
    int error;
    m_buffer[m_bufferLength] = 0;
    if (m_bufferLength >= 2) {
        if (strncmp(m_buffer, cszCRLF, 2) != 0) {
            error = Error_BadResponse;
        } else {
            ConsumeBuffer(2);
            m_state = State_ChunkHeader;
            if (m_bufferLength != 0) {
                ParseChunkHeader();
                error = 0;
            } else {
                error = ReceiveIntoBuffer();
            }
        }
    } else {
        error = ReceiveIntoBuffer();
    }
    AbortOnError(error);
}

void CHttpTransport_gServe::Connect()
{
    m_state = State_Connecting;
    AbortOnError(m_socket->Connect(network::ICSocket::Protocol_TCP, m_host.GetCharPtr(), m_port, m_connectTimeoutMillis, 0));
}

void CHttpTransport_gServe::Cleanup()
{
    if (!m_keepAlive || m_error != 0) {
        m_socket->Disconnect();
    }
    // ?: on purpose: the original selects the final state
    m_state = (m_statusCode == 304) ? State_NotModified : State_Done;
    if (m_statusCode >= 302 && m_statusCode <= 303) {
        // copy: the redirect resets m_location
        components::CStrChar location(m_location);
        RedirectTo(location);
    }
}

int CHttpTransport_gServe::GetStatus(int* progress)
{
    int status = Status_Idle;
    int percent = 0;
    switch (m_state) {
    case State_Idle:
    case State_Connect:
    case State_Connecting:
    case State_SendingHeader:
        percent = 0;
        status = m_state;
        break;
    case State_SendingBody:
        if (m_requestDataLength == 0) {
            status = Status_Sending;
            percent = 0;
        } else {
            percent = (m_bufferLength * 100) / m_requestDataLength;
            status = Status_Sending;
        }
        break;
    case State_ReceivingHeader:
    case State_ParsingHeader:
        percent = 0;
        status = Status_Receiving;
        break;
    case State_ReceivingBody:
        if (m_responseLength == 0) {
            status = Status_Receiving;
            percent = 0;
        } else {
            percent = (m_bufferLength * 100) / m_responseLength;
            status = Status_Receiving;
        }
        break;
    case State_ChunkHeader:
    case State_ChunkData:
    case State_ChunkFooter:
        if (m_responseLength == 0) {
            status = Status_Receiving;
            percent = 0;
        } else {
            percent = (m_chunkedLength * 100) / m_responseLength;
            status = Status_Receiving;
        }
        break;
    case State_ChunkTrailer:
        status = Status_Receiving;
        percent = 100;
        break;
    case State_Cleanup:
        status = Status_Failed;
        percent = 100;
        break;
    case State_Done:
    case State_NotModified:
        status = Status_Done;
        percent = 100;
        break;
    }
    if (progress != NULL) {
        *progress = percent;
    }
    return status;
}

int CHttpTransport_gServe::GetResponseCode()
{
    return m_statusCode;
}

unsigned char* CHttpTransport_gServe::GetResponsePtr()
{
    return m_response;
}

unsigned int CHttpTransport_gServe::GetResponseLen()
{
    return m_responseLength;
}

void CHttpTransport_gServe::ClearResponse()
{
    if (m_response != NULL) {
        free(m_response);
        m_response = NULL;
    }
    m_responseLength = 0;
}

void CHttpTransport_gServe::ClearChunkList()
{
    core::CLinkListNode* chunk = m_chunks.GetHead();
    while (chunk != NULL) {
        chunk->RemoveFromList();
        delete chunk;
        chunk = m_chunks.GetHead();
    }
}

unsigned char CHttpTransport_gServe::ParseHTTPHeaderLine(const char* line)
{
    if (m_statusCode == 0) {
        // status line: "HTTP/1.1 200 OK"
        if (strncmp(line, cszHTTP, 5) != 0) {
            return 0;
        }
        char* code = strchr((char*)line + 5, ' ');
        if (code == NULL) {
            return 0;
        }
        char* codeEnd = strchr(code + 1, ' ');
        if (codeEnd == NULL) {
            return 0;
        }
        *codeEnd = 0;
        m_statusCode = atoi(code);
        return m_statusCode != 0;
    }

    // header fields are matched case-sensitively, the value follows the "Name: " prefix directly
    if (m_responseLength == 0 && !m_chunked) {
        if (strncmp(line, cszContentLength, sizeof(cszContentLength) - 1) == 0) {
            m_responseLength = atoi(line + sizeof(cszContentLength) - 1);
        } else if (strncmp(line, cszTransferEncoding, sizeof(cszTransferEncoding) - 1) == 0) {
            if (strcmp(line + sizeof(cszTransferEncoding) - 1, cszChunked) == 0) {
                m_chunked = 1;
            }
        }
    }

    if (m_statusCode >= 302 && m_statusCode <= 303) {
        if (strncmp(line, cszLocation, sizeof(cszLocation) - 1) == 0) {
            m_location = line + sizeof(cszLocation) - 1;
        }
    }
    return 1;
}

void CHttpTransport_gServe::ConsumeBuffer(unsigned int count)
{
    if (count == 0) {
        return;
    }
    if (m_bufferLength > count) {
        memmove(m_buffer, m_buffer + count, m_bufferLength - count);
        m_bufferLength = m_bufferLength - count;
    } else {
        m_bufferLength = 0;
    }
}

void CHttpTransport_gServe::ReceiveChunkedResponse()
{
    int error;
    m_state = State_ChunkHeader;
    if (m_statusCode != 200) {
        error = Error_BadStatus;
    } else if (m_bufferLength != 0) {
        ParseChunkHeader();
        error = 0;
    } else {
        error = ReceiveIntoBuffer();
    }
    AbortOnError(error);
}

unsigned char CHttpTransport_gServe::CreateResponseFromChunkedData()
{
    if (m_chunkedLength == 0) {
        return 1;
    }
    m_response = (unsigned char*)malloc(m_chunkedLength);
    if (m_response == NULL) {
        return 0;
    }
    unsigned int offset = 0;
    CHttpDataChunk_gServe* chunk = (CHttpDataChunk_gServe*)m_chunks.GetHead();
    while (chunk != NULL) {
        memcpy(m_response + offset, chunk->m_data, chunk->m_size);
        unsigned int size = chunk->m_size;
        chunk->RemoveFromList();
        delete chunk;
        offset += size;
        chunk = (CHttpDataChunk_gServe*)m_chunks.GetHead();
    }
    m_responseLength = m_chunkedLength;
    return 1;
}

void CHttpTransport_gServe::RedirectTo(const components::CStrChar& url)
{
    if (m_redirect == NULL) {
        return;
    }
    if (url.GetLength() < 1) {
        return;
    }
    // same location again: stop (two NULL strings count as equal)
    if (url == m_redirect->m_url) {
        return;
    }
    // repeat the request as a GET on the new location
    network::SwpDeviceInfo info;
    memset(&info, 0, sizeof(info));
    info.m_timeoutMillis = m_redirect->m_timeoutMillis;
    info.m_connectTimeoutMillis = m_redirect->m_connectTimeoutMillis;
    Send(info, url.GetCharPtr(), NULL, 0, m_redirect->m_keepAlive, m_redirect->m_userAgent.GetCharPtr(), NULL, cszGet, NULL);
}
