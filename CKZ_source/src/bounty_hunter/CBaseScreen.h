// b074: complete owner layout from 0x119b04; Window retains all child ownership.
#ifndef CBASESCREEN_H
#define CBASESCREEN_H
#include "WindowBase.h"
class TextWindow;
class SGImageWindowCommand;
class CBaseScreen : public Window {
public:
    CBaseScreen(bool showBack, bool showTitleBar, bool showSettings);
    virtual ~CBaseScreen();
    virtual void OnStartupEasy();
    virtual void Init();
    virtual void Shutdown();
    virtual void OnCommand(Event& event);
    virtual void Paint(com::glu::platform::graphics::ICGraphics2d& graphics);
    void AddBackButton();
    void AddSettingsButton();
    int GetTitleBarHeight();
    void SetTitleText(XString text);
    Window* m_content;              // ac
    TextWindow* m_title;            // b0
    bool m_showTitleBar;            // b4
    int m_titleHeight;              // b8
    SGImageWindowCommand* m_back;   // bc
};
#endif
