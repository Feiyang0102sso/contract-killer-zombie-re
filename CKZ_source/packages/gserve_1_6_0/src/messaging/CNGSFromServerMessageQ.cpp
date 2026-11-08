// CNGSFromServerMessageQ.cpp - server message queue of the local user: message poll, ack
// bookkeeping (persisted in "CNG_IdList.dat") and the prompt messages.
// The original clears its file buffers with the C library memset, not np_memset.
#undef memset
#include <string.h>
#include "CNGSFromServerMessageQ.h"
#include "CNGS.h"
#include "CNGSLocalUser.h"
#include "CNGSJSONParser.h"
#include "CObjectMap.h"
#include "CObjectMapObject.h"
#include "CObjectMapArray.h"
#include "CNotificationHandler.h"
#include "CFileUtil_gServe.h"
#include "CArrayInputStream.h"
#include "CArrayOutputStream.h"
#include "ICFileMgr.h"
#include "CNGSTokens.h"

using com::glu::platform::components::CStrWChar;
using com::glu::platform::components::CArrayInputStream;
using com::glu::platform::components::CArrayOutputStream;
using com::glu::platform::components::ICFileMgr;
using com::glu::platform::core::CVector;

/** Name of the file with the ids still to acknowledge. */
static CStrWChar kMessageIdListFile("CNG_IdList.dat");

CNGSFromServerMessageQ::CNGSFromServerMessageQ()
{
    m_messagesById.Init(10, 10, 1);
    m_ackListSaved = 0;
    m_acksInFlight = 0;
}

void CNGSFromServerMessageQ::Reset()
{
    m_messages.Clear();
    m_messageIdsToAck.Clear();
    m_ackListSaved = 0;
    m_acksInFlight = 0;
}

unsigned char CNGSFromServerMessageQ::LoadMessagesToAckFromFileSystem()
{
    CNGSLocalUser* user = CNGS::GetInstance()->GetLocalUser();
    if (user == NULL) {
        return 0;
    }
    if (!user->m_credentials.isValid()) {
        return 0;
    }

    unsigned char data[0x1000];
    memset(data, 0, sizeof(data));
    unsigned char loaded = CFileUtil_gServe::ReadApplicationDataFile(kMessageIdListFile.GetWCharPtr(), data, sizeof(data),
                                                                     user->m_credentials.getSubDirectoryPath().GetWCharPtr());
    if (!loaded) {
        return 0;
    }

    CArrayInputStream stream;
    if (stream.Open(data, sizeof(data))) {
        wchar_t text[0x1001];
        int length = stream.ReadJMUtf(text, 0x1001);
        text[length] = 0;
        if (length > 0) {
            CStrWChar json(text);
            // the decoded tree is not freed (as in the original)
            long long messageId;
            CObjectMapValue* value = CNGSJSONParser::decodeValue(json);
            if (value != NULL && value->getType() == CObjectMapValue::TYPE_ARRAY) {
                CObjectMapArray* list = (CObjectMapArray*)value;
                int count = list->m_values.Size();
                for (int index = 0; index < count; index++) {
                    CObjectMapValue* entry = list->m_values[index];
                    if (entry != NULL && entry->getType() == CObjectMapValue::TYPE_OBJECT) {
                        CObjectMapValue* idValue = ((CObjectMapObject*)entry)->getEntry(CStrWChar(NGS_ID_TOKEN));
                        if (idValue != NULL && idValue->getType() == CObjectMapValue::TYPE_INT) {
                            messageId = ((CObjectMapInt*)idValue)->m_value;
                            m_messageIdsToAck.Add(messageId);
                        }
                    }
                }
                m_ackListSaved = 1;
                if (count > 0) {
                    CNGS::GetInstance()->GetLocalUser()->m_notificationHandler->HandleOldUnackedMessages();
                }
            }
        }
    }
    return loaded;
}

CNGSFromServerMessageQ::~CNGSFromServerMessageQ()
{
}

