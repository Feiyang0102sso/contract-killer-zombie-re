// j: keeps the original current-period score cache and the row-number re-read before adding each cell.
#include "CAfterGameStatsWindow.h"
#include "BaseDialog.h"
#include "CBH_Player.h"
#include "CCommonGameConfig.h"
#include "CAchievementManager.h"
#include "App.h"

CAfterGameStatsWindow::CAfterGameStatsWindow() : Window(false), m_row(0)
{
    m_table = new Window(false);
    m_table->SetLayoutType((ELayoutType)1);
    m_table->SetAlign(0x24);
    m_table->SetPercentWidth(90, 0, 0);
    m_table->SetPercentHeight(80, 0, 0);
    AddToFront(m_table);
    XString empty;
    XString value("VALUE");
    XString score("SCORE");
    AddTableStr(empty, value, score, false, (eGameFont)29);
    CBH_Player* player = CBH_Player::GetInstance();
    int cycleScore = player->m_gameStats.GetThisCycleScore();
    AddStat(ResString("IDS_GAME_STAT_ZOMBIES_KILLED"), player->m_gameStats.m_zombiesKilledThisCycle,
        CPlayerGameStats::PER_ZOMBIE_KILL_SCORE * player->m_gameStats.m_zombiesKilledThisCycle, false);
    AddStat(ResString("IDS_GAME_STAT_SKILLSHOTS_DONE"), player->m_gameStats.m_skillshotsThisCycle,
        CPlayerGameStats::PER_SKILLSHOT_SCORE * player->m_gameStats.m_skillshotsThisCycle, false);
    AddStat(ResString("IDS_GAME_STAT_HEADSHOTS_DONE"), player->m_gameStats.m_headshotsThisCycle,
        CPlayerGameStats::PER_HEADSHOT_SCORE * player->m_gameStats.m_headshotsThisCycle, false);
    AddStat(ResString("IDS_GAME_STAT_BOXES_SENT"), player->m_gameStats.m_boxesSentThisCycle,
        CPlayerGameStats::PER_BOX_SCORE * player->m_gameStats.m_boxesSentThisCycle, false);
    AddStat(ResString("IDS_GAME_STAT_CIVILIANS_SENT"), player->m_gameStats.m_peopleSentThisCycle,
        CPlayerGameStats::PER_CIVILIAN_SCORE * player->m_gameStats.m_peopleSentThisCycle, false);
    AddStat(ResString("IDS_GAME_STAT_CURRENT_SCORE"), 0, cycleScore, true);
    AddTableStr(ResString("IDS_GAME_STAT_CURRENT_RANK"), XString(cycleScore),
        ((App*)WindowApp::m_instance)->m_commonGameConfig->GetRankTitleForScore(cycleScore), true, (eGameFont)28);
    ((App*)WindowApp::m_instance)->m_achievementManager->CheckScoreAchievements();
}

void CAfterGameStatsWindow::AddTableStr(XString name, const XString& value,
    const XString& score, bool hideValue, eGameFont fontId)
{
    com::glu::platform::graphics::CFont* font = CFontMgr::GetInstance()->GetFont(fontId);
    TextWindow* nameWindow = new TextWindow(font);
    nameWindow->SetText(name);
    nameWindow->SetCellPos(0, m_row, 1, 1);
    m_table->AddToFront(nameWindow);
    TextWindow* valueWindow = new TextWindow(font);
    valueWindow->SetText(value);
    valueWindow->SetCellPos(1, m_row, 1, 1);
    if (hideValue) valueWindow->ClearFlags(1);
    m_table->AddToFront(valueWindow);
    TextWindow* scoreWindow = new TextWindow(font);
    scoreWindow->SetText(score);
    scoreWindow->SetCellPos(2, m_row, 1, 1);
    m_table->AddToFront(scoreWindow);
    ++m_row;
}

void CAfterGameStatsWindow::AddStat(XString name, int value, int score, bool hideValue)
{
    AddTableStr(name, XString(value), XString(score), hideValue, (eGameFont)29);
}
