// u544 full implementation; closing on an event does not clear the event and does not go on to call the base class.
#include "CWaitPurchaseWindow.h"
#include "App.h"

bool CWaitPurchaseWindow::m_close;

CWaitPurchaseWindow::CWaitPurchaseWindow() : CZombieDialogWindow(3)
{
    SetTitle(ResString("IDS_SHOP_IAP_LOADING_TITLE"));
    SetText(ResString("IDS_SHOP_IAP_LOADING_MESSAGE"), false, NULL);
}

void CWaitPurchaseWindow::ShowModal()
{
    CWaitPurchaseWindow* window = new CWaitPurchaseWindow;
    ((App*)WindowApp::m_instance)->m_mainLayer->AddModal(window);
    m_close = false;
}

void CWaitPurchaseWindow::SetClose()
{
    m_close = true;
}

void CWaitPurchaseWindow::OnCommand(Event& event)
{
    if (event.m_id == (int)0xbe344666 || event.m_id == 0x5f9d3f54 ||
        event.m_id == 0x659570dc) {
        Close();
        return;
    }
    CZombieDialogWindow::OnCommand(event);
}
