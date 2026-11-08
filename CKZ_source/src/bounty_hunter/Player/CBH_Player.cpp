// b081j: original 142d5c..144d58 full Player; keeps the original save, authentication and numeric boundaries.
#include "CBH_Player.h"
#include "App.h"
#include "DGHelper.h"
#include "CProgressSystem.h"
#include "CXmlHelper.h"
#include "tinyxml.h"
#include "CSaveManager.h"
#include "CGrenadeManager.h"
#include "CPowerUpManager.h"
#include "COffersManager.h"
#include "CAchievementManager.h"
#include "CCommonGameConfig.h"
#include "CGameAnalytics.h"
#include "CFiveDaysBonus.h"
#include "CHuntingInfo.h"
#include "CDH_WeaponsManager.h"
#include "CDH_Weapon.h"
#include "GameCenter_Facade.h"
#include "CNGS.h"
#include "CNGSLocalUser.h"
#include "CNGSSession.h"

using namespace com::glu::platform::components;

CBH_Player* CBH_Player::m_staticInstance;
int CBH_Player::m_LastCycleNumber;
int CBH_Player::m_LastSecondsSinceAppStart;
int CBH_Player::m_LastMissionsSinceAppStart;

CBH_Player* CBH_Player::GetInstance()
{
    void* found = NULL;
    CApplet::m_pApp->m_singletons->Find(0xda6a6154, &found);
    CBH_Player* player = (CBH_Player*)found;
    if (player == NULL) {
        player = new CBH_Player;
    }
    return player;
}

CBH_Player::CBH_Player() : CSingleton(0xda6a6154), m_id(1), m_lastHuntResult(0),
    m_lastMapLocationId(-1), m_lastQuestId(-1), m_stageMissionsCompleted(0),
    m_tutorialState(0), m_lastEnergyUpdateTime(CStdUtil_Cocoa::GetTimeSeconds()),
    m_gameTime(0), m_lastDailyBonusTime(0), m_dailyBonusesInRow(0),
    m_lastFiveDaysBonusTime(0), m_activeFiveDaysBonusDay(0), m_fiveDaysTeaserDay(0),
    m_rateMeShown(false), m_cycles(0), m_lastGameVersionPlayed(100), m_newPlayer(true),
    m_tapjoyPoints(8), m_friendGiftDays(8), m_unresolvedD4(-1), m_missionsStarted(0),
    m_ammoCounts(8)
{
    m_staticInstance = this;
    m_lastDailyBonusTime = CStdUtil_Cocoa::GetTimeSeconds();
    m_lastFiveDaysBonusTime = CStdUtil_Cocoa::GetTimeSeconds() - 600;
    InitPlayerStats(m_stats);
    m_stats.m_values[0].Set(100);
    m_stats.m_values[5].Set(3);
    // The original local name lives until after Init before being destroyed.
    CStrWChar name("player");
    m_playerNameReady = false;
    m_authenticationStarted = false;
    if (GameCenter_Facade::GetInstance()->isGameCenterSupported() == 1) {
        bool authenticated = GameCenter_Facade::GetInstance()->isAuthenticated();
        GameCenter_Facade* facade = GameCenter_Facade::GetInstance();
        if (authenticated == 1) {
            facade->GetLocalPLayerName(name);
            m_playerNameReady = true;
            m_authenticationStarted = true;
        } else {
            facade->AuthenticatePlayer();
        }
    }
    m_playerName.Assign(name.m_string, wcslen(name.m_string) * sizeof(wchar_t));
    m_playerName.Assign(m_playerName.Upper());
    Init();
}

void CBH_Player::Init()
{
    m_stats.m_values[0].Set(((App*)WindowApp::m_instance)->m_progressSystem->GetMaxEnergyForLevel(m_id));
    m_energyRestoreSeconds = ((App*)WindowApp::m_instance)->m_helper->m_config.GetIntegerValueByToken(
        "ONE_ENERGY_RESTORE_SEC", true);
    Load();
}

int CBH_Player::GetMaxEnergy()
{
    return ((App*)WindowApp::m_instance)->m_progressSystem->GetMaxEnergyForLevel(m_id);
}

