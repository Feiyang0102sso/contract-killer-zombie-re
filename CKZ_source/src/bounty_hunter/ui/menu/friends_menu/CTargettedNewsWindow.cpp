// j: faithfully keeps the original Targetted behavior of not using the passed-in message and leaving several fields uninitialized.
#include "CTargettedNewsWindow.h"
#include "CNewsFeedManager.h"
#include "CGPSMapGame.h"
#include "App.h"

CTargettedNewsWindow::CTargettedNewsWindow(const STargettedMessage*)
    : Window(false), m_flagb0(false), m_fieldb4(0)
{
}

void CTargettedNewsWindow::OnCommand(Event& event)
{
    if ((unsigned int)event.m_id == 0x0e6a3ddbu) {
        ((App*)WindowApp::m_instance)->m_newsFeedManager->RemoveMessage(m_message, m_messageId);
        Close();
    } else if ((unsigned int)event.m_id == 0x0f207fbau) {
        HandleRevengePressed();
    } else {
        return;
    }
    event.Clear();
}

void CTargettedNewsWindow::HandleRevengePressed()
{
    WindowApp::HandleTunnelCommand(0x62b2a2ce, 0, 0, 0);
    WindowApp::HandleTunnelCommand(EventId_StartTargettingMission, m_sender,
        (int)m_message, m_messageId);
}
