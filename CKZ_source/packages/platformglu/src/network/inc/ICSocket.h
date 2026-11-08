// ICSocket.h - asynchronous client socket of the platform layer: every request returns at once,
// HandleUpdate() advances it, IsBusy() / GetError() report the outcome.
#ifndef ICSOCKET_H
#define ICSOCKET_H

#include "CClass.h"

namespace com { namespace glu { namespace platform { namespace network {

class ICSocket : public core::CClass {
public:
    /** Runtime class id: CStringToKey("ICSocket"). */
    static const unsigned int ClassId = 0xd6595d28;

    enum eProtocol {
        Protocol_TCP = 0
    };

    /** Results of Connect / Send / Receive and values of GetError(). */
    enum eError {
        Error_None = 0,
        Error_InvalidArgument = 1,
        Error_OutOfMemory = 2,
        Error_HostNotFound = 0xf1,
        Error_NotConnected = 0xf2,
        Error_Timeout = 0xf3,
        Error_SocketCreate = 0xf4,
        Error_StreamOpen = 0xf5,
        Error_Connect = 0xf6,
        Error_Receive = 0xf7,
        Error_Send = 0xf8
    };

    ICSocket()
        : core::CClass(ClassId)
    {
    }

    /** Platform factory (CSocket_Cocoa). */
    static ICSocket* CreateInstance();

    virtual ~ICSocket() {}

    /** A connect/send/receive is still in progress. */
    virtual unsigned char IsBusy() const = 0;
    /** Error of the last operation, 0 = none, -1 while busy. */
    virtual int GetError() const = 0;
    /** Underlying OS socket error (errno) of the last failure. */
    virtual int GetSocketError() const = 0;
    virtual unsigned char* GetDataPtr() const = 0;
    /** Bytes moved by the last completed send/receive. */
    virtual unsigned int GetDataLen() const = 0;
    virtual int Connect(eProtocol protocol, const char* host, unsigned short port, int timeoutMillis, unsigned char secure) = 0;
    virtual void Disconnect() = 0;
    virtual int Send(const unsigned char* data, unsigned int length, int timeoutMillis) = 0;
    virtual int Receive(unsigned char* buffer, unsigned int capacity, int timeoutMillis) = 0;
    virtual void HandleUpdate(int elapsed) = 0;
};

} } } }

#endif
