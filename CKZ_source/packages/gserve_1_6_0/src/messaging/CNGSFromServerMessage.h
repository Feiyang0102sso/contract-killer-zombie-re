// CNGSFromServerMessage.h - messages pushed by the gServe server ("envelopes" of the message
// poll). The plain message carries ids and two texts; the JSON messages parse their text as a
// JSON object (gift, award, prompt, launch URL, or several of them at once).
// Layout of CNGSMessagePrompt confirmed by the ObjC type encoding of
// CGenericButtonViewDialog.m_pPromptMessage: ^^? q q q CStrWChar CStrWChar q i q q CStrWChar ^ ^ C.
// (b023x; created as PROVISIONAL by b016x)
#ifndef CNGSFROMSERVERMESSAGE_H
#define CNGSFROMSERVERMESSAGE_H

#include <stddef.h>
#include "CStrWChar.h"
#include "CVector.h"
#include "CApplet.h"
#include "ICCore.h"

class CObjectMapValue;

/** A message of the local user's server message queue. Everything is inline (weak vtable). */
class CNGSFromServerMessage {
public:
    /** Kind of the message object (m_subType). */
    enum CNGS_MSG_SUBTYPE {
        CNGS_MSG_SUBTYPE_ANY = 0,           // GetAllMessagesByUserAndType: no filter
        CNGS_MSG_SUBTYPE_PLAIN = 1,
        CNGS_MSG_SUBTYPE_JSON = 3,
        CNGS_MSG_SUBTYPE_AWARD = 4,
        CNGS_MSG_SUBTYPE_GIFT = 5,
        CNGS_MSG_SUBTYPE_LAUNCH_URL = 6,
        CNGS_MSG_SUBTYPE_PROMPT = 7,
        CNGS_MSG_SUBTYPE_MULTIPLE = 8
    };

    CNGSFromServerMessage()
        : m_messageId(0)
        , m_senderId(0)
        , m_recipientId(0)
        , m_senderGamertag("")
        , m_messageText("")
        , m_type(0)
        , m_subType(CNGS_MSG_SUBTYPE_PLAIN)
    {
    }

    virtual ~CNGSFromServerMessage() {}

    /** Fill in the envelope fields. */
    virtual void Init(long long messageId, long long type, long long senderId, long long recipientId,
                      com::glu::platform::components::CStrWChar senderGamertag,
                      com::glu::platform::components::CStrWChar messageText)
    {
        m_messageId = messageId;
        m_type = type;
        m_senderId = senderId;
        m_recipientId = recipientId;
        m_senderGamertag = senderGamertag;
        m_messageText = messageText;
    }

    /** -1: not valid yet, 1: expired, 0: valid now (always valid here). */
    virtual int CheckTime()
    {
        return 0;
    }

    virtual com::glu::platform::components::CStrWChar GetMessageText() const
    {
        return m_messageText;
    }

    /** Carry the message out; returns whether it was consumed. */
    virtual unsigned char Apply()
    {
        return 1;
    }

    long long m_messageId;                                          // +0x04
    long long m_senderId;                                           // +0x0c
    long long m_recipientId;                                        // +0x14
    com::glu::platform::components::CStrWChar m_senderGamertag;    // +0x1c
    com::glu::platform::components::CStrWChar m_messageText;       // +0x2c
    long long m_type;                                               // +0x3c server message type
    int m_subType;                                                  // +0x44 CNGS_MSG_SUBTYPE
};

/** Message whose text is a JSON object; base of the typed JSON messages (sizeof 0x68). */
class CNGSMessageJSONBase : public CNGSFromServerMessage {
public:
    CNGSMessageJSONBase()
        : m_startTime(0)
        , m_endTime(0)
    {
        m_subType = CNGS_MSG_SUBTYPE_JSON;
    }

    virtual ~CNGSMessageJSONBase() {}

    /** Valid between "TimeRange" start and end (server time in seconds; 0 = unbounded check off). */
    virtual int CheckTime();

    virtual com::glu::platform::components::CStrWChar GetMessageText() const
    {
        return m_text;
    }

    virtual unsigned char InitFromObject(CObjectMapValue* object)
    {
        return InitFromBasicObject(object);
    }

    /** Parse `text` as JSON and build the message it describes (NULL when it is no object). */
    static CNGSMessageJSONBase* CreateFromString(const com::glu::platform::components::CStrWChar& text);
    static CNGSMessageJSONBase* CreateFromObject(CObjectMapValue* object);

    /** "TimeRange" and "MessageText" of `object`; 1 when a message text was found. */
    unsigned char InitFromBasicObject(CObjectMapValue* object);

    long long m_startTime;                                  // +0x48 seconds
    long long m_endTime;                                    // +0x50 seconds
    com::glu::platform::components::CStrWChar m_text;      // +0x58 "MessageText"
};

