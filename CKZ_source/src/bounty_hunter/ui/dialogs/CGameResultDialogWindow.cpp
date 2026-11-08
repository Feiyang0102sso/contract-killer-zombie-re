// u538 full result window; the two same-named statics each belong to their original owner, here original 4ff55c.
#include "CGameResultDialogWindow.h"
#include "App.h"
#include "CBH_Player.h"
#include "CGameResourceWindow.h"
#include "CFontMgr.h"
#include "SGImageWindow.h"
#include "SGButton.h"
#include "ServiceWindows.h"
#include "SScreenAdaptingConstant.h"

static SScreenAdaptingConstant RESOURCE_WIN_SPACING_X(10);

CGameResultDialogWindow::CGameResultDialogWindow()
    : CZombieDialogWindow(3), m_reviveButton(NULL), m_timer(NULL)
{
    Window* titleContainer = m_titleContainer;
    titleContainer->SetOutsetSpacing(SScreenAdaptingConstant(20)(), 0, 0, 0);
    SetTitle(ResString("IDS_RESULT_TITLE_COMPLETE"));
    Window* table = new WindowTransparent(false);
    table->SetHeightByContent(0, 0);
    table->SetPercentWidth(90, 0, 0);
    table->SetAlign(36);
    table->SetOutsetSpacing(SScreenAdaptingConstant(5)(), 0, 0, 0);
    CBH_Player* player = CBH_Player::GetInstance();
    int row = 0;
    if (player->m_gameStats.m_currentBoxesSent > 0 || player->m_gameStats.m_currentPeopleSent > 0) {
        TextWindow* saved = new TextWindow(CFontMgr::GetInstance()->GetFont(26));
        saved->SetText(ResString("IDS_RESULT_STUFF_SAVED"));
        table->AddToFront(saved, 0, 1);
        int column = 1;
        if (player->m_gameStats.m_currentPeopleSent > 0) {
    CGameResourceWindow* people = new CGameResourceWindow((CGameResourceWindow::EType)4, false);
    people->SetOutsetSpacing(0, RESOURCE_WIN_SPACING_X());
    people->SetValues(player->m_gameStats.m_currentPeopleSent, 0);
    table->AddToFront(people, 1, 1);
            column = 2;
        }
        row = 1;
        if (player->m_gameStats.m_currentBoxesSent > 0) {
    CGameResourceWindow* boxes = new CGameResourceWindow((CGameResourceWindow::EType)6, false);
    boxes->SetOutsetSpacing(0, RESOURCE_WIN_SPACING_X());
    boxes->SetValues(player->m_gameStats.m_currentBoxesSent, 0);
    table->AddToFront(boxes, column, 1);
        }
    }
    if (player->m_gameStats.m_currentPeople > 0 || player->m_gameStats.m_currentProvision > 0) {
        TextWindow* received = new TextWindow(CFontMgr::GetInstance()->GetFont(26));
        ++row;
        received->SetText(ResString("IDS_RESULT_RESOURCES_RECEIVED"));
        table->AddToFront(received, 0, row);
        int column = 1;
        if (player->m_gameStats.m_currentPeople > 0) {
    CGameResourceWindow* people = new CGameResourceWindow((CGameResourceWindow::EType)4, false);
    people->SetOutsetSpacing(0, RESOURCE_WIN_SPACING_X());
    people->SetValues(player->m_gameStats.m_currentPeople, 0);
    table->AddToFront(people, 1, row);
            column = 2;
        }
        if (player->m_gameStats.m_currentProvision > 0) {
    CGameResourceWindow* provision = new CGameResourceWindow((CGameResourceWindow::EType)6, false);
    provision->SetOutsetSpacing(0, RESOURCE_WIN_SPACING_X());
    provision->SetValues(player->m_gameStats.m_currentProvision, 0);
    table->AddToFront(provision, column, row);
        }
    }
    ++row;
    TextWindow* killsTitle = new TextWindow(CFontMgr::GetInstance()->GetFont(26));
    killsTitle->SetText(ResString("IDS_RESULT_ZOMBIES_KILLED"));
    table->AddToFront(killsTitle, 0, row);
    CGameResourceWindow* kills = new CGameResourceWindow((CGameResourceWindow::EType)5, false);
    kills->SetOutsetSpacing(0, RESOURCE_WIN_SPACING_X());
    kills->SetValues(player->m_gameStats.m_currentMissionKills, 0);
    table->AddToFront(kills, 1, row);
    TextWindow* rewardsTitle = new TextWindow(CFontMgr::GetInstance()->GetFont(26));
    ++row;
    rewardsTitle->SetOutsetSpacing(SScreenAdaptingConstant(10)(), 0, 0, 0);
    rewardsTitle->SetText(ResString("IDS_RESULT_REWARDS"));
    table->AddToFront(rewardsTitle, 0, row);
    int rewardColumn = 1;
    if (player->m_gameStats.m_currentMoney > 0) {
    CGameResourceWindow* money = new CGameResourceWindow((CGameResourceWindow::EType)0, false);
    int moneyTop = SScreenAdaptingConstant(10)();
    int moneySide = RESOURCE_WIN_SPACING_X();
    money->SetOutsetSpacing(moneyTop, moneySide);
    money->SetValues(player->m_gameStats.m_currentMoney, 0);
    table->AddToFront(money, 1, row);
        rewardColumn = 2;
    }
    if (player->m_gameStats.m_currentXP > 0) {
    CGameResourceWindow* xp = new CGameResourceWindow((CGameResourceWindow::EType)3, false);
    int xpTop = SScreenAdaptingConstant(10)();
    int xpSide = RESOURCE_WIN_SPACING_X();
    xp->SetOutsetSpacing(xpTop, xpSide);
    xp->SetValues(player->m_gameStats.m_currentXP, 0);
    table->AddToFront(xp, rewardColumn, row);
    }
    AddToFront(table);
    AddButton(BUTTON_OK, 0xcbfdb644, 0, 0, true, 20);
}

