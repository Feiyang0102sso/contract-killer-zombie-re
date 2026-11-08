// j: full XP bar; keeps the repeated Player queries and the full lifetime of the format string.
#include "CPlayerXPBar.h"
#include "BaseDialog.h"
#include "SGImageWindow.h"
#include "SScreenAdaptingConstant.h"
#include "CBH_Player.h"
#include "CFontMgr.h"

static SScreenAdaptingConstant BAR_OUTSET_LEFT(20);

CPlayerXPBar::CPlayerXPBar()
    : Window(false), m_fielde8(0), m_fieldec(0), m_percent(1.0f), m_cachedXP(0)
{
    SGImageWindow* background = new SGImageWindow;
    background->SetArchetypeCharacter(1, 0);
    background->SetAnimation(36, 1);
    background->SetAlign(0x21);
    AddToFront(background);
    WindowTransparent* content = new WindowTransparent(false);
    content->SetWidthByContent(0, 0);
    content->SetHeightByContent(0, 0);
    int top = SScreenAdaptingConstant(26)();
    int bottom = BAR_OUTSET_LEFT();
    content->SetOutsetSpacing(0, top, 0, bottom);
    content->SetAlign(0x21);
    AddToFront(content);
    m_bar = new SGImageWindow;
    m_bar->SetArchetypeCharacter(1, 0);
    m_bar->SetAnimation(34, 1);
    m_bar->SetAlign(9);
    content->AddToFront(m_bar);
    m_fill = new SGClippedImageWindow;
    m_fill->SetArchetypeCharacter(1, 0);
    m_fill->SetAnimation(35, 1);
    m_fill->SetAlign(9);
    content->AddToFront(m_fill);
    SGImageWindow* levelImage = new SGImageWindow;
    levelImage->SetArchetypeCharacter(1, 0);
    levelImage->SetAnimation(62, 1);
    levelImage->SetAlign(0x22);
    AddToFront(levelImage);
    m_levelText = new TextWindow(CFontMgr::GetInstance()->GetFont(29));
    m_levelText->SetAlign(0x24);
    levelImage->AddToFront(m_levelText);
    m_xpText = new TextWindow(CFontMgr::GetInstance()->GetFont(25));
    m_xpText->SetAlign(0x24);
    TextWindow* xpText = m_xpText;
    xpText->SetOutsetSpacing(0, 0, 0, SScreenAdaptingConstant(0)());
    content->AddToFront(m_xpText);
    SetHeightByContent(0, 0);
    SetWidthByContent(0, 0);
    UpdateInfo();
}

CPlayerXPBar::~CPlayerXPBar()
{
    // The Presenter member is destroyed first; the Window tree handles the windows afterwards.
}

void CPlayerXPBar::UpdateInfo()
{
    int xpForText = CBH_Player::GetInstance()->GetStats((PLAYER_STATS_TYPE)3);
    int nextForText = CBH_Player::GetInstance()->GetXpForNextLvl();
    XString formatted = XString::Format(L"%d / %d", xpForText, nextForText);
    m_xpText->SetText(formatted);
    TextWindow* levelText = m_levelText;
    levelText->SetText(XString(CBH_Player::GetInstance()->GetLevel()));
    int xp = CBH_Player::GetInstance()->GetStats((PLAYER_STATS_TYPE)3);
    int next = CBH_Player::GetInstance()->GetXpForNextLvl();
    int current = CBH_Player::GetInstance()->GetXpForCurrentLvl();
    if (next != current) {
        SetPercent((float)(xp - current) / (float)(next - current));
    }
    m_cachedXP = xp;
}

void CPlayerXPBar::Init()
{
    // Original 1492fc is truly empty.
}

void CPlayerXPBar::Update()
{
    int cached = m_cachedXP;
    if (cached != CBH_Player::GetInstance()->GetStats((PLAYER_STATS_TYPE)3)) {
        UpdateInfo();
    }
}

void CPlayerXPBar::SetPercent(float percent)
{
    if (!(percent >= 0.0f) || !(percent <= 1.0f)) {
        return;
    }
    if (m_percent == percent) {
        return;
    }
    m_percent = percent;
    m_fill->SetClipping(percent, 1.0f);
}

void CPlayerXPBar::Paint(com::glu::platform::graphics::ICGraphics2d&)
{
    // Original 149364 is truly empty.
}
