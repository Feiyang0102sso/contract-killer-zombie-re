// j: keeps re-reading rows/columns after every external call, and the point at which messages are deleted.
#include "CNewsFeedWindow.h"
#include "CNewsFeedManager.h"
#include "CGiftRecievedNewsWindow.h"
#include "CTargettedNewsWindow.h"
#include "CMissionInviteNewsWindow.h"
#include "CZombieDialogWindow.h"
#include "CCareerWindow.h"
#include "ServiceWindows.h"
#include "SScreenAdaptingConstant.h"
#include "BaseDialog.h"
#include "CFontMgr.h"
#include "App.h"

CNewsFeedWindow::CNewsFeedWindow() : Window(false) { CreateUserInterface(); }
CNewsFeedWindow::~CNewsFeedWindow() {}

void CNewsFeedWindow::CreateUserInterface()
{
    Window* container = new Window(false);
    container->SetLayoutType((ELayoutType)0);
    m_content = new Window(false);
    m_content->SetHeightByContent(0, 0);
    m_content->SetLayoutType((ELayoutType)0);
    m_column = 0;
    m_row = 0;
    ScrollingContainer* scroll = new ScrollingContainer;
    scroll->SetLayoutType((ELayoutType)0);
    scroll->SetPercentHeight(100, 0, 0);
    scroll->SetPercentWidth(100, 0, 0);
    scroll->AddToFront(m_content);
    container->AddToFront(scroll, 0, 0);
    VertScrollerSimpleRed* scroller = new VertScrollerSimpleRed(scroll);
    scroller->SetOutsetSpacing(3, 5, 3, 4);
    container->AddToFront(scroller, 1, 0);
    AddToFront(container);
}

void CNewsFeedWindow::ReceiveGift(const SGiftReceivedMessage* message)
{
    Window* child = new CGiftRecievedNewsWindow(message);
    int column = m_column;
    Window* content = m_content;
    int row = m_row;
    m_column = column + 1;
    content->AddToFront(child, row, column);
    SeparateLineRed* line = new SeparateLineRed;
    line->SetDesiredHeight(SScreenAdaptingConstant(5, 10)());
    column = m_column;
    content = m_content;
    row = m_row;
    m_column = column + 1;
    content->AddToFront(line, row, column);
}

void CNewsFeedWindow::Targetted(const STargettedMessage* message)
{
    Window* child = new CTargettedNewsWindow(message);
    int column = m_column;
    Window* content = m_content;
    int row = m_row;
    m_column = column + 1;
    content->AddToFront(child, row, column);
    SeparateLineRed* line = new SeparateLineRed;
    line->SetDesiredHeight(SScreenAdaptingConstant(5, 10)());
    column = m_column;
    content = m_content;
    row = m_row;
    m_column = column + 1;
    content->AddToFront(line, row, column);
}

void CNewsFeedWindow::MissionInvite(const SMissionInviteMessage* message)
{
    Window* child = new CMissionInviteNewsWindow(message);
    int column = m_column;
    Window* content = m_content;
    int row = m_row;
    m_column = column + 1;
    content->AddToFront(child, row, column);
    SeparateLineRed* line = new SeparateLineRed;
    line->SetDesiredHeight(SScreenAdaptingConstant(5, 10)());
    column = m_column;
    content = m_content;
    row = m_row;
    m_column = column + 1;
    content->AddToFront(line, row, column);
}

void CNewsFeedWindow::FriendInstall(const SFriendJoinedGameMessage* message)
{
    // Original 157a02 only reads the string pointer at message +10; the header owning the type was verified and completed by the manager owner.
    XString formatted = XString::Format(ResString("IDS_FRIEND_JOINED_GAME_NEWSFEED").m_data,
        *(const wchar_t* const*)((const char*)message + 0x10));
    TextWindow* text = new TextWindow(formatted, CFontMgr::GetInstance()->GetFont(15));
    text->SetAlign(0x24);
    int column = m_column;
    Window* content = m_content;
    int row = m_row;
    m_column = column + 1;
    content->AddToFront(text, row, column);
    SeparateLineRed* line = new SeparateLineRed;
    line->SetDesiredHeight(SScreenAdaptingConstant(5, 10)());
    column = m_column;
    content = m_content;
    row = m_row;
    m_column = column + 1;
    content->AddToFront(line, row, column);
}

void CNewsFeedWindow::ReceiveServerAward(const SServerAwardMessage* message)
{
    if (message->m_type != 5) return;
    CServerGiftDialog* dialog = new CServerGiftDialog(message->m_rareCurrency,
        message->m_commonCurrency, message->m_itemType, message->m_awardId);
    XString aggregate;
    if (message->m_rareCurrency >= 1) {
        aggregate.Append(XString::Format(L"\n %d Credits", message->m_rareCurrency));
    }
    if (message->m_commonCurrency >= 1) {
        aggregate.Append(XString::Format(L"\n %d Cash", message->m_commonCurrency));
    }
    if (message->m_itemType >= 1) {
        aggregate.Append(XString::Format(L"\n %d Experience Points", message->m_itemType));
    }
    dialog->SetText(XString::Format(L"You receive %s", aggregate.m_data), false, NULL);
    dialog->AddButton((CZombieDialogWindow::EDialogButton)1, 0xffffffffu, 0, 0, true, 20);
    ((App*)WindowApp::m_instance)->m_mainLayer->AddModal(dialog);
    ((App*)WindowApp::m_instance)->m_newsFeedManager->RemoveMessage(message);
}
