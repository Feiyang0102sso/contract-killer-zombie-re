// CNetMessageQueue_gServe.cpp - gServe message queue: outgoing messages are kept sorted by priority,
// sent in JSON batches of up to five over the platform HTTP transport, and the decoded replies wait
// in m_incoming until fetchMessage() hands them out. Unsent messages survive a restart in three
// files (messages.txt: JSON payload, servers.txt: one URL per line, priority.dat: ushort per message).
#include "CNetMessageQueue_gServe.h"

#include "CHttpTransport.h"
#include "CUtf.h"
#include "CFileUtil.h"
#include "ICFileMgr.h"
#include "CNGSJSONParser.h"
#include "CNetworkAvailability.h"

using namespace com::glu::platform;
using namespace com::glu::platform::components;

/** Messages sent in one HTTP request at most. */
static const int kMaxBatchSize = 5;

unsigned int CNetMessageQueue_gServe::queueMessage(CObjectMap* message, const CNetMessageServer& server,
                                                   unsigned short priority, unsigned char option32,
                                                   unsigned char option33)
{
    // message ids are never 0
    m_lastMessageId++;
    if (m_lastMessageId == 0) {
        m_lastMessageId = 1;
    }
    message->m_messageId = m_lastMessageId;
    // original quirk: the priority argument is not passed on, the envelope gets 0
    CNetMessageEnvelope envelope(*message, server, 0, option32, option33);
    return queueMessage(envelope);
}

unsigned int CNetMessageQueue_gServe::queueMessage(const CNetMessageEnvelope& envelope)
{
    // insert before the first message with a priority not above this one
    TCListItr<CNetMessageEnvelope> itr(m_outgoing.m_head);
    while (itr.m_node != NULL && itr.m_node->m_data.m_priority > envelope.m_priority) {
        ++itr;
    }
    if (itr.m_node != NULL) {
        m_outgoing.InsertAt(envelope, itr);
    } else {
        m_outgoing.PushLast(envelope);
    }
    return envelope.m_message.m_messageId;
}

unsigned char CNetMessageQueue_gServe::fetchMessage(CObjectMap** reply, unsigned int messageId)
{
    TCListItr<CNetMessageEnvelope> itr(m_incoming.m_tail);
    while (itr.m_node != NULL) {
        if ((*itr).m_message.m_messageId == messageId) {
            *reply = new CObjectMap((*itr).m_message);
            m_incoming.Remove(itr);
            return 1;
        }
        --itr;
    }
    *reply = NULL;
    return 0;
}

unsigned char CNetMessageQueue_gServe::cancelMessage(unsigned int messageId)
{
    unsigned char cancelled = 0;
    TCListItr<CNetMessageEnvelope> itr(m_outgoing.m_head);
    while (itr.m_node != NULL) {
        if ((*itr).m_message.m_messageId == messageId) {
            cancelled = 1;
            m_outgoing.Remove(itr);
            break;
        }
        ++itr;
    }
    return cancelled;
}

int CNetMessageQueue_gServe::flush(unsigned short priority)
{
    // take the lowest priority messages (the tail) up to `priority`
    core::CVector<CNetMessageEnvelope> pending;
    while (m_outgoing.GetCount() != 0 && m_outgoing.GetLast().m_priority <= priority
           && pending.Size() < kMaxBatchSize) {
        pending.Add(m_outgoing.PullLast());
    }

    int takenCount = pending.Size();
    while (pending.Size() > 0) {
        // one request per server
        CNetMessageServer server(pending[0].m_server);
        core::CVector<CNetMessageEnvelope> batch;
        int index = 0;
        while (index < pending.Size()) {
            if (pending[index].m_server == server) {
                batch.Add(pending[index]);
                pending.RemoveAt(index);
            } else {
                index++;
            }
        }
        flushBatch(batch, server);
    }
    return takenCount;
}

void CNetMessageQueue_gServe::flushBatch(const core::CVector<CNetMessageEnvelope>& batch, const CNetMessageServer& server)
{
    core::CVector<CObjectMap> messages;
    for (int index = 0; index < batch.Size(); index++) {
        messages.Add(batch[index].m_message);
    }
    CStrWChar json = CNGSJSONParser::encodeMessages(messages);

    network::SwpDeviceInfo info;
    info.m_timeoutMillis = 60000;
    info.m_connectTimeoutMillis = 60000;

    // the request body stays alive (m_requestBuffers) until the transport finished
    int bufferSize = core::CUtf::GetUtfLen(json.m_string, json.m_length) + 1;
    unsigned char* buffer = new unsigned char[bufferSize];
    m_requestBuffers.Add(buffer);
    network::CHttpTransport* transport = new network::CHttpTransport();
    m_transports.Add(transport);
    m_batches.Add(batch);

    unsigned int length = core::CUtf::WcsToUtf8(json.m_string, json.m_length,
                                                m_requestBuffers[m_requestBuffers.Size() - 1], bufferSize);
    m_transports[m_transports.Size() - 1]->Send(info, CStrChar(server.m_url).GetCharPtr(),
                                                m_requestBuffers[m_requestBuffers.Size() - 1], length, 0, NULL,
                                                "application/json", server.getMethod());
}

