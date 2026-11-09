// CSocket_Cocoa.cpp - asynchronous TCP client socket for iOS.
// Connect: open a CFWriteStream to the host (wakes up the radio), then resolve the address and
// connect() a non-blocking BSD socket; Send / Receive poll with select() on every update.
#include "CSocket_Cocoa.h"

#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>

using namespace com::glu::platform::network;

/** Socket option set before every send (two ints, value {1, 1}); original constant. */
static const int kSendSocketOption = 0x10a3;
/** Idle time before an unused connection is closed. */
static const int kKeepAliveMillis = 30000;

ICSocket* ICSocket::CreateInstance()
{
    return new CSocket_Cocoa();
}

CSocket_Cocoa::CSocket_Cocoa()
{
    m_hostName = NULL;
    m_host = NULL;
    m_writeStream = NULL;
    m_socket = -1;
    m_hostString = "";
    m_port = 0;
    m_address = 0;
    m_connectedPort = 0;
    m_sslContext = NULL;
    m_ssl = NULL;
    ClearRequest();
    m_secure = 0;
}

CSocket_Cocoa::~CSocket_Cocoa()
{
    Disconnect();
}

unsigned char CSocket_Cocoa::IsBusy() const
{
    return m_state != State_Idle && m_state != State_Done;
}

int CSocket_Cocoa::GetError() const
{
    if (IsBusy()) {
        return -1;
    }
    return m_error;
}

int CSocket_Cocoa::GetSocketError() const
{
    return m_socketError;
}

unsigned char* CSocket_Cocoa::GetDataPtr() const
{
    return m_data;
}

unsigned int CSocket_Cocoa::GetDataLen() const
{
    return m_transferred;
}

int CSocket_Cocoa::Connect(eProtocol protocol, const char* host, unsigned short port, int timeoutMillis,
                           unsigned char secure)
{
    if (IsBusy()) {
        return -1;
    }

    ClearRequest();
    m_hostString = host;
    m_port = port;
    m_timeoutMillis = timeoutMillis;
    m_secure = secure;

    // Open a stream to the host first: this brings up the network interface
    HandleCFStreamDisconnect();
    m_hostName = CFStringCreateWithCString(NULL, host, kCFStringEncodingUTF8);
    if (m_hostName == NULL) {
        return Error_OutOfMemory;
    }
    m_host = CFHostCreateWithName(NULL, m_hostName);
    if (m_host == NULL) {
        return Error_OutOfMemory;
    }
    CFStreamCreatePairWithSocketToCFHost(NULL, m_host, port, NULL, &m_writeStream);
    if (m_writeStream == NULL) {
        return Error_OutOfMemory;
    }
    if (!CFWriteStreamOpen(m_writeStream)) {
        return Error_StreamOpen;
    }
    m_state = State_OpeningStream;
    return Error_None;
}

void CSocket_Cocoa::HandleCFStreamDisconnect()
{
    if (m_writeStream != NULL) {
        CFWriteStreamClose(m_writeStream);
        CFRelease(m_writeStream);
        m_writeStream = NULL;
    }
    if (m_host != NULL) {
        CFRelease(m_host);
        m_host = NULL;
    }
    if (m_hostName != NULL) {
        CFRelease(m_hostName);
        m_hostName = NULL;
    }
}

void CSocket_Cocoa::Disconnect()
{
    HandleSocketDisconnect();
    HandleCFStreamDisconnect();
    ClearRequest();
}

void CSocket_Cocoa::HandleSocketDisconnect()
{
    if (m_socket == -1) {
        return;
    }
    TearDownSsl();
    shutdown(m_socket, SHUT_RDWR);
    close(m_socket);
    m_socket = -1;
}

int CSocket_Cocoa::Send(const unsigned char* data, unsigned int length, int timeoutMillis)
{
    if (IsBusy()) {
        return -1;
    }

    ClearRequest();
    if (m_socket == -1) {
        return Error_NotConnected;
    }
    if (data == NULL || length == 0) {
        return Error_InvalidArgument;
    }
    m_state = State_Sending;
    m_data = (unsigned char*)data;
    m_length = length;
    m_timeoutMillis = timeoutMillis;
    return Error_None;
}

int CSocket_Cocoa::Receive(unsigned char* buffer, unsigned int capacity, int timeoutMillis)
{
    if (IsBusy()) {
        return -1;
    }

    ClearRequest();
    if (m_socket == -1) {
        return Error_NotConnected;
    }
    if (buffer == NULL) {
        return Error_InvalidArgument;
    }
    m_state = State_Receiving;
    m_data = buffer;
    m_length = capacity;
    m_timeoutMillis = timeoutMillis;
    return Error_None;
}

