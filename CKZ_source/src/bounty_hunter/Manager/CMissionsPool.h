// PROVISIONAL (created by b077): owner src/bounty_hunter/Manager/CMissionsPool.cpp not written yet
// CMissionsPool.h - a pool of mission types / difficulties (<m type diff>); only what CCommonGameConfig uses.
// b078b: Load and Save fully restored; the original provisional notes above are retained.
#ifndef CMISSIONSPOOL_H
#define CMISSIONSPOOL_H

#include "Vector_ss.h"

class TiXmlNode;
class TiXmlElement;

class CMissionsPool {
public:
    /** One <m> entry (8 bytes, copied as plain words). */
    struct SPoolElement {
        SPoolElement()
        {
        }

        int m_type;         // 0x00 CMission::StringToMissionType
        int m_difficulty;   // 0x04
    };

    /** Append the <m> children of `node`. */
    void Load(TiXmlNode* node);
    void Save(TiXmlElement* root);

    Vector<SPoolElement> m_elements;    // 0x00
};

#endif
