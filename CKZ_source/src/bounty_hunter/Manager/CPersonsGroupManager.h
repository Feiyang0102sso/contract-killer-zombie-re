// PROVISIONAL (created by b057x): owner src/bounty_hunter/Manager/CPersonsGroupManager.cpp not written yet
// CPersonsGroupManager.h - groups of persons; only what src/general/App.cpp uses is declared.
// b078b: complete manager and group/set/spawn records recovered from this unit.
#ifndef CPERSONSGROUPMANAGER_H
#define CPERSONSGROUPMANAGER_H

#include <stddef.h>
#include "xString.h"
#include "Vector_ss.h"
#include "CWaveManager.h"
class TiXmlNode;

/** One generated person; the two wave multipliers' individual meanings remain unresolved. */
struct SSpawnablePerson {
    SSpawnablePerson() : m_seed(0), m_waveMultiplier1(1.0f), m_waveMultiplier2(1.0f) {}
    /** The original by-value constructor at 0xf83bc assigns after both default strings exist. */
    SSpawnablePerson(XString typeName, unsigned int seed, XString point)
    {
        m_typeName = typeName;
        m_seed = seed;
        m_point = point;
        m_waveMultiplier1 = 1.0f;
        m_waveMultiplier2 = 1.0f;
    }
    XString m_typeName;          // 0x00
    unsigned int m_seed;         // 0x04
    XString m_point;             // 0x08
    float m_waveMultiplier1;     // 0x0c
    float m_waveMultiplier2;     // 0x10
};

class CPersonsGroupManager {
public:
    struct SPersonsSet {
        Vector<XString> m_persons; // 0x00
        int m_minCount;            // 0x10
        int m_maxCount;            // 0x14
    };
    struct SPersonsGroup {
        SPersonsGroup()
        {
            m_maxDelayMs = 0;
            m_minDelayMs = 0;
        }
        XString m_name;            // 0x00
        int m_minDelayMs;          // 0x04
        int m_maxDelayMs;          // 0x08
        Vector<SPersonsSet> m_sets; // 0x0c
    };
    CPersonsGroupManager();
    void Parse(TiXmlNode* node);
    bool GroupIsExists(const XString& name);
    SPersonsGroup* GetGroup(const XString& name);
    void GeneratePersonsSelection(Vector<SSpawnablePerson>& persons, const SPersonsGroup& group,
                                  const CWaveManager::SWave* wave);
    ~CPersonsGroupManager();

    // Former m_data00[0x10 - 0x0]: layout not known yet (size from App.cpp).
    Vector<SPersonsGroup> m_groups;
};

#endif
