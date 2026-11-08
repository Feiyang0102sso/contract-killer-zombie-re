// PROVISIONAL (created by b057x): owner src/bounty_hunter/Player/CBH_Player.cpp not written yet
// CBH_Player.h - the player (singleton); only what src/general uses.
#ifndef CBH_PLAYER_H
#define CBH_PLAYER_H

#include "CPlayerGameStats.h"
#include "CMissionsPool.h"
#include "PlayerStats.h"
#include "HashTable.h"
#include "CSingleton.h"
class CAmmo;
// b081j: full Player recovery; the PROVISIONAL above is a historical record.
class CMission;  // added by b078b

/** Statistics read with CBH_Player::GetStats (only the values src/general uses are named). */
enum PLAYER_STATS_TYPE {
    PLAYER_STATS_SOFT_CURRENCY = 2,
    PLAYER_STATS_HARD_CURRENCY = 4,
    PLAYER_STATS_PROVISION = 6,  // added by b078b
    PLAYER_STATS_PEOPLE = 8  // added by b078b
};

class CBH_Player : public com::glu::platform::components::CSingleton {
public:
    CBH_Player();
    void Init();
    void Load();
    void AddEnergy(int amount, bool allowOverflow);
    unsigned int GetGameTime() const;
    void SetPlayerStartParams();
    void SaveStats(TiXmlElement* element);
    void SaveQuests(TiXmlElement* element);
    void SaveNotifications(TiXmlElement* element);
    void SaveTjPoints(TiXmlElement* element);
    void SaveFriendGiftLasDay(TiXmlElement* element);
    void LoadStats(TiXmlElement* element);
    void LoadQuests(TiXmlElement* element);
    void LoadNotifications(TiXmlElement* element);
    void LoadTjPoints(TiXmlElement* element);
    void LoadFriendGiftLasDay(TiXmlElement* element);
    /** The player, created on first use. */
    static CBH_Player* GetInstance();

    virtual ~CBH_Player();

    int GetStats(PLAYER_STATS_TYPE type) const;
    void ReleaseResources();
    /** Add `seconds` of play time. */
    void AddGameTime(unsigned int seconds);
    void AddXp(int amount);  // added by b047x
    int GetXpForNextLvl();       // Original 1436d0: non-const, reads level/manager repeatedly.
    int GetXpForCurrentLvl();    // Original 143704.
    int GetMaxEnergy();         // Original 143090.
    /** CStatsBonusItem 0x13b3d0: type, amount, false, true. */  // added by b079
    void AddStats(PLAYER_STATS_TYPE type, int amount, bool notify, bool save);  // added by b079
    void AddCredits(int amount, const wchar_t* source);  // added by b047x (hard currency; source text)
    void ReduceCredits(int amount, const wchar_t* source); // b067h: original 1436a4, non-const, writes Encrypted and calls Save(true).
    void AddMoney(int amount);  // added by b047x (soft currency)
    void ReduceMoney(int amount); // b074i: original 143518, the real external charge call from CItem::Buy.
    void AddPeople(int amount);  // added by b078b
    void AddMedkits(int amount); // b074i: original 143648.
    bool TryMakeFiveDaysBonus(bool* newDay); // Original 143be4, the returned r6 is only 0/1.
    /** GPSMap daily teaser date cache: original 1175e6/1175f6 read/write +88. */
    int& FiveDaysTeaserDay() { return FieldAt(0x88); }
    void AddProvision(int amount);  // added by b078b
    void ReducePeople(int amount);  // added by b078b
    void ReduceProvision(int amount);  // added by b078b
    void OnMissionCompleted(const CMission* mission);  // added by b078b
    /** Original non-const symbols at 0x1439a0/0x1439c4; used by CGameApp. */  // added by b063
    bool IsPushNotificationHandled(int identity);  // added by b063
    void OnPushNotificationHandled(int identity);  // added by b063
    void OnQuestCompleted(int id); // b074i: original 143980, Save(true) after appending to the Vector.
    // b074i: the three real external notifications of Progress RegisterGameEvent11f284.
    void OnEnemyKilled();        // 0x143a24
    void OnEnemyTranquillized();  // 0x143a40
    void OnEnemyEscaped();       // 0x143a5c
    bool IsQuestCompleted(int id);  // added by b078c (symbol constness checked against CBH_Player.o)
    int GetTutorialState() const;  // added by b078c
    /** Original task filtering reads +8 as the level; retain the older field name for compatibility. */  // added by b078b
    int GetLevel() const { return m_id; }
    /** Save the player profile. */  // added by b048
    void Save(bool force);  // added by b048
    void Update();
    void RefreshMissionsPool(); // Original 144968; the external full owner is still to be recovered.
    /** Original GPSGame 11afde/11b006 access the real CMissionsPool via 204/210. */
    CMissionsPool& MissionsPool() { return *(CMissionsPool*)((char*)this + 0x204); }
    bool& RateMeShown() { return *(bool*)((char*)this + 0x8c); }  // added by b048
    /** Game mode of the player (2: hunting; read by CGPSMapScreen at 0x5c). */  // added by b048
    int GetGameMode() const  // added by b048
    {  // added by b048
        return m_lastHuntResult; // Historical m_data0c+5c layout access  // added by b048
    }  // added by b048
    /** Daily bonuses collected in a row (read by CAchievementManager at 0x7c). */  // added by b077
    int GetDailyBonusesInRow() const  // added by b077
    {  // added by b077
        return m_dailyBonusesInRow; // Historical m_data0c+7c layout access  // added by b077
    }  // added by b077

