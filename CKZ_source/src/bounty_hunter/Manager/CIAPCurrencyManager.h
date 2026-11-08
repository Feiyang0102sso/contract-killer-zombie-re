// PROVISIONAL (created by b057x): owner src/bounty_hunter/Manager/CIAPCurrencyManager.cpp not written yet
// CIAPCurrencyManager.h - in-app purchase currencies; only what src/general/App.cpp uses is declared.
// b078: recovered from CIAPCurrencyManager.o; the historical provisional notes above are retained.
#ifndef CIAPCURRENCYMANAGER_H
#define CIAPCURRENCYMANAGER_H

#include <stddef.h>
#include "xString.h"
#include "CShopItem.h"
#include "Vector_ss.h"

class CIAPCurrency;

class CIAPCurrencyManager : public CItemManager {
public:
    CIAPCurrencyManager();
    ~CIAPCurrencyManager();
    virtual int GetItemsCount();
    virtual CShopItemButton* CreateShopItemButton(int index);
    virtual void loadResources();
    virtual void releaseResources();
    virtual CItem* getItemByInternalId(int index);
    virtual void AddItem(TiXmlNode* node);
    /** The item with the string id (NULL: not managed here). */
    CItem* GetItemByStringUId(const XString& id)
    {
        return getItemByStringUId(id);
    }
    CIAPCurrency* getIAPCurrencyByAppleId(XString id) const;
    int GetAvailableItemsCount();
    void GetAvailableCurrencies(Vector<CIAPCurrency*>& items);
    void SetAllUnavailable();
    void UpdateIAPInfo(bool force);

    // Former m_data04: layout not known yet (size from App.cpp).
    Vector<CIAPCurrency*> m_items;    // 0x10
    int m_requestState;               // 0x20 initialized to zero; no other use in this unit
};

#endif
