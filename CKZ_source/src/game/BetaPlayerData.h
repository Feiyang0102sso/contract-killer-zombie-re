// BetaPlayerData.h - extra saved player data for beta builds (empty in the shipped game).
#ifndef BETAPLAYERDATA_H
#define BETAPLAYERDATA_H

#include "Serializable.h"

/** Beta-only player data; every method is empty in release (vtable: Serialize, DeSerialize). */
class BetaPlayerData : public Serializable {
public:
    BetaPlayerData();
    ~BetaPlayerData();
    virtual void Serialize(DataOutputStream& stream);
    virtual void DeSerialize(JDataInputStream& stream);
    void reset();
};

#endif