void CSocket_Cocoa::HandleUpdate(int elapsed)
{
    if (IsBusy()) {
        // Request timeout
        if (m_timeoutMillis > 0) {
            m_timeoutMillis = m_timeoutMillis - elapsed;
            if (m_timeoutMillis <= 0) {
                m_timeoutMillis = 0;
                m_error = Error_Timeout;
                m_state = State_Finishing;
            }
        }

        switch (m_state) {
        case State_SslConnecting:
            OnSslConnect();
            break;
        case State_OpeningStream: {
            CFStreamStatus status = CFWriteStreamGetStatus(m_writeStream);
            if (status == kCFStreamStatusOpening) {
                break;
            }
            if (status == kCFStreamStatusOpen) {
                m_state = State_StreamOpen;
            } else {
                m_error = Error_NotConnected;
                m_state = State_Finishing;
            }
            break;
        }
        case State_StreamOpen:
            OnCFStreamConnectFinished();
            break;
        case State_Connecting:
            OnConnect();
            break;
        case State_Sending:
            OnSend();
            break;
        case State_Receiving:
            OnReceive();
            break;
        case State_Finishing:
            Cleanup();
            break;
        default:
            break;
        }
    } else {
        // Keep-alive: close an idle connection
        if (m_timeoutMillis > 0) {
            m_timeoutMillis = m_timeoutMillis - elapsed;
            if (m_timeoutMillis <= 0) {
                m_timeoutMillis = 0;
                HandleSocketDisconnect();
                HandleCFStreamDisconnect();
            }
        }
    }
}

void CSocket_Cocoa::OnSslConnect()
{
    if (!m_secure) {
        return;
    }

    int result = SSL_connect(m_ssl);
    if (result == -1) {
        // Handshake still in progress
        return;
    }
    if (result == 1) {
        m_state = State_Finishing;
        return;
    }
    m_error = Error_NotConnected;
}

void CSocket_Cocoa::OnCFStreamConnectFinished()
{
    int error = Error_None;

    struct hostent* hostEntry = gethostbyname(m_hostString.GetCharPtr());
    if (hostEntry == NULL) {
        // Maybe a dotted address without a reverse entry
        in_addr_t address = inet_addr(m_hostString.GetCharPtr());
        if (address != INADDR_NONE) {
            hostEntry = gethostbyaddr(&address, sizeof(address), AF_INET);
        }
    }

    if (hostEntry == NULL) {
        error = Error_HostNotFound;
    } else {
        unsigned int resolvedAddress = *(unsigned int*)hostEntry->h_addr_list[0];

        // A kept-alive socket to another endpoint cannot be reused
        if (m_socket != -1 && (m_address != resolvedAddress || m_connectedPort != m_port)) {
            HandleSocketDisconnect();
        }
        m_address = resolvedAddress;
        m_connectedPort = m_port;

        if (m_socket == -1) {
            m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        }

        if (m_socket == -1) {
            error = Error_SocketCreate;
        } else {
            int nonBlocking = 1;
            if (ioctl(m_socket, FIONBIO, &nonBlocking) != 0) {
                error = Error_Connect;
            }
        }

        if (error == Error_None) {
            struct sockaddr_in socketAddress;
            socketAddress.sin_family = AF_INET;
            socketAddress.sin_addr.s_addr = m_address;
            socketAddress.sin_port = htons(m_connectedPort);
            int result = connect(m_socket, (struct sockaddr*)&socketAddress, sizeof(socketAddress));
            if (result != 0) {
                if (result == -1 && errno == EINPROGRESS) {
                    m_state = State_Connecting;
                } else {
                    error = Error_Connect;
                }
            }
        }
    }

    if (error != Error_None) {
        m_state = State_Finishing;
        m_error = error;
    }
}

void CSocket_Cocoa::OnConnect()
{
    int socketHandle = m_socket;
    if (socketHandle == -1) {
        m_error = Error_NotConnected;
    } else {
        fd_set readSet;
        fd_set writeSet;
        fd_set errorSet;
        struct timeval timeout;
        FD_ZERO(&readSet);
        FD_ZERO(&writeSet);
        FD_ZERO(&errorSet);
        FD_SET(socketHandle, &readSet);
        FD_SET(socketHandle, &writeSet);
        FD_SET(socketHandle, &errorSet);
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;

        int result = select(socketHandle + 1, &readSet, &writeSet, &errorSet, &timeout);
        if (result > 0) {
            if (FD_ISSET(m_socket, &writeSet)) {
                // Connected
                if (m_secure) {
                    SetupSsl();
                    m_state = State_SslConnecting;
                    return;
                }
            } else {
                int socketError = 0;
                socklen_t socketErrorLength = sizeof(socketError);
                getsockopt(m_socket, SOL_SOCKET, SO_ERROR, &socketError, &socketErrorLength);
                m_error = Error_Connect;
            }
        } else if (result == 0) {
            // Still connecting
            return;
        } else if (result == -1) {
            m_error = Error_Connect;
        }
    }

    m_state = State_Finishing;
}

