// b074i: real external class declaration, original 15a6c8 constructor / 4e14a0 vtable; the complete body belongs to SGImageWindow.cpp.
#ifndef SGIMAGEWINDOWCOMMAND_H
#define SGIMAGEWINDOWCOMMAND_H
#include "SGImageWindow.h"

class SGImageWindowCommand : public SGImageWindow {
public:
    SGImageWindowCommand();
    // Original 15b214/15b23c release no own members; the 115b98 derived destructor goes directly to the SG base class.
    virtual ~SGImageWindowCommand() {}
    virtual void OnPointerEvent(Event& event);
    virtual void Update();
    virtual bool IsLocalPos(int x, int y) const;
    virtual void OnFire() {} // Real bc slot; the 115c2c weak body is only bx lr.

    unsigned int m_command;       // 0x10c
    int m_commandParameter1;      // 0x110
    int m_commandParameter2;      // 0x114
    int m_commandParameter3;      // 0x118
    bool m_expandDragArea;        // 0x11c
    bool m_pressed;               // 0x11d
    // Historical m_unresolved11e: the original constructor does not initialize the two drag anchors.
    int m_pressGlobalX;           // 0x120
    int m_pressGlobalY;           // 0x124
    int m_hitPaddingX;            // 0x128
    int m_hitPaddingY;            // 0x12c, total 0x130
};
#endif
