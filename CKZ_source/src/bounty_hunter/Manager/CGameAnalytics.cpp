// CGameAnalytics.cpp - analytics events of the game, sent as Flurry custom events.
// Every event carries a wide info text and two integers; the kill and skillshot events of a
// mission can be collected and sent later (flushDeferredEvents).
#include "CGameAnalytics.h"
#include <wchar.h>
#include "CStrWChar.h"
#include "CFlurry.h"
#include "AdBannerSizes.h"
#include "App.h"
#include "CShopItem.h"
#include "CDH_WeaponsManager.h"
#include "CHuntingInfo.h"
#include "CBH_Player.h"
#include "CPlayerGameStats.h"

using namespace com::glu::platform::components;

// Flurry event names (the original array names; ZOMBSNIPER_ITEM_BUY_CLICKED holds another text).
static const char ZOMBSNIPER_EVT_TYPE_PUSH_TOKEN[] = "ZOMBSNIPER_EVT_TYPE_PUSH_TOKEN";
static const char ZOMBSNIPER_AP_START[] = "ZOMBSNIPER_AP_START";
static const char ZOMBSNIPER_AP_STOP[] = "ZOMBSNIPER_AP_STOP";
static const char ZOMBSNIPER_GAME_STOP[] = "ZOMBSNIPER_GAME_STOP";
static const char ZOMBSNIPER_GAME_START[] = "ZOMBSNIPER_GAME_START";
static const char ZOMBSNIPER_NOTIFICATION_START[] = "ZOMBSNIPER_NOTIFICATION_START";
static const char ZOMBSNIPER_IAP_SOFT_BOUGHT[] = "ZOMBSNIPER_IAP_SOFT_BOUGHT";
static const char ZOMBSNIPER_IAP_HARD_BOUGHT[] = "ZOMBSNIPER_IAP_HARD_BOUGHT";
static const char ZOMBSNIPER_CURRENCY_TOTAL_SOFT[] = "ZOMBSNIPER_CURRENCY_TOTAL_SOFT";
static const char ZOMBSNIPER_CURRENCT_TOTAL_HARD[] = "ZOMBSNIPER_CURRENCT_TOTAL_HARD";
static const char ZOMBSNIPER_IAP_BUY_CLICKED[] = "ZOMBSNIPER_IAP_BUY_CLICKED";
static const char ZOMBSNIPER_IAP_PURCHASE_SUCCESSFUL[] = "ZOMBSNIPER_IAP_PURCHASE_SUCCESSFUL";
static const char ZOMBSNIPER_IAP_PURCHASE_CANCELLED[] = "ZOMBSNIPER_IAP_PURCHASE_CANCELLED";
static const char ZOMBSNIPER_IAP_PURCHASE_FAILED[] = "ZOMBSNIPER_IAP_PURCHASE_FAILED";
static const char ZOMBSNIPER_LEVEL_ACHIEVED[] = "ZOMBSNIPER_LEVEL_ACHIEVED";
static const char ZOMBSNIPER_DAILY_BONUS_ITEM[] = "ZOMBSNIPER_DAILY_BONUS_ITEM";
static const char ZOMBSNIPER_DAILY_BONUS_01[] = "ZOMBSNIPER_DAILY_BONUS_01";
static const char ZOMBSNIPER_DAILY_BONUS_02[] = "ZOMBSNIPER_DAILY_BONUS_02";
static const char ZOMBSNIPER_DAILY_BONUS_03[] = "ZOMBSNIPER_DAILY_BONUS_03";
static const char ZOMBSNIPER_DAILY_BONUS_04[] = "ZOMBSNIPER_DAILY_BONUS_04";
static const char ZOMBSNIPER_DAILY_BONUS_05[] = "ZOMBSNIPER_DAILY_BONUS_05";
static const char ZOMBSNIPER_ONE_TIME_OFFER[] = "ZOMBSNIPER_ONE_TIME_OFFER";
static const char ZOMBSNIPER_DAILY_DEAL[] = "ZOMBSNIPER_DAILY_DEAL";
static const char ZOMBSNIPER_TUTORIAL_STEP_COMPLETED[] = "ZOMBSNIPER_TUTORIAL_STEP_COMPLETED";
static const char ZOMBSNIPER_EVT_TYPE_MISSION_STARTED[] = "ZOMBSNIPER_EVT_TYPE_MISSION_STARTED";
static const char ZOMBSNIPER_MISSION_COMPLETE[] = "ZOMBSNIPER_MISSION_COMPLETE";
static const char ZOMBSNIPER_MISSION_COMPLETE_SCORE[] = "ZOMBSNIPER_MISSION_COMPLETE_SCORE";
static const char ZOMBSNIPER_MISSION_COMPLETED_HELICOPTER[] = "ZOMBSNIPER_MISSION_COMPLETED_HELICOPTER";
static const char ZOMBSNIPER_MISSION_QUIT[] = "ZOMBSNIPER_MISSION_QUIT";
static const char ZOMBSNIPER_MISSION_FAILED[] = "ZOMBSNIPER_MISSION_FAILED";
static const char ZOMBSNIPER_SETTING_SOUND[] = "ZOMBSNIPER_SETTING_SOUND";
static const char ZOMBSNIPER_SETTING_MUSIC[] = "ZOMBSNIPER_SETTING_MUSIC";
static const char ZOMBSNIPER_SETTING_PUSH[] = "ZOMBSNIPER_SETTING_PUSH";
static const char ZOMBSNIPER_ITEM_VIEWED[] = "ZOMBSNIPER_ITEM_VIEWED";
static const char ZOMBSNIPER_ITEM_BUY_CLICKED[] = "ZOMBSNIPER_EVT_TYPE_ITEM_BUY_CLICKED";
static const char ZOMBSNIPER_ITEM_PURCHASED[] = "ZOMBSNIPER_ITEM_PURCHASED";
static const char ZOMBSNIPER_ITEM_CANCEL_CLICKED[] = "ZOMBSNIPER_ITEM_CANCEL_CLICKED";
static const char ZOMBSNIPER_ITEM_EQUIPPED[] = "ZOMBSNIPER_ITEM_EQUIPPED";
static const char ZOMBSNIPER_KILL_ZOMBIE[] = "ZOMBSNIPER_KILL_ZOMBIE";
static const char ZOMBSNIPER_EVT_TYPE_SKILLSHOT[] = "ZOMBSNIPER_EVT_TYPE_SKILLSHOT";
static const char ZOMBSNIPER_MENU_ACCESSED[] = "ZOMBSNIPER_MENU_ACCESSED";
static const char ZOMBSNIPER_CYCLE_SCORE_ACHIEVED[] = "ZOMBSNIPER_CYCLE_SCORE_ACHIEVED";
static const char ZOMBSNIPER_EVT_TYPE_AD_SHOWN[] = "ZOMBSNIPER_EVT_TYPE_AD_SHOWN";
static const char ZOMBSNIPER_EVT_TYPE_AD_BONUS[] = "ZOMBSNIPER_EVT_TYPE_AD_BONUS";
static const char ZOMBSNIPER_SM_FINISHED_WAVE[] = "ZOMBSNIPER_SM_FINISHED_WAVE";
static const char ZOMBSNIPER_SM_ZOMBIES_KILLED[] = "ZOMBSNIPER_SM_ZOMBIES_KILLED";
static const char ZOMBSNIPER_SM_XP_EARNED[] = "ZOMBSNIPER_SM_XP_EARNED";
static const char ZOMBSNIPER_SM_SOFT_EARNED[] = "ZOMBSNIPER_SM_SOFT_EARNED";
static const char ZOMBSNIPER_SM_FINAL_SCORE[] = "ZOMBSNIPER_SM_FINAL_SCORE";
static const char ZOMBSNIPER_SM_SHOP_ENTERED[] = "ZOMBSNIPER_SM_SHOP_ENTERED";
static const char ZOMBSNIPER_SM_WEAPON_PURCHASED[] = "ZOMBSNIPER_SM_WEAPON_PURCHASED";
static const char ZOMBSNIPER_IAP_AFTER_FAIL[] = "ZOMBSNIPER_IAP_AFTER_FAIL";
static const char ZOMBSNIPER_RECOMMENDED_WEAPON[] = "ZOMBSNIPER_RECOMMENDED_WEAPON";
static const char ZOMBSNIPER_SM_REVIVE[] = "ZOMBSNIPER_SM_REVIVE";

