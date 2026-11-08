// PROVISIONAL (created by b057x): owner src/bounty_hunter/Manager/CSceneSetupsManager.cpp not written yet
// CSceneSetupsManager.h - scene setups; only what src/general/App.cpp uses is declared.
// b078b: manager 0x10, setup 0x3c and spawn entry 0x0c recovered from Parse/vector copies.
#ifndef CSCENESETUPSMANAGER_H
#define CSCENESETUPSMANAGER_H

#include <stddef.h>
#include "xString.h"
#include "Vector_ss.h"
class TiXmlNode;

class CSceneSetupsManager {
public:
    struct SSceneSpawnInfo {
        XString m_name;       // 0x00
        int m_probability;    // 0x04
        XString m_point;      // 0x08
    };
    struct SSceneSetup {
        XString m_name;                      // 0x00
        Vector<SSceneSpawnInfo> m_persons;    // 0x04
        Vector<SSceneSpawnInfo> m_objects;    // 0x14
        Vector<XString> m_optionalParts;      // 0x24
        XString m_helicopterModelId;          // 0x34
        bool m_helicopterHidden;              // 0x38
    };
    CSceneSetupsManager();
    void Parse(TiXmlNode* node);
    SSceneSetup* GetSceneSetupByName(const XString& name);
    ~CSceneSetupsManager();

    // Former m_data00[0x10 - 0x0]: layout not known yet (size from App.cpp).
    Vector<SSceneSetup> m_setups;
};

#endif