int CNetMessageQueue_gServe::flush(const CNetMessageServer& server)
{
    // take the messages for `server`, lowest priority first
    core::CVector<CNetMessageEnvelope> batch;
    TCListItr<CNetMessageEnvelope> itr(m_outgoing.m_tail);
    while (m_outgoing.GetCount() != 0 && itr.m_node != NULL && batch.Size() < kMaxBatchSize) {
        if (itr.m_node->m_data.m_server == server) {
            batch.Add(itr.m_node->m_data);
            TCListItr<CNetMessageEnvelope> taken = itr;
            --itr;
            m_outgoing.Remove(taken);
        } else {
            --itr;
        }
    }

    int takenCount = batch.Size();
    if (takenCount > 0 && server.m_value10 == 0 && server.m_value14 == 0) {
        flushBatch(batch, server);
    }
    return takenCount;
}

void CNetMessageQueue_gServe::HandleUpdate(int elapsedMs)
{
    int index = 0;
    while (m_batches.Size() > index) {
        m_transports[index]->HandleUpdate(elapsedMs);
        int progress;
        if (m_transports[index]->GetStatus(&progress) != network::CHttpTransport::Status_Done) {
            index++;
            continue;
        }

        unsigned int responseLength = m_transports[index]->GetResponseLen();
        int error = m_transports[index]->GetError();
        if (responseLength != 0 && error == 0) {
            wchar_t* text = new wchar_t[responseLength + 1];
            int converted = core::CUtf::Utf8ToWcs(m_transports[index]->GetResponsePtr(), responseLength, text,
                                                  responseLength + 1);
            core::CVector<CObjectMap> replies;
            if (converted == 0) {
                // not UTF-8: widen the bytes as they are
                CStrWChar json((const char*)m_transports[index]->GetResponsePtr());
                CNGSJSONParser::decodeMessages(json, replies);
            } else {
                CStrWChar json(text);
                CNGSJSONParser::decodeMessages(json, replies);
            }
            if (text != NULL) {
                delete[] text;
            }

            // keep the replies whose request asked for one
            for (int reply = 0; reply < replies.Size(); reply++) {
                for (int message = 0; message < m_batches[index].Size(); message++) {
                    CNetMessageEnvelope& sent = m_batches[index][message];
                    if (sent.m_message.m_messageId == replies[reply].m_messageId && sent.m_option32 != 0) {
                        m_incoming.PushFirst(CNetMessageEnvelope(replies[reply], sent.m_server, 0, sent.m_option32,
                                                                 sent.m_option33));
                        break;
                    }
                }
            }
        } else {
            // failed: queue the messages marked for resending again while there is room
            for (int message = 0; message < m_batches[index].Size() && (unsigned int)m_outgoing.GetCount() < 128;
                 message++) {
                if (m_batches[index][message].m_option33) {
                    queueMessage(m_batches[index][message]);
                }
            }
            m_state0c = 1;
        }

        // the request is finished: drop its transport, body and batch
        m_transports[index]->ClearResponse();
        m_transports[index]->Cancel();
        if (m_requestBuffers[index] != NULL) {
            delete[] m_requestBuffers[index];
        }
        m_requestBuffers.RemoveAt(index);
        if (m_transports[index] != NULL) {
            delete m_transports[index];
        }
        m_transports.RemoveAt(index);
        m_batches.RemoveAt(index);
    }

    if (m_outgoing.GetCount() != 0) {
        // after a failure wait 10 s (and for a network) before trying again
        if (m_state0c != 0) {
            m_state0c += elapsedMs;
        }
        if (m_state0c == 0 || (m_state10 != NULL && m_state10->IsAvailable() && m_state0c >= 10000)) {
            flush(m_outgoing.GetLast().m_priority);
            m_state0c = 1;
        }
    }
}

