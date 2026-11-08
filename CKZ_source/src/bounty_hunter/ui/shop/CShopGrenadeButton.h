// PROVISIONAL (created by b079): owner src/bounty_hunter/ui/shop/CShopGrenadeButton.cpp not written yet
// Only the constructor and allocation size from CGrenadePackManager @0x13a62a are known here.
#ifndef CSHOPGRENADEBUTTON_H
#define CSHOPGRENADEBUTTON_H
#include "CShopRectItemButton.h"
#include "CZombieDialogWindow.h"
class CGrenadePack;
class CShopGrenadeButton : public CShopRectItemButton {
public:
    CShopGrenadeButton(CGrenadePack* item);
    // Historical thin placeholder: unsigned char m_data[0x104];
    // The original C2 first calls Rect(100), then writes the typed item at 100; this class total 104.
    virtual ~CShopGrenadeButton();
    virtual Window* CreateInfoDialog();
    virtual bool IsAfterBuyDialogShown() const { return true; }
    virtual void Update();
    class InfoWindow : public CZombieDialogWindow {
    public:
        explicit InfoWindow(CGrenadePack* pack);
        virtual ~InfoWindow();
        CGrenadePack* m_pack; // 118
        bool m_flag11c; // 11c, the original ctor writes false.
    };
    CGrenadePack* m_grenadePack; // 100
};
#endif
