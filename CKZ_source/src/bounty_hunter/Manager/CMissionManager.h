// PROVISIONAL (created by b057x): owner src/bounty_hunter/Manager/CMissionManager.cpp not written yet
// CMissionManager.h - missions; only what src/general/App.cpp uses is declared.
// b078b: recovered 0x24 layout and description filtering.
#ifndef CMISSIONMANAGER_H
#define CMISSIONMANAGER_H

#include <stddef.h>
#include "xString.h"  // added by b048
#include "CMission.h"
#include "Vector_ss.h"

class CMission;  // added by b048
class CMapLocation;

class CMissionManager {
public:
    struct SMissionDescription {
        int m_id;
        XString m_title;
        XString m_text;
    };
    // The original spelling Desctiption is part of the template symbols.
    struct SMissionsClassDesctiption {
        XString m_location;
        CMission::EMissionType m_type;
        CMission::EMissionDifficulty m_difficulty;
        int m_minLevel;
        int m_maxLevel;
        SMissionDescription m_description;
    };
    CMissionManager();
    void ParseDescriptions();
    virtual ~CMissionManager();
    CMission* GetMissionByName(const XString& name) const;  // added by b048
    void GetSuitableMissionsForLocation(Vector<const CMission*>& missions, const CMapLocation* location,
                                      CMission::EMissionDifficulty difficulty, CMission::EMissionType type) const;
    int GetRandomMissionDescriptionId(const CMission* mission);
    SMissionDescription* GetMissionDescriptionById(int id);

    // Former m_data04[0x24 - 0x4]: layout not known yet (size from App.cpp).
    Vector<SMissionsClassDesctiption> m_descriptions; // 0x04
    Vector<CMission*> m_missions;                   // 0x14
};

#endif
