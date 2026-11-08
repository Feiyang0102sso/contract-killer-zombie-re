// j: keeps the original settings menu's full side effects, the index 5 boundary, and the independent consumption discipline for each event.
#include "CSettingsWindow.h"
#include "CHelpScreen.h"
#include "SGButton.h"
#include "BaseDialog.h"
#include "SScreenAdaptingConstant.h"
#include "CGameAnalytics.h"
#include "CTutorialManager.h"
#include "CSaveManager.h"
#include "CBH_Player.h"
#include "CMissionManager.h"
#include "CHuntingInfo.h"
#include "CSwerveGame.h"
#include "DGHelper.h"
#include "AppSettings.h"
#include "App.h"
#include <string.h>

CSettingsWindow::CSettingsWindow(bool pauseMenu, bool flag) : CZombieDialogWindow(2)
{
    // Original 157c06 clears exactly five 12B slots; the last three words are not yet proven to be the fifth desc.
    memset(&m_sound, 0, 60);
    m_pauseMenu = pauseMenu;
    m_returnGps = flag;
    CGameAnalytics::logMenuAccessed(XString(L"Settings"));
    Window* host = new Window(false);
    host->SetPercentWidth(100, 0, 0);
    host->SetHeightByContent(0, 0);
    host->SetLayoutType((ELayoutType)1);
    host->SetCellPos(0, 0, 1, 1);
    Window* content;
    if (pauseMenu) content = CreatePauseMenu();
    else content = CreateSettingMenu();
    m_activeContent = content;
    m_contentHost = host;
    ShowContent(content);
    AddToFront(host);
    WindowApp::HandleTunnelCommand((int)0xed83def9u, 1, 0, 0);
}

CSettingsWindow::~CSettingsWindow() { WindowApp::HandleTunnelCommand((int)0xed83def9u, 0, 0, 0); }

Window* CSettingsWindow::CreatePauseMenu()
{
    SetTitle(ResString("IDS_PAUSED").Upper());
    SScreenAdaptingConstant spacing(3);
    Window* content = new Window(false);
    content->SetWidthByContent(0, 0);
    content->SetInsetSpacing(App::Scale() * 20, 0);
    content->SetHeightByContent(0, 0);
    content->SetLayoutType((ELayoutType)1);
    content->SetAlign(0x24);
    SGButton* button = new SGButton(2, 0, 0, 1);
    button->SetText(ResString("IDS_CONTINUE").Upper(), (eGameFont)26);
    button->SetCellPos(0, 0, 1, 1);
    button->m_command = 0x3b0ba067;
    button->m_commandParameter1 = 0;
    button->m_commandParameter2 = 0;
    button->m_commandParameter3 = 0;
    button->SetOutsetSpacing(spacing(), 0);
    content->AddToFront(button);
    int row = 1;
    if (m_returnGps) {
        button = new SGButton(2, 0, 0, 1);
        button->SetText(ResString("IDS_MENU_RETURN_TO_GPS").Upper(), (eGameFont)26);
        row = 2;
        button->SetCellPos(0, 1, 1, 1);
        button->m_command = 0x24b25450;
        button->m_commandParameter1 = 0;
        button->m_commandParameter2 = 0;
        button->m_commandParameter3 = 0;
        button->SetOutsetSpacing(spacing(), 0);
        content->AddToFront(button);
    }
    button = new SGButton(2, 0, 0, 1);
    button->SetText(ResString("IDS_MENU_SETTINGS").Upper(), (eGameFont)26);
    button->SetCellPos(0, row, 1, 1);
    button->m_command = 0x0c000066;
    button->m_commandParameter1 = 0;
    button->m_commandParameter2 = 0;
    button->m_commandParameter3 = 0;
    button->SetOutsetSpacing(spacing(), 0);
    content->AddToFront(button);
    return content;
}

