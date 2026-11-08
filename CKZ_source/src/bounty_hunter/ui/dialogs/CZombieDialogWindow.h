// PROVISIONAL (created by b057x): owner src/bounty_hunter/ui/dialogs/CZombieDialogWindow.cpp not written yet
// CZombieDialogWindow.h - styled message dialog; only what src/general uses.
#ifndef CZOMBIEDIALOGWINDOW_H
#define CZOMBIEDIALOGWINDOW_H

#include "WindowBase.h"
#include "CBH_Button.h"
class SGImageWindow;
class SGButton;
class TextWindow;
class CItem;

class CZombieDialogWindow : public Window {
public:
    /** Buttons of the dialog. */
    enum EDialogButton {
        BUTTON_OK = 1
    };

    CZombieDialogWindow(int style);
    virtual ~CZombieDialogWindow();  // added by b063 (MessageBox base destruction at 0x0e0594)
    virtual void OnStartupEasy();  // added by b063 (original MessageBox vtable +0x24)
    virtual void OnCommand(Event& event);  // added by b063 (+0x68)
    virtual void OnPointerEvent(Event& event);  // added by b063 (+0x70)

    // b081j: original 14f9e4 really returns the new SGButton*; all existing void calls can still ignore the return value.
    SGButton* AddButton(EDialogButton button, unsigned int command, int param1, int param2, bool closes, int width);
    SGButton* CreateButton(EDialogButton button);
    void ShowClosingCross(unsigned int command);
    void SetText(const XString& text, bool center, com::glu::platform::graphics::CFont* font);
    void SetTitle(const XString& title);
    void InitOkMessage(const XString& title, const XString& text, bool flag,
                       com::glu::platform::graphics::CFont* font); // original 14f8e4

    // Historical unsigned char m_dataAc[0x118 - 0xac]; layout not known yet.
    // Historical unsigned char m_dataAc[0xb8 - 0xac];
    SGImageWindow* m_background;             // ac
    Window* m_titleContainer;                // b0
    Window* m_contentContainer;              // b4
    Window* m_messageContainer;             // 0xb8, 14f5fc assigns a Window-derived entity
    // Historical unsigned char m_dataBc[0x118 - 0xbc];
    Window* m_buttonOuter;                   // bc
    Window* m_buttonGrid;                    // c0
    TextWindow* m_bodyText;                  // c4
    TextWindow* m_titleText;                 // c8
    int m_buttonCount;                       // cc
    bool m_disableEasy;                      // d0
    SGButton* m_buttons[16];                 // d4..113, not zeroed by the original ctor
    bool m_hasIgnoreButton;                  // 114
};

// b067h: owned by the same original CZombieDialogWindow.cpp; no separate pseudo-owner.
class CNextWaveZombieDialogWindow : public CZombieDialogWindow {
public:
    explicit CNextWaveZombieDialogWindow(CItem* item); // Original 1500e4, directly Zombie(style2).
    virtual ~CNextWaveZombieDialogWindow() {}
    virtual void Shutdown();
    // Original Game allocates 0x118; no new members.
};

// Original 14fffe directly Zombie(style3), GPSGame11b43c allocates 11c; body not recovered.
class CRecommendedWeaponDialog : public CZombieDialogWindow {
public:
    CRecommendedWeaponDialog(int weapon, int grenadePack);
    virtual ~CRecommendedWeaponDialog() {}
    virtual void OnCommand(Event& event);
    // Historical unsigned char m_unmapped118[4];
    unsigned int m_recommendCommand;          // 118
};

// Original u545 naturally emits this in-header weak first; here opacity is updated following the real pressed state, no new members.
class CustomButton : public CBH_Button {
public:
    virtual void Update()
    {
        float opacity = 1.0f;
        if (m_pressed) {
            opacity = 0.7f;
        }
        SetOpacity(opacity);
    }
};

class CServerGiftDialog : public CZombieDialogWindow {
public:
    CServerGiftDialog(int money, int credits, int third, int fourth);
    virtual ~CServerGiftDialog() {}
    virtual void OnCommand(Event& event);
    int m_money;                             // 118
    int m_credits;                           // 11c
    int m_third;                             // 120, the original owner only stores it
    int m_fourth;                            // 124, the original owner only stores it
};

#endif
