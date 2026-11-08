// CTabWindow.cpp: keeps the window's absolute short-integer coordinates, the full resource references and the tab-switch side effects.
#include "CTabWindow.h"
#include "App.h"
#include "BaseDialog.h"
#include "CFontMgr.h"
#include "CFont.h"
#include "CBH_Player.h"
#include "CBH_ProgressBar.h"
#include "CDH_SoundManager.h"
#include "SScreenAdaptingConstant.h"

CTabWindow::CTabWindow(bool showInfo)
    : Window(false), m_infoTab(NULL), m_showInfo(false), m_currentTab(-1)
{
    SetLayoutType(Window::LAYOUT_NONE);
    // The original stores 11 image pointers and font 24 contiguously on the stack; restored here as a single set of style parameters.
    struct TabStyle {
        const char* images[11];
        int font;
    };
    TabStyle style;
    style.images[0] = NULL;
    style.images[1] = "SUR_IDB__BOOKMARK_OFF_LEFT_PNG";
    style.images[2] = "SUR_IDB__BOOKMARK_OFF_MID_PNG";
    style.images[3] = "SUR_IDB__BOOKMARK_RIGHT_PNG";
    style.images[4] = "SUR_IDB__BOOKMARK_ON_LEFT_PNG";
    style.images[5] = "SUR_IDB__BOOKMARK_ON_MID_PNG";
    style.images[6] = "SUR_IDB__BOOKMARK_ON_RIGHT_PNG";
    style.images[7] = "SUR_IDB__BOOKMARK_ON_MID_PNG";
    style.images[8] = "SUR_IDB__BOOKMARK_ON_RIGHT_PNG";
    style.images[9] = "SUR_IDB__BOOKMARK_ON_LEFT_PNG";
    style.images[10] = "SUR_IDB__BOOKMARK_LINE_PNG";
    style.font = 24;
    for (int index = 0; index < 11; ++index) {
        if (style.images[index] != NULL) {
            m_images[index].Create(style.images[index]);
        } else {
            m_images[index] = ImageRes();
        }
    }
    m_font = CFontMgr::GetInstance()->GetFont(style.font);
    m_showInfo = showInfo;
    if (showInfo == true) {
        CreateInfoTab();
    }
}

void CTabWindow::CreateInfoTab()
{
    m_infoTab = new TTab();
    m_infoTab->m_x = SScreenAdaptingConstant(10, 20)();
    m_infoTab->m_width = SScreenAdaptingConstant(150, 300)();
    m_infoTab->m_y = m_y;
    m_infoTab->m_height = ImageHeight(m_images[7].GetSurface());
    m_infoWindow = new Window(false);
    m_infoWindow->SetLayoutType(Window::LAYOUT_NONE);
    m_infoWindow->SetRect(m_infoTab->m_x, m_infoTab->m_y, m_infoTab->m_width, m_infoTab->m_height, false);
    Window* levelGroup = new Window(false);
    levelGroup->SetPercentWidth(27, 0, 0);
    ImageWindow* scope = new ImageWindow();
    scope->SetFillType(ImageWindow::FILL_NONE);
    scope->SetImage(ImageRes("SUR_SCOPE_LEVEL"));
    scope->SetAlign(36);
    levelGroup->AddToFront(scope);
    XString level(CBH_Player::GetInstance()->m_id);
    TextWindow* levelText = new TextWindow(level, CFontMgr::GetInstance()->GetFont(9));
    levelText->SetLayoutType(Window::LAYOUT_SIMPLE);
    levelText->SetAlign(36);
    levelText->SetOutsetSpacing(-3 * App::Scale(), 0, 0, 0);
    levelGroup->AddToFront(levelText);
    levelGroup->SetLayoutType(Window::LAYOUT_TABLE);
    levelGroup->SetCellPos(0, 0, 1, 1);
    int left = 3 * App::Scale();
    int top = 3 * App::Scale();
    int bottom = 3 * App::Scale();
    levelGroup->SetOutsetSpacing(left, 0, top, bottom);
    m_infoWindow->AddToFront(levelGroup);
    Window* currencies = new Window(false);
    currencies->SetLayoutType(Window::LAYOUT_TABLE);
    currencies->SetCellPos(1, 0, 1, 1);
    currencies->SetPercentWidth(75, 0, 0);
    m_moneyBar = new CBH_ProgressBar(3, SScreenAdaptingConstant(100, 200)(), -1);
    int x = 8 * App::Scale();
    int y = App::Scale();
    m_moneyBar->SetCorner(x, y);
    m_moneyBar->SetDescImage("IDB_HUD_ICON_MONEY");
    m_moneyBar->m_barFlags = 5;
    m_moneyBar->SetTextAlign(TEXT_OUTSIDE);
    m_moneyBar->SetCurrValue(CBH_Player::GetInstance()->GetStats(PLAYER_STATS_SOFT_CURRENCY));
    currencies->AddToFront(m_moneyBar);
    m_creditsBar = new CBH_ProgressBar(3, SScreenAdaptingConstant(100, 200)(), -1);
    x = 8 * App::Scale();
    y = 21 * App::Scale();
    m_creditsBar->SetCorner(x, y);
    m_creditsBar->SetDescImage("IDB_HUD_ICON_CREDITS");
    m_creditsBar->m_barFlags = 5;
    m_creditsBar->SetTextAlign(TEXT_OUTSIDE);
    m_creditsBar->SetCurrValue(CBH_Player::GetInstance()->GetStats(PLAYER_STATS_HARD_CURRENCY));
    currencies->AddToFront(m_creditsBar);
    m_infoWindow->AddToFront(currencies);
    int right = SScreenAdaptingConstant(5, 10, 10, -1)();
    top = SScreenAdaptingConstant(5, 10, 10, -1)();
    bottom = SScreenAdaptingConstant(5, 10, 10, -1)();
    m_infoWindow->SetInsetSpacing(0, right, top, bottom);
    AddToFront(m_infoWindow);
}