void CNetMessageQueue_gServe::atExit()
{
    // the running requests are cancelled, their messages go back into the queue
    if (m_batches.Size() > 0) {
        for (int index = 0; index < m_batches.Size(); index++) {
            for (int message = 0; message < m_batches[index].Size(); message++) {
                queueMessage(m_batches[index][message]);
            }
            m_transports[index]->Cancel();
            if (m_requestBuffers[index] != NULL) {
                delete[] m_requestBuffers[index];
            }
            if (m_transports[index] != NULL) {
                delete m_transports[index];
            }
        }
        m_batches.Clear();
        m_requestBuffers.Clear();
        m_transports.Clear();
    }
    m_incoming.RemoveAll();

    // save the unsent messages
    int count = m_outgoing.GetCount();
    if (count > 0) {
        unsigned short* priorities = new unsigned short[count];
        core::CVector<CObjectMap> messages;
        TCListItr<CNetMessageEnvelope> itr(m_outgoing.m_tail);
        unsigned short serversLength = 0;
        while (itr.m_node != NULL) {
            messages.Add((*itr).m_message);
            serversLength = serversLength + CStrChar((*itr).m_server.m_url).GetLength() + 1;
            --itr;
        }
        CStrWChar json = CNGSJSONParser::encodeMessages(messages);

        // servers.txt: one URL per line, in the same order
        char* servers = new char[serversLength];
        itr = TCListItr<CNetMessageEnvelope>(m_outgoing.m_tail);
        unsigned short serversOffset = 0;
        unsigned short* priority = priorities;
        while (itr.m_node != NULL) {
            *priority = (*itr).m_priority;
            memcpy(servers + serversOffset, CStrChar((*itr).m_server.m_url).GetCharPtr(),
                   CStrChar((*itr).m_server.m_url).GetLength());
            serversOffset = serversOffset + CStrChar((*itr).m_server.m_url).GetLength() + 1;
            servers[serversOffset - 1] = '\n';
            priority++;
            --itr;
        }

        if (CFileUtil::WriteFile(m_string0f4.m_string, (const unsigned char*)json.m_string, json.m_length * 4)
            && CFileUtil::WriteFile(m_string104.m_string, (const unsigned char*)servers, serversOffset)
            && CFileUtil::WriteFile(m_string0e4.m_string, (const unsigned char*)priorities, count * 2)) {
            m_outgoing.RemoveAll();
        }
        if (priorities != NULL) {
            delete[] priorities;
        }
        if (servers != NULL) {
            delete[] servers;
        }
    }

    if (m_state10 != NULL) {
        delete m_state10;
    }
    m_state10 = NULL;
}

void CNetMessageQueue_gServe::onStart()
{
    m_state10 = new CNetworkAvailability();
    CFileUtil::GetApplicationDataPathForFile(m_string0e4, L"priority.dat");
    CFileUtil::GetApplicationDataPathForFile(m_string0f4, L"messages.txt");
    CFileUtil::GetApplicationDataPathForFile(m_string104, L"servers.txt");

    // reload the messages saved by atExit()
    if (ICFileMgr::GetInstance()->Exists(m_string0f4.m_string) && ICFileMgr::GetInstance()->Exists(m_string0e4.m_string)
        && ICFileMgr::GetInstance()->Exists(m_string104.m_string)) {
        unsigned int messagesSize = ICFileMgr::GetInstance()->Size(m_string0f4.m_string);
        wchar_t* messagesText = new wchar_t[messagesSize / 4 + 1];
        messagesText[messagesSize / 4] = 0;
        unsigned int serversSize = ICFileMgr::GetInstance()->Size(m_string104.m_string);
        char* serversText = new char[serversSize + 1];
        serversText[serversSize] = 0;
        unsigned int prioritySize = ICFileMgr::GetInstance()->Size(m_string0e4.m_string);
        unsigned short* priorities = new unsigned short[prioritySize / 2];

        if (CFileUtil::ReadFile(m_string0f4.m_string, (unsigned char*)messagesText, messagesSize)
            && CFileUtil::ReadFile(m_string104.m_string, (unsigned char*)serversText, serversSize)
            && CFileUtil::ReadFile(m_string0e4.m_string, (unsigned char*)priorities, prioritySize)) {
            core::CVector<CObjectMap> messages;
            int count = CNGSJSONParser::decodeMessages(CStrWChar(messagesText), messages);
            if (count != 0 && prioritySize == (unsigned int)(count * 2)) {
                CStrChar servers(serversText);
                unsigned int lineStart = 0;
                for (int index = 0; index < messages.Size(); index++) {
                    if (lineStart < serversSize) {
                        int lineEnd = servers.Find("\n", lineStart) - 1;
                        if (lineEnd < 0) {
                            // last line without a line feed
                            lineEnd = servers.m_length - 1;
                        }
                        queueMessage(&messages[index], CNetMessageServer(servers.GetSubString(lineStart, lineEnd)),
                                     priorities[index], 0, 1);
                        lineStart = lineEnd + 2;
                    }
                }
            }
        }
        if (messagesText != NULL) {
            delete[] messagesText;
        }
        if (serversText != NULL) {
            delete[] serversText;
        }
        if (priorities != NULL) {
            delete[] priorities;
        }
    }

    ICFileMgr::GetInstance()->Delete(m_string0f4.m_string);
    ICFileMgr::GetInstance()->Delete(m_string0e4.m_string);
    ICFileMgr::GetInstance()->Delete(m_string104.m_string);
}
