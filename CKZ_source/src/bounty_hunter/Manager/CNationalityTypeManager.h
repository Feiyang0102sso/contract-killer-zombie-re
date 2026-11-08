// PROVISIONAL (created by b057x): owner src/bounty_hunter/Manager/CNationalityTypeManager.cpp not written yet
// CNationalityTypeManager.h - nationality types; only what src/general/App.cpp uses is declared.
// b078: the owned record layout follows HashTable<...>::DeleteContents in the original.
#ifndef CNATIONALITYTYPEMANAGER_H
#define CNATIONALITYTYPEMANAGER_H

#include <stddef.h>
#include "HashTable.h"
#include "Vector_ss.h"

/** Nationality name and three string lists; the individual list roles are not yet identified. */
struct CNationalityType {
    XString m_name;                 // 0x00
    Vector<XString> m_nameGroups[3]; // 0x04, 0x14, 0x24
};

class CNationalityTypeManager {
public:
    CNationalityTypeManager();
    virtual ~CNationalityTypeManager();

    // Former m_data04: layout not known yet (size from App.cpp).
    HashTable<XString, CNationalityType*> m_types;  // 0x04
};

#endif
