// j: original 158bac..158e74; the copy/reverse destruction of by-value string arguments all come from the real class.
#include "CFriendAvatarWindow.h"
#include "BaseDialog.h"
#include "CWaitWindow.h"
#include "SScreenAdaptingConstant.h"
#include "CBH_SurfaceManager.h"
#include "App.h"

const int EventId_FriendAvatarDone = 0x7695a9f1;

CFriendAvatarWindow::CFriendAvatarWindow(int) : Window(false)
{
    com::glu::platform::components::CStrWChar empty;
    Init(empty);
}

CFriendAvatarWindow::CFriendAvatarWindow(com::glu::platform::components::CStrWChar name)
    : Window(false)
{
    Init(name);
}

void CFriendAvatarWindow::Init(com::glu::platform::components::CStrWChar name)
{
    m_fieldb4 = 0;
    m_flagb0 = false;
    m_surfaceName = NULL;
    m_flagb1 = false;
    m_flagb2 = false;
    m_name = name;
    Window* content = new Window(false);
    content->SetLayoutType((ELayoutType)1);
    content->SetAlign(0x24);
    content->SetDesiredWidth(SScreenAdaptingConstant(50)());
    content->SetDesiredHeight(SScreenAdaptingConstant(50)());
    AddToFront(content);
    m_placeholder = new ImageWindow;
    m_placeholder->SetFillType(ImageWindow::FILL_NONE);
    m_placeholder->SetImageById("IDB_BOUNTY_MENU_FRIEND_USER_PIC");
    m_placeholder->ClearFlags(1);
    content->AddToFront(m_placeholder);
    m_image = new ImageWindow;
    m_image->SetLayoutType((ELayoutType)1);
    m_image->SetAlign(9);
    content->AddToFront(m_image);
    m_wait = new CWaitWindow;
    AddToFront(m_wait);
}

CFriendAvatarWindow::~CFriendAvatarWindow()
{
    if (m_surfaceName != NULL) {
        ((App*)WindowApp::m_instance)->m_surfaceManager->ReleaseSurface(m_surfaceName);
    }
}

void CFriendAvatarWindow::Update()
{
    // Original 158e70 is truly empty; no self-invented async download implementation may be added.
}
