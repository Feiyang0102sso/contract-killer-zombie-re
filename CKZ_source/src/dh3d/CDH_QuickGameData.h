// CDH_QuickGameData.h - progress data of the quick game mode.
#ifndef CDH_QUICKGAMEDATA_H
#define CDH_QUICKGAMEDATA_H

#include "CDH_BasicGameData.h"

class CDH_QuickGameData : public CDH_BasicGameData {
public:
    CDH_QuickGameData();
    virtual void Serialize(DataOutputStream& stream);
    virtual void DeSerialize(JDataInputStream& stream);
    virtual ~CDH_QuickGameData();
    virtual void ResetImpl();
    virtual void FinishGameImpl();
    virtual void StartGameImpl();

    unsigned char m_field1c;    // 0x1c
};

#endif
