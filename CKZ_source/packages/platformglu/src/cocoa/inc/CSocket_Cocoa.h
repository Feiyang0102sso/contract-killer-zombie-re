// CSocket_Cocoa.h - non-blocking BSD socket (optional OpenSSL); host resolution goes through a
// CFStream to the host first so that the cellular radio is brought up.
#ifndef CSOCKET_COCOA_H
#define CSOCKET_COCOA_H

#include <CoreFoundation/CoreFoundation.h>
#include <CFNetwork/CFNetwork.h>
#include "ICSocket.h"
#include "CStrChar.h"

typedef struct ssl_st SSL;
typedef struct ssl_ctx_st SSL_CTX;

class CSocket_Cocoa : public com::glu::platform::network::ICSocket {
public:
    /** Request progress (m_state); HandleUpdate dispatches on it. */
    enum State {
        State_Idle = 0,
        State_OpeningStream = 1,        // CFWriteStream to the host is opening
        State_StreamOpen = 2,           // resolve the address, start connect()
        State_Connecting = 3,
        State_Sending = 4,
        State_Receiving = 5,
        State_Finishing = 6,            // Cleanup() on the next update
        State_Done = 7,
        State_SslConnecting = 8
    };

    CSocket_Cocoa();
    virtual ~CSocket_Cocoa();

    virtual unsigned char IsBusy() const;
    virtual int GetError() const;
    virtual int GetSocketError() const;
    virtual unsigned char* GetDataPtr() const;
    virtual unsigned int GetDataLen() const;
    virtual int Connect(eProtocol protocol, const char* host, unsigned short port, int timeoutMillis, unsigned char secure);
    virtual void Disconnect();
    virtual int Send(const unsigned char* data, unsigned int length, int timeoutMillis);
    virtual int Receive(unsigned char* buffer, unsigned int capacity, int timeoutMillis);
    virtual void HandleUpdate(int elapsed);

private:
    /** Forget the previous request (data, progress, errors, timeout). */
    inline void ClearRequest()
    {
        m_data = NULL;
        m_length = 0;
        m_transferred = 0;
        m_state = State_Idle;
        m_error = Error_None;
        m_socketError = 0;
        m_timeoutMillis = 0;
    }

    void HandleCFStreamDisconnect();
    void HandleSocketDisconnect();
    void OnSslConnect();
    void OnCFStreamConnectFinished();
    void OnConnect();
    void OnSend();
    void OnReceive();
    void Cleanup();
    void TearDownSsl();
    int SetupSsl();

    CFStringRef m_hostName;                             // 0x08
    CFHostRef m_host;                                   // 0x0c
    CFWriteStreamRef m_writeStream;                     // 0x10
    int m_socket;                                       // 0x14 -1 when closed
    com::glu::platform::components::CStrChar m_hostString;  // 0x18
    unsigned short m_port;                              // 0x28 requested port
    unsigned int m_address;                             // 0x2c connected IPv4 address (network order)
    unsigned short m_connectedPort;                     // 0x30
    unsigned char* m_data;                              // 0x34 send / receive buffer
    unsigned int m_length;                              // 0x38
    unsigned int m_transferred;                         // 0x3c
    int m_state;                                        // 0x40 State
    int m_error;                                        // 0x44 eError
    int m_socketError;                                  // 0x48 errno
    int m_timeoutMillis;                                // 0x4c busy: request timeout, idle: keep-alive
    unsigned char m_secure;                             // 0x50 use SSL
    SSL_CTX* m_sslContext;                              // 0x54
    SSL* m_ssl;                                         // 0x58
};

#endif
