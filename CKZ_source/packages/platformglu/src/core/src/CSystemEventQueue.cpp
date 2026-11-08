// CSystemEventQueue.cpp - growable FIFO of raw system events.
#include "CSystemEventQueue.h"

#include <stdlib.h>
#include <string.h>

namespace com { namespace glu { namespace platform { namespace core {

// ---- constants ----
static const int INITIAL_CAPACITY = 1250;   // events allocated by the first Queue()

CSystemEventQueue::CSystemEventQueue()
{
    m_events = NULL;
    m_capacity = 0;
    m_count = 0;
}

CSystemEventQueue::~CSystemEventQueue()
{
    if (m_events != NULL) {
        free(m_events);
        m_events = NULL;
    }
    m_capacity = 0;
    m_count = 0;
}

void CSystemEventQueue::GetEvent(SystemEvent& event, int index) const
{
    event.type = 0;
    event.param1 = 0;
    event.param2 = 0;

    if (index < m_count) {
        event = m_events[index];
    }
}

void CSystemEventQueue::Queue(const SystemEvent& event)
{
    if (m_capacity == 0) {
        m_events = (SystemEvent*)malloc(INITIAL_CAPACITY * sizeof(SystemEvent));
        if (m_events != NULL) {
            m_capacity = INITIAL_CAPACITY;
        }
    } else if (m_capacity == m_count) {
        // full: double the storage
        SystemEvent* grown = (SystemEvent*)malloc(m_capacity * 2 * sizeof(SystemEvent));
        if (grown != NULL) {
            memcpy(grown, m_events, m_capacity * sizeof(SystemEvent));
            if (m_events != NULL) {
                free(m_events);
                m_events = NULL;
            }
            m_events = grown;
            m_capacity = m_capacity * 2;
        }
    }

    // the last slot is never used
    if (m_count < m_capacity - 1) {
        m_events[m_count++] = event;
    }
}

void CSystemEventQueue::Queue(unsigned int type, unsigned int param1, unsigned int param2)
{
    SystemEvent event;
    event.type = type;
    event.param1 = param1;
    event.param2 = param2;
    Queue(event);
}

} } } }
