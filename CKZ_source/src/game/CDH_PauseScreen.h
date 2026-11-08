// CDH_PauseScreen.h - in-game pause menu: hosts a CSettingsWindow as a modal of the main layer.
#ifndef CDH_PAUSESCREEN_H
#define CDH_PAUSESCREEN_H

#include "CDH_SoftkeysScreen.h"

class CSettingsWindow;

/** Pause screen (0xbc bytes); toggles AppSettings::m_field1e while it lives. */
class CDH_PauseScreen : public CDH_SoftkeysScreen {
public:
    /** `pauseMenu` / `flag` are passed to CSettingsWindow. */
    CDH_PauseScreen(bool pauseMenu, bool flag);
    virtual ~CDH_PauseScreen();

    virtual void OnCommand(Event& event);
    virtual int ShouldBeModal() { return 1; }
    virtual void OnCreateState(Window** window, int state);
    virtual void OnStateInterrupt(int state, Window* window);
    virtual Window* PopulateSofkeys();

    int m_fieldac;                      // 0xac not used here
    int m_fieldb0;                      // 0xb0 not used here
    CSettingsWindow* m_settingsWindow;  // 0xb4
    int m_fieldb8;                      // 0xb8 not used here
};

#endif
