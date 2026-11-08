// CNGSFromServerMessage.cpp - the JSON messages of the gServe message queue: parsing of the
// message JSON and carrying out awards, gifts, prompts and URL launches.
#include "CNGSFromServerMessage.h"
#include "CNGS.h"
#include "CNGSLocalUser.h"
#include "CNGSSession.h"
#include "CNGSJSONParser.h"
#include "CObjectMapObject.h"
#include "CNotificationHandler.h"
#include "CNGS_Platform.h"
#include "CStrWCharBuffer.h"

using com::glu::platform::components::CStrWChar;
using com::glu::platform::components::CStrWCharBuffer;
using com::glu::platform::core::CVector;

CNGSMessageJSONBase* CNGSMessageJSONBase::CreateFromString(const CStrWChar& text)
{
    // the decoded tree is not freed (as in the original)
    return CreateFromObject(CNGSJSONParser::decodeValue(text));
}

CNGSMessageJSONBase* CNGSMessageJSONBase::CreateFromObject(CObjectMapValue* object)
{
    CNGSMessageJSONBase* message = NULL;
    CVector<CNGSMessageJSONBase*> messages;
    if (object == NULL || object->getType() != CObjectMapValue::TYPE_OBJECT) {
        return NULL;
    }

    CObjectMapObject* root = (CObjectMapObject*)object;
    CObjectMapValue* giftValue = root->getEntry(CStrWChar("Gift"));
    CObjectMapValue* awardValue = root->getEntry(CStrWChar("Award"));
    CObjectMapValue* promptValue = root->getEntry(CStrWChar("Prompt"));
    CObjectMapValue* launchURLValue = root->getEntry(CStrWChar("LaunchURL"));

    if (giftValue != NULL && giftValue->getType() == CObjectMapValue::TYPE_OBJECT) {
        message = new CNGSMessageGift();
        message->InitFromObject(giftValue);
        messages.Add(message);
    }
    if (awardValue != NULL && awardValue->getType() == CObjectMapValue::TYPE_OBJECT) {
        message = new CNGSMessageAward();
        message->InitFromObject(awardValue);
        messages.Add(message);
    }
    if (promptValue != NULL && promptValue->getType() == CObjectMapValue::TYPE_OBJECT) {
        message = new CNGSMessagePrompt();
        message->InitFromObject(promptValue);
        messages.Add(message);
    }
    if (launchURLValue != NULL && launchURLValue->getType() == CObjectMapValue::TYPE_STRING) {
        message = new CNGSMessageLaunchURL();
        message->InitFromObject(launchURLValue);
        messages.Add(message);
    }

    if (message == NULL) {
        // nothing typed: a plain JSON message (text and time range only)
        message = new CNGSMessageJSONBase();
        messages.Add(message);
    } else if (messages.Size() > 1) {
        CNGSMessageMultiple* multiple = new CNGSMessageMultiple();
        message = multiple;
        multiple->InitWithVector(messages);
    }

    if (message == NULL) {
        return NULL;
    }
    message->InitFromBasicObject(object);
    return message;
}

int CNGSMessageJSONBase::CheckTime()
{
    int result = 0;
    if (m_startTime != 0 && m_endTime != 0) {
        CNGSSession* session = CNGS::GetInstance()->GetLocalUser()->m_session;
        long long now;
        if (session == NULL) {
            now = 0;
        } else if (session->isValidNetworkTime() == 1) {
            now = session->getNetworkCurrentTimeSeconds();
        } else {
            now = 0;
        }
        if (m_startTime > now) {
            result = -1;
        }
        if (m_endTime < now) {
            result = 1;
        }
    }
    return result;
}

