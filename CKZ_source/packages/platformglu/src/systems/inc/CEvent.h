// CEvent.h - executable event delivered to the listeners registered for its id.
// (first drafted by b005 as PROVISIONAL; completed by b011x, the owner)
//
// Event plumbing: the root system of the application (CApp +0x14) holds the event system under
// the handle CHandle::FromName("...") = kEventSystemHandle. The event system holds one CRegistry
// per event id (element handle = event id); each of those registries holds the CEventListener
// objects (element handle = listener handle). Listeners unregistered while an event is being
// dispatched are parked in CEventCore and removed after the dispatch.
#ifndef CEVENT_H
#define CEVENT_H

#include <stddef.h>
#include "CExecutable.h"
#include "CPool.h"
#include "CSingleton.h"
#include "CVector.h"
#include "CSystemElement.h"

namespace com { namespace glu { namespace platform { namespace systems {

/** Handle of the event system inside the application's root system. */
static const unsigned int kEventSystemHandle = 0xf762ddf9;

/** Pool all CEvent objects are allocated from (application singleton). */
class CEventPool : public components::CPool, public components::CSingleton {
public:
    /** Key in the application singleton table: CStringToKey("CEventPool"). */
    enum { SingletonId = 0x039167ee };

    CEventPool() : components::CSingleton(SingletonId) {}

    virtual ~CEventPool();

    /** The application's pool, created on first use. */
    static inline CEventPool* GetInstance()
    {
        components::CSingleton* found = NULL;
        CApplet::m_pApp->m_singletons->Find(SingletonId, (void**)&found);
        CEventPool* pool = static_cast<CEventPool*>(found);
        if (pool == NULL) {
            pool = new CEventPool();
        }
        return pool;
    }
};

inline CEventPool::~CEventPool()
{
}

/** Dispatch state shared by all events (application singleton). */
class CEventCore : public components::CSingleton {
public:
    /** Key in the application singleton table: CStringToKey("CEventCore"). */
    enum { SingletonId = 0x03905637 };

    inline CEventCore()
        : components::CSingleton(SingletonId),
          m_pendingRemovals(100, 100)
    {
        m_isDispatching = 0;
    }

    inline virtual ~CEventCore() {}

    /** The application's event core, created on first use. */
    static inline CEventCore* GetInstance()
    {
        CEventCore* found = NULL;
        CApplet::m_pApp->m_singletons->Find(SingletonId, (void**)&found);
        CEventCore* core = found;
        if (core == NULL) {
            core = new CEventCore();
        }
        return core;
    }

    core::CVector<CSystemElement*> m_pendingRemovals;   // 0x08 listener elements to drop after the dispatch
    unsigned char m_isDispatching;                      // 0x20 an event is calling its listeners
};

class CEvent : public components::CExecutable {
public:
    /** Runtime class id: CStringToKey("CEvent"). */
    enum { ClassId = 0x02720394 };

    CEvent();
    /**
     * `param` is the handle of the only listener to notify (0 = every listener of `eventId`);
     * the event runs `runCount` times, `interval` (16.16 seconds) apart.
     */
    CEvent(core::CClass* sender, unsigned int eventId, unsigned int param, int interval, unsigned int runCount);

    virtual ~CEvent() {}

    static inline void* operator new(size_t size)
    {
        return CEventPool::GetInstance()->New(size);
    }

    static inline void operator delete(void* event)
    {
        CEventPool::GetInstance()->Delete((core::CClass*)event);
    }

    /** Deliver the event to its listeners. */
    virtual void OnExecute();

    unsigned int m_eventId;         // 0x1c
    core::CClass* m_sender;         // 0x20
    unsigned int m_param;           // 0x24 handle of the single listener to notify, 0 = all
    unsigned char m_wasEverHandled; // 0x28 some listener accepted the event at least once
    unsigned char m_wasHandled;     // 0x29 a listener accepted the event in the last dispatch
};

} } } }

#endif
