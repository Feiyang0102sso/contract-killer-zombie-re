// b074: button ABI from 0x1151b0 and vtable 0x4dd050; Window owns its children.
#ifndef CBH_BUTTON_H
#define CBH_BUTTON_H
#include "WindowBase.h"

class CBH_Button : public Window {
public:
    CBH_Button();
    virtual ~CBH_Button() {}
    virtual bool IsLocalPos(int x, int y) const;
    virtual void OnPointerEvent(Event& event);
    virtual void OnFire() {} // Original weak body 0x1154cc is a return.
    void FireCommand();
    void SetToggle(bool toggle);

    bool m_expandDragArea;         // ac, 1.5 rather than 1.0 bounds
    int m_unresolvedB0[4];         // b0..bc, not initialized by the original ctor
    int m_command;                // c0
    int m_commandParameter1;      // c4
    int m_commandParameter2;      // c8
    int m_commandParameter3;      // cc
    bool m_pressed;               // d0
    bool m_toggle;                // d1
    int m_pressGlobalX;           // d4
    int m_pressGlobalY;           // d8 (original intentionally uses LocalToGlobalX)
    int m_hitPaddingX;            // dc
    int m_hitPaddingY;            // e0
};
#endif
