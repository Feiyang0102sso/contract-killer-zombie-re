// CPlayerGameStats.cpp: original u525 full owner of statistics, skill checks and persistence.
#include "CPlayerGameStats.h"
#include "App.h"
#include "WindowApp.h"
#include "DGHelper.h"
#include "CSwerveGame.h"
#include "CBH_Player.h"
#include "CDH_Weapon.h"
#include "CDH_SoundManager.h"
#include "CCommonGameConfig.h"
#include "CGameAnalytics.h"
#include "CMission.h"
#include "CXmlHelper.h"
#include "CStdUtil_Cocoa.h"
#include <string.h>

const int CPlayerGameStats::PER_ZOMBIE_KILL_SCORE = 5;
const int CPlayerGameStats::PER_SKILLSHOT_SCORE = 10;
const int CPlayerGameStats::PER_HEADSHOT_SCORE = 10;
const int CPlayerGameStats::PER_BOX_SCORE = 10;
const int CPlayerGameStats::PER_CIVILIAN_SCORE = 50;
// The original symbol's automatic span of 832B also includes the following independent wide strings; the constant itself is only 4B.
const int CPlayerGameStats::PER_HELI_MISSION_SCORE = 100;

void CPlayerGameStats::Skillshot::OnShotEnd()
{
    if (CheckSkillshot(m_weapon, m_grenade, m_shotCount, m_kills,
                      m_hits, m_headshots, m_goreshots) == true) {
        ((App*)WindowApp::m_instance)->m_helper->m_swerveGame->OnSkillshot(m_score);
        ((App*)WindowApp::m_instance)->m_soundManager->Play((CDH_SoundManager::eSound)24, false, false);
        CPlayerGameStats& stats = CBH_Player::GetInstance()->m_gameStats;
        ++stats.m_skillshotsDone;
        ++stats.m_skillshotsThisCycle;
        ++stats.m_currentMissionSkillshots;
    }
    m_weapon = NULL;
    m_shotCount = 0;
    m_kills = 0;
    m_hits = 0;
    m_headshots = 0;
    m_goreshots = 0;
    // Original 144dd8..144de4 does not clear grenade.
}

bool CPlayerGameStats::QuantityShot::CheckSkillshot(const CDH_Weapon*, const CGrenade*,
    int, int kills, int, int, int)
{
    int now = WindowApp::m_instance->m_updateTime;
    while (m_times.Size() > 0 && (int)((unsigned int)now - (unsigned int)m_times[0]) > m_windowMS) {
        m_times.RemoveRange(0, 1);
    }
    for (int index = 0; index < kills; ++index) {
        m_times.Add(now);
    }
    if (m_times.Size() < m_threshold) {
        return false;
    }
    XString text;
    CDH_Weapon* active = ((App*)WindowApp::m_instance)->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon();
    switch (m_command) {
    case 0xfab5e72d: text = L"Splash Kill!"; break;
    case 0xfab5e72c: text = L"Multi Kill!"; break;
    case 0xfab5e72f: text = L"Unstoppable!"; break;
    case 0xfab5e72e: text = L"Rage!"; break;
    }
    CGameAnalytics::logSkillshot(text, active->m_id, true);
    WindowApp::HandleTunnelCommand(m_command, 0, 0, 0);
    for (int index = 0; index < m_threshold; ++index) {
        m_times.RemoveRange(0, 1);
    }
    return true;
}

bool CPlayerGameStats::Headbanger::CheckSkillshot(const CDH_Weapon* weapon, const CGrenade*,
    int, int, int, int headshots, int)
{
    if (weapon == NULL) {
        return false;
    }
    if (headshots != 0) {
        m_count += headshots;
        if (m_count < m_threshold) {
            return false;
        }
        CDH_Weapon* active = ((App*)WindowApp::m_instance)->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon();
        CGameAnalytics::logSkillshot(XString(L"Headbanger!"), active->m_id, true);
        WindowApp::HandleTunnelCommand(m_command, 0, 0, 0);
        m_count -= m_threshold;
        return true;
    } else {
        m_count = 0;
    }
    return false;
}