void CTabWindow::AddTab(const XString& name, Window* window, int padding)
{
    AddTab(name, window, NULL, false, padding);
}

void CTabWindow::AddTab(const XString& name, Window* window, const char* image, bool locked, int padding)
{
    TTab* tab = new TTab();
    tab->m_name = name;
    tab->m_window = window;
    tab->m_locked = locked;
    if (image != NULL) {
        tab->m_image.Create(image);
    }
    int left = ImageWidth(m_images[4].GetSurface());
    int middle = ImageWidth(m_images[5].GetSurface());
    int right = ImageWidth(m_images[6].GetSurface());
    int textWidth = m_font->MeasureTextWidth(name.c_str(), -1, -1, NULL);
    int width = left + middle + right;
    int imageWidth = ImageWidth(tab->m_image.GetSurface());
    if (imageWidth > width) {
        width = imageWidth;
    }
    if (width < textWidth) {
        width = textWidth + SScreenAdaptingConstant(3, 5)();
    }
    if (padding > 0) {
        width += padding;
    }
    int extra = SScreenAdaptingConstant(7, 20)();
    int height = GetCaptionHeight();
    short y = m_y;
    int infoWidth = 0;
    if (m_showInfo) {
        int margin = SScreenAdaptingConstant(10, 20)();
        infoWidth = margin + SScreenAdaptingConstant(150, 300)();
    }
    short tabWidth = width + extra;
    if (m_tabs.size() == 0) {
        tab->m_x = infoWidth + (m_width - infoWidth) / 2 - tabWidth / 2;
    } else {
        TTab* first = m_tabs[0];
        int totalWidth = GetTabWidth() + tabWidth - 1;
        first->m_x = infoWidth + (m_width - infoWidth) / 2 - totalWidth / 2;
        tab->m_x = 0;
    }
    tab->m_y = y;
    tab->m_width = tabWidth;
    tab->m_height = height;
    AddToFront(window);
    m_tabs.addElement(tab);
    for (int index = 0; index < m_tabs.size() - 1; ++index) {
        m_tabs[index + 1]->m_x = m_tabs[index]->m_x + m_tabs[index]->m_width - 1;
    }
    window->ClearFlags(0x2001);
    window->SetLayoutType(Window::LAYOUT_NONE);
    int windowWidth = m_width;
    int windowHeight = m_height;
    int windowY = m_y;
    int captionHeight = GetCaptionHeight();
    window->SetSize(windowWidth, windowHeight - captionHeight - windowY);
    int windowX = m_x;
    windowY = m_y;
    captionHeight = GetCaptionHeight();
    window->SetCorner(windowX, windowY + captionHeight);
    if (m_showInfo) {
        m_infoWindow->SetCorner(m_infoWindow->m_x, m_y);
    }
}

