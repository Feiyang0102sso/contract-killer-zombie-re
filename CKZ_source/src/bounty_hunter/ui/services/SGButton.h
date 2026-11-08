// b074i: original 15ae88 truly inherits CBH_Button / allocates 0xf8; the complete body belongs to SGImageWindow.cpp.
#ifndef SGBUTTON_H
#define SGBUTTON_H
#include "CBH_Button.h"
#include "CFontMgr.h"
class SGImageWindow;
class TextWindow;

// b081j: the original shared Update and the two derived vtables confirm the intermediate base class; the e4 field keeps its original name.
class BlinkingButton : public CBH_Button {
public:
    BlinkingButton() : m_blinking(false) {}
    virtual void Update();
    bool m_blinking;                 // 0xe4
};

class SGButton : public BlinkingButton {
public:
    SGButton(int archetype, int character, int normalAnimation, int pressedAnimation);
    virtual ~SGButton() {}
    virtual void Update();
    TextWindow* SetText(const XString& text, eGameFont font);

    // Historical bool m_blinking(+e4), now owned by the real BlinkingButton base class.
    SGImageWindow* m_normalImage;   // 0xe8, owned by the Window tree
    SGImageWindow* m_pressedImage;  // 0xec
    TextWindow* m_textWindow;       // 0xf0
    bool m_flagf4;                  // 0xf4
};

class SGToggleButton : public BlinkingButton {
public:
    SGToggleButton(int archetype, int character, int normalAnimation, int selectedAnimation);
    virtual ~SGToggleButton() {}
    virtual void OnFire();
    void UpdateToggle();
    SGImageWindow* m_normalImage;    // 0xe8
    SGImageWindow* m_selectedImage;  // 0xec
    bool m_selected;                // 0xf0
    bool m_allowTurnOff;            // 0xf1
};
#endif
