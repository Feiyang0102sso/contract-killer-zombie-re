// PROVISIONAL (created by b057x): owner src/bounty_hunter/CProgressSystem.cpp not written yet
// CProgressSystem.h - player level progression; only what src/general/App.cpp uses is declared.
// b074i: the original placeholder history is kept; now restores the real 0x14 owner and the 0x344 level records.
#ifndef CPROGRESSSYSTEM_H
#define CPROGRESSSYSTEM_H

#include <stddef.h>
#include "Vector_ss.h"
#include "CMission.h"  // added by b078b

class CUnit;
class TiXmlNode;

class CProgressSystem {
public:
    CProgressSystem();
    // Original 11f284 dispatches to Player's Killed/Tranquillized/Escaped; the three numbers can be verified from the call targets.
    enum EGameEvent { EVENT_ENEMY_KILLED = 0, EVENT_ENEMY_TRANQUILLIZED = 1, EVENT_ENEMY_ESCAPED = 2 };
    void RegisterGameEvent(EGameEvent event, int experience, int headshot);
    static int CalcXpForKilledUnit(CUnit* unit); // original 11f35c; real external owner, not fake-implemented here for now.
    int GetTaggingTimeMS(); // b067h: original 11f360, returns signed milliseconds, non-const.
    int GetMissionRewardMoney(CMission::EMissionType type, CMission::EMissionDifficulty difficulty, int people, int provision);  // added by b078b
    int GetMissionRewardXP(CMission::EMissionType type, CMission::EMissionDifficulty difficulty, int people, int provision);  // added by b078b
    void ParseLevels(TiXmlNode* node);
    int GetMaxLevel() const;
    int GetMaxXPForLevel(int level) const;
    int GetMaxXP() const;
    int GetMaxEnergyForLevel(int level) const;
    int GetLevel(int xp) const;
    int GetRecommendedWeaponId(CMission::EMissionType type, XString mapName);
    /** One level (contents not known). */ // Historical placeholder note.
    // Original 11e95e/11f5dc both use 0x344 as the element size; indexed by 7 mission types and 4 difficulties.

    struct SLevelInfo {
        // Original 11e950 only writes valid; there is a user default constructor, the original Vector copy does no pre-zeroing, and other fields stay indeterminate.
        SLevelInfo() : m_present(false) {}
        // int m_value; // historical 4B fake placeholder, the real members are below.
        bool m_present;                 // 0x000, a missing level only writes false to this byte.
        int m_xpThreshold;              // 0x004
        int m_maxEnergy;                // 0x008
        int m_xpBase[7][4];             // 0x00c
        int m_xpPerCivilian[7][4];      // 0x07c
        int m_xpPerProvision[7][4];     // 0x0ec, not fully zeroed in the original, only the heli row is written.
        int m_moneyBase[7][4];          // 0x15c
        int m_moneyPerCivilian[7][4];   // 0x1cc
        int m_moneyPerProvision[7][4];  // 0x23c, not fully zeroed in the original.
        int m_energy[7][4];             // 0x2ac
        int m_field31c;                 // Neither read nor written by this owner; kept as an uninitialized scalar.
        int m_recommendedWeapons[7];    // 0x320
        int m_recommendedICHeli;        // 0x33c
        int m_recommendedKAHeli;        // 0x340
    };

    Vector<SLevelInfo> m_levels;    // 0x00
    // int m_field10; // historical scalar type; the original writes 3f000000, really a float.
    float m_field10;                // 0x10, written as 0.5f after ParseLevels.
};

#endif
