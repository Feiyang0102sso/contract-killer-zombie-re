// PROVISIONAL (created by b055x): owner src/bounty_hunter/ui/shop/CShopWeaponButton.cpp not written yet
// CShopWeaponButton.h - shop button of a weapon; only what src/game/CDH_WeaponsManager.cpp uses.
#ifndef CSHOPWEAPONBUTTON_H
#define CSHOPWEAPONBUTTON_H

#include "CShopRectItemButton.h"
#include "CZombieDialogWindow.h"

class CDH_Weapon;

class CShopWeaponButton : public CShopRectItemButton {
public:
    CShopWeaponButton(CDH_Weapon* weapon);

    // Historical thin placeholder: unsigned char m_data00[0x104];
    // The original C2 first calls Rect(100), then writes the typed item at 100; this class total 104.
    virtual ~CShopWeaponButton();
    virtual Window* CreateInfoDialog();
    virtual bool IsAfterBuyDialogShown() const;
    virtual void Update();
    class InfoWindow : public CZombieDialogWindow {
    public:
        explicit InfoWindow(CDH_Weapon* weapon);
        virtual ~InfoWindow();
        CDH_Weapon* m_weapon; // 118
        bool m_availableAtOpen; // 11c
    };
    CDH_Weapon* m_weapon; // 100  // layout not known yet (size from CDH_WeaponsManager::CreateShopItemButton)
};

#endif
