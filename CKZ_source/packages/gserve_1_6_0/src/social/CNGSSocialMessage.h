// CNGSSocialMessage.h - a post for a social network, built from string parameters ("a/b/c" paths).
// Owned by social/CNGSSocialMessage.cpp (b027, written by b027x). Layout from the ObjC type encoding
// {CNGSSocialMessage=^^?^v}: vtable and one pointer (the network's parameter dictionary).
#ifndef CNGSSOCIALMESSAGE_H
#define CNGSSOCIALMESSAGE_H

#include <stddef.h>
#include "CStrWChar.h"
#include "CNGSUserCredentials.h"

class CNGSSocialMessage {
public:
    CNGSSocialMessage();
    virtual ~CNGSSocialMessage();

    /** Set (value != NULL) or remove parameter `key`; 1 when the key is accepted. */
    virtual unsigned char SetParam(const char* key, const wchar_t* value) = 0;
    /** Read parameter `key` into `value`; 1 when the node of the key exists. */
    virtual unsigned char GetParam(const char* key, com::glu::platform::components::CStrWChar& value) = 0;
    /** Request parameters of the network (NSMutableDictionary* on iOS). */
    virtual void* GetMessage() = 0;
    virtual com::glu::platform::components::CStrWChar GetTargetUserId() = 0;

    /** Message of network `type` (only Facebook has one), NULL otherwise. */
    static CNGSSocialMessage* CreateInstance(eSocialNetworkType type);

    void* m_message;    // +0x04 network specific parameter storage
};

#endif
