// CHttpTransport.cpp - asynchronous HTTP/1.1 client (GET / POST, chunked responses, redirects).
#include "CHttpTransport.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "CHttpDataChunk.h"
#include "CWUtil.h"
#include "CGenUtil.h"
#include "ICStdUtil.h"

namespace com { namespace glu { namespace platform { namespace network {

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
static const char cszTransferEncoding[] = "Transfer-Encoding: ";
static const char cszChunked[] = "chunked";
static const char cszLocation[] = "Location: ";

CHttpTransport::CHttpTransport()
{
    m_socket = ICSocket::CreateInstance();
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
    m_location = (const char*)NULL;
    m_secure = 0;
}

CHttpTransport::~CHttpTransport()
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

void CHttpTransport::Cancel()
{
    m_socket->Disconnect();
    ResetForNextRequest();
}

void CHttpTransport::ResetForNextRequest()
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
    m_secure = 0;
}

int CHttpTransport::Get(const SwpDeviceInfo& info, const char* url, unsigned char keepAlive, const char* userAgent)
{
    return Send(info, url, NULL, 0, keepAlive, userAgent, NULL, cszGet);
}

int CHttpTransport::Send(const SwpDeviceInfo& info, const char* url, unsigned char* data, unsigned int dataLength,
                         unsigned char keepAlive, const char* userAgent, const char* contentType, const char* method)
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

    int result = CWUtil::ParseUrl(url, m_host, m_port, m_path, m_secure);
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
        m_requestHeader += cszCRLF;
    }

