// PROVISIONAL (created by b078b): owner src/bounty_hunter/Manager/CStatsBonusItem.cpp not written yet
// PlayerStats.h - ten encrypted stats; constructor and Load are strong definitions in that owner.
#ifndef PLAYERSTATS_H
#define PLAYERSTATS_H
#include "DataProtection.h"
#include "xString.h"
class TiXmlNode;
class TiXmlElement;

class PlayerStats {
public:
    PlayerStats();
    void Load(TiXmlNode* node, const XString& prefix);
    Encrypted<int> m_values[10];  // 0x00..0x48; provision index 6, people index 8
};

/** These functions share the ten-entry attribute-name table in CStatsBonusItem.cpp. */
void InitPlayerStats(PlayerStats& stats);
void LoadPlayerStats(PlayerStats& stats, TiXmlNode* node, const XString& prefix);
void SavePlayerStats(const PlayerStats& stats, TiXmlElement* element, const XString& prefix);
#endif