bool CPlayerGameStats::Brainiac::CheckSkillshot(const CDH_Weapon* weapon, const CGrenade*,
    int, int, int, int headshots, int)
{
    if (weapon == NULL || headshots == 0) {
        return false;
    }
    m_count += headshots;
    if (m_count < m_threshold) {
        return false;
    }
    CDH_Weapon* active = ((App*)WindowApp::m_instance)->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon();
    CGameAnalytics::logSkillshot(XString(L"Brainiac!"), active->m_id, true);
    WindowApp::HandleTunnelCommand(m_command, 0, 0, 0);
    m_count -= m_threshold;
    return true;
}

bool CPlayerGameStats::IronHand::CheckSkillshot(const CDH_Weapon* weapon, const CGrenade*,
    int, int, int hits, int headshots, int goreshots)
{
    if (weapon == NULL) {
        return false;
    }
    if ((int)((unsigned int)hits + (unsigned int)headshots + (unsigned int)goreshots) >= 1) {
        ++m_count;
        if (m_count < m_threshold) {
            return false;
        }
        CDH_Weapon* active = ((App*)WindowApp::m_instance)->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon();
        CGameAnalytics::logSkillshot(XString(L"Iron Hands!"), active->m_id, true);
        WindowApp::HandleTunnelCommand(m_command, 0, 0, 0);
        m_count -= m_threshold;
        return true;
    } else {
        m_count = 0;
    }
    return false;
}

bool CPlayerGameStats::SevenBlows::CheckSkillshot(const CDH_Weapon* weapon, const CGrenade* grenade,
    int, int, int, int, int goreshots)
{
    if (weapon != NULL || grenade != NULL) {
        return false;
    }
    int now = WindowApp::m_instance->m_updateTime;
    while (m_times.Size() > 0 && (int)((unsigned int)now - (unsigned int)m_times[0]) > m_windowMS) {
        m_times.RemoveRange(0, 1);
    }
    for (int index = 0; index < goreshots; ++index) {
        m_times.Add(now);
    }
    if (m_times.Size() < m_threshold) {
        return false;
    }
    CDH_Weapon* active = ((App*)WindowApp::m_instance)->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon();
    CGameAnalytics::logSkillshot(XString(L"Ka-Boom!"), active->m_id, true);
    WindowApp::HandleTunnelCommand(m_command, 0, 0, 0);
    for (int index = 0; index < m_threshold; ++index) {
        m_times.RemoveRange(0, 1);
    }
    return true;
}

void CPlayerGameStats::Reset(bool keepProgress)
{
    m_currentProvision = 0;
    m_currentPeople = 0;
    m_currentPeopleSent = 0;
    m_currentBoxesSent = 0;
    m_currentMoney = 0;
    m_currentXP = 0;
    m_totalScore = 0;
    m_survivalModeScore = 0;
    m_emWavesSurvived = 0;
    m_emEvelynProtected = 0;
    m_emGoalReached = 0;
    m_healingGunUses = 0;
    if (keepProgress == true) {
        memset(&m_cycleNumber, 0, 0x30);
    } else {
        m_missionsCompleted = 0;
        m_totalMissions = 0;
        memset(&m_cycleNumber, 0, 0x30);
        m_weaponsBought = 0;
    }
    m_field54 = 0;
    memset(&m_shotgunKills, 0, 0x60);
    m_shotWeaponType = 11;
    m_throws = 0;
    m_throwKills = 0;
    memset(&m_shotCount, 0, 0x24);
}

CPlayerGameStats::CPlayerGameStats()
{
    Reset(false);
    m_appStartSeconds = CStdUtil_Cocoa::GetTimeSeconds();
    m_missionsSinceAppStart = 0;
}