    m_state = State_Connect;
    return result;
}

int CHttpTransport::Post(const SwpDeviceInfo& info, const char* url, unsigned char* data, unsigned int dataLength,
                         unsigned char keepAlive, const char* userAgent, const char* contentType)
{
    if (data == NULL || dataLength == 0) {
        return 1;
    }
    return Send(info, url, data, dataLength, keepAlive, userAgent, contentType, cszPost);
}

unsigned char CHttpTransport::IsBusy()
{
    if (m_state == State_Idle) {
        return 0;
    }
    return m_state != State_Done;
}

void CHttpTransport::HandleUpdate(int elapsed)
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

unsigned char CHttpTransport::AbortOnError(int error)
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

void CHttpTransport::SendRequestHeader()
{
    m_state = State_SendingHeader;
    const char* header = m_requestHeader.GetCharPtr();
    AbortOnError(m_socket->Send((const unsigned char*)header, strlen(header), m_timeoutMillis));
}

void CHttpTransport::SendRequest()
{
    m_state = State_SendingBody;
    AbortOnError(m_socket->Send(m_requestData, m_requestDataLength, m_timeoutMillis));
}

void CHttpTransport::ReceiveResponseHeader()
{
    m_state = State_ReceivingHeader;
    AbortOnError(ReceiveIntoBuffer());
}

void CHttpTransport::ParseResponseHeader()
{
    int error = 0;
    unsigned char done = 0;
    int lineCount = 0;
    unsigned int parsedLength = 0;      // bytes of the lines handled so far
    char* buffer = m_buffer;

    buffer[m_bufferLength] = 0;
    char* lineStart = buffer;
    char* lineEnd = strstr(lineStart, cszCRLF);
    unsigned int lineEndOffset = lineEnd - buffer + 2;

    while (lineEnd != NULL && !done && error == 0) {
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
                        memcpy(m_response, buffer + lineEndOffset, available);
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
                lineStart = buffer + lineEndOffset;
                parsedLength = lineEndOffset;
                lineEnd = strstr(lineStart, cszCRLF);
                lineEndOffset = lineEnd - buffer + 2;
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

void CHttpTransport::ReceiveResponse()
{
    int error = Error_BadStatus;
    // 200, 201, 204 and the redirects 302, 303 carry a usable body
    switch (m_statusCode) {
    case 200:
    case 201:
    case 204:
    case 302:
    case 303:
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

void CHttpTransport::ParseChunkHeader()
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
                CHttpDataChunk* chunk = new CHttpDataChunk();
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

void CHttpTransport::ParseChunkData()
{
    int error = Error_BadResponse;
    m_buffer[m_bufferLength] = 0;
    CHttpDataChunk* chunk = (CHttpDataChunk*)m_chunks.GetTail();
    if (chunk != NULL) {
        unsigned int missing = chunk->m_size - chunk->m_received;
        unsigned int available = m_bufferLength;
        unsigned int count = available > missing ? missing : available;
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

void CHttpTransport::ParseChunkFooter()
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

void CHttpTransport::Connect()
{
    m_state = State_Connecting;
    AbortOnError(m_socket->Connect(ICSocket::Protocol_TCP, m_host.GetCharPtr(), m_port, m_connectTimeoutMillis, m_secure));
}

void CHttpTransport::Cleanup()
{
    if (!m_keepAlive || m_error != 0) {
        m_socket->Disconnect();
    }
    m_state = State_Done;
    if (m_statusCode >= 302 && m_statusCode <= 303) {
        // copy: the redirect resets m_location
        components::CStrChar location(m_location);
        RedirectTo(location);
    }
}

int CHttpTransport::GetStatus(int* progress)
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
        status = Status_Done;
        percent = 100;
        break;
    }
    if (progress != NULL) {
        *progress = percent;
    }
    return status;
}

int CHttpTransport::GetError()
{
    if (IsBusy()) {
        return -1;
    }
    return m_error;
}

unsigned char* CHttpTransport::GetResponsePtr()
{
    return m_response;
}

unsigned int CHttpTransport::GetResponseLen()
{
    return m_responseLength;
}

void CHttpTransport::DetachResponse()
{
    m_response = NULL;
    m_responseLength = 0;
}

void CHttpTransport::ClearResponse()
{
    if (m_response != NULL) {
        free(m_response);
        m_response = NULL;
    }
    m_responseLength = 0;
}

void CHttpTransport::ClearChunkList()
{
    core::CLinkListNode* chunk = m_chunks.GetHead();
    while (chunk != NULL) {
        chunk->RemoveFromList();
        delete chunk;
        chunk = m_chunks.GetHead();
    }
}

unsigned char CHttpTransport::ParseHTTPHeaderLine(const char* line)
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

    if (m_responseLength == 0 && !m_chunked) {
        const char* value = GetHTTPHeaderFieldValue(line, cszContentLength);
        if (value != NULL) {
            m_responseLength = atoi(value);
        } else {
            value = GetHTTPHeaderFieldValue(line, cszTransferEncoding);
            if (value != NULL && strcmp(value, cszChunked) == 0) {
                m_chunked = 1;
            }
        }
    }

    if (m_statusCode >= 302 && m_statusCode <= 303) {
        const char* location = GetHTTPHeaderFieldValue(line, cszLocation);
        if (location != NULL) {
            m_location = location;
        }
    }
    return 1;
}

const char* CHttpTransport::GetHTTPHeaderFieldValue(const char* line, const char* fieldName)
{
    if (line == NULL || fieldName == NULL) {
        return NULL;
    }

    // trim the field name
    while (*fieldName == ' ') {
        fieldName++;
    }
    int length = strlen(fieldName);
    while (length > 0 && fieldName[length - 1] == ' ') {
        length--;
    }

    // case-insensitive prefix match
    do {
        if (*line == 0) {
            return NULL;
        }
        if ((unsigned char)tolower(*line) != (unsigned char)tolower(*fieldName)) {
            return NULL;
        }
        length--;
        fieldName++;
        line++;
    } while (length != 0);

    // skip the blanks before the value
    while (*line == ' ') {
        line++;
    }
    return line;
}

void CHttpTransport::ConsumeBuffer(unsigned int count)
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

void CHttpTransport::ReceiveChunkedResponse()
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

unsigned char CHttpTransport::CreateResponseFromChunkedData()
{
    if (m_chunkedLength == 0) {
        return 1;
    }
    m_response = (unsigned char*)malloc(m_chunkedLength);
    if (m_response == NULL) {
        return 0;
    }
    unsigned int offset = 0;
    CHttpDataChunk* chunk = (CHttpDataChunk*)m_chunks.GetHead();
    while (chunk != NULL) {
        memcpy(m_response + offset, chunk->m_data, chunk->m_size);
        unsigned int size = chunk->m_size;
        chunk->RemoveFromList();
        delete chunk;
        offset += size;
        chunk = (CHttpDataChunk*)m_chunks.GetHead();
    }
    m_responseLength = m_chunkedLength;
    return 1;
}

void CHttpTransport::RedirectTo(const components::CStrChar& url)
{
    if (m_redirect == NULL) {
        return;
    }
    if (url.GetLength() < 1) {
        return;
    }
    // same location again: stop (two NULL strings count as equal)
    const char* newUrl = url.GetCharPtr();
    const char* oldUrl = m_redirect->m_url.GetCharPtr();
    bool different;
    if (newUrl != NULL && oldUrl != NULL) {
        different = strcmp(newUrl, oldUrl) != 0;
    } else {
        different = newUrl != oldUrl;
    }
    if (!different) {
        return;
    }
    // repeat the request as a GET on the new location
    SwpDeviceInfo info;
    memset(&info, 0, sizeof(info));
    info.m_timeoutMillis = m_redirect->m_timeoutMillis;
    info.m_connectTimeoutMillis = m_redirect->m_connectTimeoutMillis;
    Send(info, url.GetCharPtr(), NULL, 0, m_redirect->m_keepAlive, m_redirect->m_userAgent.GetCharPtr(), NULL, cszGet);
}

} } } }
