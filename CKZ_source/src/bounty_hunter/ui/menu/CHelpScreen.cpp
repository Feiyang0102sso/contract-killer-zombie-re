// j: fixes the original build stamp, the original debug-page string lifetimes, and the strict four-quadrant boundaries.
#include "CHelpScreen.h"
#include "CTabWindow.h"
#include "BaseDialog.h"
#include "SScreenAdaptingConstant.h"
#include "CFontMgr.h"
#include "CGameApp.h"
#include "ICNotification.h"
#include "ICStdUtil.h"
#include "CBH_Player.h"
#include "ResMap.h"
#include "App.h"
#include <wchar.h>
using namespace com::glu::platform::components;

// Internal constants visible in the same original TU; for the real string boundaries see help-settings-original-data.json.
static const char APP_TIMESTAMP_FORMAT[] = "\n\n\n\n\n\n$Name: ZOMBSNIPER_20120906-170632 $\n%s %s";
static const char VERSION_MAJOR[] = "3";
static const char VERSION_MINOR[] = "1";
static const char VERSION_MICRO[] = "1";
static const char PUSH_NOTIFICATIONS_APP_KEY[] = "inorL-_WQ1CHzDmscKRBqA";
static const char PUSH_NOTIFICATIONS_APP_SECRET[] = "qi9DknoeRBqoqTgiQ_3pWA";
static const char GSERV_URL[] = "http://ng-zombsniper.glu.com/zombsniper/";
static const char FLURRY_API_KEY[] = "EEKPX1UWQSTW45ZKLVJL";
static const char PLAYHAVEN_APP_KEY[] = "daddc3f644b641fba5748013ec66480e";
static const char PLAYHAVEN_SECRET[] = "0176ff0d779844a9bc0d18b5ec5fc43f";
static const char TAPJOY_APP_ID[] = "e363778a-687a-416c-9601-2f9af2323db0";
// The two dynamic initializations at original 507ac0; the names have no separate original symbol, so neutral purpose names are used.
static SScreenAdaptingConstant HELP_WIDTH(320, 640, 768, 512);
static SScreenAdaptingConstant HELP_PERCENT(50, 100, 90, 80);

CHelpScreen::CHelpScreen() : CBaseScreen(true, true, false),
    m_aboutText(NULL), m_unlock(NULL), m_pendingReveal(true)
{
    SetTitleText(ResString("IDS_MENU_HELP"));
    m_tabs = new CTabWindow(false);
    m_tabs->SetPercentHeight(100, 0, 0);
    m_tabs->SetPercentWidth(100, 0, 0);
    m_content->AddToFront(m_tabs);
    char timestamp[100];
    com::glu::platform::core::ICStdUtil::SPrintF(timestamp, APP_TIMESTAMP_FORMAT, "Sep  6 2012", "17:44:27");
    int major = XString(VERSION_MAJOR).ToInt();
    int minor = XString(VERSION_MINOR).ToInt();
    int micro = XString(VERSION_MICRO).ToInt();
    XString about = XString::Format(ResString("IDS_HELP_TEXT_ABOUT").m_data, major, minor, micro);
    about.Append(XString(timestamp));
    CStrWChar token;
    CGameApp::GetInstance()->m_pushTokenSource->GetIdentity(token);
    if (token.m_length != 0) {
        m_debugText.Append(L"\nPush Token: ", wcslen(L"\nPush Token: ") * sizeof(wchar_t));
        m_debugText.Append(token.m_string, wcslen(token.m_string) * sizeof(wchar_t));
    }
    m_debugText.Append(L"\nPush Notification app key: ", wcslen(L"\nPush Notification app key: ") * sizeof(wchar_t));
    m_debugText.Append(XString(PUSH_NOTIFICATIONS_APP_KEY));
    m_debugText.Append(L"\nPush Notification app secret: ", wcslen(L"\nPush Notification app secret: ") * sizeof(wchar_t));
    m_debugText.Append(XString(PUSH_NOTIFICATIONS_APP_SECRET));
    char* dlc = ((App*)WindowApp::m_instance)->m_resMap->loadText(XString("res/common/configs/dlc_date.txt"));
    {
        XString date(dlc);
        m_debugText.Append(XString::Format(L"\nDownloadable Content date (DLC_VER): %s\n", date.m_data));
    }
    if (dlc != NULL) delete[] dlc;
    m_debugText.Append(XString("GServ url:") + XString(GSERV_URL));
    m_debugText.Append(XString("\nGServe ID:") + XString(0x1e1b620a));
    m_debugText.Append(XString::Format(L"\ngOffer id: %u", 34));
    m_debugText.Append(L"\nFlurry ID:" + XString(FLURRY_API_KEY));
    m_debugText.Append(L"\nPlayheaven public: " + XString(PLAYHAVEN_APP_KEY));
    m_debugText.Append(L"\nPlayheaven secret: " + XString(PLAYHAVEN_SECRET));
    m_debugText.Append(L"\n", wcslen(L"\n") * sizeof(wchar_t));
    m_debugText.Append(XString("Tapjoy ID:") + XString(TAPJOY_APP_ID));
    m_debugText.Append(L"\n", wcslen(L"\n") * sizeof(wchar_t));
    m_debugText.Append(XString(timestamp));
    CBH_Player* player = CBH_Player::m_staticInstance;
    if (player != NULL) {
        m_debugText.Append(L"\n", wcslen(L"\n") * sizeof(wchar_t));
        unsigned int time = player->GetGameTime();
        m_debugText.Append(XString::Format(L"\nTotal playtime id: %ud:%02uh:%02um:%02us",
            time / 86400, time / 3600, time / 60, time % 60));
    }
    AddTab(ResString("IDS_TITLE_ABOUT"), about, &m_aboutText);
    AddTab(ResString("IDS_HELP_CONTROLS"), ResString("IDS_HELP_TEXT_CONTROLS"), NULL);
    AddTab(ResString("IDS_MENU_EQUIPMENT"), ResString("IDS_HELP_TEXT_EQUIPMENT"), NULL);
    m_tabs->SetCurrTab(0, false);
    m_background.Create("SUR_MENU_HONEYCOMD_UP");
    m_unlock = new CUnlockWindow;
    AddToFront(m_unlock);
}

