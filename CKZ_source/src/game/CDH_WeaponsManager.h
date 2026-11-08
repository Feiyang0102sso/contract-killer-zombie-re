// CDH_WeaponsManager.h - the weapons of the shop, read from res/common/configs/weapons.xml.
// (b055x: replaces the PROVISIONAL of b057x / b048; their names stay as inline aliases.)
#ifndef CDH_WEAPONSMANAGER_H
#define CDH_WEAPONSMANAGER_H

#include <stddef.h>
#include "xString.h"
#include "Vector_ss.h"
#include "CShopItem.h"

class CShopItem;
class CDH_Weapon;
class TiXmlNode;

/** Manager of the CDH_Weapon items. No virtual destructor (App deletes it directly). */
class CDH_WeaponsManager : public CItemManager {
public:
    CDH_WeaponsManager();
    ~CDH_WeaponsManager();

    virtual int GetItemsCount()                                     // 0x00
    {
        return m_weapons.size();
    }
    virtual CShopItemButton* CreateShopItemButton(int index);      // 0x14
    virtual void loadResources();                                   // 0x20
    virtual CItem* getItemByInternalId(int index);                  // 0x28
    virtual void AddItem(TiXmlNode* node);                          // 0x2c

    /** Reload weapons.xml when it changed. */
    void ReloadConfig();
    /** Save and delete every weapon. */
    void CleanupAll();
    /** releaseResources() of every weapon. */
    void releaseResource();
    /** Release the 3D model of weapon `id`. */
    void release3dResources(int id);
    /** Ids of the weapons (not hidden) unlocked at `level`; returns their count. */
    int GetUnlockedWeaponsIdOnLevel(int level, Vector<int>& ids);

    /** Old name used by src/general/App.cpp (b057x): CItemManager::getItemByStringUId (slot 0x0c). */
    CShopItem* GetItemByStringUId(const XString& id)
    {
        return (CShopItem*)getItemByStringUId(id);
    }
    /** Old name used by src/dh3d (b048): CItemManager::getItemById (slot 0x10). */
    CDH_Weapon* GetWeaponById(int weaponId)
    {
        return (CDH_Weapon*)getItemById(weaponId);
    }
    /** Old name used by src/general/App.cpp (b057x): loadResources (slot 0x20). */
    void Init()
    {
        loadResources();
    }
    /** The owned weapons (used by src/dh3d, b048). */
    Vector<CDH_Weapon*>& GetWeapons()
    {
        return m_weapons;
    }

    Vector<CDH_Weapon*> m_weapons;  // 0x10
};

#endif