    /** Tutorial progress of the player (1: started from CGameScreen::Init). */  // added by b055x
    void SetTutorialState(int state);  // added by b055x
    /** Fields used by CGameScreen (0x5c game mode, 0x60, 0x64 quest id, 0xe0 started missions, 0x1e0 sound flag). */  // added by b055x
    int& FieldAt(int offset)  // added by b055x
    {  // added by b055x
        return *(int*)((char*)this + offset);  // added by b055x
    }  // added by b055x
    /** Setter (the object is taken before the value, as in the original). */  // added by b055x
    void SetFieldAt(int offset, int value)  // added by b055x
    {  // added by b055x
        *(int*)((char*)this + offset) = value;  // added by b055x
    }  // added by b055x

    static CBH_Player* m_staticInstance;    // NULL until created
    /** Collect the values of the "app stop" analytics event (m_Last*). */  // added by b077
    static void CollectShutdownAnalytics();  // added by b077
    static int m_LastCycleNumber;  // added by b077
    static int m_LastSecondsSinceAppStart;  // added by b077
    static int m_LastMissionsSinceAppStart;  // added by b077

    // Historical int m_field04(+4), owned by the real CSingleton::m_classId.
    int m_id;                               // 0x08 reported with the analytics
    // Historical unsigned char m_data0c[0xec - 0x0c], layout not known yet; now converted to real members.
    PlayerStats m_stats;                    // 0x0c
    int m_lastHuntResult;                   // 0x5c
    int m_lastMapLocationId;                // 0x60
    int m_lastQuestId;                      // 0x64
    int m_stageMissionsCompleted;           // 0x68
    int m_tutorialState;                    // 0x6c
    int m_lastEnergyUpdateTime;             // 0x70
    unsigned int m_gameTime;                // 0x74
    unsigned int m_lastDailyBonusTime;      // 0x78
    int m_dailyBonusesInRow;                // 0x7c
    unsigned int m_lastFiveDaysBonusTime;   // 0x80
    int m_activeFiveDaysBonusDay;           // 0x84
    int m_fiveDaysTeaserDay;                // 0x88
    bool m_rateMeShown;                     // 0x8c
    int m_cycles;                          // 0x90
    int m_lastGameVersionPlayed;            // 0x94
    bool m_newPlayer;                       // 0x98
    HashTable<XString, int> m_tapjoyPoints;  // 0x9c
    HashTable<XString, int> m_friendGiftDays;// 0xa8
    Vector<int> m_completedQuests;          // 0xb4
    Vector<int> m_handledNotifications;     // 0xc4
    int m_unresolvedD4;                     // 0xd4, the original only initializes it to -1
    bool m_authenticationStarted;           // 0xd8
    bool m_playerNameReady;                 // 0xd9
    XString m_playerName;                   // 0xdc
    int m_missionsStarted;                 // 0xe0
    int m_energyRestoreSeconds;             // 0xe4, written by Init
    int m_energyRemainingSeconds;           // 0xe8, not written by the original ctor
    CPlayerGameStats m_gameStats;           // 0xec
    CMissionsPool m_missionsPool;           // 0x204
    HashTable<HashTableKey<const CAmmo*>, int> m_ammoCounts; // 0x214
    // 16B POD vector that the original only constructs/destroys; element semantics not yet proven by a consumer.
    Vector<unsigned int> m_unresolved220;   // 0x220
};

#endif
