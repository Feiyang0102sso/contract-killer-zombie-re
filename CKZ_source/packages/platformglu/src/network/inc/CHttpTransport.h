// CHttpTransport.h - minimal asynchronous HTTP/1.1 client on top of ICSocket.
// A request runs as a state machine advanced by HandleUpdate(); the response body is kept in
// one malloc'd block (chunked transfers are reassembled from CHttpDataChunk nodes).
#ifndef CHTTPTRANSPORT_H
#define CHTTPTRANSPORT_H

#include "CStrChar.h"
#include "CLinkList.h"
#include "ICSocket.h"

namespace com { namespace glu { namespace platform { namespace network {

/** Connection settings of a request (only the two timeouts are read by the transport). */
struct SwpDeviceInfo {
    unsigned int m_reserved[6];         // 0x00 not used by CHttpTransport
    int m_timeoutMillis;                // 0x18 send / receive timeout
    int m_connectTimeoutMillis;         // 0x1c connect timeout
    unsigned int m_reserved20[4];       // 0x20 not used by CHttpTransport
};

class CHttpTransport {
public:
    /** Request state machine (m_state). */
    enum State {
        State_Idle = 0,
        State_Connect = 1,              // Connect() on the next update
        State_Connecting = 2,
        State_SendingHeader = 3,
        State_SendingBody = 4,
        State_ReceivingHeader = 5,
        State_ParsingHeader = 6,
        State_ReceivingBody = 7,
        State_ChunkHeader = 8,
        State_ChunkData = 9,
        State_ChunkFooter = 10,
        State_ChunkTrailer = 11,        // chunked body complete, trailer headers follow
        State_Cleanup = 12,             // finished or failed: Cleanup() on the next update
        State_Done = 13
    };

    /** GetStatus() results. */
    enum Status {
        Status_Idle = 0,
        Status_Sending = 3,
        Status_Receiving = 4,
        Status_Failed = 5,
        Status_Done = 6
    };

    /** Error codes passed to AbortOnError() by the transport itself. */
    enum Error {
        Error_None = 0,
        Error_OutOfMemory = 2,
        Error_BadStatus = 4,
        Error_BadResponse = 5
    };

    enum { BufferSize = 0x800 };

    /** What a request needs to be repeated for a redirect (Location header). */
    struct RedirectInfo {
        int m_timeoutMillis;                    // 0x00
        int m_connectTimeoutMillis;             // 0x04
        components::CStrChar m_url;             // 0x08
        unsigned char m_keepAlive;              // 0x18
        components::CStrChar m_userAgent;       // 0x1c
        components::CStrChar m_method;          // 0x2c
    };

    CHttpTransport();
    virtual ~CHttpTransport();

    /** Abort the running request. */
    void Cancel();
    void ResetForNextRequest();
    int Get(const SwpDeviceInfo& info, const char* url, unsigned char keepAlive, const char* userAgent);
    /** Start a request; -1 while busy, 1 for a bad url, 0 when started. */
    int Send(const SwpDeviceInfo& info, const char* url, unsigned char* data, unsigned int dataLength,
             unsigned char keepAlive, const char* userAgent, const char* contentType, const char* method);
    int Post(const SwpDeviceInfo& info, const char* url, unsigned char* data, unsigned int dataLength,
             unsigned char keepAlive, const char* userAgent, const char* contentType);
    unsigned char IsBusy();
    void HandleUpdate(int elapsed);
    /** Switch to State_Cleanup when `error` is set; returns 1 then. */
    unsigned char AbortOnError(int error);
    void SendRequestHeader();
    void SendRequest();
    void ReceiveResponseHeader();
    void ParseResponseHeader();
    void ReceiveResponse();
    void ParseChunkHeader();
    void ParseChunkData();
    void ParseChunkFooter();
    void Connect();
    void Cleanup();
    /** Status of the request; `progress` (may be NULL) receives a percentage. */
    int GetStatus(int* progress);
    int GetError();
    unsigned char* GetResponsePtr();
    unsigned int GetResponseLen();
    /** Hand the response block over to the caller (it is not freed any more). */
    void DetachResponse();
    void ClearResponse();
    void ClearChunkList();
    unsigned char ParseHTTPHeaderLine(const char* line);
    /** Value of `line` when it is the header `fieldName` (case-insensitive), else NULL. */
    const char* GetHTTPHeaderFieldValue(const char* line, const char* fieldName);
    /** Drop `count` bytes from the front of the receive buffer. */
    void ConsumeBuffer(unsigned int count);
    void ReceiveChunkedResponse();
    unsigned char CreateResponseFromChunkedData();
    void RedirectTo(const components::CStrChar& url);

private:
    /** Receive into the free part of m_buffer. */
    int ReceiveIntoBuffer()
    {
        return m_socket->Receive((unsigned char*)m_buffer + m_bufferLength, BufferSize - m_bufferLength, m_timeoutMillis);
    }

public:
    ICSocket* m_socket;                         // 0x004
    components::CStrChar m_host;                // 0x008
    unsigned short m_port;                      // 0x018
    components::CStrChar m_path;                // 0x01c
    components::CStrChar m_requestHeader;       // 0x02c method line and header fields
    int m_timeoutMillis;                        // 0x03c
    int m_connectTimeoutMillis;                 // 0x040
    unsigned char* m_requestData;               // 0x044 body to send (not owned)
    unsigned int m_requestDataLength;           // 0x048
    unsigned char m_keepAlive;                  // 0x04c keep the connection after the request
    unsigned char* m_response;                  // 0x050 response body (malloc'd)
    unsigned int m_responseLength;              // 0x054 Content-Length / size of m_response
    int m_statusCode;                           // 0x058 HTTP status of the response
    RedirectInfo* m_redirect;                   // 0x05c
    components::CStrChar m_location;            // 0x060 Location header of a 302/303
    unsigned char m_chunked;                    // 0x070 Transfer-Encoding: chunked
    core::CLinkList m_chunks;                   // 0x074 CHttpDataChunk list
    unsigned int m_chunkedLength;               // 0x084 total size of the announced chunks
    char m_buffer[BufferSize];                  // 0x088 receive buffer
    unsigned int m_bufferLength;                // 0x888 bytes in m_buffer
    int m_state;                                // 0x88c State
    int m_error;                                // 0x890 error passed to AbortOnError()
    int m_socketError;                          // 0x894 ICSocket::GetSocketError() at the abort
    int m_reserved898;                          // 0x898 only ever reset
    unsigned char m_secure;                     // 0x89c https
};

} } } }

#endif