CStrChar CGameAnalytics::m_deferredToken("");
CStrChar CGameAnalytics::m_lastDeferredToken("");
Vector<CGameAnalytics::SZombieKilled> CGameAnalytics::m_zombieKilled;
Vector<CGameAnalytics::SSkillshot> CGameAnalytics::m_skillshot;
bool CGameAnalytics::g_logMissionFailure;

void CGameAnalytics::SendPushTokenImpl(const char* token)
{
    CFlurry::LogCustomEvent(ZOMBSNIPER_EVT_TYPE_PUSH_TOKEN, CStrWChar(token).GetWCharPtr(), 0, 0);
}

void CGameAnalytics::SendDeferredPushToken()
{
    if (m_deferredToken.GetLength() <= 0) {
        return;
    }
    m_lastDeferredToken = m_deferredToken;
    SendPushTokenImpl(m_deferredToken.GetCharPtr());
    m_deferredToken = "";
}

void CGameAnalytics::SendPushStatistics(const char* token)
{
    m_deferredToken = token;
}

void CGameAnalytics::logAppStart()
{
    XString info;
    CFlurry::LogCustomEvent(ZOMBSNIPER_AP_START, CStrWChar(info.c_str()).GetWCharPtr(), 1, 0);
}

void CGameAnalytics::logAppStop()
{
    CBH_Player::CollectShutdownAnalytics();
    XString cycle = XString::Format(L"%d", CBH_Player::m_LastCycleNumber);
    int seconds = CBH_Player::m_LastSecondsSinceAppStart;
    int missions = CBH_Player::m_LastMissionsSinceAppStart;
    CFlurry::LogCustomEvent(ZOMBSNIPER_AP_STOP, CStrWChar(cycle.c_str()).GetWCharPtr(), seconds, missions);
}

