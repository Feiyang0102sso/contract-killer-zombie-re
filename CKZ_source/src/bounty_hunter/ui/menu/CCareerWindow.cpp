// j: complete Career plus the red scroll bar, gray progress bar and stat rows from the same original owner.
#include "CCareerWindow.h"
#include "SScreenAdaptingConstant.h"
#include "ServiceWindows.h"
#include "CBH_SurfaceManager.h"
#include "CBH_Player.h"
#include "CFontMgr.h"
#include "CDrawUtil.h"
#include "MathLib.h"
#include "App.h"

// The original PC points to the 7 entries at 4ffe20; do not follow the wrongly same-named MergedGlobals span at the top of ref.
static SScreenAdaptingConstant SCROLLER_WIDTH(3, 6);
static SScreenAdaptingConstant SCROLLER_BOTTOM(10, 20);
static SScreenAdaptingConstant ROW_RIGHT_SPACE(6, 12);
static SScreenAdaptingConstant TITLE_RIGHT_SPACE(5, 10);
static SScreenAdaptingConstant BACKGROUND_SHIFT(2, 4);
static SScreenAdaptingConstant ROW_INSET_X(4, 8);
static SScreenAdaptingConstant ROW_INSET_Y(5, 10);

CCareerWindow::CCareerWindow() : Window(false), m_elapsedMS(0)
{
    FillStats();
    m_background = ImageRes("SUR_IDB__BG_UP_FLIP_PNG");
    com::glu::platform::graphics::CFont* titleFont = CFontMgr::GetInstance()->GetFont(26);
    Window* title = new Window(false);
    title->SetPercentHeight(12, 0, 0);
    TextWindow* titleText = new TextWindow(XString("GAME STATISTICS"), titleFont);
    titleText->SetLayoutType((ELayoutType)1);
    titleText->SetAlign(0x24);
    title->AddToFront(createTD(titleText, 3), 0, 0);
    AddToFront(title, 0, 0);
    title->SetOutsetSpacing(0, 0, TITLE_RIGHT_SPACE(), 0);
    m_content = new Window(false);
    m_content->SetLayoutType((ELayoutType)1);
    m_content->SetAlign(0x24);
    m_content->SetPercentHeight(100, 0, 0);
    m_content->SetWidthByContent(0, 0);
    m_content->SetPercentWidth(100, 0, 0);
    ScrollingContainer* scroll = new ScrollingContainer;
    scroll->SetCellPos(0, 0, 1, 1);
    com::glu::platform::graphics::ICRenderSurface* frame =
        ((App*)WindowApp::m_instance)->m_surfaceManager->CreateSurface("IDB__FRAME_CAREER_PNG", true, false);
    scroll->SetDesiredWidth(ImageWidth(frame));
    WindowTransparent* rows = new WindowTransparent(false);
    rows->SetLayoutType((ELayoutType)0);
    rows->SetHeightByContent(0, 0);
    rows->SetWidthByContent(0, 0);
    for (int index = 0; index < m_stats.m_size; ++index) {
        StatItem* row = new StatItem(m_stats.m_items[index], index);
        row->SetOutsetSpacing(0, 0, ROW_RIGHT_SPACE(), 0);
        rows->AddToFront(row, 0, index);
    }
    scroll->AddToFront(rows);
    m_content->AddToFront(scroll);
    ScrollingContainer::VertScroller* scroller = new ScrollingContainer::VertScroller(scroll);
    scroller->SetOutsetSpacing(0, 0, 0, SCROLLER_BOTTOM());
    scroller->SetCellPos(1, 0, 1, 1);
    m_content->AddToFront(scroller);
    ((App*)WindowApp::m_instance)->m_surfaceManager->ReleaseSurface(frame);
    Window* lower = createTD(m_content, 3);
    AddToFront(lower, 0, 1);
    m_content->ClearFlags(0x4000);
    lower->SetPercentHeight(92, 0, 0);
}

