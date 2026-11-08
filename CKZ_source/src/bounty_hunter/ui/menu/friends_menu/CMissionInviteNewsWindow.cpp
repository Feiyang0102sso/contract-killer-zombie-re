// j: the hardcoded Win32 User and the original message lifetime are both kept.
#include "CMissionInviteNewsWindow.h"
#include "CNewsFeedManager.h"
#include "ServiceWindows.h"
#include "BaseDialog.h"
#include "CFontMgr.h"
#include "App.h"

void CMissionInviteNewsWindow::CreateUserInterface()
{
    SetHeightByContent(0, 0);
    XString user;
    user = XString("Win32 User");
    XString formatted = XString::Format(ResString("IDS_FRIENDS_INVITE_TO_MISSION").m_data, user.m_data);
    TextWindow* text = new TextWindow(formatted, CFontMgr::GetInstance()->GetFont(15));
    text->SetAlign(0x24);
    AddToFront(text, 0, 0);
    StretchImageButton* accept = new StretchImageButton(ResString("IDS_DIALOG_BUTTON_ACCEPT"),
        "IDB_HUD_TRANSPARENT_BLACK_LEFT", "IDB_HUD_TRANSPARENT_BLACK_TILE", "IDB_HUD_TRANSPARENT_BLACK_RIGHT");
    accept->SetAlign(0x24);
    accept->m_command = 0x7d371742;
    accept->m_commandParameter1 = 0;
    accept->m_commandParameter2 = 0;
    accept->m_commandParameter3 = 0;
    AddToFront(accept, 1, 0);
    StretchImageButton* ignore = new StretchImageButton(ResString("IDS_DIALOG_BUTTON_IGNORE"),
        "IDB_HUD_TRANSPARENT_BLACK_LEFT", "IDB_HUD_TRANSPARENT_BLACK_TILE", "IDB_HUD_TRANSPARENT_BLACK_RIGHT");
    ignore->SetAlign(0x24);
    ignore->m_command = 0x0d611fcc;
    ignore->m_commandParameter1 = 0;
    ignore->m_commandParameter2 = 0;
    ignore->m_commandParameter3 = 0;
    AddToFront(ignore, 2, 0);
}

CMissionInviteNewsWindow::CMissionInviteNewsWindow(const SMissionInviteMessage* message)
    : Window(false), m_fieldb4(0)
{
    m_senderId = message->m_senderId;
    m_missionId = message->m_messageId;
    m_message = message;
    CreateUserInterface();
}

void CMissionInviteNewsWindow::OnCommand(Event& event)
{
    if ((unsigned int)event.m_id == 0x7d371742u || (unsigned int)event.m_id == 0x0d611fccu) {
        ((App*)WindowApp::m_instance)->m_newsFeedManager->RemoveMessage(m_message);
        Close();
    }
}