void CGameAnalytics::logAppBackground()
{
    XString cycle = XString::Format(L"%d", CBH_Player::GetInstance()->m_gameStats.m_cycleNumber);
    int seconds = CBH_Player::GetInstance()->m_gameStats.GetTimeSinceAppForeground();
    int missions = CBH_Player::GetInstance()->m_gameStats.m_missionsSinceForeground;
    CFlurry::LogCustomEvent(ZOMBSNIPER_GAME_STOP, CStrWChar(cycle.c_str()).GetWCharPtr(), seconds, missions);
}

void CGameAnalytics::logAppForeground()
{
    XString info;
    CFlurry::LogCustomEvent(ZOMBSNIPER_GAME_START, CStrWChar(info.c_str()).GetWCharPtr(), 1, 0);
}

void CGameAnalytics::logPushNotification(bool inForeground)
{
    XString info;
    CFlurry::LogCustomEvent(ZOMBSNIPER_NOTIFICATION_START, CStrWChar(info.c_str()).GetWCharPtr(), 1, !inForeground);
}

void CGameAnalytics::logCurrencyChange(XString source, int amount, int total, ECurrencyType type)
{
    switch (type) {
    case CURRENCY_SOFT:
        CFlurry::LogCustomEvent(ZOMBSNIPER_IAP_SOFT_BOUGHT, CStrWChar(source.c_str()).GetWCharPtr(), amount, total);
        break;
    case CURRENCY_HARD:
        CFlurry::LogCustomEvent(ZOMBSNIPER_IAP_HARD_BOUGHT, CStrWChar(source.c_str()).GetWCharPtr(), amount, total);
        break;
    }
}

void CGameAnalytics::logTotalCurrency(int playerId, int amount, ECurrencyType type)
{
    XString info;
    XString::Format(L"%d", playerId);   // formatted and dropped (as in the original)
    switch (type) {
    case CURRENCY_SOFT:
        CFlurry::LogCustomEvent(ZOMBSNIPER_CURRENCY_TOTAL_SOFT, CStrWChar(info.c_str()).GetWCharPtr(), amount, 0);
        break;
    case CURRENCY_HARD:
        CFlurry::LogCustomEvent(ZOMBSNIPER_CURRENCT_TOTAL_HARD, CStrWChar(info.c_str()).GetWCharPtr(), amount, 0);
        break;
    }
}

void CGameAnalytics::logIAPBuyClicked(XString item, int price, ECurrencyType type)
{
    CFlurry::LogCustomEvent(ZOMBSNIPER_IAP_BUY_CLICKED, CStrWChar(item.c_str()).GetWCharPtr(), price, type == CURRENCY_HARD);
}

