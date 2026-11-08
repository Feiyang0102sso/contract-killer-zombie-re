// CGrenadeManager.h - the grenade types of res/common/configs/grenades.xml and the active one.
#ifndef CGRENADEMANAGER_H
#define CGRENADEMANAGER_H

#include <stddef.h>
#include "xString.h"
#include "Vector_ss.h"
#include "CShopItem.h"

class CGrenade;
class CShopItem;
class TiXmlElement;

class CGrenadeManager : public CItemManager {
public:
    CGrenadeManager();
    ~CGrenadeManager();

    virtual int GetItemsCount()                                 // 0x00
    {
        return m_items.size();
    }
    virtual CShopItemButton* CreateShopItemButton(int id);      // 0x14
    virtual CItem* getItemByInternalId(int index);              // 0x28
    virtual void AddItem(TiXmlNode* node);                      // 0x2c

    /** Write the active grenade and the owned counts under <grenades>. */
    void Save(TiXmlElement* root);
    void Load(TiXmlElement* root);
    /** The selected grenade (NULL when none). */
    CGrenade* GetActiveGrenade();
    void SetActiveGrenade(int id);

    /** Name used by src/general/App.cpp (b057x PROVISIONAL): CItemManager::getItemByStringUId (slot 0x0c). */
    CShopItem* GetItemByStringUId(const XString& id)
    {
        return (CShopItem*)getItemByStringUId(id);
    }

    Vector<CGrenade*> m_items;      // 0x10
    int m_activeGrenadeId;          // 0x20 -1: none
};

#endif