unsigned char CNGSMessageJSONBase::InitFromBasicObject(CObjectMapValue* object)
{
    if (object == NULL || object->getType() != CObjectMapValue::TYPE_OBJECT) {
        return 0;
    }

    CObjectMapValue* timeRangeValue = ((CObjectMapObject*)object)->getEntry(CStrWChar("TimeRange"));
    if (timeRangeValue != NULL && timeRangeValue->getType() == CObjectMapValue::TYPE_OBJECT
        && timeRangeValue != NULL && timeRangeValue->getType() == CObjectMapValue::TYPE_OBJECT) {
        // both spellings of the keys are accepted
        CObjectMapValue* startValue = ((CObjectMapObject*)timeRangeValue)->getEntry(CStrWChar("startTimeInSecs"));
        if (startValue == NULL) {
            startValue = ((CObjectMapObject*)timeRangeValue)->getEntry(CStrWChar("StartTimeInSecs"));
        }
        if (startValue != NULL && startValue->getType() == CObjectMapValue::TYPE_INT) {
            m_startTime = ((CObjectMapInt*)startValue)->m_value;
        }

        CObjectMapValue* endValue = ((CObjectMapObject*)timeRangeValue)->getEntry(CStrWChar("endTimeInSecs"));
        if (endValue == NULL) {
            endValue = ((CObjectMapObject*)timeRangeValue)->getEntry(CStrWChar("EndTimeInSecs"));
        }
        if (endValue != NULL && endValue->getType() == CObjectMapValue::TYPE_INT) {
            m_endTime = ((CObjectMapInt*)endValue)->m_value;
        }
    }

    CObjectMapString* textValue = (CObjectMapString*)((CObjectMapObject*)object)->getEntry(CStrWChar("MessageText"));
    if (textValue != NULL && textValue->getType() == CObjectMapValue::TYPE_STRING) {
        m_text = CStrWChar(textValue->m_value);
        return 1;
    }
    return 0;
}

unsigned char CNGSMessageAward::InitFromObject(CObjectMapValue* object)
{
    unsigned char found = 0;
    if (object != NULL && object->getType() == CObjectMapValue::TYPE_OBJECT) {
        CObjectMapObject* root = (CObjectMapObject*)object;

        CObjectMapValue* experienceValue = root->getEntry(CStrWChar("Experience"));
        if (experienceValue != NULL && experienceValue->getType() == CObjectMapValue::TYPE_INT) {
            m_experience = ((CObjectMapInt*)experienceValue)->m_value;
            found = 1;
        }

        CObjectMapValue* itemValue = root->getEntry(CStrWChar("Item"));
        if (itemValue != NULL && itemValue->getType() == CObjectMapValue::TYPE_OBJECT) {
            CObjectMapInt* itemType = (CObjectMapInt*)((CObjectMapObject*)itemValue)->getEntry(CStrWChar("ItemType"));
            CObjectMapInt* quantity = (CObjectMapInt*)((CObjectMapObject*)itemValue)->getEntry(CStrWChar("Quantity"));
            if (itemType != NULL && quantity != NULL
                && itemType->getType() == CObjectMapValue::TYPE_INT
                && quantity->getType() == CObjectMapValue::TYPE_INT) {
                m_itemQuantity = quantity->m_value;
                m_itemType = itemType->m_value;
                found = 1;
            }
        }

        CObjectMapValue* rareValue = root->getEntry(CStrWChar("CurrencyRare"));
        if (rareValue != NULL && rareValue->getType() == CObjectMapValue::TYPE_INT) {
            m_currencyRare = ((CObjectMapInt*)rareValue)->m_value;
            found = 1;
        }

        CObjectMapValue* commonValue = root->getEntry(CStrWChar("CurrencyCommon"));
        if (commonValue != NULL && commonValue->getType() == CObjectMapValue::TYPE_INT) {
            m_currencyCommon = ((CObjectMapInt*)commonValue)->m_value;
            found = 1;
        }
    }
    return found;
}

unsigned char CNGSMessageAward::Apply()
{
    CNotificationHandler* handler = CNGS::GetInstance()->GetLocalUser()->m_notificationHandler;
    if (m_currencyRare != 0) {
        handler->HandleAddRareCurrencyGiftMessage(m_currencyRare, (int)m_senderId, GetMessageText());
    }
    if (m_currencyCommon != 0) {
        handler->HandleAddCommonCurrencyGiftMessage(m_currencyCommon, (int)m_senderId, GetMessageText());
    }
    if (m_itemQuantity != 0) {
        handler->HandleAddItemsGiftMessage(m_itemType, m_itemQuantity, (int)m_senderId, GetMessageText());
    }
    if (m_experience != 0) {
        handler->HandleAddExperienceGiftMessage(m_experience, (int)m_senderId, GetMessageText());
    }
    return 0;
}

