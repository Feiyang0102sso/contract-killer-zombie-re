// PROVISIONAL (created by b057x): owner src/bounty_hunter/Manager/CShopPackManager.cpp not written yet
// CShopPackManager.h - grenade and power up packs of the shop; only what src/general uses.
// Both managers are deleted through vtable slot 0x34 and searched through slot 0xc.
#ifndef CSHOPPACKMANAGER_H
#define CSHOPPACKMANAGER_H

#include "xString.h"
#include "CShopItem.h"
#include "Vector_ss.h"

class CShopPack;
class CShopItem;  // Historical name used by App.cpp; CItem is the restored owner type.

/** Restored by b079: the former unknown 0x04..0x1f are CItemManager and m_items. */
class CShopPackManager : public CItemManager {
public:
    virtual int GetItemsCount() { return m_items.size(); }
    virtual CItem* getItemByInternalId(int index);
    virtual CShopItemButton* CreateShopItemButton(int id);
    virtual ~CShopPackManager();       // slots 0x30 / 0x34

    Vector<CShopPack*> m_items;        // 0x10
};

class CGrenadePackManager : public CShopPackManager {
public:
    CGrenadePackManager();
    virtual void AddItem(TiXmlNode* node);
    virtual CShopItemButton* CreateShopItemButton(int id);
    virtual ~CGrenadePackManager() {}
    /** The item with the string id (NULL: not managed here). */
    CShopItem* GetItemByStringUId(const XString& id) { return (CShopItem*)getItemByStringUId(id); }
    int GetUnlockedGrenadePackIdsOnLevel(int level, Vector<int>& ids);
    // Former m_data04[0x20 - 0x04]: layout not known yet, now represented by the base and vector.
};

class CPowerUpPackManager : public CShopPackManager {
public:
    CPowerUpPackManager();
    virtual void AddItem(TiXmlNode* node);
    virtual CShopItemButton* CreateShopItemButton(int id);
    virtual ~CPowerUpPackManager() {}
    /** The item with the string id (NULL: not managed here). */
    CShopItem* GetItemByStringUId(const XString& id) { return (CShopItem*)getItemByStringUId(id); }
    // Former m_data04[0x20 - 0x04]: layout not known yet, now represented by the base and vector.
};

#endif