CGameResultDialogWindow::CGameResultDialogWindow(int score, int reviveCost, bool showTimer)
    : CZombieDialogWindow(2), m_reviveButton(NULL), m_timer(NULL)
{
    Window* titleContainer = m_titleContainer;
    titleContainer->SetOutsetSpacing(SScreenAdaptingConstant(20)(), 0, 0, 0);
    SetTitle(ResString("IDS_RESULT_TITLE_COMPLETE"));
    Window* table = new WindowTransparent(false);
    table->SetHeightByContent(0, 0);
    table->SetPercentWidth(90, 0, 0);
    table->SetAlign(12);
    table->SetOutsetSpacing(SScreenAdaptingConstant(70)(), 0, 0, 0);
    CBH_Player* player = CBH_Player::GetInstance();
    TextWindow* scoreTitle = new TextWindow(CFontMgr::GetInstance()->GetFont(26));
    scoreTitle->SetText(ResString("IDS_ARCADE_SCORE_RESULT"));
    table->AddToFront(scoreTitle, 0, 1);
    CGameResourceWindow* scoreWindow = new CGameResourceWindow((CGameResourceWindow::EType)2, false);
    scoreWindow->SetOutsetSpacing(0, RESOURCE_WIN_SPACING_X());
    scoreWindow->SetValues(score, 0);
    table->AddToFront(scoreWindow, 1, 1);
    int row = 2;
    SScreenAdaptingConstant killsSpacing(10);
    TextWindow* killsTitle = new TextWindow(CFontMgr::GetInstance()->GetFont(26));
    killsTitle->SetOutsetSpacing(killsSpacing(), 0, 0, 0);
    killsTitle->SetText(ResString("IDS_RESULT_ZOMBIES_KILLED"));
    table->AddToFront(killsTitle, 0, row);
    CGameResourceWindow* kills = new CGameResourceWindow((CGameResourceWindow::EType)5, false);
    kills->SetOutsetSpacing(0, RESOURCE_WIN_SPACING_X());
    kills->SetValues(player->m_gameStats.m_currentMissionKills, 0);
    table->AddToFront(kills, 1, row);
    // One adapting constant is shared by the reward heading and both value columns.
    SScreenAdaptingConstant rewardSpacing(10);
    TextWindow* rewardsTitle = new TextWindow(CFontMgr::GetInstance()->GetFont(26));
    ++row;
    rewardsTitle->SetOutsetSpacing(rewardSpacing(), 0, 0, 0);
    rewardsTitle->SetText(ResString("IDS_RESULT_REWARDS"));
    table->AddToFront(rewardsTitle, 0, row);
    int rewardColumn = 1;
    if (player->m_gameStats.m_currentMoney > 0) {
    CGameResourceWindow* money = new CGameResourceWindow((CGameResourceWindow::EType)0, false);
    money->SetOutsetSpacing(rewardSpacing(), RESOURCE_WIN_SPACING_X());
    money->SetValues(player->m_gameStats.m_currentMoney, 0);
    table->AddToFront(money, 1, row);
        rewardColumn = 2;
    }
    if (player->m_gameStats.m_currentXP > 0) {
    CGameResourceWindow* xp = new CGameResourceWindow((CGameResourceWindow::EType)3, false);
    xp->SetOutsetSpacing(rewardSpacing(), RESOURCE_WIN_SPACING_X());
    xp->SetValues(player->m_gameStats.m_currentXP, 0);
    table->AddToFront(xp, rewardColumn, row);
    }
    AddToFront(table);
    if (reviveCost > 0) {
        if (showTimer == 1) {
            m_timer = new TextWindow(CFontMgr::GetInstance()->GetFont(26));
            m_timer->SetAlign(17);
            Window* timer = m_timer;
            int bottom = SScreenAdaptingConstant(60)();
            int left = SScreenAdaptingConstant(45)();
            timer->SetOutsetSpacing(0, 0, bottom, left);
            AddToFront(m_timer);
        }
        m_reviveButton = AddButton((EDialogButton)20, 0x5c420ec6, 0, 0, true, 20);
        m_countdown = 30000;
        Window* priceRow = new WindowTransparent(false);
        priceRow->SetLayoutType(LAYOUT_SIMPLE);
        priceRow->SetAlign(36);
        priceRow->SetWidthByContent(0, 0);
        priceRow->SetHeightByContent(0, 0);
        SGImageWindow* currency = new SGImageWindow;
        currency->SetArchetypeCharacter(1, 0);
        currency->SetAnimation(5, 1);
        currency->SetAlign(36);
        priceRow->AddToFront(currency, 0, 0);
        TextWindow* value = new TextWindow;
        {
            XString text = XString::Format(L"%d", reviveCost);
            value->SetAlign(36);
            value->SetText(text, CFontMgr::GetInstance()->GetFont(24));
            priceRow->AddToFront(value, 1, 0);
        }
        Window* priceContainer = createTD(priceRow, 3);
        priceContainer->SetAlign(34);
        priceContainer->SetOutsetSpacing(0, SScreenAdaptingConstant(10)(), 0, 0);
        m_reviveButton->AddToFront(priceContainer);
    }
    SGButton* ok = AddButton(BUTTON_OK, 0x6ef3b1d1, 0, 0, true, 20);
    int top = SScreenAdaptingConstant(5)();
    int left = SScreenAdaptingConstant(20)();
    ok->SetOutsetSpacing(top, 0, 0, left);
}

void CGameResultDialogWindow::Update()
{
    SGButton* button = m_reviveButton;
    if (button == NULL || m_timer == NULL || m_countdown < 1) {
        return;
    }
    m_countdown = (int)((unsigned int)m_countdown - (unsigned int)WindowApp::m_instance->m_updateFrameTime);
    if (m_countdown < 1) {
        button->m_flagf4 = true;
    }
    XString text(L"");
    if (m_countdown > 0) {
        text.Assign(XString::Format(ResString("IDS_RESULT_REVIVE_TIMER").m_data, m_countdown / 1000 + 1));
    }
    m_timer->SetText(text);
}
