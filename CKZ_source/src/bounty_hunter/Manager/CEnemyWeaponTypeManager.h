// CEnemyWeaponTypeManager.h - all enemy weapons of res/common/configs/enemy_weapons.xml by name.
#ifndef CENEMYWEAPONTYPEMANAGER_H
#define CENEMYWEAPONTYPEMANAGER_H

#include <stddef.h>
#include "xString.h"
#include "HashTable.h"

class CEnemyWeaponType;

class CEnemyWeaponTypeManager {
public:
    CEnemyWeaponTypeManager();
    virtual ~CEnemyWeaponTypeManager();

    /** The weapon named `name` (must exist). */
    CEnemyWeaponType* GetType(const XString& name) const;

    HashTable<XString, CEnemyWeaponType*> m_types;  // 0x04
};

#endif
