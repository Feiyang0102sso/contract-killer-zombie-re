// PROVISIONAL (created by b079): owner src/bounty_hunter/ui/shop/CShopPowerUpButton.cpp not written yet
// Only the constructor and allocation size from CPowerUpPackManager @0x13a72a are known here.
#ifndef CSHOPPOWERUPBUTTON_H
#define CSHOPPOWERUPBUTTON_H
#include "CShopRectItemButton.h"
#include "CZombieDialogWindow.h"
class CPowerUpPack;
class CShopPowerUpButton : public CShopRectItemButton {
public:
    CShopPowerUpButton(CPowerUpPack* item);
    // Historical thin placeholder: unsigned char m_data[0x104];
    // The original C2 first calls Rect(100), then writes the typed item at 100; this class total 104.
    virtual ~CShopPowerUpButton();
    virtual Window* CreateInfoDialog();
    virtual bool IsAfterBuyDialogShown() const { return true; }
    virtual void Update();
    class InfoWindow : public CZombieDialogWindow {
    public:
        explicit InfoWindow(CPowerUpPack* pack);
        virtual ~InfoWindow();
        CPowerUpPack* m_pack; // 118
        bool m_flag11c; // 11c, the original ctor writes false; not involved in this owner's destructor decisions.
    };
    CPowerUpPack* m_powerUpPack; // 100
};
#endif
