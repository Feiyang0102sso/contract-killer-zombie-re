// CNGSSocialMessage.cpp - base of the social network messages and their factory.
#include "CNGSSocialMessage.h"
#include "CFacebookMessage.h"

CNGSSocialMessage::CNGSSocialMessage()
{
    m_message = NULL;
}

CNGSSocialMessage::~CNGSSocialMessage()
{
}

CNGSSocialMessage* CNGSSocialMessage::CreateInstance(eSocialNetworkType type)
{
    CNGSSocialMessage* message = NULL;
    if (type == SOCIAL_NETWORK_FACEBOOK) {
        message = new CFacebookMessage();
    }
    return message;
}
