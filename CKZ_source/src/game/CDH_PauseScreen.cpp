// CDH_PauseScreen.cpp - in-game pause menu.
#include "CDH_PauseScreen.h"
#include "CSettingsWindow.h"
#include "SoftkeysContainer.h"
#include "App.h"
#include "AppSettings.h"

/** Command ids handled by the pause screen (hashed command names). */
static const int CMD_PAUSE_BACK = (int)0x97204784;
static const int CMD_PAUSE_RESUME = 0x13179047;
/** Tunnel command sent when the pause screen opens. */
static const int CMD_PAUSE_OPENED = 0x47b537b6;

CDH_PauseScreen::CDH_PauseScreen(bool pauseMenu, bool flag)
{
    m_settingsWindow = new CSettingsWindow(pauseMenu, flag);
    m_settingsWindow->SetLayoutType((ELayoutType)1);
    m_settingsWindow->SetAlign(0x24);
    ((App*)WindowApp::m_instance)->m_mainLayer->AddModal(m_settingsWindow);
    AppSettings* settings = ((App*)WindowApp::m_instance)->m_settings;
    settings->m_field1e = !settings->m_field1e;
    WindowApp::HandleTunnelCommand(CMD_PAUSE_OPENED, 0, 0, 0);
}

CDH_PauseScreen::~CDH_PauseScreen()
{
    AppSettings* settings = ((App*)WindowApp::m_instance)->m_settings;
    settings->m_field1e = !settings->m_field1e;
}

Window* CDH_PauseScreen::PopulateSofkeys()
{
    return new SoftkeysContainer(this);
}

void CDH_PauseScreen::OnCreateState(Window** window, int state)
{
    *window = NULL;
}

void CDH_PauseScreen::OnStateInterrupt(int state, Window* window)
{
    switch (state) {
    case 1:
        if (window->GetInterrupt() == 2) {
            ((App*)WindowApp::m_instance)->m_timeScale = 1.0f;
            SetInterrupt(2);
        }
        break;
    case 2:
        {
            int interrupt = window->GetInterrupt();
            // Two separate tests (the original keeps both compares: 2 first, then 1).
            if (interrupt == 1) {
                PopState();
            } else if (interrupt == 2) {
                PopState();
            }
        }
        break;
    case 3:
        if (window->GetInterrupt() == 2) {
            PopState();
        }
        break;
    case 4:
        switch (window->GetInterrupt()) {
        case 1:
        case 2:
            PopState();
            break;
        }
        break;
    }
}

void CDH_PauseScreen::OnCommand(Event& event)
{
    if (event.m_id == CMD_PAUSE_BACK || event.m_id == CMD_PAUSE_RESUME) {
        SetInterrupt(2);
    }
}
