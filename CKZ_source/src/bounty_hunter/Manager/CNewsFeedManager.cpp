// CNewsFeedManager.cpp - transient game/social messages.
#include "CNewsFeedManager.h"

CNewsFeedManager::CNewsFeedManager()
{
    // The vector's default constructor is the complete shipped initialization.
}

CNewsFeedManager::~CNewsFeedManager()
{
    // Original deletes through the non-polymorphic header, without destroying derived text.
    SAFE_DELETE_VECTOR(m_messages);
}

void CNewsFeedManager::AddTargettedMessage(int senderId, const com::glu::platform::components::CStrWChar& text, bool highlighted)
{
    STargettedMessage* message = new STargettedMessage();
    message->m_type = 0;
    message->m_senderId = senderId;
    message->m_highlighted = highlighted;
    message->m_text = text;
    const SMessage* added = message;
    m_messages.addElement(added);
}

void CNewsFeedManager::RemoveMessage(const SMessage* message)
{
    // The original first checks membership, then repeats the search; it does not delete the message.
    if (m_messages.indexOf(message, 0) != -1) {
        m_messages.removeElementAt(m_messages.indexOf(message, 0));
    }
}

void CNewsFeedManager::RemoveMessage(const SMessage* message, int ignored)
{
    RemoveMessage(message);
}

const SMessage* CNewsFeedManager::GetNewsAt(int index)
{
    return m_messages[index];
}

void CNewsFeedManager::AddMissionInviteMessage(int senderId, int missionId)
{
    // The shipped factory ignores missionId and leaves the third word zero.
    SMessage* message = new SMessage();
    message->m_type = 1;
    message->m_senderId = senderId;
    const SMessage* added = message;
    m_messages.addElement(added);
}

void CNewsFeedManager::AddGiftReceivedMessage(int senderId, int itemIndex, int messageId)
{
    SGiftReceivedMessage* message = new SGiftReceivedMessage();
    message->m_type = 4;
    message->m_senderId = senderId;
    message->m_messageId = messageId;
    message->m_itemIndex = itemIndex;
    const SMessage* added = message;
    m_messages.addElement(added);
}

void CNewsFeedManager::AddServerAwardMessage(int commonCurrency, int rareCurrency, int itemType, int messageId)
{
    // The last two inputs are ignored in the original 0x12dc90 factory.
    SServerAwardMessage* message = new SServerAwardMessage();
    message->m_type = 5;
    message->m_commonCurrency = commonCurrency;
    message->m_rareCurrency = rareCurrency;
    const SMessage* added = message;
    m_messages.addElement(added);
}
