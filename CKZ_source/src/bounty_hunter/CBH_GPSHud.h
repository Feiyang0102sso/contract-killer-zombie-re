// b074: GPS HUD and its owner-local subclasses; all inline bodies have original weak ranges.
#ifndef CBH_GPSHUD_H
#define CBH_GPSHUD_H
#include "CBaseScreen.h"
#include "SGImageWindowCommand.h"
#include "SG_Presenter.h"
#include "CStdUtil_Cocoa.h"
#include "MathLib.h"

class CHudStatsPanel : public Window {
public:
    CHudStatsPanel();
    virtual ~CHudStatsPanel() {}
    SGImageWindowCommand* CreateAddButton(int font);
};

class CBH_GPSHud : public CBaseScreen {
public:
    class CShopBlinkingButton : public SGImageWindowCommand {
    public:
        // No user ctor: original value-initialization zeros all 130 bytes.
        virtual ~CShopBlinkingButton() {}
        virtual void Update()
        {
            SGImageWindowCommand::Update();
            if (m_blinking) {
                float opacity = MathLib::Fabs((float)((int)((unsigned int)CStdUtil_Cocoa::GetUpTimeMS() % 600) - 300)) / 300.0f;
                SetOpacity(opacity + 0.0f);
            }
        }
    };
    CBH_GPSHud();
    virtual ~CBH_GPSHud() {}
    virtual void Paint(com::glu::platform::graphics::ICGraphics2d& graphics);
    virtual void Update();
    virtual void OnCommand(Event& event);
    virtual void OnGetFocus();
    virtual void OnLostFocus();
    CShopBlinkingButton* m_shop; // c0
    int m_unresolvedC4;         // c4, not initialized by original ctor
    SG_Presenter m_top;         // c8
    SG_Presenter m_bottom;      // 104; total140
};
#endif