void CCareerWindow::FillStats()
{
    m_stats.removeAllElements();
    CBH_Player* player = CBH_Player::GetInstance();
    m_stats.addElement(StatItemInfo("IDS_CAREER_STAT_TOTAL_KILLS", player->m_gameStats.m_zombiesKilled, false, false, true));
    m_stats.addElement(StatItemInfo("IDS_CARRER_STAT_HEADSHOTS_NUMBER", player->m_gameStats.m_headshots, false, false, true));
    m_stats.addElement(StatItemInfo("IDS_CAREER_STAT_CONTRACTS_COMPLETED", player->m_gameStats.m_missionsCompleted, false, false, true));
    m_stats.addElement(StatItemInfo("IDS_CAREER_STAT_ENCOUNTERS_NUMBER", player->m_gameStats.m_totalMissions, false, false, false));
    int success = 0;
    if (player->m_gameStats.m_totalMissions != 0) {
        success = (int)((float)player->m_gameStats.m_missionsCompleted / (float)player->m_gameStats.m_totalMissions * 100.0f);
        if (success < 0) { success = 0; }
        if (success > 100) { success = 100; }
    }
    m_stats.addElement(StatItemInfo("IDS_CAREER_STAT_SUCCESS_CONTRACTS", success, false, true, false));
    m_stats.addElement(StatItemInfo("IDS_CARRER_STAT_SHOTS_NUMBER", player->m_gameStats.m_roundsFired, false, false, false));
    m_stats.addElement(StatItemInfo("IDS_CARRER_STAT_HITS_NUMBER", player->m_gameStats.m_shotsHit, false, false, false));
    int accuracy = 0;
    if (player->m_gameStats.m_roundsFired != 0) {
        accuracy = (int)((float)player->m_gameStats.m_shotsHit / (float)player->m_gameStats.m_roundsFired * 100.0f);
        if (accuracy < 0) { accuracy = 0; }
        if (accuracy > 100) { accuracy = 100; }
    }
    m_stats.addElement(StatItemInfo("IDS_CARRER_STAT_ACCURACY", accuracy, false, true, false));
    int precision = 0;
    if (player->m_gameStats.m_shotsHit != 0) {
        precision = (int)((float)player->m_gameStats.m_headshots / (float)player->m_gameStats.m_shotsHit * 100.0f);
        if (precision < 0) { precision = 0; }
        if (precision > 100) { precision = 100; }
    }
    m_stats.addElement(StatItemInfo("IDS_CARRER_STAT_PRECISION", precision, false, true, false));
    m_stats.addElement(StatItemInfo("IDS_GAME_STAT_THROWS", player->m_gameStats.m_throws, false, false, false));
    m_stats.addElement(StatItemInfo("IDS_GAME_STAT_THROW_KILLS", player->m_gameStats.m_throwKills, false, false, false));
    m_stats.addElement(StatItemInfo("IDS_GAME_STAT_PLAYTIME", 0, true, false, false));
}

void CCareerWindow::Paint(com::glu::platform::graphics::ICGraphics2d& graphics)
{
    com::glu::platform::graphics::ICRenderSurface* image = m_background.GetSurface();
    int height = m_height;
    int contentHeight = m_content->m_height;
    int shift = BACKGROUND_SHIFT();
    DrawTileImageHoriz(graphics, image, 0, height - contentHeight - shift, m_width);
}

void CCareerWindow::OnCommand(Event&)
{
    // Original 15483c is really empty.
}

void CCareerWindow::Update()
{
    m_elapsedMS += WindowApp::m_instance->m_updateFrameTime;
    float halfDuration = (float)m_stats.m_size * 0.05f;
    float duration = (halfDuration + halfDuration + 0.3f) * 1000.0f;
    if ((float)m_elapsedMS >= duration) {
        m_content->SetFlags(0x4000);
    }
    Window::Update();
}

void CCareerWindow::OnClosed()
{
    WindowApp::HandleTunnelCommand(0x703ba476, 0, 0, 0);
}

VertScrollerSimpleRed::VertScrollerSimpleRed(ScrollingContainer* container)
{
    m_container = container;
    SetDesiredWidth(SCROLLER_WIDTH());
    SetPercentHeight(100, 0, 0);
    SetOutsetSpacing(0, 0, 0, 1);
}

void VertScrollerSimpleRed::Paint(com::glu::platform::graphics::ICGraphics2d&)
{
    if (m_container->IsFullVisible()) { return; }
    float position = m_container->ScrollPos();
    float ratio = (float)m_container->m_height / (float)m_container->m_contentHeight;
    float fraction = 0.15f;
    if (ratio > fraction) { fraction = ratio; }
    float height = (float)m_height;
    int top = MathLib::Round(height * ((1.0f - fraction) * position));
    int barHeight = MathLib::Round(height * fraction);
    int middle = m_width / 2;
    int left = SCROLLER_WIDTH();
    int right = SCROLLER_WIDTH();
    CDrawUtil::Fill(middle - left, 0, middle + right, m_height, 0xff820b0b, com::glu::platform::graphics::ICGraphics2d::Primitive_Rectangles);
    left = SCROLLER_WIDTH();
    right = SCROLLER_WIDTH();
    CDrawUtil::Fill(middle - left, top, middle + right, top + barHeight, 0xffff0000, com::glu::platform::graphics::ICGraphics2d::Primitive_Rectangles);
}

SimpleGreyProgressBar::SimpleGreyProgressBar(float percent) : Window(false), m_percent(percent)
{
    m_background.Create("SUR_IDB__PROGRESSBAR_CAREER_PNG");
    m_center.Create("SUR_IDB__PROGRESSBAR_CAREER_GREY_PNG");
    m_left.Create("SUR_IDB__PROGRESSBAR_CAREER_GREY_LEFT_PNG");
    m_right.Create("SUR_IDB__PROGRESSBAR_CAREER_GREY_RIGHT_PNG");
    int width = ImageWidth(m_background.GetSurface());
    int height = ImageHeight(m_background.GetSurface());
    SetSize(width, height);
}

