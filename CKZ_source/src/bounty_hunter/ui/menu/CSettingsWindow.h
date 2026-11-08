// PROVISIONAL (created by b055x): owner src/bounty_hunter/ui/menu/CSettingsWindow.cpp not written yet
// CSettingsWindow.h - settings / pause menu window; only what src/game uses.
#ifndef CSETTINGSWINDOW_H
#define CSETTINGSWINDOW_H

#include "CZombieDialogWindow.h"
class SGButton;
class TextWindow;

// The historical thin header used Window; the real base class constructor at original 157bf0 is CZombieDialogWindow(2).
class CSettingsWindow : public CZombieDialogWindow {
public:
    /** `pauseMenu`: the in-game pause menu instead of the main settings (names guessed). */
    CSettingsWindow(bool pauseMenu, bool flag);
    virtual ~CSettingsWindow();
    virtual void OnStartupEasy();
    virtual void OnEasyFinished();
    virtual void OnClosed();
    virtual void OnCommand(Event& event);
    struct OnOffButtonDesc {
        Window* Create(const char* label, const char* unusedOn, const char* unusedOff, bool pressed);
        SGButton* m_button;
        TextWindow* m_label;
        Window* m_container;
    };
    class StateButton : public Window {
    public:
        StateButton(const char* label);
        virtual ~StateButton() {}
        virtual void OnCommand(Event& event);
        void UpdateValText();
        SGButton* m_button; // ac
        TextWindow* m_label;
        TextWindow* m_value;
        Vector<XString> m_values; // b8
        int m_selectedIndex; // c8
        int m_unresolvedCc;
    };
    Window* CreatePauseMenu();
    Window* CreateSettingMenu();
    void ShowContent(Window* content);
    // Historical thin header: unsigned char m_dataac[0x164 - 0xac];
    OnOffButtonDesc m_sound; // 118
    OnOffButtonDesc m_music; // 124
    OnOffButtonDesc m_fire; // 130
    OnOffButtonDesc m_push; // 13c
    unsigned int m_reserved148[3]; // The original memset covers 148..153; no business access seen.
    StateButton* m_sensitivity; // 154
    Window* m_activeContent; // 158
    Window* m_contentHost; // 15c
    bool m_pauseMenu; // 160
    bool m_returnGps; // 161
};

#endif
