// PROVISIONAL (created by b079): owner src/bounty_hunter/ui/shop/CShopCommonItemButton.cpp not written yet
// Only the constructor and allocation size from CShopPackManager @0x13a502 are known here.
#ifndef CSHOPCOMMONITEMBUTTON_H
#define CSHOPCOMMONITEMBUTTON_H
#include "CShopItemButton.h"
class CItem;
class TextWindow;
class CShopCommonItemButton : public CShopItemButton {
public:
    CShopCommonItemButton(CItem* item);
    // Historical thin placeholder: unsigned char m_data[0xfc];
    // Original 15bef6/15bf58 write the window and the price text respectively; Item base f4, this class total fc.
    virtual ~CShopCommonItemButton();
    virtual Window* CreateInfoDialog();
    virtual void SetPrice(int price);
    virtual void Paint(com::glu::platform::graphics::ICGraphics2d& graphics);
    virtual void Update();
    Window* m_priceContainer; // f4
    TextWindow* m_priceText;  // f8
};
#endif
