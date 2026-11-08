// CKeyInputMapping.cpp - turns key events into action messages.
#include "CKeyInputMapping.h"

#include "CApplet.h"
#include "CApp.h"
#include "CAppExecutor.h"
#include "CInputMap.h"
#include "CEventListener.h"
#include "CMessage.h"
#include "CHandleFactory.h"
#include "CRegistryItr.h"
#include "CStdUtil_Cocoa.h"

namespace com { namespace glu { namespace platform { namespace systems {

CKeyInputMapping::~CKeyInputMapping()
{
    Destroy();
}

void CKeyInputMapping::Destroy()
{
    if (m_listener == NULL) {
        return;
    }
    m_listener->UnregisterAll();
    if (m_listener != NULL) {
        delete m_listener;
    }
    m_listener = NULL;
}

unsigned char CKeyInputMapping::Init()
{
    CEventListener* listener = new CEventListener();
    m_listener = listener;
    if (listener == NULL) {
        return 0;
    }
    unsigned int handle = core::CHandleFactory::GetInstance()->CreateHandle();
    listener->m_object = this;
    listener->m_handle = handle;
    listener->m_callback = EventCB;
    m_listener->Register(Event_6740942f, DefaultPriority);
    m_listener->Register(Event_AppResume, DefaultPriority);
    m_listener->Register(Event_518e1a3b, DefaultPriority);
    m_listener->Register(Event_KeyPressed, DefaultPriority);
    m_listener->Register(Event_KeyReleased, DefaultPriority);
    m_isKeyDown = 0;
    m_heldTime = 0;
    m_lastPostTime = 0;
    return 1;
}

unsigned char CKeyInputMapping::EventCB(CEvent& event, void* listener)
{
    CKeyInputMapping* mapping = (CKeyInputMapping*)((CEventListener*)listener)->m_object;
    CMessage& message = (CMessage&)event;
    unsigned int keyCode = 0;
    if (message.m_argumentCount >= 1) {
        keyCode = (unsigned int)message.m_arguments[0].m_data;
    }
    mapping->HandleEvent(event.m_eventId, keyCode, message.m_argumentCount);
    return 0;
}

unsigned char CKeyInputMapping::Register(unsigned int inputMapHandle, unsigned int action, core::eKeyCode keyCode,
                                         unsigned char flags, int repeatDelay, int repeatInterval,
                                         unsigned char repeatAfterFirst)
{
    unsigned char registered = 0;
    CSystemElement* element = NULL;
    CExecutableRegistry* executables = CApplet::m_pApp->GetExecutor()->m_registry;
    if (executables->Query(0, inputMapHandle, &element)) {
        CInputMap* inputMap = (CInputMap*)element->m_object;
        CKeyInputMapping* mapping = new CKeyInputMapping();
        if (mapping != NULL) {
            mapping->Init();
            mapping->m_action = action;
            mapping->m_keyCode = keyCode;
            mapping->m_flags = flags;
            mapping->m_repeatDelay = repeatDelay;
            mapping->m_repeatInterval = repeatInterval;
            mapping->m_repeatAfterFirst = repeatAfterFirst;
            inputMap->Add(*inputMap->CreateSystemElement(core::CHandleFactory::GetInstance()->CreateHandle(), mapping,
                                                         ClassId, DefaultPriority));
            registered = 1;
        }
    }
    return registered;
}

void CKeyInputMapping::Unregister(unsigned int inputMapHandle, unsigned int action, core::eKeyCode keyCode)
{
    CSystemElement* element = NULL;
    CExecutableRegistry* executables = CApplet::m_pApp->GetExecutor()->m_registry;
    if (!executables->Query(0, inputMapHandle, &element)) {
        return;
    }
    CInputMap* inputMap = (CInputMap*)element->m_object;
    CRegistryItr itr(*inputMap);
    while (!itr.IsDone()) {
        CSystemElement* mappingElement = &*itr;
        CKeyInputMapping* mapping = (CKeyInputMapping*)mappingElement->m_object;
        if (mapping->m_classId != ClassId) {
            continue;
        }
        if (keyCode != 0 && mapping->m_keyCode != keyCode) {
            itr++;
            continue;
        }
        if (action != 0 && mapping->m_action != action) {
            itr++;
            continue;
        }
        if (mapping != NULL) {
            delete mapping;
        }
        itr++;
        inputMap->Remove(*mappingElement);
        if (mappingElement != NULL) {
            delete mappingElement;
        }
    }
}

unsigned char CKeyInputMapping::HandleEvent(unsigned int eventId, unsigned int keyCode, unsigned int argumentCount)
{
    switch (eventId) {
    case Event_KeyPressed:
        if (m_keyCode == keyCode) {
            if (m_flags & Flag_PostOnPress) {
                PostAction();
            }
            m_isKeyDown = 1;
        }
        break;
    case Event_KeyReleased:
        if (m_keyCode != keyCode) {
            break;
        }
        if (m_flags & Flag_PostOnRelease) {
            PostAction();
        }
        // fall through: the key is up again
    case Event_6740942f:
    case Event_AppResume:
    case Event_518e1a3b:
        m_isKeyDown = 0;
        m_heldTime = 0;
        m_lastPostTime = 0;
        break;
    }
    return 0;
}

void CKeyInputMapping::OnExecute()
{
    if (!(m_flags & Flag_PostWhileHeld) || !m_isKeyDown) {
        return;
    }
    int heldBefore = m_heldTime;
    m_heldTime = heldBefore + CApplet::m_pApp->GetExecutor()->m_frameMillis;
    if (heldBefore < 1) {
        return;
    }
    int sinceRepeatStart = m_heldTime - m_repeatDelay;
    if (sinceRepeatStart < 1) {
        return;
    }
    if (m_repeatInterval >= 1) {
        // post every m_repeatInterval ms, catching up on missed repeats
        for (;;) {
            if (m_lastPostTime != 0) {
                if (!m_repeatAfterFirst) {
                    return;
                }
                if (sinceRepeatStart - m_lastPostTime < m_repeatInterval) {
                    return;
                }
            }
            PostAction();
            int step = m_repeatInterval;
            if (sinceRepeatStart < step) {
                step = sinceRepeatStart;
            }
            m_lastPostTime += step;
        }
    }
    if (m_lastPostTime == 0 || m_repeatAfterFirst) {
        PostAction();
        m_lastPostTime = m_heldTime;
    }
}

void CKeyInputMapping::PostAction()
{
    if (m_action == 0) {
        return;
    }
    CMessage* message = new CMessage(this, m_action, m_targetHandle, 2,
                                     CMessage::ArgumentType_UInt32, 0, (void*)m_keyCode,
                                     CMessage::ArgumentType_UInt32, 0, (void*)CStdUtil_Cocoa::GetUpTimeMS());
    if (message == NULL) {
        return;
    }
    CExecutableRegistry* executables = CApplet::m_pApp->GetExecutor()->m_registry;
    executables->Add(*executables->CreateSystemElement(core::CHandleFactory::GetInstance()->CreateHandle(), message,
                                                       CMessage::ClassId, 0x73333308));
}

} } } }
