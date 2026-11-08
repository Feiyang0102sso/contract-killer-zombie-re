// b074i: original 148cf8 clearly shows real CBH_Button inheritance, total 0xf8; only external declarations and layout here.
#ifndef CGEARSETTINGSBUTTON_H
#define CGEARSETTINGSBUTTON_H
#include "CBH_Button.h"
class SGImageWindowRotatable;

class CGearSettingsButton : public CBH_Button {
public:
    CGearSettingsButton();
    virtual ~CGearSettingsButton();
    virtual void OnCommand(Event& event);
    virtual void Update();
    virtual void Paint(com::glu::platform::graphics::ICGraphics2d& graphics);

    SGImageWindowRotatable* m_firstGear;  // 0xe4, owned by the Window tree
    SGImageWindowRotatable* m_secondGear; // 0xe8
    // Historical int m_fieldec/m_fieldf0; original 148eee/148f02 clearly read/compute them as float.
    float m_firstAngle;                  // 0xec
    float m_secondAngle;                 // 0xf0
    bool m_flagf4;                      // 0xf4
};
#endif
