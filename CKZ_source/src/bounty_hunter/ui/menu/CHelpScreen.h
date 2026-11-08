// j: complete layout of the original Help and the four-quadrant unlock window of the same owner.
#ifndef CHELPSCREEN_H
#define CHELPSCREEN_H
#include "CBaseScreen.h"
class TextWindow;
class CTabWindow;
class CUnlockWindow : public Window {
public:
    struct FingerZone {
        FingerZone(int x, int y, int width, int height)
            : m_x(x), m_y(y), m_width(width), m_height(height), m_flag(false) {}
        int m_x;
        int m_y;
        int m_width;
        int m_height;
        bool m_flag;
    };
    CUnlockWindow();
    virtual ~CUnlockWindow();
    virtual void OnPointerPressed(Event& event);
    virtual void OnPointerReleased(Event& event);
    virtual void OnPointerDragged(Event& event);
    void InitFingerZones();
    FingerZone* m_zones[4]; // ac
    bool m_pressed; // bc
    bool m_unlocked; // bd
    int m_nextZone; // c0
};
class CHelpScreen : public CBaseScreen {
public:
    CHelpScreen();
    virtual ~CHelpScreen();
    virtual void Paint(com::glu::platform::graphics::ICGraphics2d& graphics);
    virtual void Update();
    virtual void OnCommand(Event& event);
    void AddTab(XString title, XString text, TextWindow** output);
    XString m_debugText; // c0
    ImageRes m_background; // c4
    TextWindow* m_aboutText; // cc
    CTabWindow* m_tabs; // d0
    CUnlockWindow* m_unlock; // d4
    bool m_pendingReveal; // d8
};
#endif