void CBH_Player::Load()
{
    TiXmlElement* player = ((App*)WindowApp::m_instance)->m_saveManager->GetRootElement()->FirstChildElement("player");
    if (player == NULL) {
        SetPlayerStartParams();
        Save(true);
        return;
    }
    m_newPlayer = false;
    LoadStats(player);
    LoadQuests(player);
    LoadNotifications(player);
    LoadTjPoints(player);
    LoadFriendGiftLasDay(player);
    m_gameStats.Load(player);
    m_missionsPool.Load(player->FirstChildElement("pool"));
    ((App*)WindowApp::m_instance)->m_achievementManager->Load(player);
    ((App*)WindowApp::m_instance)->m_grenadeManager->Load(player);
    ((App*)WindowApp::m_instance)->m_powerUpManager->Load(player);
    ((App*)WindowApp::m_instance)->m_offersManager->Load(player);
}

void CBH_Player::Update()
{
    int now = CStdUtil_Cocoa::GetTimeSeconds();
    int elapsed = now - m_lastEnergyUpdateTime;
    if (elapsed > m_energyRestoreSeconds) {
        AddEnergy(elapsed / m_energyRestoreSeconds, false);
        m_lastEnergyUpdateTime = now;
        Save(true); // The original AddEnergy already Saves; it is still saved once more here.
    } else {
        m_energyRemainingSeconds = m_energyRestoreSeconds - elapsed;
    }
    if (m_playerNameReady) {
        return;
    }
    if (GameCenter_Facade::GetInstance()->isGameCenterSupported() != 1) {
        return;
    }
    int state = GameCenter_Facade::GetInstance()->GetAuthenticationState();
    if (state == 0 || state == 3) {
        if (!m_authenticationStarted) {
            GameCenter_Facade::GetInstance()->AuthenticatePlayer();
            m_authenticationStarted = true;
        } else {
            m_playerName.Assign(L"PLAYER", wcslen(L"PLAYER") * sizeof(wchar_t));
            m_playerNameReady = true;
        }
    } else if (state == 2) {
        CStrWChar name("player");
        GameCenter_Facade::GetInstance()->GetLocalPLayerName(name);
        m_playerName.Assign(name.m_string, wcslen(name.m_string) * sizeof(wchar_t));
        m_playerName.Assign(m_playerName.Upper());
        m_playerNameReady = true;
    }
}

void CBH_Player::AddEnergy(int amount, bool allowOverflow)
{
    int current = m_stats.m_values[0].Get();
    if (allowOverflow) {
        m_stats.m_values[0].Set(current + amount);
    } else if (current <= ((App*)WindowApp::m_instance)->m_progressSystem->GetMaxEnergyForLevel(m_id)) {
        int value = m_stats.m_values[0].Get() + amount;
        int maximum = ((App*)WindowApp::m_instance)->m_progressSystem->GetMaxEnergyForLevel(m_id);
        if (value > maximum) {
            m_stats.m_values[0].Set(((App*)WindowApp::m_instance)->m_progressSystem->GetMaxEnergyForLevel(m_id));
        } else {
            m_stats.m_values[0].Set(m_stats.m_values[0].Get() + amount);
        }
    }
    Save(true);
}

void CBH_Player::Save(bool force)
{
    m_gameTime += ((App*)WindowApp::m_instance)->ConsumeActiveTime();
    TiXmlElement* root = ((App*)WindowApp::m_instance)->m_saveManager->GetRootElement();
    TiXmlElement* player = CXmlHelper::ObtainElement(root, XString("player"));
    SaveStats(player);
    SaveQuests(player);
    SaveNotifications(player);
    SaveTjPoints(player);
    SaveFriendGiftLasDay(player);
    m_gameStats.Save(player);
    ((App*)WindowApp::m_instance)->m_grenadeManager->Save(player);
    ((App*)WindowApp::m_instance)->m_powerUpManager->Save(player);
    ((App*)WindowApp::m_instance)->m_offersManager->Save(player);
    m_missionsPool.Save(player);
    ((App*)WindowApp::m_instance)->m_achievementManager->Save(player);
    if (force == 1) {
        ((App*)WindowApp::m_instance)->m_saveManager->SaveOnNextUpdate(0);
    }
}

void CBH_Player::AddGameTime(unsigned int seconds) { m_gameTime += seconds; }
unsigned int CBH_Player::GetGameTime() const { return m_gameTime; }

