// CBalanceManager.h - balance setups of res/common/configs/balance.xml (one per device class).
#ifndef CBALANCEMANAGER_H
#define CBALANCEMANAGER_H

#include <stddef.h>
#include "xString.h"
#include "Vector_ss.h"

class TiXmlNode;

class CBalanceManager {
public:
    /** Multipliers of one <balance_setup>. */
    struct SBalanceSetup {
        XString m_name;                     // 0x00 low_performance / high_performance
        int m_maxSceneUnitsCount;           // 0x04
        float m_waveSizeMultiplier;         // 0x08
        float m_zombieHPMultiplier;         // 0x0c
        float m_zombieDamageMultiplier;     // 0x10
        float m_civilianHPMultiplier;       // 0x14
        float m_civilianDamageMultiplier;   // 0x18
    };

    CBalanceManager();
    ~CBalanceManager();

    void Parse(TiXmlNode* node);
    /** Pick the setup of this device class (the first one when it is missing). */
    void ChooseCurrentBalanceSetup();

    Vector<SBalanceSetup> m_setups;     // 0x00
    int m_currentSetup;                 // 0x10
};

#endif
