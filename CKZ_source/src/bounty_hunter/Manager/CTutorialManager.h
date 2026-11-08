// PROVISIONAL (created by b057x): owner src/bounty_hunter/Manager/CTutorialManager.cpp not written yet
// CTutorialManager.h - tutorial hints (a window); only what src/general/App.cpp uses is declared.
// Restored by b079: the preceding PROVISIONAL comment is historical.
#ifndef CTUTORIALMANAGER_H
#define CTUTORIALMANAGER_H

#include <stddef.h>
#include "WindowBase.h"
#include "SGImageWindow.h"

class TextWindow;

/** Character image with a localized tutorial caption. */
class CTutorialFloatingWindow : public SGImageWindow {
public:
    enum EPlace { PLACE_TOP = 0, PLACE_CENTER = 1 };
    CTutorialFloatingWindow();
    inline virtual ~CTutorialFloatingWindow() {}
    void SetText(const char* text, EPlace place);

    TextWindow* m_text;         // 0x10c
};

class CTutorialManager : public WindowTransparent {
public:
    CTutorialManager();
    inline virtual ~CTutorialManager() {}
    int GetState() const;
    void NextTutorialStep(int steps, bool skipMessage);
    void ShowStateInitMessage();
    void ShowFloatingMessage(const char* text, CTutorialFloatingWindow::EPlace place);
    /** The first string argument is unused in the shipped implementation. */
    void ShowMessage(const char* unused, const char* text, bool once);
    bool ShowSafehouse() const;
    bool IsShowShop();
    bool IsShowHome();
    int GetCurrentShopTabFilter();
    int GetCurrentShopTab();
    bool CanCloseShop();
    bool CanCloseNetwork();
    virtual void Update();
    bool IsShopAttraction();
    bool IsStartButtonBlink();
    bool IsEquipLocked(int slot);
    int IsEquipButtonBlink(int slot);
    bool IsBuyButtonBlink();
    bool OnlyTutorialMissions() const;
    bool IsShopBackButtonHide();
    bool IsMapShopBlink() const;
    const char* GetActiveTutorialMissionName() const;
    int IsMapLocationBlink();
    bool IsSwitchWeaponButtonBlink();
    bool IsShowSwitchWeaponButton();
    int IsWeaponButtonBlink(int slot);
    bool IsMenuBarBlocked();
    virtual void OnCommand(Event& event);
    /** True when a mission starts by itself (tutorial). */  // added by b048
    bool IsAutoStartMission();  // added by b048
    bool IsPlayerInTutorial();  // added by b077
    /** Achievements are not given during this part of the tutorial. */  // added by b077
    bool IsAchievementsForbidden();  // added by b077
    /** True once the first (tutorial) mission was finished. */  // added by b055x
    bool FirstMissionCompleted() const;  // added by b055x
    /** Name of the first tutorial mission. */  // added by b055x
    const char* GetFirstTutorialMission() const;  // added by b055x
    /** Quest id of the active tutorial step. */  // added by b055x
    int GetActiveQuestId() const;  // added by b055x

    // Former m_dataac[0xb8 - 0xac]: layout not known yet (size from App.cpp).
    const char* m_lastMessage;                // 0xac constructor intentionally leaves this unset
    bool m_messagesSuppressed;                // 0xb0
    CTutorialFloatingWindow* m_floating;       // 0xb4
};

void ShowFloatingMessage(const char* text);
void ResetFloatingMessage();

#endif
