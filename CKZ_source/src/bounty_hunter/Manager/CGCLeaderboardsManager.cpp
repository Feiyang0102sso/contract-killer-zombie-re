// CGCLeaderboardsManager.cpp - Game Center leaderboards of the game.
#include "CGCLeaderboardsManager.h"
#include <wchar.h>
#include "GameCenter_Facade.h"
#include "App.h"
#include "CBH_Player.h"
#include "CPlayerGameStats.h"
#include "CAchievementManager.h"

const char* CGCLeaderboardsManager::BOARD_ZOMBIES_KILLED = "ZOMBSNIPER_BOARD_ZOMBIE_KILLED";
const char* CGCLeaderboardsManager::BOARD_CIVILIANS_RESCUED = "ZOMBSNIPER_BOARD_CIVILIANS_RESCUED";
const char* CGCLeaderboardsManager::BOARD_XP = "ZOMBSNIPER_BOARD_XP";
const char* CGCLeaderboardsManager::BOARD_HEADSHOTS = "ZOMBSNIPER_BOARD_HEADSHOTS";
const char* CGCLeaderboardsManager::BOARD_MISSIONS_COMPLETED = "ZOMBSNIPER_BOARD_MISSIONS_COMPLETED";
const char* CGCLeaderboardsManager::BOARD_ACHIEVE = "ZOMBSNIPER_BOARD_ACHIEVE";
const char* CGCLeaderboardsManager::BOARD_BOARD_ACCURACY = "ZOMBSNIPER_BOARD_ACCURACY";
const char* CGCLeaderboardsManager::BOARD_SURVIVAL_MODE_SCORE = "ZOMBSNIPER_BOARD_SURVIVAL_MODE_SCORE";

CGCLeaderboardsManager::CGCLeaderboardsManager()
    : m_state(0), m_isFriendsBoard(false)
{
}

void CGCLeaderboardsManager::Update()
{
    if (GameCenter_Facade::GetInstance()->isGameCenterSupported() != true) {
        return;
    }
    if (GameCenter_Facade::GetInstance()->isAuthenticated() != true) {
        return;
    }
    if (m_state == 0) {
        GameCenter_LeaderboardFilter filters[2] = {
            { "all", BOARD_SURVIVAL_MODE_SCORE, 1, 5, 0 },
            { "friend", BOARD_SURVIVAL_MODE_SCORE, 1, 5, 1 }
        };
        GameCenter_Facade::GetInstance()->StartLoadingLeaderboards(filters, 2);
        m_state = 1;
    } else if (m_state == 1) {
        GameCenter_LeaderboardData friendData;
        GameCenter_Facade::GetInstance()->GetLocalPlayerData("friend", friendData);
        if (friendData.name.GetLength() > 0 && GameCenter_Facade::GetInstance()->GetLeaderboardSize("friend") > 1) {
            FillLeaderBoards("friend");
            m_isFriendsBoard = true;
            m_state = 2;
        } else {
            GameCenter_LeaderboardData allData;
            GameCenter_Facade::GetInstance()->GetLocalPlayerData("all", allData);
            if (allData.name.GetLength() > 0) {
                FillLeaderBoards("all");
                m_isFriendsBoard = false;
                m_state = 2;
            }
        }
    }
}

void CGCLeaderboardsManager::FillLeaderBoards(const char* board)
{
    SAFE_DELETE_VECTOR(m_leaderboards);
    GameCenter_LeaderboardData player;
    GameCenter_Facade::GetInstance()->GetLocalPlayerData(board, player);
    bool playerListed = false;
    // One row pointer for the whole function: the original reuses a single stack slot for the
    // listed rows and the appended player row (addElement takes it by reference).
    SLeaderboardData* row;
    for (int index = 0; index < GameCenter_Facade::GetInstance()->GetLeaderboardSize(board); index++) {
        GameCenter_LeaderboardData data;
        GameCenter_Facade::GetInstance()->GetLeaderboardData(board, index, data);
        row = new SLeaderboardData();
        row->m_isPlayer = false;
        row->m_name = data.name.GetWCharPtr();
        row->m_rank = data.rank;
        row->m_score = data.score;
        const wchar_t* playerName = player.name.GetWCharPtr();
        if (XString::CmpData(row->m_name.m_data, row->m_name.ByteSize(), playerName, wcslen(playerName) * sizeof(wchar_t), 0x7fffffff) == 0) {
            row->m_isPlayer = true;
            playerListed = true;
        }
        m_leaderboards.addElement(row);
    }
    if (!playerListed) {
        row = new SLeaderboardData();
        row->m_isPlayer = true;
        row->m_name = player.name.GetWCharPtr();
        row->m_rank = player.rank;
        row->m_score = player.score;
        m_leaderboards.addElement(row);
    }
}

void CGCLeaderboardsManager::ReloadLeaderboards()
{
    m_state = 0;
}

bool CGCLeaderboardsManager::UpdateLeaderboards()
{
    int zombiesKilled = CBH_Player::GetInstance()->m_gameStats.m_zombiesKilled;
    int civiliansRescued = CBH_Player::GetInstance()->m_gameStats.m_civiliansRescued;
    int xp = CBH_Player::GetInstance()->GetStats((PLAYER_STATS_TYPE)3);
    int headshots = CBH_Player::GetInstance()->m_gameStats.m_headshots;
    int missionsCompleted = CBH_Player::GetInstance()->m_gameStats.m_missionsCompleted;
    int achievementScore = ((App*)WindowApp::m_instance)->m_achievementManager->GetOverallAchievementScore();
    float hitsPercent = (float)CBH_Player::GetInstance()->m_gameStats.m_shotsHit * 100.0f;
    float shots = (float)CBH_Player::GetInstance()->m_gameStats.m_roundsFired;
    int survivalScore = CBH_Player::GetInstance()->m_gameStats.m_survivalModeScore;
    if (GameCenter_Facade::GetInstance()->isGameCenterSupported() != true) {
        return false;
    }
    if (GameCenter_Facade::GetInstance()->isAuthenticated() != true) {
        return false;
    }
    GameCenter_Facade::GetInstance()->StartPostingScore(BOARD_ZOMBIES_KILLED, zombiesKilled);
    GameCenter_Facade::GetInstance()->StartPostingScore(BOARD_CIVILIANS_RESCUED, civiliansRescued);
    GameCenter_Facade::GetInstance()->StartPostingScore(BOARD_XP, xp);
    GameCenter_Facade::GetInstance()->StartPostingScore(BOARD_HEADSHOTS, headshots);
    GameCenter_Facade::GetInstance()->StartPostingScore(BOARD_MISSIONS_COMPLETED, missionsCompleted);
    GameCenter_Facade::GetInstance()->StartPostingScore(BOARD_ACHIEVE, achievementScore);
    GameCenter_Facade::GetInstance()->StartPostingScore(BOARD_BOARD_ACCURACY, (int)(hitsPercent / shots));
    GameCenter_Facade::GetInstance()->StartPostingScore(BOARD_SURVIVAL_MODE_SCORE, survivalScore);
    return true;
}
