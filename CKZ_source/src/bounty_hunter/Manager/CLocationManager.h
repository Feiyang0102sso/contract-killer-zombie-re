// PROVISIONAL (created by b057x): owner src/bounty_hunter/Manager/CLocationManager.cpp not written yet
// CLocationManager.h - locations; only what src/general/App.cpp uses is declared.
// b078: recovered table ownership and location loading; historical comments retained.
#ifndef CLOCATIONMANAGER_H
#define CLOCATIONMANAGER_H

#include <stddef.h>
#include "HashTable.h"
class CLocation;

class CLocationManager {
public:
    CLocationManager();
    virtual ~CLocationManager();
    CLocation* GetLocation(const XString& name) const;

    // Former m_data04: layout not known yet (size from App.cpp).
    HashTable<XString, CLocation*> m_locations;    // 0x04
};

#endif