void CHelpScreen::AddTab(XString title, XString text, TextWindow** output)
{
    Window* container = new Window(false);
    container->SetCorner(m_tabs->m_x, m_tabs->m_y);
    container->SetLayoutType((ELayoutType)0);
    ScrollingContainer* scroll = new ScrollingContainer;
    scroll->SetCellPos(0, 0, 1, 1);
    scroll->SetOutsetSpacing(10, 10, 10, 10);
    scroll->SetPercentWidth(100, 0, 0);
    scroll->SetPercentHeight(100, 0, 0);
    WindowTransparent* content = new WindowTransparent(false);
    content->SetPercentWidth(100, 0, 0);
    content->SetHeightByContent(0, 0);
    content->SetLayoutType((ELayoutType)0);
    TextWindow* textWindow = new TextWindow(text, CFontMgr::GetInstance()->GetFont(15));
    textWindow->SetLayoutType((ELayoutType)1);
    textWindow->SetAlign(0x0c);
    textWindow->SetPercentWidth(80, 0, 0);
    textWindow->SetHeightByContent(0, 0);
    content->AddToFront(textWindow);
    scroll->AddToFront(content);
    container->AddToFront(scroll);
    ScrollingContainer::VertScroller* scroller = new ScrollingContainer::VertScroller(scroll);
    scroller->SetCellPos(1, 0, 1, 1);
    scroller->SetOutsetSpacing(3, 5, 3, 4);
    container->AddToFront(scroller);
    CTabWindow* tabs = m_tabs;
    tabs->AddTab(title, container, SScreenAdaptingConstant(15, 20)());
    if (output != NULL) *output = textWindow;
}

CHelpScreen::~CHelpScreen() { WindowApp::HandleTunnelCommand(0x703ba476, 0, 0, 0); }
void CHelpScreen::Paint(com::glu::platform::graphics::ICGraphics2d& graphics)
{
    DrawTileImageHoriz(graphics, m_background.GetSurface(), 0, 0, m_width);
}
void CHelpScreen::Update()
{
    if (m_unlock->m_unlocked && m_pendingReveal) {
        m_pendingReveal = false;
        m_aboutText->SetText(m_debugText);
    }
}
void CHelpScreen::OnCommand(Event& event)
{
    if ((unsigned int)event.m_id == 0x97204784u) {
        ReverseEasy(0x200);
        event.Clear();
    }
    CBaseScreen::OnCommand(event);
}

CUnlockWindow::CUnlockWindow() : Window(false), m_pressed(false), m_unlocked(false), m_nextZone(0)
{
    InitFingerZones();
    ClearFlags(0x200000);
}
void CUnlockWindow::InitFingerZones()
{
    int width = m_width;
    int height = m_height;
    width /= 2;
    height /= 2;
    m_zones[0] = new FingerZone(0, 0, width, height);
    m_zones[1] = new FingerZone(width, 0, width, height);
    m_zones[2] = new FingerZone(0, height, width, height);
    m_zones[3] = new FingerZone(width, height, width, height);
}
CUnlockWindow::~CUnlockWindow()
{
    for (int index = 0; index != 4; ++index) {
        if (m_zones[index] != NULL) {
            delete m_zones[index];
            m_zones[index] = NULL;
        }
    }
}
void CUnlockWindow::OnPointerPressed(Event& event)
{
    m_pressed = true;
    FingerZone* zone = m_zones[0];
    if (event.m_param1 < zone->m_x + zone->m_width && event.m_param1 > zone->m_x &&
        event.m_param2 > zone->m_y && event.m_param2 < zone->m_y + zone->m_height) m_nextZone = 1;
}
void CUnlockWindow::OnPointerReleased(Event&)
{
    m_pressed = false;
    for (int index = 0; index != 4; ++index) m_zones[index]->m_flag = false;
    m_nextZone = 0;
}
void CUnlockWindow::OnPointerDragged(Event& event)
{
    if (!m_pressed) return;
    FingerZone* zone = m_zones[m_nextZone];
    if (event.m_param1 >= zone->m_x + zone->m_width || event.m_param1 <= zone->m_x ||
        event.m_param2 <= zone->m_y || event.m_param2 >= zone->m_y + zone->m_height) return;
    ++m_nextZone;
    if (m_nextZone != 4) return;
    for (int index = 0; index != 4; ++index) m_zones[index]->m_flag = false;
    m_nextZone = 0;
    m_unlocked = true;
}