void SimpleGreyProgressBar::Paint(com::glu::platform::graphics::ICGraphics2d& graphics)
{
    DrawImage(graphics, m_background.GetSurface(), 0, 0, 0, 0);
    if (m_percent > 0.0f) {
        DrawImage(graphics, m_left.GetSurface(), 0, 0, 0, 0);
    }
    int leftWidth = ImageWidth(m_left.GetSurface());
    int fillPixels = (int)((float)(m_width - leftWidth * 2) * m_percent);
    DrawTileImageHoriz(graphics, m_center.GetSurface(), leftWidth, 0, fillPixels);
    if (m_percent > 0.0f) {
        DrawImage(graphics, m_right.GetSurface(), fillPixels + leftWidth, 0, 0, 0);
    }
    Window::Paint(graphics);
}

StatItem::StatItem(const CCareerWindow::StatItemInfo& info, int index) : Window(false), m_index(index)
{
    com::glu::platform::graphics::CFont* labelFont = CFontMgr::GetInstance()->GetFont(29);
    com::glu::platform::graphics::CFont* valueFont = CFontMgr::GetInstance()->GetFont(29);
    ImageWindow* frame = new ImageWindow;
    frame->SetImageById("IDB__FRAME_CAREER_PNG");
    int insetX = ROW_INSET_X();
    int insetY = ROW_INSET_Y();
    frame->SetInsetSpacing(insetX, insetY);
    frame->SetAlign(0x21);
    WindowTransparent* label = new WindowTransparent(false);
    label->SetLayoutType((ELayoutType)1);
    label->SetAlign(0x21);
    label->SetWidthByContent(0, 0);
    label->SetHeightByContent(0, 0);
    int labelColumn = 0;
    if (info.m_gameCenter) {
        ImageWindow* gameCenter = new ImageWindow;
        gameCenter->SetImage(ImageRes("SUR_IDB__GAMECENTER_ICON_PNG"));
        gameCenter->SetOutsetSpacing(App::Scale() * 2, 0, 0, 0);
        gameCenter->SetAlign(0x21);
        label->AddToFront(gameCenter, 0, 0);
        labelColumn = 1;
    }
    TextWindow* name = new TextWindow(ResString(info.m_name).Upper(), labelFont);
    label->AddToFront(name, labelColumn, 0);
    frame->AddToFront(label);
    Window* value = new Window(false);
    value->SetLayoutType((ELayoutType)1);
    value->SetAlign(0x22);
    value->SetHeightByContent(0, 0);
    value->SetPercentWidth(42, 0, 0);
    int valueColumn = 0;
    if (info.m_percent) {
        SimpleGreyProgressBar* progress = new SimpleGreyProgressBar((float)info.m_value * 0.01f);
        progress->SetLayoutType((ELayoutType)1);
        progress->SetAlign(0x24);
        value->AddToFront(createTD(progress, 3), 0, 0);
        valueColumn = 1;
    }
    TextWindow* valueText = new TextWindow;
    if (info.m_time) {
        CBH_Player* player = CBH_Player::m_staticInstance;
        if (player != NULL) {
            unsigned int seconds = player->GetGameTime();
            unsigned int minutes = (seconds / 60) % 60;
            unsigned int hours = (seconds / 3600) % 24;
            if (seconds >= 86400) {
                valueText->SetText(XString::Format(ResString("IDS_GAME_TIME_FORMAT_DAYS").m_data, seconds / 86400, hours, minutes), valueFont);
            } else if (hours != 0) {
                valueText->SetText(XString::Format(ResString("IDS_GAME_TIME_FORMAT_HOURS").m_data, hours, minutes), valueFont);
            } else if (minutes != 0) {
                valueText->SetText(XString::Format(ResString("IDS_GAME_TIME_FORMAT_MINUTES").m_data, minutes), valueFont);
            } else {
                valueText->SetText(XString::Format(ResString("IDS_GAME_TIME_FORMAT_MINUTES").m_data, 1), valueFont);
            }
        }
    } else {
        XString text(info.m_value);
        if (info.m_percent) { text.Append(XString(" %")); }
        valueText->SetText(text, valueFont);
    }
    valueText->SetAlign(0x22);
    valueText->SetPercentWidth(22, 0, 0);
    value->AddToFront(valueText, valueColumn, 0);
    frame->AddToFront(value);
    AddToFront(frame);
    SetHeightByContent(0, 0);
    SetWidthByContent(0, 0);
    m_flagb0 = false;
}

void StatItem::OnStartupEasy()
{
    // Original 1550d0 is really empty.
}