CPlayerGameStats::~CPlayerGameStats()
{
    // After the delete callback the original rereads the container data/size; NULL elements that were not deleted are kept.
    for (int index = 0; index < m_skillshots.Size(); ++index) {
        delete m_skillshots[index];
    }
    if (m_skillshots.m_data != NULL) {
        m_skillshots.m_size = 0;
    }
}

void CPlayerGameStats::Save(TiXmlElement* parent)
{
    TiXmlElement* element = CXmlHelper::ObtainElement(parent, XString("game_stats"));
    element->Clear();
    element->SetAttribute("m_completedMissions", m_missionsCompleted);
    element->SetAttribute("m_totalMissions", m_totalMissions);
    element->SetAttribute("m_totalCycles", m_cycleNumber);
    element->SetAttribute("m_zombiesKilled", m_zombiesKilled);
    element->SetAttribute("m_zombiesKilledByEvelyn", m_field28);
    element->SetAttribute("m_civiliansResqued", m_civiliansRescued);
    element->SetAttribute("m_civiliansEvacuated", m_field30);
    element->SetAttribute("m_shotsNumber", m_roundsFired);
    element->SetAttribute("m_hitsNumber", m_shotsHit);
    element->SetAttribute("m_headshotsNumber", m_headshots);
    element->SetAttribute("m_runningHeadshotsNumber", m_headshotAchieved);
    element->SetAttribute("m_civilianHeadshotsNumber", m_field4c);
    element->SetAttribute("m_goreshotsNumber", m_field40);
    element->SetAttribute("m_barrelBlowsNumber", m_barrelsDestroyed);
    element->SetAttribute("m_weaponsEarned", m_weaponsBought);
    element->SetAttribute("m_weaponSwapped", m_field54);
    element->SetAttribute("m_numHealingGunShots", m_healingGunUses);
    element->SetAttribute("m_numShotgunKills", m_shotgunKills);
    element->SetAttribute("m_numSMGKills", m_smgKills);
    element->SetAttribute("m_numRifleKills", m_sniperKills);
    element->SetAttribute("m_numRevolverKills", m_revolverKills);
    element->SetAttribute("m_numLauncherKills", m_launcherKills);
    element->SetAttribute("m_numRevolverHeadshots", m_handgunHeadshots);
    element->SetAttribute("m_numProvisionScrounged", m_provisionsCollected);
    element->SetAttribute("m_zombiesKilledThisCycle", m_zombiesKilledThisCycle);
    element->SetAttribute("m_skillshotsDone", m_skillshotsDone);
    element->SetAttribute("m_boxesSent", m_boxesSent);
    element->SetAttribute("m_peopleSent", m_peopleSent);
    element->SetAttribute("m_skillshotsDoneThisCycle", m_skillshotsThisCycle);
    element->SetAttribute("m_headshotsNumberThisCycle", m_headshotsThisCycle);
    element->SetAttribute("m_carryBoxMissionsThisCycle", m_carryBoxMissionsThisCycle);
    element->SetAttribute("m_peopleMissionsThisCycle", m_peopleMissionsThisCycle);
    element->SetAttribute("m_heliMissionsThisCycle", m_helicopterMissionsThisCycle);
    element->SetAttribute("m_boxesSentThisCycle", m_boxesSentThisCycle);
    element->SetAttribute("m_peopleSentThisCycle", m_peopleSentThisCycle);
    element->SetAttribute("m_numEasyCopterMissions", m_helicoptersSent[0]);
    element->SetAttribute("m_numMediumCopterMissions", m_helicoptersSent[1]);
    element->SetAttribute("m_numHardCopterMissions", m_helicoptersSent[2]);
    element->SetAttribute("m_totalScore", m_totalScore);
    element->SetAttribute("m_throwsNumber", m_throws);
    element->SetAttribute("m_throwKillsNumber", m_throwKills);
    element->SetAttribute("m_bestArcadeScore", m_survivalModeScore);
    element->SetAttribute("m_survivalWave", m_emWavesSurvived);
    element->SetAttribute("m_survivalWaveForEvelynTakesNoDamage", m_emEvelynProtected);
    element->SetAttribute("m_survivalGoalsWithFootballBomb", m_emGoalReached);
}

