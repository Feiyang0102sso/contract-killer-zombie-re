// CEventListener.h - receives the events it registered for through a callback.
// (first drafted by b005 as PROVISIONAL, extended by b006; completed by b011x, the owner of
// systems/src/CEventListener.cpp)
// The listener is stored in the per-event registries of the event system under its handle, so
// a listener needs a non-zero m_handle before it registers.
#ifndef CEVENTLISTENER_H
#define CEVENTLISTENER_H

#include <stddef.h>
#include "CPriorityClass.h"

namespace com { namespace glu { namespace platform { namespace systems {

class CEvent;
class CSystem;

class CEventListener : public core::CPriorityClass {
public:
    /**
     * Event callback; `listener` is the CEventListener the event was delivered to. A non-zero
     * result marks the event as handled.
     */
    typedef unsigned char (*Callback)(CEvent& event, void* listener);

    /** Runtime class id: CStringToKey("CEventListener"). */
    static const unsigned int ClassId = 0xa4303bb2;

    CEventListener()
    {
        m_classId = ClassId;
        m_object = NULL;
        m_handle = 0;
    }

    virtual ~CEventListener()
    {
        if (m_handle != 0) {
            UnregisterAll();
            m_handle = 0;
        }
        m_object = NULL;
        m_callback = NULL;
    }

    /** Receive events with id `eventId`; `priority` orders the listeners. 0 without event system. */
    unsigned char Register(unsigned int eventId, int priority);
    /** Register for every id of the 0-terminated list `eventIds` (at least one id). */
    unsigned char Register(const unsigned int* eventIds, int priority);
    /** Register in `eventSystem`, creating the registry of `eventId` when it does not exist yet. */
    void RegisterIdSpecfic(unsigned int eventId, int priority, CSystem& eventSystem);
    /** Drop every event registration of this listener. */
    void UnregisterAll();

    void* m_object;             // 0x0c object the callback works on
    unsigned int m_handle;      // 0x10 handle the listener is registered under
    Callback m_callback;        // 0x14
};

} } } }

#endif
