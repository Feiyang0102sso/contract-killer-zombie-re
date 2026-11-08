// CDH_PlayerData.h - saved progress of the player (skills, achievements, data of every game mode).
#ifndef CDH_PLAYERDATA_H
#define CDH_PLAYERDATA_H

#include "Serializable.h"
#include "CAchievements.h"

class CDH_BasicGameData;
class CDH_QuickGameData;
class CDH_GameDayData;
class BetaPlayerData;

/** Game mode (values not known). */
enum game_type_t {
    GAME_TYPE_QUICK = 0
};

class CDH_PlayerData : public Serializable {
public:
    /** Skill index (values not known). */
    enum skill_type_t {
        SKILL_COUNT = 5
    };

    CDH_PlayerData();
    ~CDH_PlayerData();

    virtual void Serialize(DataOutputStream& stream);
    virtual void DeSerialize(JDataInputStream& stream);

    /** Back to a new player (the game mode data objects are kept). */
    void reset();
    /** Skill level as a factor (level / 5). */
    float GetSkillLevelCoef(skill_type_t skill) const;
    /** Data of game mode `type` (every mode shares the quick game data). */
    CDH_BasicGameData* GetGameData(game_type_t type);

    int m_skillLevels[SKILL_COUNT];         // 0x04 only the low byte is used as the level
    int m_field18[5];                       // 0x18 not touched here
    unsigned char m_field2c;                // 0x2c
    CAchievements m_achievements;           // 0x30
    CDH_QuickGameData* m_quickGameData;     // 0x54
    CDH_QuickGameData* m_quickGameData2;    // 0x58
    CDH_GameDayData* m_gameDayData;         // 0x5c
    int m_field60[7];                       // 0x60
    unsigned char m_field7c;                // 0x7c
    int m_field80;                          // 0x80 not touched here
    int m_field84;                          // 0x84
    int m_field88;                          // 0x88
    int m_field8c;                          // 0x8c
    unsigned char m_field90;                // 0x90
    int m_field94;                          // 0x94 200000 by default
    int m_field98;                          // 0x98 10000 by default
    int m_field9c;                          // 0x9c 15 by default
    int m_fielda0;                          // 0xa0 not touched here
    bool m_fielda4;                         // 0xa4
    float m_fielda8;                        // 0xa8 1.0 by default
    float m_fieldac;  // changed by b049: vldr + vadd.f32 at 0x168ac8, no integer conversion                          // 0xac
    BetaPlayerData* m_betaData;             // 0xb0
};

#endif