void CPlayerGameStats::Load(TiXmlElement* parent)
{
    TiXmlElement* element = parent->FirstChildElement("game_stats");
    if (element == NULL) {
        return;
    }
    m_missionsCompleted = CXmlHelper::GetAttributeValue(element, "m_completedMissions").ToInt();
    m_totalMissions = CXmlHelper::GetAttributeValue(element, "m_totalMissions").ToInt();
    m_cycleNumber = CXmlHelper::GetAttributeValue(element, "m_totalCycles").ToInt();
    m_zombiesKilled = CXmlHelper::GetAttributeValue(element, "m_zombiesKilled").ToInt();
    m_field28 = CXmlHelper::GetAttributeValue(element, "m_zombiesKilledByEvelyn").ToInt();
    m_civiliansRescued = CXmlHelper::GetAttributeValue(element, "m_civiliansResqued").ToInt();
    m_field30 = CXmlHelper::GetAttributeValue(element, "m_civiliansEvacuated").ToInt();
    m_roundsFired = CXmlHelper::GetAttributeValue(element, "m_shotsNumber").ToInt();
    m_shotsHit = CXmlHelper::GetAttributeValue(element, "m_hitsNumber").ToInt();
    m_headshots = CXmlHelper::GetAttributeValue(element, "m_headshotsNumber").ToInt();
    m_headshotAchieved = CXmlHelper::GetAttributeValue(element, "m_runningHeadshotsNumber").ToInt();
    m_field4c = CXmlHelper::GetAttributeValue(element, "m_civilianHeadshotsNumber").ToInt();
    m_field40 = CXmlHelper::GetAttributeValue(element, "m_goreshotsNumber").ToInt();
    m_barrelsDestroyed = CXmlHelper::GetAttributeValue(element, "m_barrelBlowsNumber").ToInt();
    m_weaponsBought = CXmlHelper::GetAttributeValue(element, "m_weaponsEarned").ToInt();
    m_field54 = CXmlHelper::GetAttributeValue(element, "m_weaponSwapped").ToInt();
    m_healingGunUses = CXmlHelper::GetAttributeValueOrUseDefault(element, "m_numHealingGunShots", "0").ToInt();
    m_shotgunKills = CXmlHelper::GetAttributeValue(element, "m_numShotgunKills").ToInt();
    m_smgKills = CXmlHelper::GetAttributeValue(element, "m_numSMGKills").ToInt();
    m_sniperKills = CXmlHelper::GetAttributeValue(element, "m_numRifleKills").ToInt();
    m_revolverKills = CXmlHelper::GetAttributeValue(element, "m_numRevolverKills").ToInt();
    m_launcherKills = CXmlHelper::GetAttributeValue(element, "m_numLauncherKills").ToInt();
    m_handgunHeadshots = CXmlHelper::GetAttributeValueOrUseDefault(element, "m_numRevolverHeadshots", "0").ToInt();
    m_provisionsCollected = CXmlHelper::GetAttributeValue(element, "m_numProvisionScrounged").ToInt();
    m_zombiesKilledThisCycle = CXmlHelper::GetAttributeValue(element, "m_zombiesKilledThisCycle").ToInt();
    m_boxesSent = CXmlHelper::GetAttributeValue(element, "m_boxesSent").ToInt();
    m_peopleSent = CXmlHelper::GetAttributeValue(element, "m_peopleSent").ToInt();
    m_skillshotsDone = CXmlHelper::GetAttributeValue(element, "m_skillshotsDone").ToInt();
    m_skillshotsThisCycle = CXmlHelper::GetAttributeValue(element, "m_skillshotsDoneThisCycle").ToInt();
    m_headshotsThisCycle = CXmlHelper::GetAttributeValue(element, "m_headshotsNumberThisCycle").ToInt();
    m_carryBoxMissionsThisCycle = CXmlHelper::GetAttributeValue(element, "m_carryBoxMissionsThisCycle").ToInt();
    m_peopleMissionsThisCycle = CXmlHelper::GetAttributeValue(element, "m_peopleMissionsThisCycle").ToInt();
    m_helicopterMissionsThisCycle = CXmlHelper::GetAttributeValue(element, "m_heliMissionsThisCycle").ToInt();
    m_boxesSentThisCycle = CXmlHelper::GetAttributeValue(element, "m_boxesSentThisCycle").ToInt();
    m_peopleSentThisCycle = CXmlHelper::GetAttributeValue(element, "m_peopleSentThisCycle").ToInt();
    m_helicoptersSent[0] = CXmlHelper::GetAttributeValue(element, "m_numEasyCopterMissions").ToInt();
    m_helicoptersSent[1] = CXmlHelper::GetAttributeValue(element, "m_numMediumCopterMissions").ToInt();
    m_helicoptersSent[2] = CXmlHelper::GetAttributeValue(element, "m_numHardCopterMissions").ToInt();
    m_totalScore = CXmlHelper::GetAttributeValue(element, "m_totalScore").ToInt();
    if (element->Attribute("m_throwsNumber") != NULL) {
        m_throws = CXmlHelper::GetAttributeValue(element, "m_throwsNumber").ToInt();
    }
    if (element->Attribute("m_throwKillsNumber") != NULL) {
        m_throwKills = CXmlHelper::GetAttributeValue(element, "m_throwKillsNumber").ToInt();
    }
    if (element->Attribute("m_bestArcadeScore") != NULL) {
        m_survivalModeScore = CXmlHelper::GetAttributeValue(element, "m_bestArcadeScore").ToInt();
    }
    if (element->Attribute("m_survivalWave") != NULL) {
        m_emWavesSurvived = CXmlHelper::GetAttributeValue(element, "m_survivalWave").ToInt();
    }
    if (element->Attribute("m_survivalWaveForEvelynTakesNoDamage") != NULL) {
        m_emEvelynProtected = CXmlHelper::GetAttributeValue(element, "m_survivalWaveForEvelynTakesNoDamage").ToInt();
    }
    if (element->Attribute("m_survivalGoalsWithFootballBomb") != NULL) {
        m_emGoalReached = CXmlHelper::GetAttributeValue(element, "m_survivalGoalsWithFootballBomb").ToInt();
    }
}
void CPlayerGameStats::AddCompletedMissions(int count) { m_missionsCompleted += count; }
void CPlayerGameStats::AddTotalMissions(int count) { m_totalMissions += count; }
void CPlayerGameStats::AddZombiesKilledByEvelyn(int count) { m_field28 += count; }
void CPlayerGameStats::AddBarrelBlow(int count) { m_barrelsDestroyed += count; }
void CPlayerGameStats::AddCiviliansResqued(int count) { m_civiliansRescued += count; }
void CPlayerGameStats::AddWeaponsEarned(int count) { m_weaponsBought += count; }
void CPlayerGameStats::AddProvisionScrounged(int count) { m_provisionsCollected += count; }
void CPlayerGameStats::AddNumHealingGunShots(int count) { m_healingGunUses += count; }

