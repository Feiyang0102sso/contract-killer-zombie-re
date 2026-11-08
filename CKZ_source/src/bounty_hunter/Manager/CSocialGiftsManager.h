// PROVISIONAL (created by b057x): owner src/bounty_hunter/Manager/CSocialGiftsManager.cpp not written yet
// CSocialGiftsManager.h - social gifts; only what src/general/App.cpp uses is declared.
#ifndef CSOCIALGIFTSMANAGER_H
#define CSOCIALGIFTSMANAGER_H

#include <stddef.h>
#include "Vector_ss.h"
#include "xString.h"

class CItemManager;

/** Configured item id and the manager that grants it. */
struct SGift {
    int m_itemId;                    // 0x00
    CItemManager* m_itemManager;      // 0x04
};

class CSocialGiftManager {
public:
    CSocialGiftManager();
    ~CSocialGiftManager();
    void ParseConfig();
    CItemManager* GetItemManager(XString& type);
    int GetGiftsCount();
    SGift* GetGift(int index);

    // Former m_data00[0x10 - 0x0]: layout not known yet (size from App.cpp).
    Vector<SGift*> m_gifts;          // 0x00, restored by b079
};

#endif