void CGameAnalytics::logIAPPurchaseSuccessful(XString item, int price, ECurrencyType type)
{
    CFlurry::LogCustomEvent(ZOMBSNIPER_IAP_PURCHASE_SUCCESSFUL, CStrWChar(item.c_str()).GetWCharPtr(), price, type == CURRENCY_HARD);
    if (g_logMissionFailure) {
        logIAPAfterFail(item);
    }
    // Was it the weapon the hunting info recommended?
    XString& recommended = ((App*)WindowApp::m_instance)->m_huntingInfo->m_text04;
    if (recommended.Length() != 0) {
        CItem* weapon = ((App*)WindowApp::m_instance)->m_weaponsManager->getItemByStringUId(recommended);
        if (weapon != NULL) {
            logRecommendedWeaponPurchase(item, weapon->getId());
        }
    }
}

void CGameAnalytics::logIAPAfterFail(XString item)
{
    int playerId = CBH_Player::GetInstance()->m_id;
    CFlurry::LogCustomEvent(ZOMBSNIPER_IAP_AFTER_FAIL, CStrWChar(item.c_str()).GetWCharPtr(), playerId, 0);
}

void CGameAnalytics::logRecommendedWeaponPurchase(XString item, int weaponId)
{
    int playerId = CBH_Player::GetInstance()->m_id;
    CFlurry::LogCustomEvent(ZOMBSNIPER_RECOMMENDED_WEAPON, CStrWChar(item.c_str()).GetWCharPtr(), weaponId, playerId);
}

void CGameAnalytics::logIAPPurchaseCanceled(XString item, int price, ECurrencyType type)
{
    CFlurry::LogCustomEvent(ZOMBSNIPER_IAP_PURCHASE_CANCELLED, CStrWChar(item.c_str()).GetWCharPtr(), price, type == CURRENCY_HARD);
}

void CGameAnalytics::logIAPPurchaseFailed(XString item, int price, ECurrencyType type)
{
    CFlurry::LogCustomEvent(ZOMBSNIPER_IAP_PURCHASE_FAILED, CStrWChar(item.c_str()).GetWCharPtr(), price, type == CURRENCY_HARD);
}

void CGameAnalytics::logLevelGained(int level)
{
    XString info;
    CFlurry::LogCustomEvent(ZOMBSNIPER_LEVEL_ACHIEVED, CStrWChar(info.c_str()).GetWCharPtr(), level, 0);
}

void CGameAnalytics::logDailyBonus(XString bonus, int day)
{
    CFlurry::LogCustomEvent(ZOMBSNIPER_DAILY_BONUS_ITEM, CStrWChar(bonus.c_str()).GetWCharPtr(), day, 0);
    switch (day) {
    case 0:
        CFlurry::LogCustomEvent(ZOMBSNIPER_DAILY_BONUS_01, CStrWChar(bonus.c_str()).GetWCharPtr(), 0, 0);
        break;
    case 1:
        CFlurry::LogCustomEvent(ZOMBSNIPER_DAILY_BONUS_02, CStrWChar(bonus.c_str()).GetWCharPtr(), 0, 0);
        break;
    case 2:
        CFlurry::LogCustomEvent(ZOMBSNIPER_DAILY_BONUS_03, CStrWChar(bonus.c_str()).GetWCharPtr(), 0, 0);
        break;
    case 3:
        CFlurry::LogCustomEvent(ZOMBSNIPER_DAILY_BONUS_04, CStrWChar(bonus.c_str()).GetWCharPtr(), 0, 0);
        break;
    case 4:
        CFlurry::LogCustomEvent(ZOMBSNIPER_DAILY_BONUS_05, CStrWChar(bonus.c_str()).GetWCharPtr(), 0, 0);
        break;
    }
}

void CGameAnalytics::logOneTimeOfferResult(const XString& item, int price, EOTOResult result)
{
    CFlurry::LogCustomEvent(ZOMBSNIPER_ONE_TIME_OFFER, CStrWChar(item.c_str()).GetWCharPtr(), price, result);
}

void CGameAnalytics::logDailyDealResult(const XString& item, int price, EOTOResult result)
{
    CFlurry::LogCustomEvent(ZOMBSNIPER_DAILY_DEAL, CStrWChar(item.c_str()).GetWCharPtr(), price, result);
}

void CGameAnalytics::logTutorialStepCompleted(int step)
{
    XString info;
    CFlurry::LogCustomEvent(ZOMBSNIPER_TUTORIAL_STEP_COMPLETED, CStrWChar(info.c_str()).GetWCharPtr(), step, 0);
}

