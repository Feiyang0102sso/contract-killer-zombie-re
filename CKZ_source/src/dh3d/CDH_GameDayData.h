// CDH_GameDayData.h - progress data of the day based game mode.
#ifndef CDH_GAMEDAYDATA_H
#define CDH_GAMEDAYDATA_H

#include "CDH_BasicGameData.h"
#include "CAchievements.h"

class CDH_GameDayData : public CDH_BasicGameData {
public:
    CDH_GameDayData();
    virtual void Serialize(DataOutputStream& stream);
    virtual void DeSerialize(JDataInputStream& stream);
    virtual ~CDH_GameDayData();
    virtual void ResetImpl();
    virtual void FinishGameImpl();
    virtual void StartGameImpl();

    DataOutputStream m_dayState;                                // 0x1c raw state of the day (saved as is)
    int m_field2c;                                              // 0x2c
    bool m_field30;                                             // 0x30
    Vector<CAchievements::achievements_type_t> m_achievements;  // 0x34 reached during the day
};

#endif
