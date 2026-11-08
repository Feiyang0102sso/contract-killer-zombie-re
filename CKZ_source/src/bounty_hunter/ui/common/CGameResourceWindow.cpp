// j: original 14887c..148ce8 full two-resource window; base/derived are both fc.
#include "CGameResourceWindow.h"
#include "BaseDialog.h"
#include "SGImageWindow.h"
#include "SScreenAdaptingConstant.h"
#include "CFontMgr.h"
#include "CBH_Player.h"
#include "DGHelper.h"

CGameResourceWindow::CGameResourceWindow(EType type, bool largeSpacing)
    : m_type(type), m_value(0), m_maximum(0)
{
    WindowTransparent* content = new WindowTransparent(false);
    content->SetWidthByContent(0, 0);
    content->SetHeightByContent(0, 0);
    content->SetAlign(0x10);
    SGImageWindow* icon = new SGImageWindow;
    icon->SetArchetypeCharacter(1, 0);
    icon->SetAlign(0x14);
    int animation = 0;
    switch (m_type) {
    case 0: animation = 6; break;
    case 1: animation = 5; break;
    case 2: animation = 7; break;
    case 3: case 8: animation = 36; break;
    case 4: animation = 39; break;
    case 5: animation = 72; break;
    case 6: animation = 40; break;
    case 7: animation = 41; break;
    }
    icon->SetAnimation(animation, 1);
    m_icon = icon;
    content->AddToFront(icon, 0, 0);
    int spacing = SScreenAdaptingConstant(8)();
    if (!largeSpacing) {
        spacing = SScreenAdaptingConstant(4)();
    }
    // Original 14899a allocates first, then queries the font manager, and only reads m_type at 1489d6; the type must not be cached across the external call.
    TextWindow* valueText = (TextWindow*)malloc(sizeof(TextWindow));
    CFontMgr* fontManager = CFontMgr::GetInstance();
    int font = 0;
    switch (m_type) {
    case 0: case 4: case 5: case 6: case 7: font = 29; break;
    case 1: case 8: font = 28; break;
    case 2: font = 27; break;
    case 3: font = 26; break;
    }
    new (valueText) TextWindow(fontManager->GetFont(font));
    m_valueText = valueText;
    m_valueText->SetOutsetSpacing(0, 0, 0, spacing);
    m_valueText->SetAlign(0x14);
    content->AddToFront(m_valueText, 1, 0);
    AddToFront(content);
    SetWidthByContent(0, 0);
    SetHeightByContent(0, 0);
    m_displayFormat = 0;
    UpdateDisplay();
    SetAlign(0x10);
    UpdateDisplay();
}

void CGameResourceWindow::UpdateDisplay()
{
    XString text;
    switch (m_displayFormat) {
    case 0:
        text = XString(m_value);
        break;
    case 1:
        text = DGHelper::FormatMoneyString(m_value);
        break;
    case 2:
        {
            XString value(m_value);
            XString prefix = value + L"/";
            XString maximum(m_maximum);
            text = prefix + maximum;
        }
        break;
    case 3:
        m_valueText->ClearFlags(1);
        return;
    }
    m_valueText->SetText(text);
}

void CGameResourceWindow::SetValues(int value, int maximum)
{
    m_value = value;
    m_maximum = maximum;
    UpdateDisplay();
}

void CGameResourceWindow::Update()
{
    if (m_pressed && m_command != 0) {
        SetOpacity(0.7f);
    } else {
        SetOpacity(1.0f);
    }
}

CPlayerGameResourceWindow::CPlayerGameResourceWindow(EType type, bool largeSpacing)
    : CGameResourceWindow(type, largeSpacing)
{
    m_displayFormat = 0;
    if (type == 7) {
        m_displayFormat = 3;
    } else if (type == 2) {
        m_displayFormat = 2;
    } else if ((unsigned int)type <= 1) {
        m_displayFormat = 1;
    }
    UpdateDisplay();
}

void CPlayerGameResourceWindow::Update()
{
    CBH_Player* player = CBH_Player::GetInstance();
    bool changed = true;
    switch (m_type) {
    case 0:
        m_value = player->GetStats((PLAYER_STATS_TYPE)2);
        m_maximum = 0;
        break;
    case 1:
        m_value = player->GetStats((PLAYER_STATS_TYPE)4);
        m_maximum = 0;
        break;
    case 2:
        {
            int value = player->GetStats((PLAYER_STATS_TYPE)0);
            int maximum = player->GetMaxEnergy();
            m_value = value;
            m_maximum = maximum;
        }
        break;
    case 3:
        m_value = player->GetStats((PLAYER_STATS_TYPE)3);
        m_maximum = 0;
        break;
    case 4:
        {
            int value = player->GetStats((PLAYER_STATS_TYPE)8);
            int maximum = player->GetStats((PLAYER_STATS_TYPE)9);
            m_value = value;
            m_maximum = maximum;
        }
        break;
    case 6:
        {
            int value = player->GetStats((PLAYER_STATS_TYPE)6);
            int maximum = player->GetStats((PLAYER_STATS_TYPE)7);
            m_value = value;
            m_maximum = maximum;
        }
        break;
    default:
        changed = false;
        break;
    }
    if (changed) {
        UpdateDisplay();
    }
    CGameResourceWindow::Update();
}
