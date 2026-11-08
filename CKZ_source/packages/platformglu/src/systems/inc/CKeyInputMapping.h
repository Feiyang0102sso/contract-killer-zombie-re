// CKeyInputMapping.h - posts a message (the "action") when a key is pressed, released or held.
// (created by b011x)
#ifndef CKEYINPUTMAPPING_H
#define CKEYINPUTMAPPING_H

#include <stddef.h>
#include "CInputMapping.h"
#include "CKeyCode.h"

namespace com { namespace glu { namespace platform { namespace systems {

class CEvent;
class CEventListener;

class CKeyInputMapping : public CInputMapping {
public:
    /** Runtime class id: CStringToKey("CKeyInputMapping"). */
    enum { ClassId = 0x764a96e1 };

    /** m_flags bits. */
    enum {
        Flag_PostOnPress = 1,       // post when the key goes down
        Flag_PostWhileHeld = 2,     // post repeatedly while the key is held (OnExecute)
        Flag_PostOnRelease = 4      // post when the key goes up
    };

    /** Event ids the mapping listens to. */
    enum {
        Event_KeyPressed = 0xc7424be6,      // CStringToKey("KeyPressed"), argument 0: key code
        Event_KeyReleased = 0x43a39818,     // CStringToKey("KeyReleased"), argument 0: key code
        Event_AppResume = 0x75424363,       // CStringToKey("AppResume")
        Event_6740942f = 0x6740942f,        // name unknown, resets the key state
        Event_518e1a3b = 0x518e1a3b         // name unknown, resets the key state
    };

    CKeyInputMapping()
    {
        m_classId = ClassId;
        m_listener = NULL;
        m_keyCode = (core::eKeyCode)0;
        m_flags = 0;
        m_action = 0;
        m_repeatDelay = 0;
        m_repeatInterval = 0;
        m_repeatAfterFirst = 1;
        m_isKeyDown = 0;
        m_heldTime = 0;
        m_lastPostTime = 0;
    }

    virtual ~CKeyInputMapping();

    virtual void OnExecute();
    virtual unsigned char Init();
    virtual void Destroy();

    /** CEventListener callback: forwards key events to HandleEvent of the owning mapping. */
    static unsigned char EventCB(CEvent& event, void* listener);

    /**
     * Map `keyCode` to the message `action` in the input map registered under `inputMapHandle`
     * in the application executor. Returns 1 when the mapping was added.
     */
    static unsigned char Register(unsigned int inputMapHandle, unsigned int action, core::eKeyCode keyCode,
                                  unsigned char flags, int repeatDelay, int repeatInterval,
                                  unsigned char repeatAfterFirst);
    /** Remove the mappings matching `action` and `keyCode` (0 matches everything). */
    static void Unregister(unsigned int inputMapHandle, unsigned int action, core::eKeyCode keyCode);

    unsigned char HandleEvent(unsigned int eventId, unsigned int keyCode, unsigned int argumentCount);
    /** Post m_action as a CMessage through the application executor. */
    void PostAction();

    CEventListener* m_listener;     // 0x20
    core::eKeyCode m_keyCode;       // 0x24
    unsigned char m_flags;          // 0x28 Flag_*
    unsigned int m_action;          // 0x2c message id posted, 0 = nothing
    int m_repeatDelay;              // 0x30 ms the key must be held before repeating
    int m_repeatInterval;           // 0x34 ms between repeats, < 1 = post once
    unsigned char m_repeatAfterFirst; // 0x38 keep posting after the first post while held
    unsigned char m_isKeyDown;      // 0x39
    int m_heldTime;                 // 0x3c ms the key has been held
    int m_lastPostTime;             // 0x40 m_heldTime of the last post (0 = none yet)
};

} } } }

#endif
