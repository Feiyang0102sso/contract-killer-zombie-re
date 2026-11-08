// CHuntingInfo.cpp: restored from original 11c984..11ccf2, keeping the partial initialization and the external call order.
#include "CHuntingInfo.h"
#include "App.h"
#include "CBH_Player.h"
#include "CMission.h"
#include "CMissionManager.h"
#include "CGCLeaderboardsManager.h"
#include "CGameAnalytics.h"
#include "DGHelper.h"
#include "CDH_Weapon.h"

CHuntingInfo::CHuntingInfo()
    : m_field00(false), m_text04(0), m_assignedData(),
      m_result(BountyResultPending), m_flag3c(false), m_field54(0),
      m_scoreAtStart(0), m_levelIncreased(false), m_currentWave(0),
      m_civiliansRelativeHp(1.0f)
{
    // The original constructor does not clear 8..10, 40..50 and 5c..68; statistics reset is handled by Clean.
}

CHuntingInfo::SAssignedData::SAssignedData()
    : m_questId(-1), m_field08(-1), m_field14(0), m_field18(0),
      m_flag20(false), m_flag21(false)
{
}

void CHuntingInfo::AfterSceneActivity()
{
    if (m_field00) {
        return;
    }
    CMission* mission = ((App*)WindowApp::m_instance)->m_missionManager->GetMissionByName(
        ((App*)WindowApp::m_instance)->m_huntingInfo->m_assignedData.m_missionName);
    if (mission->m_type != CMission::MISSION_EM) {
        CBH_Player::GetInstance()->m_gameStats.AddTotalMissions(1);
    }
    ((App*)WindowApp::m_instance)->m_leaderboardsManager->UpdateLeaderboards();
    if (m_result != BountyResultWin) {
        CBH_Player* player = CBH_Player::GetInstance();
        player->FieldAt(0x5c) = 1;
        player->Save(true);
        return;
    }
    WindowApp::HandleCommand(0x45f9bf40, 0, 0, 0);
    CBH_Player* player = CBH_Player::GetInstance();
    player->FieldAt(0x5c) = 2;
    player->Save(true);
    CBH_Player::GetInstance()->m_gameStats.NotifyMissionWin(mission);
    CBH_Player::GetInstance()->m_gameStats.AddCiviliansResqued(m_peopleEarned);
    CBH_Player::GetInstance()->m_gameStats.AddProvisionScrounged(m_provisionEarned);
    if (mission->m_type != CMission::MISSION_EM) {
        CBH_Player::GetInstance()->m_gameStats.AddCompletedMissions(1);
    }
    if (m_assignedData.m_questId >= 1) {
        CBH_Player::GetInstance()->OnQuestCompleted(m_assignedData.m_questId);
    }
    // The original rereads the result after the external call; the earlier branch must not replace this check.
    if (m_result != BountyResultWin) {
        return;
    }
    CDH_Weapon* first = ((App*)WindowApp::m_instance)->m_helper->m_gamePlayManager.GetGameData()->GetWeapon(0);
    CDH_Weapon* second = ((App*)WindowApp::m_instance)->m_helper->m_gamePlayManager.GetGameData()->GetWeapon(1);
    int firstId = 0;
    int secondId = 0;
    if (first != NULL) {
        firstId = first->m_id;
    }
    if (second != NULL) {
        secondId = second->m_id;
    }
    CGameAnalytics::logMissionCompleted(mission->m_name, firstId, secondId);
    int level = CBH_Player::GetInstance()->m_id;
    int score = CBH_Player::GetInstance()->m_gameStats.GetThisCycleScore() - m_scoreAtStart;
    CGameAnalytics::logMissionCompleteScore(mission->m_name, level, score);
    if (mission->m_type == CMission::MISSION_EM) {
        int survivalLevel = CBH_Player::GetInstance()->m_id;
        CGameAnalytics::logSurvivalModeFinishedWave(survivalLevel, m_currentWave);
        CGameAnalytics::logSurvivalModeZombiesKilled(survivalLevel, CBH_Player::GetInstance()->FieldAt(0x170));
        CGameAnalytics::logSurvivalModeXpEarned(survivalLevel, m_arcadeExperience);
        CGameAnalytics::logSurvivalModeSoftEarned(survivalLevel, m_arcadeMoney);
        CGameAnalytics::logSurvivalModeFinalScore(survivalLevel, m_arcadePoints);
    }
}

bool CHuntingInfo::IsPlayerWin() const
{
    return m_result == BountyResultWin;
}

void CHuntingInfo::SetHuntingData(SAssignedData data)
{
    m_assignedData = data;
    CleanHuntingStatistic();
}

void CHuntingInfo::CleanHuntingStatistic()
{
    m_field00 = false;
    m_currentWave = 0;
    m_civiliansRelativeHp = 1.0f;
    m_field54 = 0;
    m_arcadePoints = 0;
    m_arcadeExperience = 0;
    m_arcadeMoney = 0;
    m_result = BountyResultPending;
    m_flag3c = false;
    m_levelIncreased = false;
    m_counter5c = 0;
    m_enemiesKilled = 0;
    m_enemiesTranquillized = 0;
    m_enemiesEscaped = 0;
    m_peopleEarned = 0;
    m_provisionEarned = 0;
    m_field48 = 0;
    m_field4c = 0;
    m_peopleSent = 0;
    m_scoreAtStart = CBH_Player::GetInstance()->m_gameStats.GetThisCycleScore();
}

void CHuntingInfo::SetBountyResult(EBountyResult result)
{
    m_result = result;
}

void CHuntingInfo::OnEnemyKilled()
{
    ++m_enemiesKilled;
}

void CHuntingInfo::OnEnemyTranqullized()
{
    ++m_enemiesTranquillized;
}

void CHuntingInfo::OnEnemyEscaped()
{
    ++m_enemiesEscaped;
}

void CHuntingInfo::AddArcadePoints(int points, int experience, int cash)
{
    m_arcadePoints += points;
    m_arcadeExperience += experience;
    m_arcadeMoney += cash;
    CBH_Player::GetInstance()->m_gameStats.NotifyArcadeScoreChanged(m_arcadePoints);
}
