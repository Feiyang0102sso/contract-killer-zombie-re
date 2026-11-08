// CCommonGameConfig.h - res/common/configs/common.xml: kill scores, ranks, mission pools by level
// and provision boxes.
#ifndef CCOMMONGAMECONFIG_H
#define CCOMMONGAMECONFIG_H

#include <stddef.h>
#include "xString.h"
#include "Vector_ss.h"
#include "CMissionsPool.h"

class TiXmlNode;

class CCommonGameConfig {
public:
    /** A rank (title) reached at m_score points; sorted by score. */
    struct SRank {
        int m_score;            // 0x00
        XString m_title;        // 0x04
        XString m_achievement;  // 0x08
    };

    /** Mission pool of the levels [m_minLevel, m_maxLevel]. */
    struct SPoolInfo {
        SPoolInfo()
        {
        }

        CMissionsPool m_pool;   // 0x00
        int m_minLevel;         // 0x10
        int m_maxLevel;         // 0x14
    };

    /** A provision box and its group. */
    struct SProvisionBox {
        XString m_name;         // 0x00
        XString m_group;        // 0x04
    };

    CCommonGameConfig();
    virtual ~CCommonGameConfig();

    void ParseScore(TiXmlNode* node);
    void ParseRanks(TiXmlNode* node);
    void ParseMissionTypesPools(TiXmlNode* node);
    void ParseBoxes(TiXmlNode* node);
    /** Title of the highest rank reached with `score` (the first one when none is). */
    XString& GetRankTitleForScore(int score);
    /** Indices of all ranks reached with `score`. */
    void GetAllRankAchievementsForScore(int score, Vector<int>& ranks);
    /** Mission pool of `level` (the first pool when none matches). */
    CMissionsPool GetMissionsPoolForLevel(int level);
    XString& GetRandomBoxGroupName();
    /** Group of the box `name` (the first group when not found). */
    XString& GetBoxGroupName(const XString& name);

    int m_headshotScore;            // 0x04
    int m_multikillScore;           // 0x08
    int m_megakillScore;            // 0x0c
    int m_unstoppableScore;         // 0x10
    int m_dominatingScore;          // 0x14
    int m_multiheadshotScore;       // 0x18
    int m_decapitatorScore;         // 0x1c
    int m_deadlyPrecisionScore;     // 0x20
    int m_kaboomScore;              // 0x24
    Vector<SRank> m_ranks;          // 0x28
    Vector<SPoolInfo> m_pools;      // 0x38
    Vector<SProvisionBox> m_boxes;  // 0x48
};

#endif
