// CEvent.cpp - event delivery to the registered CEventListener objects.
#include "CEvent.h"

#include "CApplet.h"
#include "CApp.h"
#include "CEventListener.h"
#include "CRegistry.h"
#include "CRegistryItr.h"

namespace com { namespace glu { namespace platform { namespace systems {

CEvent::CEvent()
{
    m_classId = ClassId;
    m_eventId = 0;
    m_deleteWhenDone = 1;
    m_sender = NULL;
    m_param = 0;
    m_runCount = OneShot;
    m_wasEverHandled = 0;
    m_wasHandled = 0;
}

CEvent::CEvent(core::CClass* sender, unsigned int eventId, unsigned int param, int interval, unsigned int runCount)
{
    m_classId = ClassId;
    m_eventId = eventId;
    m_deleteWhenDone = 1;
    m_sender = sender;
    m_param = param;
    m_interval = interval;
    m_runCount = runCount;
    m_wasEverHandled = 0;
    m_wasHandled = 0;
}

void CEvent::OnExecute()
{
    CEventCore* core = CEventCore::GetInstance();
    int pendingCount = core->m_pendingRemovals.Size();
    core->m_isDispatching = 1;

    CSystem* rootSystem = (CSystem*)CApplet::m_pApp->m_subsystem14;
    m_wasHandled = 0;

    CSystemElement* element;
    if (rootSystem->Query(0, kEventSystemHandle, &element)) {
        CSystem* eventSystem = (CSystem*)element->m_object;
        CSystemElement* eventElement;
        if (eventSystem->Query(0, m_eventId, &eventElement)) {
            CRegistry* listeners = (CRegistry*)eventElement->m_object;
            if (m_param != 0) {
                // only the listener registered under m_param
                if (listeners->Query(0, m_param, &element)) {
                    CEventListener* listener = (CEventListener*)element->m_object;
                    if (listener->m_callback(*this, listener)) {
                        if (!m_wasHandled) {
                            m_wasHandled = 1;
                        }
                        if (!m_wasEverHandled) {
                            m_wasEverHandled = 1;
                        }
                    }
                }
            } else {
                // broadcast to every listener of the event
                for (CRegistryItr itr(*listeners); !itr.IsDone(); itr++) {
                    CEventListener* listener = (CEventListener*)(*itr).m_object;
                    if (listener->m_callback(*this, listener)) {
                        if (!m_wasHandled) {
                            m_wasHandled = 1;
                        }
                        if (!m_wasEverHandled) {
                            m_wasEverHandled = 1;
                        }
                    }
                }
            }
        }
    }
    core->m_isDispatching = 0;

    // listeners unregistered during the dispatch are removed now
    if (pendingCount < core->m_pendingRemovals.Size()) {
        if (rootSystem->Query(0, kEventSystemHandle, &element)) {
            CSystem* eventSystem = (CSystem*)element->m_object;
            for (int index = pendingCount; index < core->m_pendingRemovals.Size() - pendingCount; index++) {
                CRegistry* listeners = NULL;
                CSystemElement* eventElement;
                if (eventSystem->Query(0, m_eventId, &eventElement)) {
                    listeners = (CRegistry*)eventElement->m_object;
                    listeners->Remove(*core->m_pendingRemovals[index]);
                }
                if (core->m_pendingRemovals[index] != NULL) {
                    delete core->m_pendingRemovals[index];
                }
                if (listeners != NULL && listeners->IsEmpty()) {
                    // last listener gone: drop the registry of the event
                    eventSystem->Remove(*eventElement);
                    delete listeners;
                    if (eventElement != NULL) {
                        delete eventElement;
                    }
                }
            }
        }
        core->m_pendingRemovals.SetSize(pendingCount);
    }
}

} } } }
