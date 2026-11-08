// PROVISIONAL (created by b057x): owner src/bounty_hunter/Player/CPlayerGameStats.cpp not written yet
// CPlayerGameStats.h - play statistics of the player; only what src/general uses.
#ifndef CPLAYERGAMESTATS_H
#define CPLAYERGAMESTATS_H

#include "CVector.h"
class TiXmlElement;
class CDH_Weapon;
class CGrenade;
class CMission;

// b081j: the old PROVISIONAL notes below are kept as history; this owner restores the real vptr-less 118-byte statistics object.
class CPlayerGameStats {
public:
    CPlayerGameStats();
    ~CPlayerGameStats();
    void Reset(bool keepProgress);
    void Save(TiXmlElement* parent);
    void Load(TiXmlElement* parent);
    int GetTimeSinceAppStart();

    static const int PER_ZOMBIE_KILL_SCORE;
    static const int PER_SKILLSHOT_SCORE;
    static const int PER_HEADSHOT_SCORE;
    static const int PER_BOX_SCORE;
    static const int PER_CIVILIAN_SCORE;
    static const int PER_HELI_MISSION_SCORE;

    // The original five derived vtables only have the two destructor slots and Check; the trailing zero is the boundary between tables.
    struct Skillshot {
        Skillshot(int score) : m_score(score), m_weapon(0), m_shotCount(0),
            m_hits(0), m_headshots(0), m_goreshots(0) {}
        // Original 146678 and the other derived D1 are only bx lr; the base destructor indeed has no side effects.
        virtual ~Skillshot() {}
        virtual bool CheckSkillshot(const CDH_Weapon*, const CGrenade*, int, int, int, int, int) = 0;
        void OnShotEnd();
        int m_score;                        // +04
        const CDH_Weapon* m_weapon;          // +08
        const CGrenade* m_grenade;           // +0c, not initialized by the original constructor
        int m_shotCount;                     // +10
        int m_kills;                         // +14, not initialized by the original constructor
        int m_hits;                          // +18
        int m_headshots;                     // +1c
        int m_goreshots;                     // +20
    };
    struct QuantityShot : Skillshot {
        QuantityShot(int threshold, int windowMS, unsigned int command, int score)
            : Skillshot(score)
        {
            // Original container initialization precedes these scalar stores.
            m_threshold = threshold;
            m_windowMS = windowMS;
            m_command = command;
            m_times.EnsureCapacity(threshold);
        }
        virtual bool CheckSkillshot(const CDH_Weapon*, const CGrenade*, int, int, int, int, int);
        int m_threshold;
        int m_windowMS;
        unsigned int m_command;
        com::glu::platform::core::CVector<int> m_times;
    };
    struct Headbanger : Skillshot {
        Headbanger(int threshold, unsigned int command, int score)
            : Skillshot(score), m_threshold(threshold), m_command(command), m_count(0) {}
        virtual bool CheckSkillshot(const CDH_Weapon*, const CGrenade*, int, int, int, int, int);
        int m_threshold;
        unsigned int m_command;
        int m_count;
    };
    struct Brainiac : Skillshot {
        Brainiac(int threshold, unsigned int command, int score)
            : Skillshot(score), m_threshold(threshold), m_command(command), m_count(0) {}
        virtual bool CheckSkillshot(const CDH_Weapon*, const CGrenade*, int, int, int, int, int);
        int m_threshold;
        unsigned int m_command;
        int m_count;
    };
    struct IronHand : Skillshot {
        IronHand(int threshold, unsigned int command, int score)
            : Skillshot(score), m_threshold(threshold), m_command(command), m_count(0) {}
        virtual bool CheckSkillshot(const CDH_Weapon*, const CGrenade*, int, int, int, int, int);
        int m_threshold;
        unsigned int m_command;
        int m_count;
    };
    struct SevenBlows : Skillshot {
        SevenBlows(int threshold, int windowMS, unsigned int command, int score)
            : Skillshot(score)
        {
            // Original container initialization precedes these scalar stores.
            m_threshold = threshold;
            m_windowMS = windowMS;
            m_command = command;
        }
        virtual bool CheckSkillshot(const CDH_Weapon*, const CGrenade*, int, int, int, int, int);
        int m_threshold;
        int m_windowMS;
        unsigned int m_command;
        com::glu::platform::core::CVector<int> m_times;
    };
    void NotifyAppForeground();
    // b074i: 11ca40/11cccc real external calls; the implementation belongs to the full statistics owner.
    void AddTotalMissions(int count);             // 0x145de0
    void NotifyMissionWin(const CMission* mission); // 0x145ef8
    void AddCiviliansResqued(int count);          // 0x145df8, keeps the original symbol spelling
    void AddProvisionScrounged(int count);        // 0x145e08
    void AddCompletedMissions(int count);         // 0x145dd8
    void NotifyArcadeScoreChanged(int score);     // 0x146504
    // b067h: original external call from Game; the concrete body still belongs to the statistics owner.
    void NotifyMissionBegin(const CMission* mission); // 0x145f40
    void NotifySurvivalWave(int wave); // 0x146514
    void NotifySurvivalWaveForEvelynTakesNoDamage(int wave); // 0x146524
    void NotifyThrow(); // 0x1464f8
    void NotifySurvivalGoal(); // 0x146534
    void AddEasyCopterMissions(int count); // 0x145e54
    void AddMediumCopterMissions(int count); // 0x145e60
    void AddHardCopterMissions(int count); // 0x145e6c
    // b070g: real cross-owner call from the original Unit/Controller; the implementation still belongs to CPlayerGameStats.cpp.
    void NotifyShotBegin(const CDH_Weapon* weapon, const CGrenade* grenade, unsigned int count); // 0x146288
    void NotifyShotEnd(); // 0x14630c
    void NotifyKill();
    void NotifyHit();
    void NotifyHeadshot();
    void NotifyCivilianHeadshot();
    void NotifyRunningHeadshot();
    void NotifyGoreshot();
    void NotifyCivilianKilled();
    void AddNumHealingGunShots(int count);
    void AddZombiesKilledByEvelyn(int count);
    void AddBarrelBlow(int count);
    void AddWeaponsEarned(int count);  // added by b055x
    void AddBoxesSent(int count);  // added by b078b
    void AddPeopleSent(int count);  // added by b078b
    /** Six current-mission result fields, written at 0x12c848..0x12c860. */  // added by b078b
    void RecordMissionRewards(int provision, int people, int peopleSent, int boxesSent, int money, int xp)
    {
        int* values = &m_currentProvision;
        values[0] = provision;
        values[1] = people;
        values[2] = peopleSent;
        values[3] = boxesSent;
        values[4] = money;
        values[5] = xp;
    }
    /** Score of the current score cycle. */  // added by b077
    void NotifyNewCycleBegin(); // Original 145e78.
    int GetThisCycleScore();  // added by b077
    /** Seconds since the app came to the foreground. */  // added by b077
    int GetTimeSinceAppForeground();  // added by b077
    // Counter getters (b145): CheckAllAchievements inlines one call-free getter per counter read;
    // the original inline order of its five title checks (stack slots of their name copies) needs them.
    int GetBarrelsDestroyed() const { return m_barrelsDestroyed; }
    int GetCiviliansRescued() const { return m_civiliansRescued; }
    int GetEmEvelynProtected() const { return m_emEvelynProtected; }
    int GetEmGoalReached() const { return m_emGoalReached; }
    int GetEmWavesSurvived() const { return m_emWavesSurvived; }
    int GetHandgunHeadshots() const { return m_handgunHeadshots; }
    int GetHeadshotAchieved() const { return m_headshotAchieved; }
    int GetHeadshots() const { return m_headshots; }
    int GetHealingGunUses() const { return m_healingGunUses; }
    int GetLauncherKills() const { return m_launcherKills; }
    int GetMissionsCompleted() const { return m_missionsCompleted; }
    int GetProvisionsCollected() const { return m_provisionsCollected; }
    int GetRevolverKills() const { return m_revolverKills; }
    int GetRoundsFired() const { return m_roundsFired; }
    int GetShotgunKills() const { return m_shotgunKills; }
    int GetSmgKills() const { return m_smgKills; }
    int GetSniperKills() const { return m_sniperKills; }
    int GetWeaponsBought() const { return m_weaponsBought; }
    int GetZombiesKilled() const { return m_zombiesKilled; }
    int GetHelicoptersSent(int index) const { return m_helicoptersSent[index]; }