void CSocket_Cocoa::OnSend()
{
    int socketHandle = m_socket;
    if (socketHandle == -1) {
        m_error = Error_NotConnected;
        m_state = State_Finishing;
        return;
    }

    fd_set readSet;
    fd_set writeSet;
    fd_set errorSet;
    struct timeval timeout;
    int option[2];
    FD_ZERO(&readSet);
    FD_ZERO(&writeSet);
    FD_ZERO(&errorSet);
    timeout.tv_sec = 0;
    FD_SET(socketHandle, &readSet);
    option[0] = 1;
    option[1] = 1;
    FD_SET(socketHandle, &writeSet);
    FD_SET(socketHandle, &errorSet);
    timeout.tv_usec = 0;

    if (setsockopt(socketHandle, SOL_SOCKET, kSendSocketOption, option, sizeof(option)) < 0 && errno == EINVAL) {
        m_state = State_Finishing;
        m_error = Error_NotConnected;
        return;
    }

    int result = select(m_socket + 1, &readSet, &writeSet, &errorSet, &timeout);
    if (result < 1 || !FD_ISSET(m_socket, &writeSet)) {
        m_state = State_Finishing;
        m_error = Error_NotConnected;
        return;
    }

    bool wouldBlock = false;
    int sent;
    if (m_secure) {
        sent = SSL_write(m_ssl, m_data + m_transferred, m_length - m_transferred);
        int sslError = SSL_get_error(m_ssl, sent);
        if (sslError == SSL_ERROR_WANT_READ || sslError == SSL_ERROR_WANT_WRITE) {
            wouldBlock = true;
        }
    } else {
        sent = send(m_socket, m_data + m_transferred, m_length - m_transferred, 0);
    }

    if (sent > 0) {
        m_transferred = m_transferred + sent;
        if (m_transferred >= m_length) {
            m_state = State_Finishing;
        }
        return;
    }
    if (sent == -1) {
        if (wouldBlock || errno == EAGAIN) {
            return;
        }
        m_error = Error_Send;
        m_state = State_Finishing;
        return;
    }
    if (sent == 0) {
        m_error = Error_NotConnected;
        m_state = State_Finishing;
        return;
    }
    m_state = State_Finishing;
}

void CSocket_Cocoa::OnReceive()
{
    int socketHandle = m_socket;
    if (socketHandle == -1) {
        m_error = Error_NotConnected;
        m_state = State_Finishing;
        return;
    }

    fd_set readSet;
    fd_set writeSet;
    fd_set errorSet;
    struct timeval timeout;
    FD_ZERO(&readSet);
    FD_ZERO(&writeSet);
    FD_ZERO(&errorSet);
    timeout.tv_sec = 0;
    FD_SET(socketHandle, &readSet);
    FD_SET(socketHandle, &writeSet);
    FD_SET(socketHandle, &errorSet);
    timeout.tv_usec = 0;

    int result = select(socketHandle + 1, &readSet, &writeSet, &errorSet, &timeout);
    if (result <= 0) {
        m_state = State_Finishing;
        m_error = Error_NotConnected;
        return;
    }

    bool wouldBlock = false;
    int received;
    if (m_secure) {
        received = SSL_read(m_ssl, m_data + m_transferred, m_length - m_transferred);
        int sslError = SSL_get_error(m_ssl, received);
        if (sslError == SSL_ERROR_WANT_READ || sslError == SSL_ERROR_WANT_WRITE) {
            wouldBlock = true;
        }
    } else {
        received = recv(m_socket, m_data + m_transferred, m_length - m_transferred, 0);
    }

    if (received > 0) {
        m_transferred = m_transferred + received;
        m_state = State_Finishing;
        return;
    }
    if (received == -1) {
        if (wouldBlock || errno == EAGAIN) {
            return;
        }
        m_error = Error_Receive;
        m_state = State_Finishing;
        return;
    }
    if (received == 0) {
        m_error = Error_NotConnected;
        m_state = State_Finishing;
        return;
    }
    m_state = State_Finishing;
}

void CSocket_Cocoa::Cleanup()
{
    if (m_error != Error_None) {
        m_socketError = errno;
        HandleSocketDisconnect();
        HandleCFStreamDisconnect();
    } else {
        // Success: keep the connection for the next request
        m_timeoutMillis = kKeepAliveMillis;
    }
    m_state = State_Done;
}

void CSocket_Cocoa::TearDownSsl()
{
    if (!m_secure) {
        return;
    }
    if (m_ssl != NULL) {
        SSL_shutdown(m_ssl);
        SSL_free(m_ssl);
    }
    if (m_sslContext != NULL) {
        SSL_CTX_free(m_sslContext);
    }
    m_sslContext = NULL;
    m_ssl = NULL;
    m_secure = 0;
}

int CSocket_Cocoa::SetupSsl()
{
    int result = Error_None;
    if (m_secure) {
        SSL_library_init();
        result = Error_Connect;
        SSL_load_error_strings();
        m_sslContext = SSL_CTX_new(SSLv23_client_method());
        if (m_sslContext != NULL) {
            m_ssl = SSL_new(m_sslContext);
            if (m_ssl != NULL) {
                int status = SSL_set_fd(m_ssl, m_socket);
                result = status == 1 ? Error_None : Error_Connect;
            }
        }
    }
    return result;
}