void CBH_Player::AddMoney(int amount)
{
    m_stats.m_values[2].Set(m_stats.m_values[2].Get() + amount);
    WindowApp::HandleTunnelCommand(0x83bb38a2, amount, 0, 0);
    Save(true);
}

void CBH_Player::ReduceMoney(int amount)
{
    m_stats.m_values[2].Set(m_stats.m_values[2].Get() - amount);
    WindowApp::HandleTunnelCommand(0x83bb38a2, -amount, 0, 0);
    Save(true);
}

void CBH_Player::AddPeople(int amount)
{
    m_stats.m_values[8].Set(m_stats.m_values[8].Get() + amount);
    if (m_stats.m_values[8].Get() < 0) {
        m_stats.m_values[8].Set(0);
    }
}

void CBH_Player::ReducePeople(int amount)
{
    m_stats.m_values[8].Set(m_stats.m_values[8].Get() - amount);
    if (m_stats.m_values[8].Get() < 0) {
        m_stats.m_values[8].Set(0);
    }
}

void CBH_Player::AddProvision(int amount)
{
    m_stats.m_values[6].Set(m_stats.m_values[6].Get() + amount);
    if (m_stats.m_values[6].Get() < 0) {
        m_stats.m_values[6].Set(0);
    }
}

void CBH_Player::ReduceProvision(int amount)
{
    m_stats.m_values[6].Set(m_stats.m_values[6].Get() - amount);
    if (m_stats.m_values[6].Get() < 0) {
        m_stats.m_values[6].Set(0);
    }
}

void CBH_Player::AddMedkits(int amount)
{
    m_stats.m_values[5].Set(m_stats.m_values[5].Get() + amount);
    Save(true);
}

void CBH_Player::ReleaseResources() {} // Original 143674 is a real empty method.

void CBH_Player::AddCredits(int amount, const wchar_t* source)
{
    m_stats.m_values[4].Set(m_stats.m_values[4].Get() + amount);
    Save(true);
}

void CBH_Player::ReduceCredits(int amount, const wchar_t* source)
{
    m_stats.m_values[4].Set(m_stats.m_values[4].Get() - amount);
    Save(true);
}

int CBH_Player::GetXpForNextLvl()
{
    int level = m_id;
    int maximum = ((App*)WindowApp::m_instance)->m_progressSystem->GetMaxLevel();
    int nextLevel = m_id;
    if (level != maximum) {
        ++nextLevel;
    }
    return ((App*)WindowApp::m_instance)->m_progressSystem->GetMaxXPForLevel(nextLevel);
}

int CBH_Player::GetXpForCurrentLvl()
{
    return ((App*)WindowApp::m_instance)->m_progressSystem->GetMaxXPForLevel(m_id);
}

void CBH_Player::AddXp(int amount) { AddStats((PLAYER_STATS_TYPE)3, amount, false, true); }

