// k: complete interfaces of the three classes in original 1505ec..150a2c; sizes cross-checked against the actual HUD new.
#ifndef CCIVILIANHPBAR_H
#define CCIVILIANHPBAR_H
#include "WindowBase.h"
#include "SGImageWindow.h"
#include "BaseDialog.h"
#include "SG_Presenter.h"

class CCivilianHPBar : public Window {
public:
    CCivilianHPBar();
    virtual ~CCivilianHPBar();
    virtual void Init();
    virtual void Update();
    virtual void Paint(com::glu::platform::graphics::ICGraphics2d& graphics);
    void SetPercent(float percent);
    void SetScale(float scale);

    SG_Presenter m_presenter;      // ac..e7, still really constructed/destructed by the original, even though this owner does not use it directly.
    int m_unresolvedE8;            // e8, only zeroed by the original constructor; semantics not guessed.
    int m_unresolvedEc;            // ec, same as above.
    float m_percent;               // f0
    SGImageWindow* m_background;   // f4
    SGImageWindow* m_changeEffect; // f8
    SGClippedImageWindow* m_fill;  // fc, total 100.
};

class CCivilianNameBar : public Window {
public:
    CCivilianNameBar(XString name);
    virtual ~CCivilianNameBar() {} // Original 1509e4 only calls Window; the tree is responsible for the text child window.
    TextWindow* m_text;            // ac, total b0.
};

class CCivilianBar : public Window {
public:
    CCivilianBar(CCivilianHPBar* hpBar, CCivilianNameBar* nameBar);
    virtual ~CCivilianBar() {} // Original 150a08 only calls Window.
    void SetHPBar(CCivilianHPBar* hpBar);
    void SetNameBar(CCivilianNameBar* nameBar);
    CCivilianHPBar* GetHPBar();
    CCivilianHPBar* m_hpBar;     // ac
    CCivilianNameBar* m_nameBar; // b0, total b4.
};
#endif
