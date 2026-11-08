// b074: complete safehouse menu owner, original 118360..118b24.
#include "CBH_NetworkWindow.h"
#include "CBH_GPSHud.h"
#include "CBH_MainMenuBar.h"
#include "CCareerWindow.h"
#include "CFriendsWindow.h"
#include "CFactionsWindow.h"
#include "CAchievementsWindow.h"
#include "CSettingsWindow.h"
#include "CTutorialManager.h"
#include "CGameAnalytics.h"
#include "GameCenter_Facade.h"
#include "SScreenAdaptingConstant.h"
#include "App.h"

static MenuButtonDesc BUTTON_DESCRIPTION[] = {
    {"IDB__SAFEHOUSE_STORE_PNG", "IDB__SAFEHOUSE_STORE_ACTIVE_PNG", (int)0x84523dc0, 0xe84cf49e, "IDS_BUTTON_SHOP"},
    {"IDB__SAFEHOUSE_AWARDS_PNG", "IDB__SAFEHOUSE_AWARDS_ACTIVE_PNG", 0x2c66b471, 0, "IDS_BUTTON_ACHIEVEMENTS"},
    {"IDB__SAFEHOUSE_BOARDS_PNG", "IDB__SAFEHOUSE_BOARDS_ACTIVE_PNG", 0x3752d6b8, 0, "IDS_BUTTON_STATS"}
};

CBH_NetworkWindow::CBH_NetworkWindow() : CBaseScreen(true, true, true)
{
    m_logOnFocus = false;
    SetSize(WindowApp::m_instance->m_screenWidth, WindowApp::m_instance->m_screenHeight);
    CHudStatsPanel* stats = new CHudStatsPanel();
    stats->SetAlign(0xc);
    AddToFront(stats);
    CCareerWindow* career = new CCareerWindow();
    career->SetPercentHeight(80, 0, 0);
    career->SetOutsetSpacing(0, 0, 0, SScreenAdaptingConstant(10)());
    m_content->AddToFront(career, 0, 0);
    Window* bar = CreateMenuBar();
    bar->SetPercentHeight(25, 0, 0);
    m_content->AddToFront(bar, 0, 1);
}

Window* CBH_NetworkWindow::CreateMenuBar()
{
    Window* container = new Window(false);
    container->SetPercentHeight(100, 0, 0);
    CBH_MainMenuBar* bar = new CBH_MainMenuBar(-1, -1, -1, -1);
    bar->SetPercentHeight(100, 0, 0);
    bar->SetPercentWidth(100, 0, 0);
    bar->SetLayoutType(LAYOUT_SIMPLE);
    bar->SetAlign(8);
    Vector<MenuButtonDesc> buttons;
    for (int index = 0; index < 3; ++index) {
        if (BUTTON_DESCRIPTION[index].m_command != 0x3752d6b8 || GameCenter_Facade::GetInstance()->isGameCenterSupported() == 1) {
            buttons.addElement(BUTTON_DESCRIPTION[index]);
        }
    }
    if (buttons.m_size != 0) {
        bar->SetButtons(buttons.m_items, buttons.m_size);
    }
    container->SetLayoutType(LAYOUT_SIMPLE);
    container->SetAlign(8);
    container->AddToFront(bar);
    return container;
}

void CBH_NetworkWindow::Init()
{
    CGameAnalytics::logMenuAccessed(XString(L"Safehouse"));
}

void CBH_NetworkWindow::ReleaseResources()
{
    // Original 1185b4 is return-only.
}

void CBH_NetworkWindow::Update()
{
    // Original 1185b8 is return-only.
}

void CBH_NetworkWindow::OnCommand(Event& event)
{
    switch (event.m_id) {
    case (int)0x84523dc0:
        WindowApp::HandleTunnelCommand((int)0xc4794df3, -1, 0, 0);
        event.Clear();
        break;
    case 0x5f523cd9:
    case (int)0x97204784:
        WindowApp::HandleTunnelCommand((int)0x9be6e5f9, 0, 0, 0);
        if (((App*)WindowApp::m_instance)->m_tutorialManager->CanCloseNetwork() == 1) {
            ReverseEasy(0x200);
            event.Clear();
        }
        break;
    case (int)0x9720b34a: {
        CSettingsWindow* settings = new CSettingsWindow(false, false);
        settings->SetLayoutType(LAYOUT_SIMPLE);
        settings->SetAlign(0x24);
        ((App*)WindowApp::m_instance)->m_mainLayer->AddModal(settings);
        event.Clear();
        break;
    }
    case (int)0x9d557067: {
        CFactionsWindow* factions = new CFactionsWindow();
        ((App*)WindowApp::m_instance)->m_mainLayer->AddModal(factions);
        WindowApp::HandleTunnelCommand(0x718a0676, 0, 0, 0);
        event.Clear();
        break;
    }
    case 0x37c37da6: {
        CFriendsWindow* friends = new CFriendsWindow();
        ((App*)WindowApp::m_instance)->m_mainLayer->AddModal(friends);
        WindowApp::HandleTunnelCommand(0x718a0676, 0, 0, 0);
        event.Clear();
        break;
    }
    case (int)0xd3fb7516:
    case 0x265a64dd:
        event.Clear();
        break;
    case 0x2c66b471:
        if (((App*)WindowApp::m_instance)->m_tutorialManager->IsMenuBarBlocked() == 1) {
            WindowApp::HandleTunnelCommand((int)0xd17ff5d4, 0, 0, 0);
        } else {
            CAchievementsWindow* achievements = new CAchievementsWindow();
            ((App*)WindowApp::m_instance)->m_mainLayer->AddModal(achievements);
        }
        event.Clear();
        break;
    case 0x3752d6b8:
        if (((App*)WindowApp::m_instance)->m_tutorialManager->IsMenuBarBlocked() == 1) {
            WindowApp::HandleTunnelCommand((int)0xd17ff5d4, 0, 0, 0);
        } else if (GameCenter_Facade::GetInstance()->isGameCenterSupported() == 1) {
            bool authenticated = GameCenter_Facade::GetInstance()->isAuthenticated();
            GameCenter_Facade* center = GameCenter_Facade::GetInstance();
            if (authenticated == 1) {
                center->RunLeaderboardsView(NULL);
                CGameAnalytics::logMenuAccessed(XString(L"Leaderboards"));
            } else {
                center->AuthenticatePlayer();
            }
        }
        event.Clear();
        break;
    }
}

void CBH_NetworkWindow::OnClosed()
{
    // Original return-only method.
}

CBH_NetworkWindow::~CBH_NetworkWindow()
{
    ReleaseResources();
}

void CBH_NetworkWindow::Shutdown()
{
    // Original return-only method.
}

void CBH_NetworkWindow::OnGetFocus()
{
    if (m_logOnFocus) {
        m_logOnFocus = false;
        CGameAnalytics::logMenuAccessed(XString(L"Safehouse"));
    }
    if (((App*)WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial() == 1) {
        WindowApp::HandleTunnelCommand((int)0xb4655f2d, 0, 0, 0);
    }
}

void CBH_NetworkWindow::OnStartupEasy()
{
    EasyParams params(this, (EasyLib::EMethod)5, (EasyLib::EMethod)5);
    params.SetFullTime(0.5f);
    params.SetOpacity(0.0f, 1.0f);
    StartEasy(params, 0x100);
}