void CPlayerGameStats::AddBoxesSent(int count)
{
    m_boxesSent += count;
    m_boxesSentThisCycle += count;
}

void CPlayerGameStats::AddPeopleSent(int count)
{
    m_peopleSent += count;
    m_peopleSentThisCycle += count;
    CGameAnalytics::logHelicopterMissionCompleted(count, m_currentMissionCiviliansKilled);
}

void CPlayerGameStats::AddEasyCopterMissions(int count) { m_helicoptersSent[0] += count; }
void CPlayerGameStats::AddMediumCopterMissions(int count) { m_helicoptersSent[1] += count; }
void CPlayerGameStats::AddHardCopterMissions(int count) { m_helicoptersSent[2] += count; }

void CPlayerGameStats::NotifyNewCycleBegin()
{
    ++m_cycleNumber;
    m_totalScore += m_peopleSentThisCycle * PER_CIVILIAN_SCORE
        + (m_headshotsThisCycle + m_boxesSentThisCycle + m_skillshotsThisCycle) * 10
        + m_zombiesKilledThisCycle * PER_ZOMBIE_KILL_SCORE;
    memset(&m_zombiesKilledThisCycle, 0, 0x20);
}

int CPlayerGameStats::GetThisCycleScore()
{
    return m_zombiesKilledThisCycle * PER_ZOMBIE_KILL_SCORE
        + m_peopleSentThisCycle * PER_CIVILIAN_SCORE
        + (m_headshotsThisCycle + m_boxesSentThisCycle + m_skillshotsThisCycle) * 10;
}