Window* CSettingsWindow::CreateSettingMenu()
{
    XString title(L"Settings");
    SetTitle(title);
    Window* content = new Window(false);
    content->SetPercentWidth(90, 0, 0);
    content->SetHeightByContent(0, 0);
    content->SetInsetSpacing(App::Scale() * 20, 0, 0, 0);
    content->SetLayoutType((ELayoutType)1);
    content->SetAlign(0x24);
    Window* row = m_sound.Create("IDS_MENU_SOUND", NULL, NULL, ((App*)WindowApp::m_instance)->m_settings->IsSoundEnabled());
    row->SetAlign(2);
    row->SetCellPos(0, 0, 1, 1);
    content->AddToFront(row);
    row = m_music.Create("IDS_MENU_MUSIC", NULL, NULL, ((App*)WindowApp::m_instance)->m_settings->IsMusicEnabled());
    row->SetLayoutType((ELayoutType)2);
    row->SetCellPos(0, 1, 1, 1);
    content->AddToFront(row);
    int pushRow = 2;
    if (!((App*)WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial() ||
        ((App*)WindowApp::m_instance)->m_tutorialManager->GetState() >= 4) {
        pushRow = 3;
        row = m_fire.Create("IDS_MENU_FIRE_BUTTON", NULL, NULL, !((App*)WindowApp::m_instance)->m_settings->IsFireButtonEnabled());
        row->SetLayoutType((ELayoutType)2);
        row->SetCellPos(0, 2, 1, 1);
        content->AddToFront(row);
    }
    row = m_push.Create("IDS_MENU_PUSHES", NULL, NULL, ((App*)WindowApp::m_instance)->m_settings->IsLocalPushEnabled());
    row->SetLayoutType((ELayoutType)2);
    row->SetCellPos(0, pushRow, 1, 1);
    content->AddToFront(row);
    StateButton* sensitivity = new StateButton("IDS_MENU_SENSITIVITY");
    m_sensitivity = sensitivity;
    sensitivity->m_values.addElement(ResString("IDS_MENU_SENSITIVITY_LOW"));
    m_sensitivity->m_values.addElement(ResString("IDS_MENU_SENSITIVITY_MED"));
    m_sensitivity->m_values.addElement(ResString("IDS_MENU_SENSITIVITY_HIGH"));
    int value = ((App*)WindowApp::m_instance)->m_settings->m_field38;
    int index = 0;
    if (value >= 76) index = 1;
    if (value >= 101) {
        index = 5;
        if (value < 151) index = 2;
    }
    m_sensitivity->m_selectedIndex = index;
    m_sensitivity->UpdateValText();
    m_sensitivity->SetLayoutType((ELayoutType)2);
    m_sensitivity->SetCellPos(0, pushRow + 1, 1, 1);
    content->AddToFront(m_sensitivity);
    Window* helpRow = new Window(false);
    helpRow->SetHeightByContent(0, 0);
    helpRow->SetLayoutType((ELayoutType)1);
    helpRow->SetCellPos(0, pushRow + 2, 1, 1);
    SGButton* button = new SGButton(2, 0, 0, 1);
    button->SetText(ResString("IDS_MENU_HELP"), (eGameFont)26);
    button->SetLayoutType((ELayoutType)1);
    button->SetAlign(0x24);
    button->m_command = (int)0xb916cc2au;
    button->m_commandParameter1 = 0;
    button->m_commandParameter2 = 0;
    button->m_commandParameter3 = 0;
    helpRow->AddToFront(button);
    content->AddToFront(helpRow);
    Window* bottom = new Window(false);
    bottom->SetOutsetSpacing(SScreenAdaptingConstant(15)(), 0, 0, 0);
    bottom->SetHeightByContent(0, 0);
    bottom->SetLayoutType((ELayoutType)2);
    bottom->SetCellPos(0, pushRow + 3, 1, 1);
    button = new SGButton(2, 0, 0, 1);
    button->SetText(ResString("IDS_MENU_BACK").Upper(), (eGameFont)26);
    button->SetLayoutType((ELayoutType)1);
    button->SetAlign(0x11);
    button->m_command = 0x6daa228b;
    button->m_commandParameter1 = 0;
    button->m_commandParameter2 = 0;
    button->m_commandParameter3 = 0;
    bottom->AddToFront(button);
    button = new SGButton(2, 0, 0, 1);
    button->SetText(ResString("IDS_BTN_DONE"), (eGameFont)26);
    button->SetLayoutType((ELayoutType)1);
    button->SetAlign(0x12);
    button->m_command = (int)0xb3aa22e5u;
    button->m_commandParameter1 = 0;
    button->m_commandParameter2 = 0;
    button->m_commandParameter3 = 0;
    bottom->AddToFront(button);
    content->AddToFront(bottom);
    return content;
}

Window* CSettingsWindow::OnOffButtonDesc::Create(const char* label, const char*, const char*, bool pressed)
{
    m_container = new Window(false);
    m_container->SetPercentWidth(50, 0, 0);
    m_container->SetHeightByContent(0, 0);
    m_container->SetAlign(4);
    m_label = new TextWindow(Window::ResString(label), CFontMgr::GetInstance()->GetFont(29));
    TextWindow* text = m_label;
    text->SetOutsetSpacing(0, 0, App::Scale() * 10, 0);
    m_label->SetLayoutType((Window::ELayoutType)1);
    m_label->SetAlign(1);
    m_container->AddToFront(m_label);
    m_button = new SGButton(2, 0, 6, 5);
    m_button->SetLayoutType((Window::ELayoutType)1);
    m_button->SetAlign(2);
    m_button->SetToggle(true);
    m_button->m_pressed = pressed;
    m_button->SetWidthByContent(0, 0);
    m_button->SetHeightByContent(0, 0);
    m_container->AddToFront(m_button);
    return m_container;
}

void CSettingsWindow::ShowContent(Window* content)
{
    EasyParams easy(this, (EasyLib::EMethod)5, (EasyLib::EMethod)5);
    easy.SetFullTime(2.5f);
    easy.SetOpacity(0.0f, 1.0f);
    m_contentHost->AddToFront(content);
    content->StartEasy(easy, 0x100);
}
void CSettingsWindow::OnStartupEasy()
{
    EasyParams easy(this, (EasyLib::EMethod)5, (EasyLib::EMethod)5);
    easy.SetFullTime(0.5f);
    easy.SetOpacity(0.0f, 1.0f);
    StartEasy(easy, 0x100);
}
void CSettingsWindow::OnEasyFinished() {}
void CSettingsWindow::OnClosed() { WindowApp::HandleTunnelCommand(0x13179047, 0, 0, 0); }

void CSettingsWindow::OnCommand(Event& event)
{
    switch ((unsigned int)event.m_id) {
    case 0x97204784u:
    case 0x6daa228bu:
        if (m_pauseMenu) {
            m_activeContent->ReverseEasy(0x200);
            m_activeContent = CreatePauseMenu();
            ShowContent(m_activeContent);
        } else ReverseEasy(0x200);
        break;
    case 0x0c000066u:
        if (m_activeContent != NULL) m_activeContent->ReverseEasy(0x200);
        m_activeContent = CreateSettingMenu();
        ShowContent(m_activeContent);
        break;
    case 0xb916cc2au: {
        CHelpScreen* help = new CHelpScreen;
        ((App*)WindowApp::m_instance)->m_mainLayer->AddModal(help);
        break;
    }
    case 0x74146762u:
        ((App*)WindowApp::m_instance)->m_saveManager->PauseSaving();
        ((App*)WindowApp::m_instance)->m_saveManager->Reset();
        WindowApp::m_instance->RestartApplication();
        event.Clear();
        break;
    case 0x3b0ba067u:
        ReverseEasy(0x200);
        event.Clear();
        break;
    case 0x24b25450u: {
        CMission* mission = ((App*)WindowApp::m_instance)->m_missionManager->GetMissionByName(
            ((App*)WindowApp::m_instance)->m_huntingInfo->m_assignedData.m_missionName);
        CGameAnalytics::logMissionQuit(mission->m_name, CBH_Player::GetInstance()->m_id);
        ((App*)WindowApp::m_instance)->m_helper->m_swerveGame->ShowReturnToGpsConfirmation();
        Close();
        event.Clear();
        break;
    }
    case 0xb3aa22e5u: {
        bool oldMusic = ((App*)WindowApp::m_instance)->m_settings->IsMusicEnabled();
        bool newMusic = m_music.m_button->m_pressed;
        if ((oldMusic ^ newMusic) == 1) CGameAnalytics::logMusicSwitch(newMusic);
        bool oldSound = ((App*)WindowApp::m_instance)->m_settings->IsSoundEnabled();
        bool newSound = m_sound.m_button->m_pressed;
        if ((oldSound ^ newSound) == 1) CGameAnalytics::logSoundSwitch(newSound);
        ((App*)WindowApp::m_instance)->m_settings->SetMusicEnabled(m_music.m_button->m_pressed);
        ((App*)WindowApp::m_instance)->m_settings->SetSoundEnabled(m_sound.m_button->m_pressed);
        if (m_fire.m_button != NULL) ((App*)WindowApp::m_instance)->m_settings->SetFireButtonEnabled(!m_fire.m_button->m_pressed);
        ((App*)WindowApp::m_instance)->m_settings->SetLocalPushEnabled(m_push.m_button->m_pressed);
        int value = 75;
        int index = m_sensitivity->m_selectedIndex;
        if (index != 0) {
            value = 150;
            if (index == 1) value = 100;
        }
        ((App*)WindowApp::m_instance)->m_settings->m_field38 = value;
        ((App*)WindowApp::m_instance)->m_settings->Save();
        Event back(Event::NONE, 0x6daa228b, 0, 0, 0);
        OnCommand(back);
        break;
    }
    }
}

CSettingsWindow::StateButton::StateButton(const char* label) : Window(false), m_selectedIndex(0)
{
    SetPercentWidth(50, 0, 0);
    SetHeightByContent(0, 0);
    SetAlign(4);
    m_label = new TextWindow(ResString(label), CFontMgr::GetInstance()->GetFont(29));
    TextWindow* text = m_label;
    text->SetOutsetSpacing(0, 0, App::Scale() * 10, 0);
    m_label->SetLayoutType((ELayoutType)1);
    m_label->SetAlign(1);
    AddToFront(m_label);
    m_button = new SGButton(2, 0, 6, 6);
    SGButton* commandButton = m_button;
    commandButton->m_command = 0x363a6887;
    commandButton->m_commandParameter1 = 0;
    commandButton->m_commandParameter2 = 0;
    commandButton->m_commandParameter3 = 0;
    m_button->SetLayoutType((ELayoutType)1);
    m_button->SetAlign(2);
    m_button->SetWidthByContent(0, 0);
    m_button->SetHeightByContent(0, 0);
    m_value = new TextWindow(XString(""), CFontMgr::GetInstance()->GetFont(29));
    m_value->SetAlign(0x24);
    m_button->AddToFront(m_value);
    AddToFront(m_button);
}
void CSettingsWindow::StateButton::UpdateValText()
{
    if (m_selectedIndex >= 0 && m_selectedIndex < m_values.m_size) {
        XString value(m_values[m_selectedIndex]);
        m_value->SetText(value);
    }
}
void CSettingsWindow::StateButton::OnCommand(Event& event)
{
    if ((unsigned int)event.m_id == 0x363a6887u) {
        ++m_selectedIndex;
        if (m_selectedIndex >= m_values.m_size) m_selectedIndex = 0;
        UpdateValText();
        event.Clear();
    }
}
