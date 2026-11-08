// CAchievementManager.cpp - achievements: config, progress from the player statistics, saving and
// posting the progress to Game Center.
#include "CAchievementManager.h"
#include <wchar.h>
#include "CStdUtil_Cocoa.h"
#include "GameCenter_Facade.h"
#include "App.h"
#include "ResMap.h"
#include "tinyxml.h"
#include "CXmlHelper.h"
#include "CBH_Player.h"
#include "CPlayerGameStats.h"
#include "CTutorialManager.h"
#include "CPopupController.h"
#include "CCommonGameConfig.h"

CAchievementManager::CAchievementManager()
    : m_lastCheckTime(0)
{
    ResMap* resMap = ((App*)WindowApp::m_instance)->m_resMap;
    char* text = resMap->loadText(XString(L"res/common/configs/achievements.xml"));
    TiXmlDocument document;
    document.Parse(text);
    if (text != NULL) {
        delete[] text;
    }
    ParseAchievements(document.FirstChild("achievements"));
}

void CAchievementManager::ParseAchievements(TiXmlNode* node)
{
    for (TiXmlNode* child = node->FirstChild("achievement"); child != NULL; child = child->NextSibling("achievement")) {
        CAchievement* achievement = new CAchievement();
        achievement->m_name = CXmlHelper::GetAttributeValue(child, "name");
        achievement->m_value = CXmlHelper::GetAttributeValue(child, "value").ToInt();
        achievement->m_currentValue = 0;
        achievement->m_steps = CXmlHelper::GetAttributeValue(child, "steps").ToInt();
        achievement->m_imageId = CXmlHelper::GetAttributeValue(child, "imageId");
        achievement->m_screenName = Window::ResStringSafe(CXmlHelper::GetAttributeValue(child, "screenName"));
        achievement->m_description = Window::ResStringSafe(CXmlHelper::GetAttributeValue(child, "description"));
        achievement->m_completedDescription = Window::ResStringSafe(CXmlHelper::GetAttributeValue(child, "completedDescription"));
        achievement->m_hidden = CXmlHelper::GetAttributeValue(child, "hidden").ToBool();
        achievement->m_points = CXmlHelper::GetAttributeValue(child, "points").ToInt();
        m_achievements.Set(achievement->m_name, achievement);
        m_names.addElement(achievement->m_name);
    }
}

void CAchievementManager::Save(TiXmlElement* root)
{
    TiXmlElement* list = CXmlHelper::ObtainElement(root, XString("achievements"));
    list->Clear();
    for (int index = 0; index < m_achievements.m_count; index++) {
        CAchievement* achievement = *m_achievements.GetAt(index, NULL);
        TiXmlElement* element = new TiXmlElement("achievement");
        element->SetStringAttribute("name", XString::AnsiString(achievement->m_name.c_str()));
        element->SetAttribute("progress", achievement->m_progress);
        element->SetAttribute("synced", achievement->m_synced);
        list->InsertEndChild(*element);
        delete element;
    }
}

/** Set the saved progress of the achievement `name` (ignored when it no longer exists). */
static inline void RestoreProgress(HashTable<XString, CAchievement*>& achievements, XString name, int progress, bool synced)
{
    CAchievement** found = achievements.Get(name);
    if (found != NULL) {
        (*found)->m_progress = progress;
        (*found)->m_synced = synced;
    }
}

void CAchievementManager::Load(TiXmlElement* root)
{
    TiXmlElement* list = root->FirstChildElement("achievements");
    if (list == NULL) {
        return;
    }
    TiXmlElement* element = list->FirstChildElement("achievement");
    while (element != NULL) {
        XString name = CXmlHelper::GetAttributeValue(element, "name");
        int progress = CXmlHelper::GetAttributeValue(element, "progress").ToInt();
        bool synced = CXmlHelper::GetAttributeValue(element, "synced").ToBool();
        RestoreProgress(m_achievements, name, progress, synced);
        element = element->NextSiblingElement("achievement");
    }
}

/** Check the title achievement `name`: reached when `rank` is among the ranks of the score. */
static inline void CheckTitleAchievement(CAchievementManager* manager, const XString& name, const Vector<int>& ranks, int rank)
{
    bool hasTitle = ranks.indexOf(rank) != -1;
    manager->CheckAchievement(name, hasTitle);
}