void CPlayerGameStats::NotifyMissionWin(const CMission* mission)
{
    switch (mission->m_type) {
    case 2: ++m_peopleMissionsThisCycle; break;
    case 3: ++m_carryBoxMissionsThisCycle; break;
    case 4: ++m_helicopterMissionsThisCycle; break;
    }
    ++m_missionsSinceAppStart;
    ++m_missionsSinceForeground;
}

void CPlayerGameStats::NotifyMissionBegin(const CMission*)
{
    m_currentMissionCiviliansKilled = 0;
    m_currentMissionKills = 0;
    m_currentMissionSkillshots = 0;
    m_currentMissionHeadshots = 0;
    for (int index = 0; index < m_skillshots.Size(); ++index) {
        delete m_skillshots[index];
    }
    if (m_skillshots.m_data != NULL) {
        m_skillshots.m_size = 0;
    }
    // Original 145fa0..145fc8 caches all eight config values before the first new.
    CCommonGameConfig* config = ((App*)WindowApp::m_instance)->m_commonGameConfig;
    int splash = config->m_multikillScore;
    int multi = config->m_megakillScore;
    int unstoppable = config->m_unstoppableScore;
    int rage = config->m_dominatingScore;
    int headbanger = config->m_multiheadshotScore;
    int brainiac = config->m_decapitatorScore;
    int ironHand = config->m_deadlyPrecisionScore;
    int sevenBlows = config->m_kaboomScore;
    m_skillshots.Add(new QuantityShot(3, 2000, 0xfab5e72d, splash));
    m_skillshots.Add(new QuantityShot(5, 3000, 0xfab5e72c, multi));
    m_skillshots.Add(new QuantityShot(6, 4000, 0xfab5e72f, unstoppable));
    m_skillshots.Add(new QuantityShot(15, 10000, 0xfab5e72e, rage));
    m_skillshots.Add(new Headbanger(3, 0xfab5e729, headbanger));
    m_skillshots.Add(new Brainiac(10, 0xfab5e72b, brainiac));
    m_skillshots.Add(new IronHand(10, 0xfab5e728, ironHand));
    m_skillshots.Add(new SevenBlows(7, 1000, 0xfab5e72a, sevenBlows));
}

void CPlayerGameStats::NotifyShotBegin(const CDH_Weapon* weapon, const CGrenade* grenade, unsigned int count)
{
    for (int index = 0; index < m_skillshots.Size(); ++index) {
        Skillshot* skillshot = m_skillshots[index];
        skillshot->m_weapon = weapon;
        skillshot->m_grenade = grenade;
        skillshot->m_shotCount = count;
        skillshot->m_kills = 0;
        skillshot->m_hits = 0;
        skillshot->m_headshots = 0;
        skillshot->m_goreshots = 0;
    }
    m_shotWeaponType = 11;
    if (weapon != NULL) {
        m_shotWeaponType = weapon->m_weaponType;
        if (weapon->m_projectileType == 1) {
            m_shotWeaponType = 6;
        }
    } else {
        m_shotWeaponType = 5;
    }
    if (grenade != NULL) {
        m_shotWeaponType = 10;
    }
    m_shotCount = count;
    m_shotHits = 0;
    m_shotHeadshots = 0;
    m_shotGoreshots = 0;
}

