// PROVISIONAL (created by b057x): owner src/bounty_hunter/Manager/CNewsFeedManager.cpp not written yet
// b078c: implementation recovered; the preceding PROVISIONAL comment is historical.
// CNewsFeedManager.h - news feed; only what src/general/App.cpp uses is declared.
// b078c: recovered tagged message records and the owning pointer vector.
#ifndef CNEWSFEEDMANAGER_H
#define CNEWSFEEDMANAGER_H

#include <stddef.h>
#include "CStrWChar.h"  // added by b047x
#include "Vector_ss.h"

/** Non-polymorphic message header; the +8 payload is interpreted by its type. */
struct SMessage {
    int m_type;       // 0 targeted, 1 mission invite, 4 gift, 5 server award
    int m_senderId;
    union {
        int m_messageId;
        bool m_highlighted;
        int m_commonCurrency;
        int m_missionId; // b081j: original Invite159b76 and manager12dc30 share the +8 payload.
    };
};

// Derived record names are inferred; allocation sizes/field offsets are verified.
struct SMissionInviteMessage : SMessage {}; // The original only does new 0xc; no trailing field is added.
struct STargettedMessage : SMessage {
    com::glu::platform::components::CStrWChar m_text; // 0x0c
};
struct SGiftReceivedMessage : SMessage {
    int m_itemIndex; // 0x0c
};
struct SServerAwardMessage : SMessage {
    int m_rareCurrency; // 0x0c
    int m_itemType;    // 0x10 left zero by the shipped factory
    int m_awardId;     // 0x14 left zero by the shipped factory
};

class CNewsFeedManager {
public:
    CNewsFeedManager();
    ~CNewsFeedManager();
    void AddTargettedMessage(int senderId, const com::glu::platform::components::CStrWChar& text, bool highlighted);  // added by b047x (flag: message type 1001)
    void AddMissionInviteMessage(int senderId, int missionId);  // added by b047x
    void AddGiftReceivedMessage(int senderId, int itemIndex, int messageId);  // added by b047x
    void AddServerAwardMessage(int commonCurrency, int rareCurrency, int itemType, int messageId);  // added by b047x (third argument always 0 in CNotificationHandler)
    void RemoveMessage(const SMessage* message);
    void RemoveMessage(const SMessage* message, int ignored);
    const SMessage* GetNewsAt(int index);

    // Former m_data00[0x10 - 0x0]: layout not known yet (size from App.cpp).
    Vector<const SMessage*> m_messages;
};

#endif