CNGSFromServerMessage* CNGSFromServerMessageQ::CreateMessageFromObjectEnvelopeObject(CObjectMapValue* envelopeValue)
{
    if (envelopeValue == NULL || envelopeValue->getType() != CObjectMapValue::TYPE_OBJECT) {
        return NULL;
    }
    CObjectMapObject* envelope = (CObjectMapObject*)envelopeValue;
    CStrWChar senderGamertag("");

    long long messageId;
    CObjectMapValue* idValue = envelope->getEntry(CStrWChar(NGS_ID_TOKEN));
    if (idValue == NULL) {
        messageId = 0;
    } else if (idValue->getType() == CObjectMapValue::TYPE_INT) {
        messageId = ((CObjectMapInt*)idValue)->m_value;
    } else {
        messageId = 0;
    }

    long long senderId;
    CObjectMapValue* senderValue = envelope->getEntry(CStrWChar("senderid"));
    if (senderValue == NULL) {
        senderId = 0;
    } else if (senderValue->getType() == CObjectMapValue::TYPE_INT) {
        senderId = ((CObjectMapInt*)senderValue)->m_value;
    } else {
        senderId = 0;
    }

    CObjectMapValue* gamertagValue = envelope->getEntry(CStrWChar("senderGamertag"));
    if (gamertagValue != NULL && gamertagValue->getType() == CObjectMapValue::TYPE_STRING) {
        senderGamertag = CStrWChar(((CObjectMapString*)gamertagValue)->m_value);
    }

    long long type;
    CObjectMapValue* typeValue = envelope->getEntry(CStrWChar("type"));
    if (typeValue == NULL) {
        type = 0;
    } else if (typeValue->getType() == CObjectMapValue::TYPE_INT) {
        type = ((CObjectMapInt*)typeValue)->m_value;
    } else {
        type = 0;
    }

    long long recipientId;
    CObjectMapValue* recipientValue = envelope->getEntry(CStrWChar("recipientid"));
    if (recipientValue == NULL) {
        recipientId = 0;
    } else if (recipientValue->getType() == CObjectMapValue::TYPE_INT) {
        recipientId = ((CObjectMapInt*)recipientValue)->m_value;
    } else {
        recipientId = 0;
    }

    CNGSFromServerMessage* message = NULL;
    CObjectMapValue* textValue = envelope->getEntry(CStrWChar("message"));
    if (textValue != NULL && textValue->getType() == CObjectMapValue::TYPE_STRING) {
        CStrWChar rawText(((CObjectMapString*)textValue)->m_value);
        CStrWChar text;

        // turn the escaped quotes \" back into "
        int quote = rawText.Find(L"\\\"", 0);
        if (quote < 0) {
            text = rawText;
        } else {
            int start = 0;
            if (quote == 0) {
                start = 1;
                quote = rawText.Find(L"\\\"", 1);
            }
            while (quote >= 0) {
                text += rawText.GetSubString(start, quote - 1);
                start = quote + 1;
                quote = rawText.Find(L"\\\"", quote + 2);
            }
            text += rawText.GetSubString(start, rawText.GetLength() - 1);
        }

        switch (type) {
        case 40:
        case 41:
        case 100:
        case 300:
            message = CNGSMessageJSONBase::CreateFromString(text);
            message->Init(messageId, type, senderId, recipientId, senderGamertag, text);
            break;
        case 11:
        case 20:
            message = new CNGSFromServerMessage();
            message->Init(messageId, type, senderId, recipientId, senderGamertag, text);
            break;
        case 30:
            // open the URL right away; nothing is kept
            CApplet::m_pCore->LaunchURL(text.GetWCharPtr(), 0);
            AckMessageById(messageId);
            message = NULL;
            break;
        default:
            if (type >= 1000) {
                // game specific messages
                message = new CNGSFromServerMessage();
                message->Init(messageId, type, senderId, recipientId, senderGamertag, text);
            } else {
                // unknown type: acknowledge and drop it
                AckMessageById(messageId);
                message = NULL;
            }
            break;
        }
    }
    return message;
}

void CNGSFromServerMessageQ::AckMessageById(long long messageId)
{
    m_messageIdsToAck.Add(messageId);
    if (isReady() && m_writeRequestsOutstanding == 0) {
        AckMessages();
    }
}

bool CNGSFromServerMessageQ::MarkMessageForAck(long long messageId)
{
    CNGSFromServerMessage* message = NULL;
    m_messagesById.Find((unsigned int)messageId, (void**)&message);
    bool found = false;
    if (message != NULL) {
        m_messagesById.Remove((unsigned int)message->m_messageId);
        found = true;
        m_messages.Remove(message);
        AckMessageById(message->m_messageId);
    }
    return found;
}

int CNGSFromServerMessageQ::AckMessages()
{
    CNGSLocalUser* user = CNGS::GetInstance()->GetLocalUser();
    int sent = 0;
    if (isReady() && m_writeRequestsOutstanding == 0 && user != NULL && user->isValid() && m_acksInFlight == 0
        && m_messageIdsToAck.Size() > 0) {
        m_acksInFlight = m_messageIdsToAck.Size();
        sent = AcknowledgeMessageList(GetObjectMapRepresentationOfIdList(m_messageIdsToAck));
        if (sent > 0) {
            m_acksInFlight = m_messageIdsToAck.Size();
            AddReadRequestOutstanding();
        }
    }
    return sent;
}