void CAchievementManager::CheckAllAchievements(bool checkScores)
{
    CPlayerGameStats& stats = CBH_Player::GetInstance()->m_gameStats;
    CBH_Player* player = CBH_Player::GetInstance();
    CheckAchievement(XString("ZOMBSNIPER_ACHIEVE_COMPLETE_A_TUTORIAL"), !((App*)WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial());
    CheckAchievement(XString("ZOMBSNIPER_ACHIEVE_KILL_THEM_ALL_01"), stats.m_zombiesKilled);
    CheckAchievement(XString("ZOMBSNIPER_ACHIEVE_KILL_THEM_ALL_02"), stats.m_zombiesKilled);
    CheckAchievement(XString("ZOMBSNIPER_ACHIEVE_KILL_THEM_ALL_03"), stats.m_zombiesKilled);
    CheckAchievement(XString("ZOMBSNIPER_ACHIEVE_KILL_THEM_ALL_04"), stats.m_zombiesKilled);
    CheckAchievement(XString("ZOMBSNIPER_ACHIEVE_KILL_THEM_ALL_05"), stats.m_zombiesKilled);
    CheckAchievement(XString("ZOMBSNIPER_ACHIEVE_KILL_ZOMBIES_WITH_SHOTGUN"), stats.m_shotgunKills);
    CheckAchievement(XString("ZOMBSNIPER_ACHIEVE_KILL_ZOMBIES_WITH_SMG"), stats.m_smgKills);
    CheckAchievement(XString("ZOMBSNIPER_ACHIEVE_KILL_ZOMBIES_WITH_REVOLVER"), stats.m_revolverKills);
    CheckAchievement(XString("ZOMBSNIPER_ACHIEVE_KILL_ZOMBIES_WITH_SNIPER"), stats.m_sniperKills);
    CheckAchievement(XString("ZOMBSNIPER_ACHIEVE_KILL_ZOMBIES_WITH_LAUNCHER"), stats.m_launcherKills);
    CheckAchievement(XString("ZOMBSNIPER_ACHIEVE_RESCUE_CIVILIANS_01"), stats.m_civiliansRescued);
    CheckAchievement(XString("ZOMBSNIPER_ACHIEVE_RESCUE_CIVILIANS_02"), stats.m_civiliansRescued);
    CheckAchievement(XString("ZOMBSNIPER_ACHIEVE_RESCUE_CIVILIANS_03"), stats.m_civiliansRescued);
    CheckAchievement(XString("ZOMBSNIPER_ACHIEVE_GET_PROVISIONS_01"), stats.m_provisionsCollected);
    CheckAchievement(XString("ZOMBSNIPER_ACHIEVE_GET_PROVISIONS_02"), stats.m_provisionsCollected);
    CheckAchievement(XString("ZOMBSNIPER_ACHIEVE_GET_PROVISIONS_03"), stats.m_provisionsCollected);
    int helicoptersSent = stats.m_helicoptersSent[0] + stats.m_helicoptersSent[1] + stats.m_helicoptersSent[2];
    CheckAchievement(XString("ZOMBSNIPER_ACHIEVE_SEND_HELICOPTERS_01"), helicoptersSent);
    CheckAchievement(XString("ZOMBSNIPER_ACHIEVE_SEND_HELICOPTERS_02"), helicoptersSent);
    CheckAchievement(XString("ZOMBSNIPER_ACHIEVE_SEND_HELICOPTERS_03"), helicoptersSent);
    CheckAchievement(XString("ZOMBSNIPER_ACHIEVE_REACH_LEVEL_5"), player->m_id);
    CheckAchievement(XString("ZOMBSNIPER_ACHIEVE_REACH_LEVEL_10"), player->m_id);
    CheckAchievement(XString("ZOMBSNIPER_ACHIEVE_REACH_LEVEL_15"), player->m_id);
    CheckAchievement(XString("ZOMBSNIPER_ACHIEVE_REACH_LEVEL_20"), player->m_id);
    CheckAchievement(XString("ZOMBSNIPER_ACHIEVE_REACH_LEVEL_25"), player->m_id);
    CheckAchievement(XString("ZOMBSNIPER_ACHIEVE_GET_HEADSHOTS_01"), stats.m_headshots);
    CheckAchievement(XString("ZOMBSNIPER_ACHIEVE_GET_HEADSHOTS_02"), stats.m_headshots);
    CheckAchievement(XString("ZOMBSNIPER_ACHIEVE_GET_HEADSHOTS_03"), stats.m_headshots);
    CheckAchievement(XString("ZOMBSNIPER_ACHIEVE_GET_HEADSHOTS_04"), stats.m_headshots);
    CheckAchievement(XString("ZOMBSNIPER_ACHIEVE_KILL_BARRELS_01"), stats.m_barrelsDestroyed);
    CheckAchievement(XString("ZOMBSNIPER_ACHIEVE_KILL_BARRELS_02"), stats.m_barrelsDestroyed);
    CheckAchievement(XString("ZOMBSNIPER_ACHIEVE_KILL_BARRELS_03"), stats.m_barrelsDestroyed);
    CheckAchievement(XString("ZOMBSNIPER_ACHIEVE_KILL_BARRELS_04"), stats.m_barrelsDestroyed);
    CheckAchievement(XString("ZOMBSNIPER_ACHIEVE_FIRE_ROUNDS_001"), stats.m_roundsFired);
    CheckAchievement(XString("ZOMBSNIPER_ACHIEVE_FIRE_ROUNDS_002"), stats.m_roundsFired);
    CheckAchievement(XString("ZOMBSNIPER_ACHIEVE_FIRE_ROUNDS_003"), stats.m_roundsFired);
    CheckAchievement(XString("ZOMBSNIPER_ACHIEVE_GET_A_HEADSHOT"), stats.m_headshotAchieved);
    CheckAchievement(XString("ZOMBSNIPER_ACHIEVE_HEALING_GUNS_01"), stats.m_healingGunUses);
    CheckAchievement(XString("ZOMBSNIPER_ACHIEVE_HEALING_GUNS_02"), stats.m_healingGunUses);
    CheckAchievement(XString("ZOMBSNIPER_ACHIEVE_USE_HEALING_GUNS_03"), stats.m_healingGunUses);
    CheckAchievement(XString("ZOMBSNIPER_ACHIEVE_USE_HEALING_GUNS_04"), stats.m_healingGunUses);
    CheckAchievement(XString("ZOMBSNIPER_ACHIEVE_UNLOCK_DAILY_BONUSES_IN_A_ROW"), player->GetDailyBonusesInRow());
    CheckAchievement(XString("ZOMBSNIPER_ACHIEVE_BUY_5_WEAPONS"), stats.m_weaponsBought);
    CheckAchievement(XString("ZOMBSNIPER_ACHIEVE_BUY_12_WEAPONS"), stats.m_weaponsBought);
    CheckAchievement(XString("ZOMBSNIPER_ACHIEVE_BUY_20_WEAPONS"), stats.m_weaponsBought);
    CheckAchievement(XString("ZOMBSNIPER_ACHIEVE_HEADSHOTS_WITH_A_HANDGUN"), stats.m_handgunHeadshots);
    CheckAchievement(XString("ZOMBSNIPER_ACHIEVE_COMPLETE_MISSIONS_01"), stats.m_missionsCompleted);
    CheckAchievement(XString("ZOMBSNIPER_ACHIEVE_COMPLETE_MISSIONS_02"), stats.m_missionsCompleted);
    CheckAchievement(XString("ZOMBSNIPER_ACHIEVE_COMPLETE_MISSIONS_03"), stats.m_missionsCompleted);
    CheckAchievement(XString("ZOMBSNIPER_ACHIEVE_COMPLETE_MISSIONS_04"), stats.m_missionsCompleted);
    CheckAchievement(XString("ZOMBSNIPER_ACHIEVE_EM_WAVES_01"), stats.m_emWavesSurvived);
    CheckAchievement(XString("ZOMBSNIPER_ACHIEVE_EM_WAVES_02"), stats.m_emWavesSurvived);
    CheckAchievement(XString("ZOMBSNIPER_ACHIEVE_EM_WAVES_03"), stats.m_emWavesSurvived);
    CheckAchievement(XString("ZOMBSNIPER_ACHIEVE_EM_PROTECT_EVELYN_01"), stats.m_emEvelynProtected - 1);
    CheckAchievement(XString("ZOMBSNIPER_ACHIEVE_EM_PROTECT_EVELYN_02"), stats.m_emEvelynProtected - 1);
    CheckAchievement(XString("ZOMBSNIPER_ACHIEVE_EM_GOAL"), stats.m_emGoalReached);
    if (checkScores == true) {
        {
            Vector<int> ranks;
            ranks.ensureCapacity(6);
            ((App*)WindowApp::m_instance)->m_commonGameConfig->GetAllRankAchievementsForScore(stats.GetThisCycleScore(), ranks);
            CheckTitleAchievement(this, XString("ZOMBSNIPER_ACHIEVE_GET_A_HUNTER_TITLE"), ranks, 1);
            CheckTitleAchievement(this, XString("ZOMBSNIPER_ACHIEVE_GET_A_KILLER_TITLE"), ranks, 2);
            CheckTitleAchievement(this, XString("ZOMBSNIPER_ACHIEVE_GET_A_SMASHER_TITLE"), ranks, 3);
            CheckTitleAchievement(this, XString("ZOMBSNIPER_ACHIEVE_GET_A_SLAYER_TITLE"), ranks, 4);
            CheckTitleAchievement(this, XString("ZOMBSNIPER_ACHIEVE_GET_A_EXECUTIONEER_TITLE"), ranks, 5);
        }
        CheckAchievement(XString("ZOMBSNIPER_ACHIEVE_SCORE_TOTAL_01"), stats.m_totalScore);
        CheckAchievement(XString("ZOMBSNIPER_ACHIEVE_SCORE_TOTAL_02"), stats.m_totalScore);
        CheckAchievement(XString("ZOMBSNIPER_ACHIEVE_SCORE_TOTAL_03"), stats.m_totalScore);
    }
}

void CAchievementManager::CheckAchievement(XString name, int value)
{
    if (value < 1) {
        return;
    }
    CAchievement** found = m_achievements.Get(name);
    if (found == NULL) {
        return;
    }
    CAchievement* achievement = *found;
    achievement->m_currentValue = value;
    int progress = achievement->m_progress;
    int steps = achievement->m_steps;
    if (progress < steps) {
        int target = achievement->m_value;
        int stepValue = target / steps;
        if (stepValue < 1) {
            return;
        }
        // the last step needs the full value (no rounding loss)
        int nextValue = stepValue * (progress + 1);
        if (progress + 1 == steps) {
            nextValue = target;
        }
        if (nextValue <= value) {
            int reached = value / stepValue;
            if (reached < 0) {
                reached = 0;
            } else {
                // `?:`: the original limits with a conditional move before the copy
                reached = (reached > steps) ? steps : reached;
            }
            if (reached <= progress) {
                return;
            }
            GiveAchievement(name, reached);
        }
    }
    if (achievement->m_synced) {
        return;
    }
    if (GameCenter_Facade::GetInstance()->isGameCenterSupported() == true) {
        float percent = (float)(achievement->m_progress * 100) / (float)achievement->m_steps;
        GameCenter_Facade::GetInstance()->StartPostingAchievement(XString::AnsiString(achievement->m_name.c_str()), percent);
        achievement->m_synced = true;
    }
}

bool CAchievementManager::GiveAchievement(XString name, int progress)
{
    CAchievement** found = m_achievements.Get(name);
    if (found == NULL) {
        return false;
    }
    CAchievement* achievement = *found;
    if (achievement->m_progress >= progress) {
        return false;
    }
    if (progress > achievement->m_steps) {
        return false;
    }
    achievement->m_progress = progress;
    achievement->m_synced = false;
    if (achievement->m_steps == progress) {
        ((App*)WindowApp::m_instance)->m_popupController->AddAchievementPopup(achievement);
    }
    CBH_Player::GetInstance()->Save(true);
    return true;
}

void CAchievementManager::Update(bool force)
{
    if (((App*)WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial() == true
        && ((App*)WindowApp::m_instance)->m_tutorialManager->IsAchievementsForbidden()) {
        return;
    }
    // After a long pause (app in the background) only restart the timer.
    if (m_lastCheckTime != 0 && CStdUtil_Cocoa::GetUpTimeMS() - m_lastCheckTime > 4000) {
        m_lastCheckTime = CStdUtil_Cocoa::GetUpTimeMS();
        return;
    }
    // `?:`: the original selects the interval with a conditional move
    unsigned int interval = force ? 2000 : 1000;
    if (CStdUtil_Cocoa::GetUpTimeMS() - m_lastCheckTime <= interval) {
        return;
    }
    m_lastCheckTime = CStdUtil_Cocoa::GetUpTimeMS();
    CheckAllAchievements(false);
    if (force) {
        return;
    }
    if (GameCenter_Facade::GetInstance()->isGameCenterSupported() == true) {
        GameCenter_Facade::GetInstance()->FetchPosting();
    }
}

int CAchievementManager::GetOverallAchievementScore()
{
    int score = 0;
    for (int index = 0; index < m_names.size(); index++) {
        CAchievement** found = m_achievements.Get(m_names[index]);
        if (found == NULL) {
            return 0;
        }
        score += (*found)->m_points;
    }
    return score;
}

void CAchievementManager::CheckScoreAchievements()
{
    CheckAllAchievements(true);
}

CAchievementManager::~CAchievementManager()
{
    m_achievements.DeleteContents();
}
