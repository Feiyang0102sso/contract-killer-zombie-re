// PointerEventController.h - turns raw touches into gesture events (tap, double tap, long press, move).
#ifndef POINTEREVENTCONTROLLER_H
#define POINTEREVENTCONTROLLER_H

#include "Vector_ss.h"

class PointerEventController {
public:
    /** Event::m_type values. */
    enum {
        EVENT_NONE = -1,
        EVENT_TAP = 0,          // released and no second touch within 400 ms
        EVENT_DOUBLE_TAP = 1,   // second touch within 400 ms
        EVENT_LONG_PRESS = 2,   // held for 500 ms
        EVENT_MOVE = 3,         // (m_deltaX, m_deltaY) from the last position
        EVENT_UP = 4,
        EVENT_DOWN = 5
    };

    /** A touch position (copied as one 8 byte block, as in the original). */
    struct Position {
        int x;
        int y;
    };

    struct Event {
        Event()
        {
        }

        Event(const Position& position, int deltaX, int deltaY, int type)
            : m_position(position)
        {
            m_deltaY = deltaY;
            m_deltaX = deltaX;
            m_type = type;
        }

        Event& operator=(const Event& other)
        {
            m_type = other.m_type;
            m_position.x = other.m_position.x;
            m_position.y = other.m_position.y;
            m_deltaX = other.m_deltaX;
            m_deltaY = other.m_deltaY;
            return *this;
        }

        Position m_position;    // 0x00
        int m_deltaX;           // 0x08
        int m_deltaY;           // 0x0c
        int m_type;             // 0x10
    };

    PointerEventController();

    /** Advance the clock by `elapsedMs` and report taps / long presses. */
    void TouchUpdate(int elapsedMs);
    void TouchDown(int x, int y);
    void TouchUp();
    void TouchMove(int x, int y);
    /** Take the oldest event; false (type EVENT_NONE) when there is none. */
    bool GetEvent(Event& event);

    Vector<Event> m_events;     // 0x00
    bool m_touching;            // 0x10
    Position m_lastPosition;    // 0x14
    int m_touchTime;            // 0x1c m_time of the last touch / move
    bool m_released;            // 0x20
    bool m_moved;               // 0x21
    int m_time;                 // 0x24 milliseconds
};

#endif
