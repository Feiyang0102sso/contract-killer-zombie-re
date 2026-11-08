// b074i: original 148f60 real Window inheritance / original Hud allocates 108; only external declarations and size.
#ifndef CPLAYERXPBAR_H
#define CPLAYERXPBAR_H
#include "WindowBase.h"
#include "SG_Presenter.h"
class SGImageWindow;
class SGClippedImageWindow;
class TextWindow;
class CPlayerXPBar : public Window {
public:
    CPlayerXPBar();
    virtual ~CPlayerXPBar();
    virtual void Init();
    virtual void Update();
    virtual void Paint(com::glu::platform::graphics::ICGraphics2d& graphics);
    void SetPercent(float percent);
    void UpdateInfo();
    // Historical unsigned char m_unresolvedAc[0x108 - 0xac], now converted to the full original layout.
    SG_Presenter m_presenter;       // ac, keeps the real constructor/destructor.
    int m_fielde8;
    int m_fieldec;
    float m_percent;               // f0
    int m_cachedXP;                // f4
    SGImageWindow* m_bar;          // f8
    TextWindow* m_levelText;       // fc
    TextWindow* m_xpText;          // 100
    SGClippedImageWindow* m_fill;  // 104
};
#endif
