// k: original 1505ec..150a2c, complete 20 ranges; keeps the original equal-value early return, unused presenter and tree ownership.
#include "CCivilianHPBar.h"
#include "App.h"
#include "CFontMgr.h"
using namespace com::glu::platform::graphics;

CCivilianHPBar::CCivilianHPBar() : Window(false), m_unresolvedE8(0),
    m_unresolvedEc(0), m_percent(1.0f)
{
    App::LoadSGArchetype(0, 0);
    m_background = new SGImageWindow;
    m_background->SetArchetypeCharacter(0, 0);
    m_background->SetAnimation(0, 1);
    m_background->SetAlign(9);
    AddToFront(m_background);
    m_fill = new SGClippedImageWindow;
    m_fill->SetArchetypeCharacter(0, 0);
    m_fill->SetAnimation(1, 1);
    m_fill->SetAlign(9);
    AddToFront(m_fill);
    m_changeEffect = new SGImageWindow;
    m_changeEffect->SetArchetypeCharacter(0, 0);
    m_changeEffect->SetAnimation(9, 0);
    m_changeEffect->SetLayoutType(LAYOUT_NONE);
    m_changeEffect->ClearFlags(1);
    AddToFront(m_changeEffect);
    SetHeightByContent(0, 0);
    SetWidthByContent(0, 0);
    SetPercent(1.0f); // The original already set percent=1 first, so this call actually takes the equal-value early return.
}

CCivilianHPBar::~CCivilianHPBar() {}
void CCivilianHPBar::Init() {} // Original 15080c bx lr+nop.
void CCivilianHPBar::Paint(ICGraphics2d&) {} // Original 150834 bx lr+nop.

void CCivilianHPBar::SetPercent(float percent)
{
    // The original BLT also rejects unordered; must not be changed to only check percent<0.
    if (!(percent >= 0.0f)) {
        return;
    }
    if (!(percent <= 1.0f)) {
        return;
    }
    if (m_percent == percent) {
        return;
    }
    m_percent = percent;
    m_fill->SetClipping(percent, 1.0f);
    int x = (int)(m_fill->m_imageWidth * percent);
    int y = (short)((unsigned short)m_fill->m_imageY + m_fill->m_imageHeight / 2);
    m_changeEffect->SetCenter(x, y);
    m_changeEffect->SetFlags(1);
    m_changeEffect->SetRepeatCount(1, false);
}

void CCivilianHPBar::Update()
{
    if (m_changeEffect->m_presenter.HasFinished()) {
        m_changeEffect->ClearFlags(1);
    }
}

void CCivilianHPBar::SetScale(float scale)
{
    int percent = (int)(scale * 100.0f); // The 100.0f of original 150874; the three calls share one conversion.
    m_fill->SetScale(percent);
    m_background->SetScale(percent);
    m_changeEffect->SetScale(percent);
}

void CCivilianBar::SetHPBar(CCivilianHPBar* hpBar)
{
    if (m_hpBar != NULL) {
        m_hpBar->Close();
    }
    m_hpBar = hpBar;
    AddToFront(hpBar);
}

void CCivilianBar::SetNameBar(CCivilianNameBar* nameBar)
{
    if (m_nameBar != NULL) {
        m_nameBar->Close();
    }
    m_nameBar = nameBar;
    AddToFront(nameBar);
}

CCivilianHPBar* CCivilianBar::GetHPBar() { return m_hpBar; }

CCivilianBar::CCivilianBar(CCivilianHPBar* hpBar, CCivilianNameBar* nameBar) :
    Window(false), m_hpBar(NULL), m_nameBar(NULL)
{
    SetHPBar(hpBar);
    SetNameBar(nameBar);
    SetWidthByContent(0, 0);
    SetHeightByContent(0, 0);
}

CCivilianNameBar::CCivilianNameBar(XString name) : Window(false)
{
    m_text = new TextWindow(name, CFontMgr::GetInstance()->GetFont(23));
    AddToFront(m_text);
    SetWidthByContent(0, 0);
    SetHeightByContent(0, 0);
}
