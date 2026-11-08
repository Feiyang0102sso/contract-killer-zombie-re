// PROVISIONAL (created by b078): owner src/bounty_hunter/ui/shop/CShopIAPButton.cpp not written yet
#ifndef CSHOPIAPBUTTON_H
#define CSHOPIAPBUTTON_H

#include "CShopCommonItemButton.h"
class CIAPCurrency;

/** Store button declaration only; size 0xfc from CIAPCurrencyManager::CreateShopItemButton. */
class CShopIAPButton : public CShopCommonItemButton {
public:
    CShopIAPButton(CIAPCurrency* currency);
    // Historical thin placeholder: unsigned char m_data[0xfc];
    // Original 15cbea/15c278 call Common and add no members; total fc, not an extra fc appended.
    virtual ~CShopIAPButton();
    virtual Window* CreateInfoDialog() { return NULL; } // original 15cd20.
    virtual bool IsIAP() const { return true; } // original 15cd24.
};

#endif
