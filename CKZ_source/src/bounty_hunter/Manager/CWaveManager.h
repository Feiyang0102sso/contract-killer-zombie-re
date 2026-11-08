// PROVISIONAL (created by b057x): owner src/bounty_hunter/Manager/CWaveManager.cpp not written yet
// CWaveManager.h - zombie waves; only what src/general/App.cpp uses is declared.
#ifndef CWAVEMANAGER_H
#define CWAVEMANAGER_H

#include <stddef.h>
#include "Vector_ss.h"
#include "xString.h"
#include "CMission.h"

class TiXmlNode;
class CItem;

class CWaveManager {
public:
    /** A configured spawn group of one wave (sector, type/group and count). */
    struct SSpawnEvent {
        XString m_sector;
        XString m_type;
        int m_count;
    };
    /** Only the multiplier consumed by GeneratePersonsSelection is mapped. */  // added by b078b
    struct SWave {
        // Former m_data00[0x20]; the complete configured record was recovered by b080.
        int m_id;                         // 0x00
        int m_level;                      // 0x04
        XString m_filter;                 // 0x08
        Vector<SSpawnEvent> m_events;      // 0x0c
        float m_cashXpMultiplier;         // 0x1c
        float m_personMultiplier; // 0x20, copied to both spawn multipliers at 0x12f518
        float m_threshold;                // 0x24 original attribute spelling: treshold
        float m_pause;                    // 0x28 seconds
        int m_recommendationId;           // 0x2c
        bool m_mustBeClear;               // 0x30
    };
    struct SWaveRecommendation {
        int m_id;
        Vector<XString> m_items;
    };
    CWaveManager();
    ~CWaveManager();
    void Load(const wchar_t* path, Vector<Vector<SWave> >* waves);
    void ReloadRecommendations();
    void Parse(TiXmlNode* root, Vector<Vector<SWave> >* waves);
    SWave* GetWave(CMission::EMissionType type, CMission::EMissionDifficulty difficulty, int id);
    SWave* GetWave(Vector<Vector<SWave> >* waves, int id);
    SWave* GetRandomWave(CMission::EMissionType type, CMission::EMissionDifficulty difficulty,
                         int level, const XString& filter);
    SWave* GetRandomWave(Vector<Vector<SWave> >* waves, int level, const XString& filter);
    SWave* GetRandomWave(Vector<Vector<SWave> >* waves, int firstLevel, int lastLevel, const XString& filter);
    CItem* GetRecommendedItem(int id);

    /** Every requested character must occur somewhere in the wave's filter string. */
    bool ApplyFilter(const XString& available, const XString& requested)
    {
        int availableLength = available.Length();
        int requestedLength = requested.Length();
        for (int requestedIndex = 0; requestedIndex < requestedLength; requestedIndex++) {
            int availableIndex = 0;
            while (true) {
                if (availableIndex >= availableLength) {
                    return false;
                }
                if (available.Char(availableIndex) == requested.Char(requestedIndex)) {
                    break;
                }
                availableIndex++;
            }
        }
        return true;
    }

    // Former m_data00[0xf0 - 0x0]: layout not known yet (size from App.cpp).
    Vector<SWaveRecommendation*> m_recommendations; // 0x00
    Vector<Vector<SWave> > m_ic[3];                 // 0x10
    Vector<Vector<SWave> > m_ka[3];                 // 0x40
    Vector<Vector<SWave> > m_cb[3];                 // 0x70
    Vector<Vector<SWave> > m_st[3];                 // 0xa0
    Vector<Vector<SWave> > m_quest;                 // 0xd0
    Vector<Vector<SWave> > m_em;                    // 0xe0
};

#endif
