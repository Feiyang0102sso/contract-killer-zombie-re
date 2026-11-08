// CEnergyManager.h - the energy items of the shop (CStatsBonusItem).
#ifndef CENERGYMANAGER_H
#define CENERGYMANAGER_H

#include <stddef.h>
#include "Vector_ss.h"
#include "CShopItem.h"

class CStatsBonusItem;

class CEnergyManager : public CItemManager {
public:
    ~CEnergyManager();

    virtual int GetItemsCount()                                 // 0x00
    {
        return m_items.size();
    }
    virtual CShopItemButton* CreateShopItemButton(int id);      // 0x14
    virtual void loadResources();                               // 0x20
    virtual CItem* getItemByInternalId(int index);              // 0x28
    virtual void AddItem(TiXmlNode* node);                      // 0x2c

    /** Delete every item. */
    void freeResources();

    Vector<CStatsBonusItem*> m_items;   // 0x10
};

#endif