CNGSFromServerMessage* CNGSFromServerMessageQ::GetMessageById(long long messageId)
{
    void* found = NULL;
    m_messagesById.Find((unsigned int)messageId, &found);
    return (CNGSFromServerMessage*)found;
}

unsigned char CNGSFromServerMessageQ::SyncAllFromServer(CNGSNotifyFunctor* notify)
{
    unsigned char sent = 0;
    if (isReady() && m_writeRequestsOutstanding == 0) {
        m_readNotify = notify;
        sent = CheckForEnvelopesFromServer() != 0;
    }
    return sent;
}

unsigned int CNGSFromServerMessageQ::CheckForEnvelopesFromServer()
{
    CObjectMap* message = new CObjectMap();
    message->beginObject();
    InsertHeaderIntoMessage(message, "message poll");
    message->beginObject(CStrWChar(NGS_USER_TOKEN));
    message->addEntry(CStrWChar(NGS_ID_TOKEN), CObjectMapInt(CNGS::GetInstance()->GetLocalUser()->GetClientID()));
    message->endObject();
    message->endObject();

    unsigned int messageId = SendMessageToServer(message, "resources/message/poll",
        new CNGSFromServerMessageQFunctor(this, &CNGSFromServerMessageQ::HandleCheckForEnvelopesFromServerResponse));
    if (messageId != 0) {
        AddReadRequestOutstanding();
    }
    return messageId;
}

int CNGSFromServerMessageQ::GetAllMessagesByUserAndType(CVector<long long>& ids, long long senderId, long long type,
                                                        CNGSFromServerMessage::CNGS_MSG_SUBTYPE subType)
{
    int count = 0;
    if (type != 0) {
        for (int index = 0; index < m_messages.Size(); index++) {
            CNGSFromServerMessage* message = m_messages[index];
            if (message->m_type == type && (senderId == 0 || message->m_senderId == senderId)
                && (subType == CNGSFromServerMessage::CNGS_MSG_SUBTYPE_ANY || message->m_subType == subType)) {
                long long messageId = message->m_messageId;
                ids.Add(messageId);
                count++;
            }
        }
    } else if (senderId == 0) {
        // no filter: every message
        for (int index = 0; index < m_messages.Size(); index++) {
            long long messageId = m_messages[index]->m_messageId;
            ids.Add(messageId);
            count++;
        }
    } else {
        for (int index = 0; index < m_messages.Size(); index++) {
            CNGSFromServerMessage* message = m_messages[index];
            if (message->m_senderId == senderId && (type == 0 || message->m_type == type)) {
                long long messageId = message->m_messageId;
                count++;
                ids.Add(messageId);
            }
        }
    }
    return count;
}

void CNGSFromServerMessageQ::HandleCheckForEnvelopesFromServerResponse(CObjectMap* response, CNGSFromServerMessageQFunctor* functor)
{
    if (WasErrorInResponse(response, functor, "HandleCheckForEnvelopesFromServerResponse") == 0) {
        CObjectMapValue* listValue = response->getDataAt(CStrWChar("list"), CStrWChar(L" "));
        if (listValue != NULL && listValue->getType() == CObjectMapValue::TYPE_ARRAY) {
            CObjectMapArray* list = (CObjectMapArray*)listValue;
            int count = list->m_values.Size();
            unsigned char newMessages = 0;
            for (int index = 0; index < count; index++) {
                CNGSFromServerMessage* message = CreateMessageFromObjectEnvelopeObject(list->m_values[index]);
                // a message already known is skipped (and not freed, as in the original)
                if (message != NULL && !m_messagesById.Find((unsigned int)message->m_messageId, NULL)) {
                    int timeState = message->CheckTime();
                    if (timeState < 0) {
                        // not valid yet: dropped
                        delete message;
                        message = NULL;
                    } else if (timeState >= 1) {
                        // expired: acknowledge and drop it
                        long long expiredId = message->m_messageId;
                        m_messageIdsToAck.Add(expiredId);
                        delete message;
                        message = NULL;
                    } else {
                        m_messagesById.Insert((unsigned int)message->m_messageId, message);
                        newMessages = 1;
                        m_messages.Add(message);
                    }
                }
            }

            CNotificationHandler* handler = CNGS::GetInstance()->GetLocalUser()->m_notificationHandler;
            handler->HandleReceivedEnvelope();
            if (newMessages) {
                handler->HandleNewMessagesReceived();
            }
        }
    }
    CompleteReadRequestOutstanding();
}

