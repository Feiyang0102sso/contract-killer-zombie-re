// PROVISIONAL (created by b048): owner src/bounty_hunter/Manager/CMission.cpp not written yet
// CMission.h - one mission; only what src/dh3d uses is declared.
// b078b: complete 0xb4 layout from construction, parsing and reward accessors.
#ifndef CMISSION_H
#define CMISSION_H

#include "xString.h"  // added by b055x
#include "PlayerStats.h"
#include "CScriptManager.h"

class TiXmlNode;
class CZombieDialogWindow;

class CMission {
public:
    /** Difficulty of a mission (reported by CGameAnalytics::logMissionStarted; values not known yet). */  // added by b077
    enum EMissionDifficulty {  // added by b077
        DIFFICULTY_ANY = 0,
        DIFFICULTY_EASY = 1,  // added by b077; b078b corrects the provisional value 0
        DIFFICULTY_MEDIUM = 2,
        DIFFICULTY_HARD = 3
    };  // added by b077
    /** Config codes from the original MISSION_TYPE_MAP, not expanded guessed names. */
    enum EMissionType {
        MISSION_ANY = 0, MISSION_KA = 1, MISSION_IC = 2, MISSION_CB = 3,
        MISSION_ST = 4, MISSION_QU = 5, MISSION_EM = 6
    };

    CMission(TiXmlNode* node);
    void Parse(TiXmlNode* node);
    void ParseInfo(TiXmlNode* node);
    static EMissionType StringToMissionType(const XString& name);
    static XString MissionTypeToString(EMissionType type);
    int GetRewardPeople() const;
    int GetRewardProvision() const;
    bool IsAutopickable() const;
    bool IsSuitableForLevel(int level) const;
    void OnMissionCompleted(int money, int xp) const;
    int GetBaseRewardMoney() const;
    int GetBaseRewardXP() const;
    int GetMapIconAnimation() const;
    int GetDialogDifficultyIconAnimation() const;
    XString GetDifficultyString() const;
    int GetMapIconDifficultyAnimation() const;
    bool CheckMissionConditions(CZombieDialogWindow** dialog) const;

    /** Name of the mission (XString at 0x00, from CGameScreen::Init). */  // added by b055x
    const XString& GetName() const  // added by b055x
    {  // added by b055x
        return m_name;  // added by b055x
    }  // added by b055x
    /** Analytics and state updates when the mission starts. */  // added by b055x
    void OnMissionStarted() const;  // added by b055x

    // Former m_data00[0x24]: layout not known yet (b048).
    XString m_name;                 // 0x00
    XString m_sceneSetupName;       // 0x04
    XString m_zombieMissionName;    // 0x08
    XString m_locationName;         // 0x0c
    XString m_screenName;           // 0x10
    XString m_description;          // 0x14
    XString m_descriptionWithFriend; // 0x18 config key, not resolved on load
    XString m_descriptionSound;     // 0x1c
    XString m_successSound;         // 0x20
    EMissionType m_type;            // 0x24 (4: the loading screen shows the after game statistics)
    bool m_autopickable;            // 0x28
    int m_minLevel;                 // 0x2c
    int m_maxLevel;                 // 0x30
    PlayerStats m_reward;           // 0x34 ten encrypted stats (0x50 bytes)
    int m_minPeople;                // 0x84
    int m_minProvision;             // 0x88
    EMissionDifficulty m_difficulty; // 0x8c
    bool m_withFriend;              // 0x90 attribute presence, not its boolean value
    XString m_scriptName;           // 0x94
    Vector<CScriptManager::SScriptParam> m_scriptParams; // 0x98
    float m_tutorialMultiplier;     // 0xa8 2 for tutorial missions 2/3/4, otherwise 1
    int m_numWaves;                 // 0xac
    XString m_waveFilter;           // 0xb0
};

#endif