void CPlayerGameStats::NotifyShotEnd()
{
    for (int index = 0; index < m_skillshots.Size(); ++index) {
        m_skillshots[index]->OnShotEnd();
    }
    if ((unsigned int)m_shotWeaponType - 10u >= 2) {
        ++m_roundsFired;
        m_shotsHit += m_shotHits > 0;
        int headshot = m_shotHeadshots > 0;
        m_headshots += headshot;
        m_currentMissionHeadshots += headshot;
        m_headshotsThisCycle += headshot;
        m_field40 += m_shotGoreshots > 0;
    }
    m_shotCount = 0;
}

void CPlayerGameStats::NotifyHit()
{
    for (int index = 0; index < m_skillshots.Size(); ++index) {
        ++m_skillshots[index]->m_hits;
    }
    ++m_shotHits;
}

void CPlayerGameStats::NotifyHeadshot()
{
    for (int index = 0; index < m_skillshots.Size(); ++index) {
        ++m_skillshots[index]->m_headshots;
    }
    ++m_shotHeadshots;
    if (m_shotWeaponType == 0) {
        ++m_handgunHeadshots;
    }
}

void CPlayerGameStats::NotifyGoreshot()
{
    for (int index = 0; index < m_skillshots.Size(); ++index) {
        ++m_skillshots[index]->m_goreshots;
    }
    if (m_shotCount != 0) {
        ++m_shotGoreshots;
    }
}

void CPlayerGameStats::NotifyKill()
{
    for (int index = 0; index < m_skillshots.Size(); ++index) {
        ++m_skillshots[index]->m_kills;
    }
    ++m_zombiesKilled;
    ++m_zombiesKilledThisCycle;
    ++m_currentMissionKills;
    switch (m_shotWeaponType) {
    case 10: ++m_throwKills; break;
    case 0: ++m_revolverKills; break;
    case 1:
    case 7: ++m_shotgunKills; break;
    case 2:
    case 8:
    case 9: ++m_smgKills; break;
    case 3: ++m_sniperKills; break;
    case 6: ++m_launcherKills; break;
    }
}

void CPlayerGameStats::NotifyRunningHeadshot() { ++m_headshotAchieved; }
void CPlayerGameStats::NotifyCivilianHeadshot() { ++m_field4c; }
void CPlayerGameStats::NotifyCivilianKilled() { ++m_currentMissionCiviliansKilled; }
void CPlayerGameStats::NotifyAppForeground()
{
    m_foregroundSeconds = CStdUtil_Cocoa::GetTimeSeconds();
    m_missionsSinceForeground = 0;
}
void CPlayerGameStats::NotifyThrow() { ++m_throws; }
void CPlayerGameStats::NotifyArcadeScoreChanged(int score)
{
    if (m_survivalModeScore < score) {
        m_survivalModeScore = score;
    }
}
void CPlayerGameStats::NotifySurvivalWave(int wave)
{
    int maximum = m_emWavesSurvived;
    if (maximum < wave) {
        maximum = wave;
    }
    m_emWavesSurvived = maximum;
}
void CPlayerGameStats::NotifySurvivalWaveForEvelynTakesNoDamage(int wave)
{
    int maximum = m_emEvelynProtected;
    if (maximum < wave) {
        maximum = wave;
    }
    m_emEvelynProtected = maximum;
}
void CPlayerGameStats::NotifySurvivalGoal() { ++m_emGoalReached; }
int CPlayerGameStats::GetTimeSinceAppStart()
{
    return CStdUtil_Cocoa::GetTimeSeconds() - m_appStartSeconds;
}
int CPlayerGameStats::GetTimeSinceAppForeground()
{
    return CStdUtil_Cocoa::GetTimeSeconds() - m_foregroundSeconds;
}