CObjectMapArray* GetObjectMapRepresentationOfIdList(CVector<long long>& ids)
{
    int count = ids.Size();
    CObjectMapArray* list = new CObjectMapArray();
    for (int index = 0; index < count; index++) {
        CObjectMapObject* entry = new CObjectMapObject();
        entry->addEntry(CStrWChar(NGS_ID_TOKEN), new CObjectMapInt(ids[index]));
        list->m_values.Add(entry);
    }
    return list;
}

unsigned int CNGSFromServerMessageQ::AcknowledgeMessageList(CObjectMapArray* list)
{
    CNGSLocalUser* user = CNGS::GetInstance()->GetLocalUser();
    unsigned int sent = 0;
    if (isReady() && m_writeRequestsOutstanding == 0 && user != NULL && user->isValid()) {
        CObjectMapObject* message = new CObjectMapObject();
        InsertHeaderIntoMessageObject(message, "message ack", 0);
        message->addEntry(CStrWChar("messages"), list);
        message->addEntry(CStrWChar(NGS_USER_TOKEN), user->GetUserObject());
        sent = SendMessageObjectToServer(message, "resources/message/ack",
            new CNGSFromServerMessageQFunctor(this, &CNGSFromServerMessageQ::HandleAcknowledgeEnvelopesFromServerResponse));
    }
    return sent;
}

void CNGSFromServerMessageQ::HandleAcknowledgeEnvelopesFromServerResponse(CObjectMap* response, CNGSFromServerMessageQFunctor* functor)
{
    // the caller name differs from the function name in the original
    if (WasErrorInResponse(response, functor, "HandleAcknowledgeFromServerEnvelopesResponse") == 0) {
        // drop the acknowledged ids from the front
        int remaining = m_messageIdsToAck.Size() - m_acksInFlight;
        for (int index = 0; index < remaining; index++) {
            m_messageIdsToAck[index] = m_messageIdsToAck[m_acksInFlight + index];
        }
        m_messageIdsToAck.SetSize(remaining);
        m_acksInFlight = 0;
        if (m_ackListSaved) {
            WriteMessageIdsToAckToFS();
        }
    } else {
        WriteMessageIdsToAckToFS();
    }
    CompleteReadRequestOutstanding();
}

int CNGSFromServerMessageQ::WriteMessageIdsToAckToFS()
{
    CNGSLocalUser* user = CNGS::GetInstance()->GetLocalUser();
    if (user != NULL) {
        CObjectMapArray* list = GetObjectMapRepresentationOfIdList(m_messageIdsToAck);
        CStrWChar json = CNGSJSONParser::encodeValue(list);
        if (list != NULL) {
            delete list;
        }

        CArrayOutputStream stream;
        unsigned char data[0x1000];
        memset(data, 0, sizeof(data));
        if (stream.Open(data, sizeof(data))) {
            CStrWChar directory = CNGSUserCredentials::getFullDirectoryPathForSubdir(user->m_credentials.getSubDirectoryPath());
            if (ICFileMgr::GetInstance()->MkDir(directory.GetWCharPtr())) {
                stream.WriteJMUtf(json.GetWCharPtr());
                if (CFileUtil_gServe::WriteApplicationDataFile(kMessageIdListFile.GetWCharPtr(), data, sizeof(data),
                                                               user->m_credentials.getSubDirectoryPath().GetWCharPtr())) {
                    m_ackListSaved = m_messageIdsToAck.Size() > 0;
                }
            }
        }
    }
    return 0;
}

unsigned char CNGSFromServerMessageQ::ProcessPromptMessages()
{
    CNGSLocalUser* user = CNGS::GetInstance()->GetLocalUser();
    unsigned char result = 0;
    if (user != NULL && user->isAuthenticated(0)) {
        CVector<long long> ids;
        if (GetAllMessagesByUserAndType(ids, 0, 300, CNGSFromServerMessage::CNGS_MSG_SUBTYPE_ANY) > 0) {
            result = GetMessageById(ids[0])->Apply();
        }
    }
    return result;
}

void CNGSFromServerMessageQFunctor::requestCompleteCallback(CObjectMap* response)
{
    // plain pointer-to-member test (see CNGSTimeFunctor)
    if (m_callback) {
        (m_target->*m_callback)(response, this);
    }
}