int CTabWindow::GetCaptionHeight()
{
    return ImageHeight(m_images[2].GetSurface());
}

int CTabWindow::GetTabWidth()
{
    int result = 0;
    // Size read in the loop test (not cached): keeps the original indexed loop form (b117-inl).
    for (int index = 0; index < m_tabs.size(); ++index) {
        result += m_tabs[index]->m_width - 1;
    }
    return result;
}

void CTabWindow::OnPointerEvent(Event& event)
{
    if (event.m_type != Event::POINTER_PRESSED) {
        return;
    }
    int x = event.m_param1;
    int y = event.m_param2;
    for (int index = 0; index < m_tabs.size(); ++index) {
        TTab* tab = m_tabs[index];
        if (x > tab->m_x && x < tab->m_x + tab->m_width && y > tab->m_y && y < tab->m_y + tab->m_height) {
            SetCurrTab(index, true);
        }
    }
}

void CTabWindow::SetCurrTab(int index, bool fromPointer)
{
    if (m_currentTab == index) {
        return;
    }
    TTab* tab = m_tabs[index];
    if (tab->m_locked && fromPointer == true) {
        WindowApp::HandleTunnelCommand(0xde1f7407, 0, 0, 0);
        return;
    }
    if (m_currentTab >= 0) {
        m_tabs[m_currentTab]->m_window->ClearFlags(0x2001);
        tab = m_tabs[index];
    }
    Window* window = tab->m_window;
    window->SetFlags(0x2001);
    m_currentTab = index;
    ((App*)WindowApp::m_instance)->m_soundManager->Play((CDH_SoundManager::eSound)27, false, false);
    Window::EasyParams easy(window, EasyLib::OUT_CUBIC);
    easy.SetFullTime(0.5f);
    easy.SetOpacity(0.0f, 1.0f);
    window->StartEasy(easy, 0x100);
    WindowApp::HandleTunnelCommand(0x5ce88d33, index, 0, 0);
}

