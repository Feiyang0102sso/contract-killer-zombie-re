// CDialogQueueWindow.cpp: u537 full queue; pending windows are not deleted by the queue.
#include "CDialogQueueWindow.h"
#include "App.h"

CDialogQueueWindow* CDialogQueueWindow::m_activeQueue = NULL;

CDialogQueueWindow::CDialogQueueWindow(Window* parent) : Window(false),
    m_currentDialog(NULL), m_finishCommand(0), m_parent(parent)
{
    if (m_parent == NULL) {
        m_parent = this;
    }
    m_finishParameters[0] = 0;
    m_finishParameters[1] = 0;
    m_finishParameters[2] = 0;
    m_dialogs.ensureCapacity(8);
    m_activeQueue = this;
}

CDialogQueueWindow::~CDialogQueueWindow()
{
    if (m_activeQueue == this) {
        m_activeQueue = NULL;
    }
    m_parent->CloseChildIfExists(m_currentDialog);
}

void CDialogQueueWindow::Init()
{
    // Original 149f68 full body is bx lr/nop, with no base Init or other side effects.
}

void CDialogQueueWindow::AddDialogInQueue(Window* dialog)
{
    if (m_activeQueue != NULL) {
        m_activeQueue->m_dialogs.insertElementAt(dialog, 0);
    } else {
        Window* main = ((App*)WindowApp::m_instance)->m_mainLayer;
        if (main != NULL) {
            main->AddModal(dialog);
        }
    }
}

void CDialogQueueWindow::Update()
{
    if (m_currentDialog == NULL || (m_currentDialog->m_flags & 0x40000000) != 0) {
        ShowNextDialog();
    }
}

void CDialogQueueWindow::ShowNextDialog()
{
    if (m_dialogs.size() >= 1) {
        m_currentDialog = m_dialogs[m_dialogs.size() - 1];
        m_parent->AddModal(m_currentDialog);
        // size is reread after the AddModal callback; original 149ff6 has an explicit read.
        m_dialogs.removeElementAt(m_dialogs.size() - 1);
    } else {
        m_currentDialog = NULL;
        if (m_finishCommand != 0) {
            WindowApp::HandleTunnelCommand(m_finishCommand, m_finishParameters[0],
                m_finishParameters[1], m_finishParameters[2]);
            m_finishCommand = 0;
        }
    }
}
