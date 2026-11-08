// PROVISIONAL (created by b077): owner src/bounty_hunter/ui/shop/CShopEnergyButton.cpp not written yet
// CShopEnergyButton.h - shop button of an energy item; only what CEnergyManager uses.
#ifndef CSHOPENERGYBUTTON_H
#define CSHOPENERGYBUTTON_H

#include "CShopCommonItemButton.h"

class CItem;

class CShopEnergyButton : public CShopCommonItemButton {
public:
    CShopEnergyButton(CItem* item);

    // Historical thin placeholder: unsigned char m_data[0xfc];
    // Original 15cbea/15c278 call Common and add no members; total fc, not an extra fc appended.
    virtual ~CShopEnergyButton();     // layout not known yet (size from CEnergyManager::CreateShopItemButton)
};

#endif