void CBH_Player::AddStats(PLAYER_STATS_TYPE type, int amount, bool allowOverflow, bool save)
{
    switch ((int)type) {
    case 0: {
        int current = m_stats.m_values[0].Get();
        if (allowOverflow) {
            m_stats.m_values[0].Set(current + amount);
        } else if (current <= ((App*)WindowApp::m_instance)->m_progressSystem->GetMaxEnergyForLevel(m_id)) {
            int value = m_stats.m_values[0].Get() + amount;
            int maximum = ((App*)WindowApp::m_instance)->m_progressSystem->GetMaxEnergyForLevel(m_id);
            if (value > maximum) {
                m_stats.m_values[0].Set(((App*)WindowApp::m_instance)->m_progressSystem->GetMaxEnergyForLevel(m_id));
            } else {
                m_stats.m_values[0].Set(m_stats.m_values[0].Get() + amount);
            }
        }
        break;
    }
    case 2:
        m_stats.m_values[2].Set(m_stats.m_values[2].Get() + amount);
        WindowApp::HandleTunnelCommand(0x83bb38a2, amount, 0, 0);
        break;
    case 3: {
        m_stats.m_values[3].Set(m_stats.m_values[3].Get() + amount);
        CProgressSystem* progress = ((App*)WindowApp::m_instance)->m_progressSystem;
        int level = progress->GetLevel(m_stats.m_values[3].Get());
        int maximum = ((App*)WindowApp::m_instance)->m_progressSystem->GetMaxXP();
        if (level > m_id) {
            m_id = level;
            m_stats.m_values[0].Set(((App*)WindowApp::m_instance)->m_progressSystem->GetMaxEnergyForLevel(level));
            CGameAnalytics::logLevelGained(level);
        }
        if (m_stats.m_values[3].Get() > maximum) {
            m_stats.m_values[3].Set(maximum);
        }
        WindowApp::HandleTunnelCommand(0x45e781f9, amount, 0, 0);
        break;
    }
    case 4:
        m_stats.m_values[4].Set(m_stats.m_values[4].Get() + amount);
        break;
    case 5:
        m_stats.m_values[5].Set(m_stats.m_values[5].Get() + amount);
        break;
    case 6:
        m_stats.m_values[6].Set(m_stats.m_values[6].Get() + amount);
        if (m_stats.m_values[6].Get() < 0) {
            // Original 1438ec clears people(+4c), not provision(+3c).
            m_stats.m_values[8].Set(0);
        }
        break;
    case 7:
        m_stats.m_values[7].Set(m_stats.m_values[7].Get() + amount);
        break;
    case 8:
        m_stats.m_values[8].Set(m_stats.m_values[8].Get() + amount);
        if (m_stats.m_values[8].Get() < 0) {
            m_stats.m_values[8].Set(0);
        }
        break;
    case 9:
        m_stats.m_values[9].Set(m_stats.m_values[9].Get() + amount);
        break;
    }
    if (save == 1) {
        Save(true);
    }
}

bool CBH_Player::IsQuestCompleted(int id)
{
    for (int index = 0; index < m_completedQuests.m_size; ++index) {
        if (m_completedQuests.m_items[index] == id) {
            return true;
        }
    }
    return false;
}

void CBH_Player::OnQuestCompleted(int id)
{
    m_completedQuests.addElement(id);
    Save(true);
}

bool CBH_Player::IsPushNotificationHandled(int id)
{
    for (int index = 0; index < m_handledNotifications.m_size; ++index) {
        if (m_handledNotifications.m_items[index] == id) {
            return true;
        }
    }
    return false;
}

void CBH_Player::OnPushNotificationHandled(int id)
{
    m_handledNotifications.addElement(id);
    if (m_handledNotifications.m_size >= 201) {
        m_handledNotifications.removeElementAt(0);
    }
    Save(true);
}

void CBH_Player::OnMissionCompleted(const CMission* mission)
{
    if (((App*)WindowApp::m_instance)->m_huntingInfo->m_assignedData.m_questId < 0) {
        ++m_stageMissionsCompleted;
    }
    Save(true);
}

void CBH_Player::OnEnemyKilled() { ((App*)WindowApp::m_instance)->m_huntingInfo->OnEnemyKilled(); }
void CBH_Player::OnEnemyTranquillized() { ((App*)WindowApp::m_instance)->m_huntingInfo->OnEnemyTranqullized(); }
void CBH_Player::OnEnemyEscaped() { ((App*)WindowApp::m_instance)->m_huntingInfo->OnEnemyEscaped(); }

void CBH_Player::SetPlayerStartParams()
{
    m_stats.m_values[2].Set(((App*)WindowApp::m_instance)->m_helper->m_config.GetIntegerValueByToken("PLAYER_START_MONEY", true));
    m_stats.m_values[4].Set(((App*)WindowApp::m_instance)->m_helper->m_config.GetIntegerValueByToken("PLAYER_START_CREDITS", true));
    m_stats.m_values[8].Set(0);
    m_stats.m_values[9].Set(1);
    m_stats.m_values[6].Set(0);
    m_stats.m_values[7].Set(1);
    m_cycles = 0;
    CItem* item = ((App*)WindowApp::m_instance)->m_weaponsManager->firstItem();
    while (item != NULL) {
        item->setAvailable(false);
        item = ((App*)WindowApp::m_instance)->m_weaponsManager->nextItem();
    }
    int weaponId = ((App*)WindowApp::m_instance)->m_helper->m_config.GetIntegerValueByToken("PLAYER_START_WEAPON_1_WID", true);
    ((App*)WindowApp::m_instance)->m_helper->m_config.GetIntegerValueByToken("PLAYER_START_WEAPON_1_AMMO_PACKS", true);
    ((App*)WindowApp::m_instance)->m_weaponsManager->getItemById(weaponId)->setAvailable(true);
    DGHelper* helper = ((App*)WindowApp::m_instance)->m_helper;
    int weapon = ((App*)WindowApp::m_instance)->m_weaponsManager->getItemById(weaponId)->getId();
    helper->m_gamePlayManager.SetWeapon(0, weapon);
    helper = ((App*)WindowApp::m_instance)->m_helper;
    weapon = ((App*)WindowApp::m_instance)->m_weaponsManager->getItemById(weaponId)->getId();
    helper->m_gamePlayManager.SetWeapon(1, weapon);
    ((App*)WindowApp::m_instance)->m_offersManager->InitiateOTOProcess();
}

