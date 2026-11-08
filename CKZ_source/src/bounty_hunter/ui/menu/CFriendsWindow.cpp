// j: the original friends main window, 14 ranges; keeps the unused field, the two login texts and the refresh range.
#include "CFriendsWindow.h"
#include "CFriendListWindow.h"
#include "CNewsFeedWindow.h"
#include "CGiftsWindow.h"
#include "CTabWindow.h"
#include "CImageButton.h"
#include "CNewsFeedManager.h"
#include "SScreenAdaptingConstant.h"
#include "DGHelper.h"
#include "App.h"

CFriendsWindow::CFriendsWindow() : CBaseScreen(true, true, true),
    m_gameCenterLogin(NULL), m_field120(0), m_flag124(false), m_flag125(false)
{
    CreateUserInteface();
    CreateLoginButtons();
}

void CFriendsWindow::CreateUserInteface()
{
    SetTitleText(ResString("IDS_BUTTON_FRIENDS"));
    m_tabs = new CTabWindow(false);
    m_tabs->AddTab(ResString("IDS_FRIENDS_TAB_NEWS_FEED"), CreateNewsFeedTab(), NULL, false, -1);
    {
        CTabWindow* tabs = m_tabs; // Original 15636a caches before ResString/new FriendList.
        XString title = ResString("IDS_FRIENDS_TAB");
        m_friends = new CFriendListWindow;
        tabs->AddTab(title, m_friends, NULL, false, -1);
    }
    m_tabs->AddTab(ResString("IDS_FRIENDS_TAB_GIFTS"), new CGiftsWindow, NULL, false, -1);
    m_tabs->SetPercentHeight(100, 0, 0);
    m_tabs->SetPercentWidth(100, 0, 0);
    m_tabs->SetCurrTab(1, false);
    m_tabs->SetTabsWidthByWindowWidth();
    m_content->AddToFront(m_tabs);
}

void CFriendsWindow::CreateLoginButtons()
{
    com::glu::platform::components::CStrWChar gameCenter(ResString("IDS_LOG_IN").m_data);
    com::glu::platform::components::CStrWChar facebook(ResString("IDS_LOG_IN").m_data);
    Window* container = new Window(false);
    container->SetLayoutType((ELayoutType)1);
    container->SetAlign(0x0a);
    int width = container->m_width;
    container->SetSize(width, GetTitleBarHeight());
    container->SetPercentWidth(SScreenAdaptingConstant(65, 70)(), 0, 0);
    CImageButton* icon = new CImageButton;
    icon->SetAlign(0x24);
    icon->AddImage("IDB_GAMECENTER_ICON_LARGE", 0, 0);
    container->AddToFront(icon, 0, 0);
    m_gameCenterLogin = new CImageButton;
    {
        XString label(gameCenter.m_string);
        DGHelper::CropName(label, 18);
        m_gameCenterLogin->SetText(label, 0x24, NULL, 0, 100);
        container->AddToFront(m_gameCenterLogin, 1, 0);
        m_gameCenterLogin->SetPercentHeight(100, 0, 0);
        m_gameCenterLogin->SetPercentWidth(40, 0, 0);
    }
    icon = new CImageButton;
    icon->AddImage("IDB_FACEBOOK_ICON_LARGE", 0, 0);
    container->AddToFront(icon, 2, 0);
    m_facebookLogin = new CImageButton;
    XString label(facebook.m_string);
    DGHelper::CropName(label, 18);
    m_facebookLogin->SetText(label, 0x24, NULL, 0, 100);
    m_facebookLogin->SetPercentHeight(100, 0, 0);
    container->AddToFront(m_facebookLogin, 3, 0);
    m_facebookLogin->SetPercentWidth(40, 0, 0);
    AddToFront(container);
}

Window* CFriendsWindow::CreateNewsFeedTab()
{
    m_news = new CNewsFeedWindow;
    for (int index = 0; index < ((App*)WindowApp::m_instance)->m_newsFeedManager->m_messages.m_size; ++index) {
        const SMessage* message = ((App*)WindowApp::m_instance)->m_newsFeedManager->GetNewsAt(index);
        switch (message->m_type) {
        case 0: m_news->Targetted((const STargettedMessage*)message); break;
        case 1: m_news->MissionInvite((const SMissionInviteMessage*)message); break;
        case 2: m_news->FriendInstall((const SFriendJoinedGameMessage*)message); break;
        case 4: m_news->ReceiveGift((const SGiftReceivedMessage*)message); break;
        case 5: m_news->ReceiveServerAward((const SServerAwardMessage*)message); break;
        }
    }
    return m_news;
}

void CFriendsWindow::Update() { Window::Update(); }

void CFriendsWindow::OnCommand(Event& event)
{
    if ((unsigned int)event.m_id == 0x62b2a2ceu) {
        Close();
        event.Clear();
    } else if ((unsigned int)event.m_id == 0xa4377002u) {
        m_selectedGift = event.m_param1;
    }
    CBaseScreen::OnCommand(event);
}

void CFriendsWindow::RefreshFriendTab()
{
    m_tabs->Close();
    CreateUserInteface();
}
