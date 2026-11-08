// TimeValue.h - pausable stopwatch on the window application clock, and a timer queue (created by b051x).
#ifndef TIMEVALUE_H
#define TIMEVALUE_H

#include <wchar.h>
#include "Vector_ss.h"
#include "xString.h"

/** Stopwatch measured in seconds of WindowApp time; can be paused. */
class TimeValue {
public:
    TimeValue();

    /** Freeze the stopwatch (no effect when already paused). */
    void Pause();
    /** Seconds elapsed since the start (up to the pause when paused). */
    float TimeSec() const;
    /** Move the start so that TimeSec() returns `seconds`. */
    void SetTimeSec(float seconds);

    /** "m:ss" (when `shortFormat` and below one hour) or "h:mm:ss" into `buffer`. */
    static wchar_t* Format(wchar_t* buffer, int seconds, bool shortFormat);
    static XString Format(int seconds, bool shortFormat);

    /** Sign bit test on the raw float (the original tests the bits, not the value). */
    static bool IsNegative(const float& value)
    {
        return *(const int*)&value < 0;
    }

    float m_pauseTime;  // 0x00 clock at the pause, -1 while running
    float m_startTime;  // 0x04 clock at the start
};

/** Runs actions after a delay; update() advances its own clock. */
class TimeManager {
public:
    /** Something to run when its timer expires. */
    class Action {
    public:
        virtual ~Action()
        {
        }
        /** 1 when the manager deletes the action once it is done. */
        // Historically guessed names DeleteWhenDone/Run; b074i promoted them per TapjoyUpdater's real const symbols/virtual slots.
        virtual int IsDestructible() const = 0;
        virtual void Execute(TimeManager* manager) = 0;
    };

    /** One queued action and the manager time it is due at. */
    struct Timer {
        Timer()
        {
            m_action = NULL;
            m_dueTime = 0;
            m_cancelled = false;
        }

        Timer(const Timer& other)
        {
            m_action = other.m_action;
            m_dueTime = other.m_dueTime;
            m_cancelled = false;
        }

        Action* m_action;           // 0x00
        unsigned long m_dueTime;    // 0x04
        bool m_cancelled;           // 0x08 (never set in TimeValue.o)
    };

    TimeManager();
    ~TimeManager();

    /** Advance the clock by `elapsed` and run every due action. */
    virtual void update(unsigned long elapsed);

    /** Drop every queued action. */
    void clear();
    /** Delete the action of `timer` when it asks for it, and empty the timer. */
    void destroy(Timer& timer);
    /** Run `action` `delay` time units from now. */
    void add(Action* action, unsigned long delay);

private:
    /** Insert into the queue kept sorted by due time (latest first). */
    inline void add_impl(Timer timer);

    unsigned long m_time;       // 0x04 manager clock
    Vector<Timer> m_pending;    // 0x08 added while update() runs
    Vector<Timer> m_timers;     // 0x18 sorted, the next due timer last
    bool m_updating;            // 0x28
};

#endif