/** Several JSON messages delivered in one envelope (sizeof 0x80). */
class CNGSMessageMultiple : public CNGSMessageJSONBase {
public:
    CNGSMessageMultiple()
    {
        m_subType = CNGS_MSG_SUBTYPE_MULTIPLE;
    }

    virtual ~CNGSMessageMultiple() {}

    virtual void Init(long long messageId, long long type, long long senderId, long long recipientId,
                      com::glu::platform::components::CStrWChar senderGamertag,
                      com::glu::platform::components::CStrWChar messageText);
    virtual unsigned char Apply();

    virtual void InitWithVector(const com::glu::platform::core::CVector<CNGSMessageJSONBase*>& messages)
    {
        m_messages = messages;
    }

    /** Take the last message (NULL when none is left). */
    CNGSMessageJSONBase* PopMessage();

    com::glu::platform::core::CVector<CNGSMessageJSONBase*> m_messages;    // +0x68
};

/** Experience, currencies and items for the local user (sizeof 0x90). */
class CNGSMessageAward : public CNGSMessageJSONBase {
public:
    CNGSMessageAward()
        : m_experience(0)
        , m_currencyRare(0)
        , m_currencyCommon(0)
        , m_itemType(0)
        , m_itemQuantity(0)
    {
        m_subType = CNGS_MSG_SUBTYPE_AWARD;
    }

    virtual ~CNGSMessageAward() {}

    virtual unsigned char Apply();
    virtual unsigned char InitFromObject(CObjectMapValue* object);

    long long m_experience;         // +0x68 "Experience"
    long long m_currencyRare;       // +0x70 "CurrencyRare"
    long long m_currencyCommon;     // +0x78 "CurrencyCommon"
    long long m_itemType;           // +0x80 "Item"."ItemType"
    long long m_itemQuantity;       // +0x88 "Item"."Quantity"
};

/** An award from another user; "OnReceipt" is an award for the sender (sizeof 0x94). */
class CNGSMessageGift : public CNGSMessageAward {
public:
    CNGSMessageGift()
        : m_onReceipt(NULL)
    {
        m_subType = CNGS_MSG_SUBTYPE_GIFT;
    }

    virtual ~CNGSMessageGift()
    {
        if (m_onReceipt != NULL) {
            delete m_onReceipt;
            m_onReceipt = NULL;
        }
    }

    virtual void Init(long long messageId, long long type, long long senderId, long long recipientId,
                      com::glu::platform::components::CStrWChar senderGamertag,
                      com::glu::platform::components::CStrWChar messageText);
    virtual unsigned char InitFromObject(CObjectMapValue* object);

    CNGSMessageAward* m_onReceipt;  // +0x90 owned
};

/** Opens a URL; the placeholders {udid}, {facebookid}, ... {gserveid} are filled in (sizeof 0x78). */
class CNGSMessageLaunchURL : public CNGSMessageJSONBase {
public:
    CNGSMessageLaunchURL()
    {
        m_subType = CNGS_MSG_SUBTYPE_LAUNCH_URL;
    }

    virtual ~CNGSMessageLaunchURL() {}

    virtual unsigned char Apply()
    {
        CApplet::m_pCore->LaunchURL(com::glu::platform::components::CStrWChar(m_url).GetWCharPtr(), 0);
        return 1;
    }

    virtual unsigned char InitFromObject(CObjectMapValue* object);

    com::glu::platform::components::CStrWChar m_url;   // +0x68
};

/** Question with two answers; each answer is itself a message applied when chosen (sizeof 0x74). */
class CNGSMessagePrompt : public CNGSMessageJSONBase {
public:
    CNGSMessagePrompt()
        : m_okMessage(NULL)
        , m_cancelMessage(NULL)
        , m_shown(0)
    {
        m_subType = CNGS_MSG_SUBTYPE_PROMPT;
    }

    virtual ~CNGSMessagePrompt()
    {
        if (m_okMessage != NULL) {
            delete m_okMessage;
            m_okMessage = NULL;
        }
        if (m_cancelMessage != NULL) {
            delete m_cancelMessage;
            m_cancelMessage = NULL;
        }
    }

    virtual void Init(long long messageId, long long type, long long senderId, long long recipientId,
                      com::glu::platform::components::CStrWChar senderGamertag,
                      com::glu::platform::components::CStrWChar messageText);
    /** Show the prompt dialog once. */
    virtual unsigned char Apply();
    virtual unsigned char InitFromObject(CObjectMapValue* object);

    CNGSMessageJSONBase* m_okMessage;       // +0x68 "OnYes", applied for any button but the first
    CNGSMessageJSONBase* m_cancelMessage;   // +0x6c "OnNo", applied for the first button
    unsigned char m_shown;                  // +0x70 the dialog was shown
};

#endif
