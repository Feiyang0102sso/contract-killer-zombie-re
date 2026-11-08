// j: complete Gift message window; the original receive path uses the server format, while the callable friend two-button path is still kept.
#include "CGiftRecievedNewsWindow.h"
#include "CNewsFeedManager.h"
#include "CSocialGiftsManager.h"
#include "CShopItem.h"
#include "ServiceWindows.h"
#include "BaseDialog.h"
#include "CFontMgr.h"
#include "SScreenAdaptingConstant.h"
#include "App.h"

CGiftRecievedNewsWindow::CGiftRecievedNewsWindow(const SGiftReceivedMessage* message)
    : Window(false), m_item(NULL), m_message(message)
{
    SGift* gift = ((App*)WindowApp::m_instance)->m_socialGiftManager->GetGift(message->m_itemIndex);
    if (gift != NULL) {
        CItem* item = gift->m_itemManager->getItemById(gift->m_itemId);
        XString friendName("");
        CreateUserInterface(friendName, item->GetName(), false);
        m_item = item;
    }
    m_fieldb0 = 0;
    m_messageId = message->m_messageId;
}

void CGiftRecievedNewsWindow::CreateUserInterface(const XString& friendName, const XString& itemName, bool fromFriend)
{
    SetHeightByContent(0, 0);
    XString formatted;
    if (fromFriend) {
        formatted = XString::Format(ResString("IDS_GIFT_FROM_FRIEND_NEWSFEED").m_data, itemName.m_data, friendName.m_data);
    } else {
        formatted = XString::Format(ResString("IDS_GIFT_FROM_SERVER_NEWSFEED").m_data, itemName.m_data);
    }
    TextWindow* text = new TextWindow(formatted, CFontMgr::GetInstance()->GetFont(15));
    text->SetAlign(0x21);
    AddToFront(text, 0, 0);
    text->SetPercentWidth(60, 0, 0);
    Window* buttons = new Window(false);
    if (fromFriend) {
        StretchImageButton* accept = new StretchImageButton(ResString("IDS_GIFT_ACCEPT"),
            "IDB_HUD_TRANSPARENT_BLACK_LEFT", "IDB_HUD_TRANSPARENT_BLACK_TILE", "IDB_HUD_TRANSPARENT_BLACK_RIGHT");
        accept->SetAlign(0x24);
        accept->m_command = 0x755a6d24;
        accept->m_commandParameter1 = (int)this;
        accept->m_commandParameter2 = 0;
        accept->m_commandParameter3 = 0;
        accept->ForceSetWidth(SScreenAdaptingConstant(90, 180)());
        buttons->AddToFront(accept, 0, 0);
        StretchImageButton* decline = new StretchImageButton(ResString("IDS_GIFT_DECLINE"),
            "IDB_HUD_TRANSPARENT_BLACK_LEFT", "IDB_HUD_TRANSPARENT_BLACK_TILE", "IDB_HUD_TRANSPARENT_BLACK_RIGHT");
        decline->SetAlign(0x24);
        decline->m_command = 0x2352e724;
        decline->m_commandParameter1 = (int)this;
        decline->m_commandParameter2 = 0;
        decline->m_commandParameter3 = 0;
        decline->ForceSetWidth(SScreenAdaptingConstant(90, 180)());
        buttons->AddToFront(decline, 1, 0);
    }
    AddToFront(buttons, 1, 0);
    buttons->SetHeightByContent(0, 0);
    buttons->SetPercentWidth(40, 0, 0);
    SeparateLineRed* separator = new SeparateLineRed;
    int spacing = SScreenAdaptingConstant(5, 10)();
    separator->SetOutsetSpacing(spacing, 0, 0, spacing);
    separator->SetDesiredHeight(SScreenAdaptingConstant(5, 10)());
    separator->SetCellPos(0, 1, 2, 1);
    AddToFront(separator);
}

void CGiftRecievedNewsWindow::OnCommand(Event& event)
{
    if ((unsigned int)event.m_id == 0x2352e724u) {
        if (event.m_param1 != (int)this) { return; }
        Close();
        ((App*)WindowApp::m_instance)->m_newsFeedManager->RemoveMessage(m_message);
        m_messageId = -1;
    } else if ((unsigned int)event.m_id == 0x755a6d24u) {
        if (event.m_param1 != (int)this) { return; }
        HandleGiftAccepted();
    } else {
        return;
    }
    event.Clear();
}

void CGiftRecievedNewsWindow::HandleGiftAccepted()
{
    if (m_item != NULL) {
        // Original virtual slot 20; the real CItem interface is setAvailable(bool), not increasing the item count.
        m_item->setAvailable(true);
    }
    Close();
    ((App*)WindowApp::m_instance)->m_newsFeedManager->RemoveMessage(m_message);
    m_messageId = -1;
}