void CGameAnalytics::logMissionStarted(XString mission, CMission::EMissionDifficulty difficulty)
{
    CFlurry::LogCustomEvent(ZOMBSNIPER_EVT_TYPE_MISSION_STARTED, CStrWChar(mission.c_str()).GetWCharPtr(), difficulty, 0);
}

void CGameAnalytics::logMissionCompleted(XString mission, int value1, int value2)
{
    CFlurry::LogCustomEvent(ZOMBSNIPER_MISSION_COMPLETE, CStrWChar(mission.c_str()).GetWCharPtr(), value1, value2);
    g_logMissionFailure = false;
}

void CGameAnalytics::logMissionCompleteScore(XString mission, int score, int value)
{
    CFlurry::LogCustomEvent(ZOMBSNIPER_MISSION_COMPLETE_SCORE, CStrWChar(mission.c_str()).GetWCharPtr(), score, value);
}

void CGameAnalytics::logHelicopterMissionCompleted(int mission, int value)
{
    XString info = XString::Format(L"%d", mission);
    CFlurry::LogCustomEvent(ZOMBSNIPER_MISSION_COMPLETED_HELICOPTER, CStrWChar(info.c_str()).GetWCharPtr(), value, 0);
}

void CGameAnalytics::logMissionQuit(XString mission, int value)
{
    CFlurry::LogCustomEvent(ZOMBSNIPER_MISSION_QUIT, CStrWChar(mission.c_str()).GetWCharPtr(), value, 0);
    g_logMissionFailure = false;
}

void CGameAnalytics::logMissionFailed(XString mission, int value1, int value2)
{
    CFlurry::LogCustomEvent(ZOMBSNIPER_MISSION_FAILED, CStrWChar(mission.c_str()).GetWCharPtr(), value1, value2);
    g_logMissionFailure = true;
}

void CGameAnalytics::logSoundSwitch(bool enabled)
{
    XString info;
    CFlurry::LogCustomEvent(ZOMBSNIPER_SETTING_SOUND, CStrWChar(info.c_str()).GetWCharPtr(), enabled, 0);
}

void CGameAnalytics::logMusicSwitch(bool enabled)
{
    XString info;
    CFlurry::LogCustomEvent(ZOMBSNIPER_SETTING_MUSIC, CStrWChar(info.c_str()).GetWCharPtr(), enabled, 0);
}

void CGameAnalytics::logPushSwitch(bool enabled)
{
    XString info;
    CFlurry::LogCustomEvent(ZOMBSNIPER_SETTING_PUSH, CStrWChar(info.c_str()).GetWCharPtr(), enabled, 0);
}

void CGameAnalytics::logItemViewed(XString item, int price, ECurrencyType type)
{
    CFlurry::LogCustomEvent(ZOMBSNIPER_ITEM_VIEWED, CStrWChar(item.c_str()).GetWCharPtr(), price, type != CURRENCY_SOFT);
}

void CGameAnalytics::logBuyItemClicked(XString item, int price, ECurrencyType type)
{
    CFlurry::LogCustomEvent(ZOMBSNIPER_ITEM_BUY_CLICKED, CStrWChar(item.c_str()).GetWCharPtr(), price, type != CURRENCY_SOFT);
}

void CGameAnalytics::logItemPurchased(XString item, int price, ECurrencyType type)
{
    CFlurry::LogCustomEvent(ZOMBSNIPER_ITEM_PURCHASED, CStrWChar(item.c_str()).GetWCharPtr(), price, type != CURRENCY_SOFT);
}

void CGameAnalytics::logItemPurchaseCancelled(XString item, int price, ECurrencyType type)
{
    CFlurry::LogCustomEvent(ZOMBSNIPER_ITEM_CANCEL_CLICKED, CStrWChar(item.c_str()).GetWCharPtr(), price, type != CURRENCY_SOFT);
}

void CGameAnalytics::logItemEquipped(XString item)
{
    CFlurry::LogCustomEvent(ZOMBSNIPER_ITEM_EQUIPPED, CStrWChar(item.c_str()).GetWCharPtr(), 0, 0);
}

void CGameAnalytics::flushDeferredEvents()
{
    for (int index = 0; index < m_zombieKilled.size(); index++) {
        SZombieKilled& event = m_zombieKilled[index];
        logZombieKilled(event.m_name, event.m_value, event.m_killType, false);
    }
    m_zombieKilled.removeAllElements();
    for (int index = 0; index < m_skillshot.size(); index++) {
        logSkillshot(m_skillshot[index].m_name, m_skillshot[index].m_value, false);
    }
    m_skillshot.removeAllElements();
}