unsigned char CNGSMessageGift::InitFromObject(CObjectMapValue* object)
{
    unsigned char found = 0;
    if (object != NULL && object->getType() == CObjectMapValue::TYPE_OBJECT) {
        CObjectMapObject* root = (CObjectMapObject*)object;

        CObjectMapValue* awardValue = root->getEntry(CStrWChar("Award"));
        if (awardValue != NULL && awardValue->getType() == CObjectMapValue::TYPE_OBJECT) {
            found = 1;
            CNGSMessageAward::InitFromObject(awardValue);
        }

        // award given back to the sender when the gift is received
        CObjectMapValue* onReceiptValue = root->getEntry(CStrWChar("OnReceipt"));
        if (onReceiptValue != NULL && onReceiptValue->getType() == CObjectMapValue::TYPE_OBJECT) {
            m_onReceipt = new CNGSMessageAward();
            found = 1;
            m_onReceipt->InitFromObject(onReceiptValue);
        }
    }
    return found;
}

void CNGSMessageMultiple::Init(long long messageId, long long type, long long senderId, long long recipientId,
                               CStrWChar senderGamertag, CStrWChar messageText)
{
    CNGSFromServerMessage::Init(messageId, type, senderId, recipientId, senderGamertag, messageText);
    for (int index = 0; index < m_messages.Size(); index++) {
        m_messages[index]->Init(messageId, type, senderId, recipientId, senderGamertag, messageText);
    }
}

CNGSMessageJSONBase* CNGSMessageMultiple::PopMessage()
{
    CNGSMessageJSONBase* message = NULL;
    int count = m_messages.Size();
    if (count > 0) {
        message = m_messages[count - 1];
        m_messages.RemoveRange(count - 1, count);
    }
    return message;
}

unsigned char CNGSMessageMultiple::Apply()
{
    unsigned char applied = 0;
    CNGSMessageJSONBase* message = PopMessage();
    while (message != NULL) {
        applied |= message->Apply();
        message = PopMessage();
    }
    return applied;
}

void CNGSMessageGift::Init(long long messageId, long long type, long long senderId, long long recipientId,
                           CStrWChar senderGamertag, CStrWChar messageText)
{
    CNGSFromServerMessage::Init(messageId, type, senderId, recipientId, senderGamertag, messageText);
    if (m_onReceipt != NULL) {
        // the receipt goes the other way: from the recipient back to the sender
        m_onReceipt->Init(messageId, type, recipientId, senderId, CStrWChar(""), messageText);
    }
}

