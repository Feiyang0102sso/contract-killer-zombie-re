// CSystemEventQueue.h - growable FIFO of raw system events (touch, accelerometer, lifecycle...).
// The platform layer (CApplet) queues events from OS callbacks and drains them once per frame.
#ifndef CSYSTEMEVENTQUEUE_H
#define CSYSTEMEVENTQUEUE_H

#include <stddef.h>

namespace com { namespace glu { namespace platform { namespace core {

/** Array-backed event queue (16 bytes). */
class CSystemEventQueue {
public:
    /** One queued event: an id and two parameters (12 bytes). */
    struct SystemEvent {
        unsigned int type;
        unsigned int param1;
        unsigned int param2;
    };

    CSystemEventQueue();
    virtual ~CSystemEventQueue();

    /** Copy the event at `index` into `event`; out of range leaves `event` zeroed. */
    void GetEvent(SystemEvent& event, int index) const;
    /** Append a copy of `event`, growing the storage when full. */
    void Queue(const SystemEvent& event);
    /** Append the event { type, param1, param2 }. */
    void Queue(unsigned int type, unsigned int param1, unsigned int param2);

    int GetCount() const { return m_count; }
    /** Forget queued events but keep the storage. */
    void Clear() { m_count = 0; }

    SystemEvent* m_events;      // +0x04
    int m_capacity;             // +0x08 slots allocated in m_events
    int m_count;                // +0x0C slots used
};

} } } }

#endif
