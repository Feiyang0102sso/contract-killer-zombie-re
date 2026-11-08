// PointerEventController.cpp - gesture events from raw touches.
#include "PointerEventController.h"

/** Second touch within this time is a double tap; a release older than this is a tap. */
static const int DOUBLE_TAP_TIME = 400;
/** Holding this long is a long press. */
static const int LONG_PRESS_TIME = 500;

PointerEventController::PointerEventController()
{
    m_touching = false;
    m_lastPosition.x = 0;
    m_lastPosition.y = 0;
    m_touchTime = 0;
    m_released = true;
    m_moved = false;
    m_time = 0;
}

void PointerEventController::TouchUpdate(int elapsedMs)
{
    m_time += elapsedMs;
    if (!m_touching) {
        return;
    }
    int released = m_released;  // int copy: the original keeps (m_released & 1) in a register
    if (m_moved) {
        if (released) {
            m_touching = false;
            m_moved = false;
        }
        return;
    }
    int held = m_time - m_touchTime;
    if (!released) {
        if (held >= LONG_PRESS_TIME) {
            m_events.addElement(Event(m_lastPosition, 0, 0, EVENT_LONG_PRESS));
        }
    } else if (held >= DOUBLE_TAP_TIME) {
        m_events.addElement(Event(m_lastPosition, 0, 0, EVENT_TAP));
        m_touching = false;
    }
}

void PointerEventController::TouchDown(int x, int y)
{
    m_moved = false;
    if (m_touching) {
        if (m_time - m_touchTime <= DOUBLE_TAP_TIME) {
            m_events.addElement(Event(m_lastPosition, 0, 0, EVENT_DOUBLE_TAP));
            m_touching = false;
        } else {
            m_events.addElement(Event(m_lastPosition, 0, 0, EVENT_TAP));
            m_touching = true;
            int now = m_time;  // read before the position stores (original load order)
            m_lastPosition.x = x; m_lastPosition.y = y;
            m_touchTime = now;
        }
    } else {
        m_touching = true;
        int now = m_time;  // read before the position stores (original load order)
        m_lastPosition.x = x; m_lastPosition.y = y;
        m_touchTime = now;
        Position position;
        position.x = x;
        position.y = y;
        m_events.addElement(Event(position, 0, 0, EVENT_DOWN));
    }
    m_released = false;
}

void PointerEventController::TouchUp()
{
    if (m_touching) {
        m_released = true;
    }
    m_events.addElement(Event(m_lastPosition, 0, 0, EVENT_UP));
}

void PointerEventController::TouchMove(int x, int y)
{
    if (m_touching && !m_released) {
        m_moved = true;
        m_events.addElement(Event(m_lastPosition, x - m_lastPosition.x, y - m_lastPosition.y, EVENT_MOVE));
        m_lastPosition.x = x;
        m_lastPosition.y = y;
        m_touchTime = m_time;
    }
}

bool PointerEventController::GetEvent(Event& event)
{
    if (m_events.size() == 0) {
        event.m_type = EVENT_NONE;
        return false;
    }
    event = m_events[0];
    m_events.removeElementAt(0);
    return true;
}