    // Counters read by CAchievementManager::CheckAllAchievements (offsets from CBH_Player + 0xec;  // added by b077
    // names from the achievements that use them, the rest of the layout is not known yet).  // added by b077
    // Historical thin header: unsigned char m_data00[0x18];
    int m_currentProvision;
    int m_currentPeople;
    int m_currentPeopleSent;
    int m_currentBoxesSent;
    int m_currentMoney;
    int m_currentXP;   // 0x00  // added by b077
    int m_missionsCompleted;        // 0x18  // added by b077
    // Historical thin header: unsigned char m_data1c[0x20 - 0x1c];
    int m_totalMissions;  // added by b077
    int m_cycleNumber;              // 0x20 (CGameAnalytics::logAppBackground)  // added by b077
    int m_zombiesKilled;            // 0x24  // added by b077
    int m_field28;                  // 0x28  // added by b077
    int m_civiliansRescued;         // 0x2c  // added by b077
    int m_field30;                  // 0x30  // added by b077
    int m_roundsFired;              // 0x34  // added by b077
    int m_shotsHit;                 // 0x38 (accuracy board of CGCLeaderboardsManager)  // added by b077
    int m_headshots;                // 0x3c  // added by b077
    int m_field40;                  // 0x40  // added by b077
    int m_barrelsDestroyed;         // 0x44  // added by b077
    int m_headshotAchieved;         // 0x48 "get a headshot"  // added by b077
    int m_field4c;                  // 0x4c  // added by b077
    int m_weaponsBought;            // 0x50  // added by b077
    int m_field54;                  // 0x54  // added by b077
    int m_healingGunUses;           // 0x58  // added by b077
    int m_shotgunKills;             // 0x5c  // added by b077
    int m_smgKills;                 // 0x60  // added by b077
    int m_sniperKills;              // 0x64  // added by b077
    int m_revolverKills;            // 0x68  // added by b077
    int m_launcherKills;            // 0x6c  // added by b077
    int m_handgunHeadshots;         // 0x70  // added by b077
    int m_provisionsCollected;      // 0x74  // added by b077
    // Historical thin header: unsigned char m_data78[0xb0 - 0x78];
    int m_skillshotsDone;            // +78
    int m_boxesSent;
    int m_peopleSent;
    int m_currentMissionKills;       // +84
    int m_currentMissionSkillshots;
    int m_currentMissionHeadshots;
    int m_zombiesKilledThisCycle;    // +90
    int m_skillshotsThisCycle;
    int m_headshotsThisCycle;
    int m_carryBoxMissionsThisCycle;
    int m_peopleMissionsThisCycle;
    int m_helicopterMissionsThisCycle;
    int m_boxesSentThisCycle;
    int m_peopleSentThisCycle;  // added by b077
    int m_helicoptersSent[3];       // 0xb0  // added by b077
    int m_totalScore;               // 0xbc  // added by b077
    // Historical thin header: unsigned char m_datac0[0xc8 - 0xc0];
    int m_throws;
    int m_throwKills;  // added by b077
    int m_survivalModeScore;        // 0xc8 (survival board of CGCLeaderboardsManager)  // added by b077
    int m_emWavesSurvived;          // 0xcc endless mode  // added by b077
    int m_emEvelynProtected;        // 0xd0 (achievement value is this - 1)  // added by b077
    int m_emGoalReached;            // 0xd4  // added by b077
    // Historical thin header: unsigned char m_datad8[0xf8 - 0xd8];
    int m_shotWeaponType;            // +d8
    int m_shotCount;
    int m_shotHits;
    int m_shotHeadshots;
    int m_shotGoreshots;
    unsigned int m_appStartSeconds;
    unsigned int m_foregroundSeconds;
    int m_missionsSinceAppStart;  // added by b077
    int m_missionsSinceForeground;  // 0xf8 (CGameAnalytics::logAppBackground)  // added by b077
    int m_currentMissionCiviliansKilled; // +fc
    com::glu::platform::core::CVector<Skillshot*> m_skillshots; // +100, 18-byte platform container
};

#endif
