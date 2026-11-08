// j: original 155f18..15622c full owner; does not add friend downloads or event cleanup the original lacks.
#include "CFriendListWindow.h"
#include "CImageButton.h"
#include "CCareerWindow.h"
#include "CFontMgr.h"
#include "WindowApp.h"

CFriendListWindow::CFriendListWindow() : Window(false), m_windowAc(NULL), m_friends(NULL)
{
    Window* invite = new Window(false);
    invite->m_debugFrame = true;
    AddToFront(invite, 0, 0);
    invite->SetPercentWidth(30, 0, 0);
    invite->AddToFront(CreateInviteButton());
    Window* friends = new Window(false);
    friends->AddToFront(CreateFriendsWindow());
    AddToFront(friends, 1, 0);
    friends->SetPercentWidth(70, 0, 0);
}

Window* CFriendListWindow::CreateInviteButton()
{
    Window* content = new Window(false);
    CImageButton* image = new CImageButton;
    image->AddImage("IDB_INVITE_FRIEND", 0, 0);
    content->AddToFront(image, 0, 0);
    Window* bottom = new Window(false);
    content->AddToFront(bottom, 0, 1);
    TextWindow* text = new TextWindow(ResString("IDS_FRIENDS_INVITE_BUTTON"), CFontMgr::GetInstance()->GetFont(17));
    text->SetHeightByContent(0, 0);
    text->SetWidthByContent(0, 0);
    text->SetAlign(0x0c);
    bottom->AddToFront(text);
    return content;
}

Window* CFriendListWindow::CreateFriendsWindow()
{
    Window* content = new Window(false);
    content->SetLayoutType((ELayoutType)0);
    m_friends = new Window(false);
    m_friends->SetPercentWidth(100, 0, 0);
    m_friends->SetHeightByContent(0, 0);
    m_friends->SetLayoutType((ELayoutType)0);
    ScrollingContainer* scroll = new ScrollingContainer;
    scroll->SetLayoutType((ELayoutType)0);
    scroll->SetPercentHeight(100, 0, 0);
    scroll->SetPercentWidth(100, 0, 0);
    scroll->AddToFront(m_friends);
    content->AddToFront(scroll, 0, 0);
    VertScrollerSimpleRed* scroller = new VertScrollerSimpleRed(scroll);
    scroller->SetOutsetSpacing(3, 5, 3, 4);
    content->AddToFront(scroller, 1, 0);
    return content;
}

void CFriendListWindow::OnCommand(Event& event)
{
    if ((unsigned int)event.m_id == 0x236889e6u) {
        WindowApp::HandleTunnelCommand((int)0x89dba0ecu, 0, 0, 0);
    }
}