bool CBH_Player::TryMakeFiveDaysBonus(bool* newDay)
{
    unsigned int now = CStdUtil_Cocoa::GetTimeSeconds();
    if (now == (unsigned int)-1) {
        return false;
    }
    *newDay = false;
    CNGSSession* session = CNGS::GetInstance()->GetLocalUser()->m_session;
    bool validTime = session->isValidNetworkTime() == 1;
    if (validTime) {
        now = session->getNetworkCurrentTimeSeconds();
    }
    if (now < m_lastFiveDaysBonusTime) {
        return false;
    }
    int today = DGHelper::GetDaysFromTimestamp(now);
    int previous = DGHelper::GetDaysFromTimestamp(m_lastFiveDaysBonusTime);
    if (today == previous) {
        return false;
    }
    *newDay = true;
    if (!validTime) {
        return false;
    }
    if (today - previous >= 2) {
        m_activeFiveDaysBonusDay = 0;
        m_dailyBonusesInRow = 0;
    } else if (today < previous) {
        m_dailyBonusesInRow = 0;
        m_lastFiveDaysBonusTime = now;
        Save(true);
        return false;
    }
    ((App*)WindowApp::m_instance)->m_fiveDaysBonus->GiveBonus(m_activeFiveDaysBonusDay);
    m_lastFiveDaysBonusTime = now;
    ++m_activeFiveDaysBonusDay;
    ++m_dailyBonusesInRow;
    if (m_activeFiveDaysBonusDay >= 5) {
        m_activeFiveDaysBonusDay = 0;
    }
    Save(true);
    return true;
}

CBH_Player::~CBH_Player()
{
    m_staticInstance = NULL;
    CollectShutdownAnalytics();
}

void CBH_Player::CollectShutdownAnalytics()
{
    if (m_staticInstance == NULL) {
        return;
    }
    m_LastCycleNumber = m_staticInstance->m_gameStats.m_cycleNumber;
    m_LastSecondsSinceAppStart = m_staticInstance->m_gameStats.GetTimeSinceAppStart();
    m_LastMissionsSinceAppStart = m_staticInstance->m_gameStats.m_missionsSinceAppStart;
}

int CBH_Player::GetStats(PLAYER_STATS_TYPE type) const { return m_stats.m_values[(int)type].Get(); }
int CBH_Player::GetTutorialState() const { return m_tutorialState; }
void CBH_Player::SetTutorialState(int state) { m_tutorialState = state; Save(true); }

void CBH_Player::RefreshMissionsPool()
{
    {
        CMissionsPool pool = ((App*)WindowApp::m_instance)->m_commonGameConfig->GetMissionsPoolForLevel(m_id);
        m_missionsPool.m_elements = pool.m_elements;
    }
    Save(true);
}