void CGameAnalytics::logZombieKilled(XString name, int value, EKillType killType, bool deferred)
{
    if (deferred == true) {
        SZombieKilled event = { name, value, killType };
        m_zombieKilled.addElement(event);
    } else {
        CFlurry::LogCustomEvent(ZOMBSNIPER_KILL_ZOMBIE, CStrWChar(name.c_str()).GetWCharPtr(), value, killType);
    }
}

void CGameAnalytics::logSkillshot(XString name, int value, bool deferred)
{
    if (deferred == true) {
        SSkillshot event = { name, value };
        m_skillshot.addElement(event);
    } else {
        CFlurry::LogCustomEvent(ZOMBSNIPER_EVT_TYPE_SKILLSHOT, CStrWChar(name.c_str()).GetWCharPtr(), value, 0);
    }
}

void CGameAnalytics::logMenuAccessed(XString menu)
{
    CFlurry::LogCustomEvent(ZOMBSNIPER_MENU_ACCESSED, CStrWChar(menu.c_str()).GetWCharPtr(), 0, 0);
}

void CGameAnalytics::logCycleScoreAchieved(int cycle, int score, int value)
{
    XString info = XString::Format(L"%d", cycle);
    CFlurry::LogCustomEvent(ZOMBSNIPER_CYCLE_SCORE_ACHIEVED, CStrWChar(info.c_str()).GetWCharPtr(), score, value);
}

void CGameAnalytics::logAdShown(int value)
{
    XString info(L"1");
    CFlurry::LogCustomEvent(ZOMBSNIPER_EVT_TYPE_AD_SHOWN, CStrWChar(info.c_str()).GetWCharPtr(), value, 0);
}

void CGameAnalytics::logAdBonus(int value1, int value2)
{
    XString info(L"2");
    CFlurry::LogCustomEvent(ZOMBSNIPER_EVT_TYPE_AD_BONUS, CStrWChar(info.c_str()).GetWCharPtr(), value1, value2);
}

void CGameAnalytics::logSurvivalModeFinishedWave(int wave, int value)
{
    XString info(wave);
    CFlurry::LogCustomEvent(ZOMBSNIPER_SM_FINISHED_WAVE, CStrWChar(info.c_str()).GetWCharPtr(), value, 0);
}

void CGameAnalytics::logSurvivalModeZombiesKilled(int wave, int value)
{
    XString info(wave);
    CFlurry::LogCustomEvent(ZOMBSNIPER_SM_ZOMBIES_KILLED, CStrWChar(info.c_str()).GetWCharPtr(), value, 0);
}

void CGameAnalytics::logSurvivalModeXpEarned(int wave, int value)
{
    XString info(wave);
    CFlurry::LogCustomEvent(ZOMBSNIPER_SM_XP_EARNED, CStrWChar(info.c_str()).GetWCharPtr(), value, 0);
}

void CGameAnalytics::logSurvivalModeSoftEarned(int wave, int value)
{
    XString info(wave);
    CFlurry::LogCustomEvent(ZOMBSNIPER_SM_SOFT_EARNED, CStrWChar(info.c_str()).GetWCharPtr(), value, 0);
}

void CGameAnalytics::logSurvivalModeFinalScore(int wave, int value)
{
    XString info(wave);
    CFlurry::LogCustomEvent(ZOMBSNIPER_SM_FINAL_SCORE, CStrWChar(info.c_str()).GetWCharPtr(), value, 0);
}

void CGameAnalytics::logSurvivalModeShopEntered(int wave, bool entered)
{
    XString info(wave);
    CFlurry::LogCustomEvent(ZOMBSNIPER_SM_SHOP_ENTERED, CStrWChar(info.c_str()).GetWCharPtr(), entered, 0);
}

void CGameAnalytics::logSurvivalModeWeaponPurchased(int wave, int weaponId)
{
    XString info(wave);
    CFlurry::LogCustomEvent(ZOMBSNIPER_SM_WEAPON_PURCHASED, CStrWChar(info.c_str()).GetWCharPtr(), weaponId, 0);
}

void CGameAnalytics::logCivilianRevivePurchased()
{
    CFlurry::LogCustomEvent(ZOMBSNIPER_SM_REVIVE, CStrWChar().GetWCharPtr(), 0, 0);
}
