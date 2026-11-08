// TimeValue.cpp - stopwatch on the WindowApp clock and the delayed action queue.
#include "TimeValue.h"
#include "QuickSort.h"
#include "WindowApp.h"

TimeValue::TimeValue()
{
    m_pauseTime = -1.0f;
    m_startTime = WindowApp::m_instance->m_timeSeconds;
}

void TimeValue::Pause()
{
    if (IsNegative(m_pauseTime)) {
        m_pauseTime = WindowApp::m_instance->m_timeSeconds;
    }
}

float TimeValue::TimeSec() const
{
    if (!IsNegative(m_pauseTime)) {
        return m_pauseTime - m_startTime;
    }
    return WindowApp::m_instance->m_timeSeconds - m_startTime;
}

void TimeValue::SetTimeSec(float seconds)
{
    if (!IsNegative(m_pauseTime)) {
        m_startTime = m_pauseTime - seconds;
    } else {
        m_startTime = WindowApp::m_instance->m_timeSeconds - seconds;
    }
}

wchar_t* TimeValue::Format(wchar_t* buffer, int seconds, bool shortFormat)
{
    int minutes = seconds / 60;
    int hours = seconds / 3600;
    int minutesPart = minutes - hours * 60;
    int secondsPart = seconds % 60;

    if (shortFormat && hours == 0) {
        __wsprintf(buffer, 0, L"%d:%02d", minutesPart, secondsPart);
    } else {
        __wsprintf(buffer, 0, L"%d:%02d:%02d", hours, minutesPart, secondsPart);
    }
    return buffer;
}

XString TimeValue::Format(int seconds, bool shortFormat)
{
    wchar_t buffer[32];
    return XString(Format(buffer, seconds, shortFormat));
}

TimeManager::TimeManager()
    : m_time(0)
{
    m_updating = false;
}

TimeManager::~TimeManager()
{
    clear();
}

void TimeManager::clear()
{
    for (int index = 0; index < m_timers.size(); index++) {
        destroy(m_timers[index]);
    }
    m_timers.removeAllElements();
    m_time = 0;
}

void TimeManager::destroy(Timer& timer)
{
    if (timer.m_action != NULL && timer.m_action->IsDestructible() == 1) {
        delete timer.m_action;
    }
    timer.m_action = NULL;
    timer.m_dueTime = 0;
}

void TimeManager::add(Action* action, unsigned long delay)
{
    Timer timer;
    timer.m_action = action;
    timer.m_dueTime = m_time + delay;
    if (m_updating) {
        // update() is walking m_timers: queue it until the walk is over.
        m_pending.addElement(timer);
    } else {
        add_impl(timer);
    }
}

inline void TimeManager::add_impl(Timer timer)
{
    struct Helper {
        /** Later due time first, so the next timer to run is the last element. */
        static int Comparator(const void* left, const void* right)
        {
            return ((const Timer*)left)->m_dueTime < ((const Timer*)right)->m_dueTime;
        }
    };

    m_timers.addElement(timer);
    QuickSort::Run(m_timers.m_items, m_timers.size(), sizeof(Timer), Helper::Comparator);
}

void TimeManager::update(unsigned long elapsed)
{
    int count = m_timers.size();
    if (count == 0) {
        return;
    }

    m_time += elapsed;
    m_updating = true;

    // Run the due timers from the end of the sorted queue.
    Timer* timers = m_timers.m_items;
    int index = count - 1;
    do {
        Timer& timer = timers[index];
        if (timer.m_dueTime > m_time) {
            break;
        }
        timer.m_action->Execute(this);
        destroy(timer);
        index--;
    } while (index >= 0);
    m_timers.setSize(index + 1);
    m_updating = false;

    // Move the timers added meanwhile into the queue.
    if (m_pending.size() > 0) {
        for (int index = 0; index < m_pending.size(); index++) {
            add_impl(m_pending[index]);
        }
        m_pending.removeAllElements();
    }

    if (m_timers.size() == 0) {
        m_time = 0;
    }
}
