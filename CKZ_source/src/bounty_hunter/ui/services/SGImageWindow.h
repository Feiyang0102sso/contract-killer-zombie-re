// PROVISIONAL (created by b079): owner src/bounty_hunter/ui/services/SGImageWindow.cpp not written yet
// Virtual overrides from SGImageWindow's vtable; size 0x10c from CTutorialFloatingWindow.
#ifndef SGIMAGEWINDOW_H
#define SGIMAGEWINDOW_H

#include "WindowBase.h"
#include "SG_Presenter.h"
// b081j: complete owner recovered; the historical PROVISIONAL comment above is kept.

class SGImageWindow : public Window {
public:
    SGImageWindow();
    virtual ~SGImageWindow();
    virtual void Init();
    virtual void OnCommand(Event& event);
    virtual void Update();
    virtual void Paint(com::glu::platform::graphics::ICGraphics2d& graphics);

    void SetArchetypeCharacter(int character, int variant);
    void SetAnimation(int animation, unsigned char repeat);
    void SetBlinkCmd(unsigned int command); // b074i: original 15a46c, writes +b4.
    void SetBlink(bool enabled); // Original 15a454, writes +c0, then restores opacity when needed.
    void SetTranslateCoords(bool enabled);
    void UpdateSize();
    void SetRepeatCount(int count, bool closeWhenFinished);
    void SetScale(int percent);

    // Historical: unsigned char m_unresolved[0x10c - 0xac];
    // b074i: UpdateSize at original 15a4f4/15a4f8 writes short integers; Base119b98 reads +bc as signed.
    int m_repeatCount;                       // 0xac
    bool m_closeWhenFinished;                // 0xb0
    unsigned int m_blinkCommand;             // 0xb4
    // b117-vb: 0xb8 is a CRectangle<short> value-initialized in the constructor (original
    // memset(8) lowers to str +0xbc, str +0xb8, and makes SGImageWindow::C2 expensive enough that
    // SGImageWindowRotatable::C2 / SGHudImageWindow::C2 stay out of line). The four short names
    // are kept as aliases because other units still use them.
    union {
        struct {
            short m_imageX;                  // 0xb8
            short m_imageY;                  // 0xba
            short m_imageWidth;              // 0xbc
            short m_imageHeight;             // 0xbe
        };
        com::glu::platform::components::CRectangle<short> m_imageRect; // 0xb8
    };
    // Historical m_unresolvedC0[0x10c - 0xc0]; the first bool is now made concrete per SetBlink.
    bool m_blinking;                         // 0xc0
    SG_Presenter m_presenter;                // 0xc4
    com::glu::platform::components::CRectangle<short> m_frameBounds; // 0x100
    bool m_translateCoords;                  // 0x108
};

class SGImageWindowRotatable : public SGImageWindow {
public:
    SGImageWindowRotatable();
    virtual ~SGImageWindowRotatable() {}
    virtual void Paint(com::glu::platform::graphics::ICGraphics2d& graphics);
    int m_angle;                            // 0x10c
};

class SGHudImageWindow : public SGImageWindow {
public:
    SGHudImageWindow() {}
    bool CheckCollision(int x, int y, float factor);
};

class SGClippedImageWindow : public SGHudImageWindow {
public:
    SGClippedImageWindow();
    virtual ~SGClippedImageWindow() {}
    void SetClipping(float x, float y);
    virtual void Paint(com::glu::platform::graphics::ICGraphics2d& graphics);
    float m_clipX;                           // 0x10c
    float m_clipY;                           // 0x110
};

#endif