unsigned char CNGSMessageLaunchURL::InitFromObject(CObjectMapValue* object)
{
    if (object != NULL && object->getType() == CObjectMapValue::TYPE_STRING) {
        CNGSLocalUser* user = CNGS::GetInstance()->GetLocalUser();
        CStrWCharBuffer url(CStrWChar(((CObjectMapString*)object)->m_value).GetWCharPtr());

        CStrWChar udidKey(L"{udid}");
        CStrWChar udid = CApplet::m_pCore->GetDeviceIdentifier();
        url.Replace(udidKey.GetWCharPtr(), udid.GetWCharPtr(), 0, url.GetLength(), 0, udidKey.GetLength());

        if (user != NULL) {
            CStrWChar networkId = user->m_credentials.getIDForSocialNetwork(SOCIAL_NETWORK_FACEBOOK);
            if (networkId.GetLength() > 0) {
                CStrWChar key(L"{facebookid}");
                url.Replace(key.GetWCharPtr(), networkId.GetWCharPtr(), 0, url.GetLength(), 0, key.GetLength());
            }

            networkId = user->m_credentials.getIDForSocialNetwork(SOCIAL_NETWORK_GAME_CENTER);
            if (networkId.GetLength() > 0) {
                CStrWChar key(L"{gamecenterid}");
                url.Replace(key.GetWCharPtr(), networkId.GetWCharPtr(), 0, url.GetLength(), 0, key.GetLength());
            }

            networkId = user->m_credentials.getIDForSocialNetwork(SOCIAL_NETWORK_TWITTER);
            if (networkId.GetLength() > 0) {
                CStrWChar key(L"{twitterid}");
                url.Replace(key.GetWCharPtr(), networkId.GetWCharPtr(), 0, url.GetLength(), 0, key.GetLength());
            }

            networkId = user->m_credentials.getIDForSocialNetwork(SOCIAL_NETWORK_OPENFEINT);
            if (networkId.GetLength() > 0) {
                CStrWChar key(L"{openfeintid}");
                url.Replace(key.GetWCharPtr(), networkId.GetWCharPtr(), 0, url.GetLength(), 0, key.GetLength());
            }

            // "mypaceid" is the original's spelling
            networkId = user->m_credentials.getIDForSocialNetwork(SOCIAL_NETWORK_MYSPACE);
            if (networkId.GetLength() > 0) {
                CStrWChar key(L"{mypaceid}");
                url.Replace(key.GetWCharPtr(), networkId.GetWCharPtr(), 0, url.GetLength(), 0, key.GetLength());
            }

            CStrWCharBuffer userIdText(L"");
            userIdText.Insert(userIdText.GetLength(), user->m_credentials.m_userId);
            CStrWCharBuffer key(L"{gserveid}");
            url.Replace(key.GetWCharPtr(), userIdText.GetWCharPtr(), 0, url.GetLength(), 0, key.GetLength());
        }

        m_url = CStrWChar(url.GetWCharPtr());
    }
    return 0;
}

void CNGSMessagePrompt::Init(long long messageId, long long type, long long senderId, long long recipientId,
                             CStrWChar senderGamertag, CStrWChar messageText)
{
    CNGSFromServerMessage::Init(messageId, type, senderId, recipientId, senderGamertag, messageText);
    if (m_okMessage != NULL) {
        m_okMessage->Init(messageId, type, senderId, recipientId, senderGamertag, messageText);
    }
    if (m_cancelMessage != NULL) {
        m_cancelMessage->Init(messageId, type, senderId, recipientId, senderGamertag, messageText);
    }
}

unsigned char CNGSMessagePrompt::Apply()
{
    if (m_shown == 0) {
        CNGSMessageJSONBase* okMessage = m_okMessage;
        CNGSMessageJSONBase* cancelMessage = m_cancelMessage;
        CVector<const CStrWChar*> buttonLabels;
        CStrWChar okLabel;
        CStrWChar cancelLabel;
        CStrWChar text = GetMessageText();

        // button 0 answers "no", button 1 answers "yes"
        if (cancelMessage != NULL) {
            cancelLabel = cancelMessage->GetMessageText();
            buttonLabels.Add(&cancelLabel);
        }
        if (okMessage != NULL) {
            okLabel = okMessage->GetMessageText();
            buttonLabels.Add(&okLabel);
        }
        m_shown = 1;
        CNGS_Platform::GetInstance()->showPromptMessageDialog(text, this, &buttonLabels, 0);
    }
    return 0;
}

unsigned char CNGSMessagePrompt::InitFromObject(CObjectMapValue* object)
{
    unsigned char found = 0;
    if (object != NULL && object->getType() == CObjectMapValue::TYPE_OBJECT) {
        CObjectMapObject* root = (CObjectMapObject*)object;

        CObjectMapValue* yesValue = root->getEntry(CStrWChar("OnYes"));
        if (yesValue != NULL && yesValue->getType() == CObjectMapValue::TYPE_OBJECT) {
            found = 1;
            m_okMessage = CreateFromObject(yesValue);
        }

        CObjectMapValue* noValue = root->getEntry(CStrWChar("OnNo"));
        if (noValue != NULL && noValue->getType() == CObjectMapValue::TYPE_OBJECT) {
            found = 1;
            m_cancelMessage = CreateFromObject(noValue);
        }
    }
    return found;
}
