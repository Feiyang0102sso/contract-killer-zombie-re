// PROVISIONAL (created by b057x): owner src/bounty_hunter/Manager/CZombieMissionManager.cpp not written yet
// CZombieMissionManager.h - zombie missions; only what src/general/App.cpp uses is declared.
#ifndef CZOMBIEMISSIONMANAGER_H
#define CZOMBIEMISSIONMANAGER_H

#include <stddef.h>
#include "Vector_ss.h"
#include "CMission.h"
class CZombieMission;

class CZombieMissionManager {
public:
    CZombieMissionManager();
    virtual ~CZombieMissionManager();
    CZombieMission* Generate(CMission::EMissionType type, CMission::EMissionDifficulty difficulty,
                             int waveCount, const XString& filter);

    // Former m_data04[0x14 - 0x4]: layout not known yet (size from App.cpp).
    Vector<CZombieMission*> m_missions;  // 0x04; factory does not append its returned object
};

#endif