void CTabWindow::Paint(com::glu::platform::graphics::ICGraphics2d& graphics)
{
    if (m_currentTab == -1) {
        return;
    }
    if (m_images[0].m_resource != NULL) {
        com::glu::platform::graphics::ICRenderSurface* surface = m_images[0].GetSurface();
        DrawTileImageRect(graphics, surface, m_x, m_y, m_width, m_height);
    }
    for (int index = 0; index < m_tabs.size(); ++index) {
        if (index == m_currentTab) {
            continue;
        }
        TTab* tab = m_tabs[index];
        com::glu::platform::graphics::ICRenderSurface* left = m_images[1].GetSurface();
        com::glu::platform::graphics::ICRenderSurface* middle = m_images[2].GetSurface();
        com::glu::platform::graphics::ICRenderSurface* right = m_images[3].GetSurface();
        DrawLineImageHoriz(graphics, left, middle, right, tab->m_x, tab->m_y, tab->m_width);
        if (tab->m_name.ByteSize() == 0 || tab->m_image.m_resource == NULL) {
            DrawFontText(graphics, m_font, tab->m_name.c_str(), tab->m_x + tab->m_width / 2,
                          tab->m_y + tab->m_height / 2, 36);
            if (tab->m_image.m_resource == NULL) {
                continue;
            }
        }
        com::glu::platform::graphics::ICRenderSurface* image = tab->m_image.GetSurface();
        DrawImage(graphics, image, tab->m_x + tab->m_width / 2, tab->m_y + tab->m_height, 20, 0);
    }
    com::glu::platform::graphics::ICRenderSurface* line = m_images[10].GetSurface();
    int lineX = m_x;
    int lineY = m_y;
    int captionHeight = GetCaptionHeight();
    int firstHeight = ImageHeight(m_images[10].GetSurface());
    int width = m_width;
    int secondHeight = ImageHeight(m_images[10].GetSurface());
    DrawTileImageRect(graphics, line, lineX, lineY + captionHeight - firstHeight, width, secondHeight);
    TTab* tab = m_tabs[m_currentTab];
    if (tab->m_name.ByteSize() == 0 || tab->m_image.m_resource == NULL) {
        com::glu::platform::graphics::ICRenderSurface* left = m_images[4].GetSurface();
        com::glu::platform::graphics::ICRenderSurface* middle = m_images[5].GetSurface();
        com::glu::platform::graphics::ICRenderSurface* right = m_images[6].GetSurface();
        DrawLineImageHoriz(graphics, left, middle, right, tab->m_x, tab->m_y, tab->m_width);
        DrawFontText(graphics, m_font, tab->m_name.c_str(), tab->m_x + tab->m_width / 2,
                      tab->m_y + tab->m_height / 2, 36);
    } else {
        int tabY = tab->m_y;
        int selectedHeight = ImageHeight(m_images[7].GetSurface());
        int normalHeight = ImageHeight(m_images[2].GetSurface());
        com::glu::platform::graphics::ICRenderSurface* left = m_images[9].GetSurface();
        com::glu::platform::graphics::ICRenderSurface* middle = m_images[7].GetSurface();
        com::glu::platform::graphics::ICRenderSurface* right = m_images[8].GetSurface();
        DrawLineImageHoriz(graphics, left, middle, right, tab->m_x, tabY - selectedHeight + normalHeight, tab->m_width);
        int textX = tab->m_x + tab->m_width / 2;
        int textY = tab->m_y;
        com::glu::platform::graphics::CFont* font = m_font;
        const wchar_t* name = tab->m_name.c_str();
        int margin = SScreenAdaptingConstant(5, 10)();
        DrawFontText(graphics, font, name, textX, textY + margin, 36);
    }
    if (tab->m_image.m_resource != NULL) {
        com::glu::platform::graphics::ICRenderSurface* image = tab->m_image.GetSurface();
        DrawImage(graphics, image, tab->m_x + tab->m_width / 2, tab->m_y + tab->m_height, 20, 0);
    }
    if (m_showInfo) {
        DrawInfoTab(graphics);
    }
}

void CTabWindow::DrawInfoTab(com::glu::platform::graphics::ICGraphics2d& graphics)
{
    com::glu::platform::graphics::ICRenderSurface* left = m_images[9].GetSurface();
    com::glu::platform::graphics::ICRenderSurface* middle = m_images[7].GetSurface();
    com::glu::platform::graphics::ICRenderSurface* right = m_images[8].GetSurface();
    int y = m_y;
    int x = m_infoTab->m_x;
    int selectedHeight = ImageHeight(m_images[7].GetSurface());
    int normalHeight = ImageHeight(m_images[2].GetSurface());
    DrawLineImageHoriz(graphics, left, middle, right, x, y - selectedHeight + normalHeight, m_infoTab->m_width);
}

CTabWindow::~CTabWindow()
{
    SAFE_DELETE_VECTOR(m_tabs);
    m_tabs.removeAllElements();
    if (m_infoTab != NULL) {
        delete m_infoTab;
        m_infoTab = NULL;
    }
}

void CTabWindow::OnGetFocus()
{
    UpdateInfoTab();
}

void CTabWindow::UpdateInfoTab()
{
    if (!m_showInfo) {
        return;
    }
    m_moneyBar->SetCurrValue(CBH_Player::GetInstance()->GetStats(PLAYER_STATS_SOFT_CURRENCY));
    m_creditsBar->SetCurrValue(CBH_Player::GetInstance()->GetStats(PLAYER_STATS_HARD_CURRENCY));
}

void CTabWindow::SetTabsWidthByWindowWidth()
{
    HandleUpdate(false);
    int count = m_tabs.size();
    int width = m_width;
    int spacing = SScreenAdaptingConstant(7, 20)();
    int tabWidth = (width - spacing * count) / count;
    TTab* first = m_tabs[0];
    first->m_x = m_width / 2 - (tabWidth * count) / 2;
    first->m_width = tabWidth;
    for (int index = 1; index < m_tabs.size(); ++index) {
        TTab* tab = m_tabs[index];
        tab->m_x = m_tabs[index - 1]->m_x + tabWidth - 1;
        tab->m_width = tabWidth;
    }
}
