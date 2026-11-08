// PROVISIONAL (created by b057x): owner src/bounty_hunter/Manager/CQuestManager.cpp not written yet
// b078c: implementation recovered; the preceding PROVISIONAL comment is historical.
// CQuestManager.h - quests; only what src/general/App.cpp uses is declared.
// b078c: recovered quest/dialog records and filtering; original comments retained.
#ifndef CQUESTMANAGER_H
#define CQUESTMANAGER_H

#include <stddef.h>
#include "xString.h"
#include "Vector_ss.h"
class TiXmlNode;

class CQuestManager {
public:
    struct SDialog {
        enum ECondition { CONDITION_ANY = 0, CONDITION_START = 1, CONDITION_WIN = 2 };
        static ECondition GetDialogCondition(XString condition);
        ECondition m_condition; // 0x00
        XString m_text;         // 0x04
        XString m_title;        // 0x08
        const char* m_image;    // 0x0c only assigned when the image attribute is nonempty
    };
    struct SQuest {
        int m_id;                        // 0x00
        int m_minXp;                     // 0x04
        int m_requiredQuestId;           // 0x08
        XString m_screenName;             // 0x0c
        XString m_locationName;           // 0x10
        XString m_missionName;            // 0x14
        Vector<SDialog> m_dialogs;        // 0x18 untouched by this parser
        Vector<SDialog> m_gameDialogs;    // 0x28
    };
    CQuestManager();
    void Parse(TiXmlNode* node);
    void ParseDialog(Vector<SDialog>& dialogs, TiXmlNode* node, const char* prefix, const XString& title);
    SQuest* FindAvailableQuestForLocations(const Vector<XString>& locations);
    SQuest* GetQuestById(int id);
    ~CQuestManager();

    // Former m_data00[0x10 - 0x0]: layout not known yet (size from App.cpp).
    Vector<SQuest> m_quests;
};

#endif
