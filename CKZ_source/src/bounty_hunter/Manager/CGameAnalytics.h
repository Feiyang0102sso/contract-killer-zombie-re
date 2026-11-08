// CGameAnalytics.h - analytics events of the game (Flurry custom events), the push token and the
// kill / skillshot events that are collected during a mission and sent afterwards.
#ifndef CGAMEANALYTICS_H
#define CGAMEANALYTICS_H

#include "xString.h"
#include "Vector_ss.h"
#include "CStrChar.h"
#include "CMission.h"

class CGameAnalytics {
public:
    /** Currencies reported by logTotalCurrency. */
    enum ECurrencyType {
        CURRENCY_SOFT = 0,
        CURRENCY_HARD = 1
    };

    /** Result of a one-time offer / daily deal dialog (values not known yet). */
    enum EOTOResult {
        OTO_RESULT_NONE = 0
    };

    /** How a zombie was killed (values not known yet). */
    enum EKillType {
        KILL_TYPE_NONE = 0
    };

    /** A deferred "zombie killed" event. */
    struct SZombieKilled {
        XString m_name;         // 0x00
        int m_value;            // 0x04
        EKillType m_killType;   // 0x08
    };

    /** A deferred "skillshot" event. */
    struct SSkillshot {
        XString m_name;         // 0x00
        int m_value;            // 0x04
    };

    static void SendPushTokenImpl(const char* token);
    /** Send the push token stored by SendPushStatistics (once). */
    static void SendDeferredPushToken();
    /** Keep `token` for SendDeferredPushToken. */
    static void SendPushStatistics(const char* token);
    static void logAppStart();
    static void logAppStop();
    static void logAppBackground();
    static void logAppForeground();
    static void logPushNotification(bool inForeground);
    static void logCurrencyChange(XString source, int amount, int total, ECurrencyType type);
    static void logTotalCurrency(int playerId, int amount, ECurrencyType type);
    static void logIAPBuyClicked(XString item, int price, ECurrencyType type);
    static void logIAPPurchaseSuccessful(XString item, int price, ECurrencyType type);
    static void logIAPAfterFail(XString item);
    static void logRecommendedWeaponPurchase(XString item, int weaponId);
    static void logIAPPurchaseCanceled(XString item, int price, ECurrencyType type);
    static void logIAPPurchaseFailed(XString item, int price, ECurrencyType type);
    static void logLevelGained(int level);
    static void logDailyBonus(XString bonus, int day);
    static void logOneTimeOfferResult(const XString& item, int price, EOTOResult result);
    static void logDailyDealResult(const XString& item, int price, EOTOResult result);
    static void logTutorialStepCompleted(int step);
    static void logMissionStarted(XString mission, CMission::EMissionDifficulty difficulty);
    static void logMissionCompleted(XString mission, int value1, int value2);
    static void logMissionCompleteScore(XString mission, int score, int value);
    static void logHelicopterMissionCompleted(int mission, int value);
    static void logMissionQuit(XString mission, int value);
    static void logMissionFailed(XString mission, int value1, int value2);
    static void logSoundSwitch(bool enabled);
    static void logMusicSwitch(bool enabled);
    static void logPushSwitch(bool enabled);
    static void logItemViewed(XString item, int price, ECurrencyType type);
    static void logBuyItemClicked(XString item, int price, ECurrencyType type);
    static void logItemPurchased(XString item, int price, ECurrencyType type);
    static void logItemPurchaseCancelled(XString item, int price, ECurrencyType type);
    static void logItemEquipped(XString item);
    /** Send the collected kill / skillshot events. */
    static void flushDeferredEvents();
    /** `deferred`: only collect the event (sent by flushDeferredEvents). */
    static void logZombieKilled(XString name, int value, EKillType killType, bool deferred);
    static void logSkillshot(XString name, int value, bool deferred);
    static void logMenuAccessed(XString menu);
    static void logCycleScoreAchieved(int cycle, int score, int value);
    static void logAdShown(int value);
    static void logAdBonus(int value1, int value2);
    static void logSurvivalModeFinishedWave(int wave, int value);
    static void logSurvivalModeZombiesKilled(int wave, int value);
    static void logSurvivalModeXpEarned(int wave, int value);
    static void logSurvivalModeSoftEarned(int wave, int value);
    static void logSurvivalModeFinalScore(int wave, int value);
    static void logSurvivalModeShopEntered(int wave, bool entered);
    static void logSurvivalModeWeaponPurchased(int wave, int weaponId);
    static void logCivilianRevivePurchased();

    static com::glu::platform::components::CStrChar m_deferredToken;
    static com::glu::platform::components::CStrChar m_lastDeferredToken;
    static Vector<SZombieKilled> m_zombieKilled;
    static Vector<SSkillshot> m_skillshot;
    /** Set while a purchase follows a failed mission (logIAPPurchaseSuccessful also sends logIAPAfterFail). */
    static bool g_logMissionFailure;
};

#endif
