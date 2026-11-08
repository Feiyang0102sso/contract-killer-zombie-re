// PROVISIONAL (created by b055x): owner src/bounty_hunter/ui/shop/CShopItemButton.cpp not written yet
// CShopItemButton.h - base of the shop item buttons (nothing used yet; CItemManager::CreateShopItemButton returns it).
#ifndef CSHOPITEMBUTTON_H
#define CSHOPITEMBUTTON_H

#include "CBH_Button.h"
#include "CShopItem.h"
class CItem;
class CZombieDialogWindow;
// b081j: original 15cd38 constructs CBH_Button directly, original vtable 4e21e0; only the real external interface is added here.
class CShopItemButton : public CBH_Button {
public:
    CShopItemButton(CItem* item);
    virtual ~CShopItemButton();
    virtual void Update();
    virtual void Paint(com::glu::platform::graphics::ICGraphics2d& graphics);
    virtual bool IsAfterBuyDialogShown() const { return false; }
    virtual Window* CreateInfoDialog() { return NULL; } // original 15d6cc.
    virtual void SetPrice(int price) {} // original 15d6d0, writes no members.
    virtual bool IsIAP() const { return false; }
    int GetPrice();
    int Buy();
    bool CheckBuyConditions(CZombieDialogWindow** dialog);
    Window* AddHotMarker();
    Window* AddNewMarker();
    Window* AddSalesMarker();
    XString GetItemName()
    {
        if (m_item != NULL) {
            return m_item->GetName();
        }
        return *XString::pEmpty;
    }
    CItem* m_item;            // e4
    int m_price;             // e8
    bool m_flagEc;           // ec
    int m_cachedPrice;       // f0, total f4; not initialized by the original when item=NULL.
};

#endif