void CBH_Player::SaveStats(TiXmlElement* element)
{
    SavePlayerStats(m_stats, element, XString(L""));
    element->SetAttribute("timePlayed", m_gameTime);
    element->SetAttribute("cycles", m_cycles);
    element->SetAttribute("level", m_id);
    element->SetAttribute("tutorial_state", m_tutorialState);
    element->SetAttribute("last_daily_bonus_time", m_lastDailyBonusTime);
    element->SetAttribute("daily_bonus_row", m_dailyBonusesInRow);
    element->SetAttribute("last_five_days_bonus_time", m_lastFiveDaysBonusTime);
    element->SetAttribute("active_five_days_bonus_day", m_activeFiveDaysBonusDay);
    element->SetAttribute("app_rated", m_rateMeShown);
    element->SetAttribute("last_energy_update_time", m_lastEnergyUpdateTime);
    element->SetAttribute("last_game_version_played", m_lastGameVersionPlayed);
    element->SetAttribute("stage_missions_completed", m_stageMissionsCompleted);
    element->SetAttribute("last_hunt_res", m_lastHuntResult);
    element->SetAttribute("last_mapLoc_id", m_lastMapLocationId);
    element->SetAttribute("last_quest_id", m_lastQuestId);
    CDH_Weapon* weapon = ((App*)WindowApp::m_instance)->m_helper->m_gamePlayManager.GetGameData()->GetWeapon(0);
    if (weapon != NULL) {
        element->SetAttribute("weapon_slot1", weapon->getId());
    }
    weapon = ((App*)WindowApp::m_instance)->m_helper->m_gamePlayManager.GetGameData()->GetWeapon(1);
    if (weapon != NULL) {
        element->SetAttribute("weapon_slot2", weapon->getId());
    }
}

void CBH_Player::LoadStats(TiXmlElement* element)
{
    LoadPlayerStats(m_stats, element, XString(L""));
    m_gameTime = CXmlHelper::GetAttributeValueOrUseDefault(element, "timePlayed", "0").ToInt();
    m_cycles = CXmlHelper::GetAttributeValue(element, "cycles").ToInt();
    m_id = CXmlHelper::GetAttributeValue(element, "level").ToInt();
    m_tutorialState = CXmlHelper::GetAttributeValue(element, "tutorial_state").ToInt();
    m_lastDailyBonusTime = CXmlHelper::GetAttributeValue(element, "last_daily_bonus_time").ToInt();
    m_dailyBonusesInRow = CXmlHelper::GetAttributeValue(element, "daily_bonus_row").ToInt();
    m_rateMeShown = CXmlHelper::GetAttributeValue(element, "app_rated").ToBool();
    m_lastEnergyUpdateTime = CXmlHelper::GetAttributeValue(element, "last_energy_update_time").ToInt();
    m_stageMissionsCompleted = CXmlHelper::GetAttributeValue(element, "stage_missions_completed").ToInt();
    m_lastHuntResult = CXmlHelper::GetAttributeValue(element, "last_hunt_res").ToInt();
    m_lastMapLocationId = CXmlHelper::GetAttributeValue(element, "last_mapLoc_id").ToInt();
    m_lastQuestId = CXmlHelper::GetAttributeValue(element, "last_quest_id").ToInt();
    m_lastGameVersionPlayed = CXmlHelper::GetAttributeValueOrUseDefault(element, "last_game_version_played", XString(L"100")).ToInt();
    m_lastFiveDaysBonusTime = CXmlHelper::GetAttributeValueOrUseDefault(element, "last_five_days_bonus_time", XString(L"0")).ToInt();
    m_activeFiveDaysBonusDay = CXmlHelper::GetAttributeValueOrUseDefault(element, "active_five_days_bonus_day", XString(L"0")).ToInt();
    int weaponId = CXmlHelper::GetAttributeValue(element, "weapon_slot1").ToInt();
    CItem* item = ((App*)WindowApp::m_instance)->m_weaponsManager->getItemById(weaponId);
    if (item != NULL) {
        DGHelper* helper = ((App*)WindowApp::m_instance)->m_helper;
        helper->m_gamePlayManager.SetWeapon(0, item->getId());
    }
    weaponId = CXmlHelper::GetAttributeValue(element, "weapon_slot2").ToInt();
    item = ((App*)WindowApp::m_instance)->m_weaponsManager->getItemById(weaponId);
    if (item != NULL) {
        DGHelper* helper = ((App*)WindowApp::m_instance)->m_helper;
        helper->m_gamePlayManager.SetWeapon(1, item->getId());
    }
    if (m_stats.m_values[4].Get() < 0) {
        m_stats.m_values[4].Set(0);
    }
}

void CBH_Player::SaveQuests(TiXmlElement* element)
{
    TiXmlElement* parent = CXmlHelper::ObtainElement(element, XString("completed_quests"));
    parent->Clear();
    for (int index = 0; index < m_completedQuests.m_size; ++index) {
        int id = m_completedQuests.m_items[index];
        TiXmlElement* child = new TiXmlElement("q");
        child->SetAttribute("id", id);
        parent->InsertEndChild(*child);
        delete child;
    }
}

void CBH_Player::LoadQuests(TiXmlElement* element)
{
    TiXmlElement* parent = element->FirstChildElement("completed_quests");
    if (parent == NULL) {
        return;
    }
    TiXmlElement* child = parent->FirstChildElement("q");
    m_completedQuests.removeAllElements();
    while (child != NULL) {
        int id = CXmlHelper::GetAttributeValue(child, "id").ToInt();
        m_completedQuests.addElement(id);
        child = child->NextSiblingElement("q");
    }
}

void CBH_Player::SaveNotifications(TiXmlElement* element)
{
    TiXmlElement* parent = CXmlHelper::ObtainElement(element, XString("handled_push_notifications"));
    if (parent == NULL) {
        return;
    }
    parent->Clear();
    for (int index = 0; index < m_handledNotifications.m_size; ++index) {
        int id = m_handledNotifications.m_items[index];
        TiXmlElement* child = new TiXmlElement("n");
        child->SetAttribute("id", id);
        parent->InsertEndChild(*child);
        delete child;
    }
}

void CBH_Player::LoadNotifications(TiXmlElement* element)
{
    TiXmlElement* parent = element->FirstChildElement("handled_push_notifications");
    if (parent == NULL) {
        return;
    }
    TiXmlElement* child = parent->FirstChildElement("n");
    m_handledNotifications.removeAllElements();
    while (child != NULL) {
        int id = CXmlHelper::GetAttributeValue(child, "id").ToInt();
        m_handledNotifications.addElement(id);
        child = child->NextSiblingElement("n");
    }
}

void CBH_Player::SaveTjPoints(TiXmlElement* element)
{
    TiXmlElement* parent = CXmlHelper::ObtainElement(element, XString("tapjoy_info"));
    parent->Clear();
    for (int index = 0; index < m_tapjoyPoints.m_count; ++index) {
        TiXmlElement* child = new TiXmlElement("tapjoy_points");
        XString* key;
        int value = *m_tapjoyPoints.GetAt(index, &key);
        {
            XString::AnsiString text(key->m_data);
            child->SetStringAttribute("devId", text);
        }
        child->SetAttribute("points", value);
        parent->InsertEndChild(*child);
        delete child;
    }
}

void CBH_Player::LoadTjPoints(TiXmlElement* element)
{
    TiXmlElement* parent = element->FirstChildElement("tapjoy_info");
    if (parent == NULL) {
        return;
    }
    TiXmlElement* child = parent->FirstChildElement("tapjoy_points");
    while (child != NULL) {
        XString key = CXmlHelper::GetAttributeValue(child, "devId");
        int value = CXmlHelper::GetAttributeValue(child, "points").ToInt();
        m_tapjoyPoints.Set(key, value);
        child = child->NextSiblingElement("tapjoy_points");
    }
}

void CBH_Player::SaveFriendGiftLasDay(TiXmlElement* element)
{
    if (element == NULL) {
        return;
    }
    TiXmlElement* parent = CXmlHelper::ObtainElement(element, XString("friendGiftsInfo"));
    parent->Clear();
    for (int index = 0; index < m_friendGiftDays.m_count; ++index) {
        TiXmlElement* child = new TiXmlElement("friendGift");
        XString* key;
        int value = *m_friendGiftDays.GetAt(index, &key);
        {
            XString::AnsiString text(key->m_data);
            child->SetStringAttribute("socialUserGUID", text);
        }
        child->SetAttribute("lastDay", value);
        parent->InsertEndChild(*child);
        delete child;
    }
}

void CBH_Player::LoadFriendGiftLasDay(TiXmlElement* element)
{
    if (element == NULL) {
        return;
    }
    TiXmlElement* parent = element->FirstChildElement("friendGiftsInfo");
    if (parent == NULL) {
        return;
    }
    TiXmlElement* child = parent->FirstChildElement("friendGift");
    while (child != NULL) {
        XString key = CXmlHelper::GetAttributeValue(child, "socialUserGUID");
        int value = CXmlHelper::GetAttributeValue(child, "lastDay").ToInt();
        m_friendGiftDays.Set(key, value);
        child = child->NextSiblingElement("friendGift");
    }
}
