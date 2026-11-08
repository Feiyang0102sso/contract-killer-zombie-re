// j ABI closure: original 15dc26 calls Item, f4/f8/fc are three real pointers, total 100; the complete owner is left for a later wave.
#ifndef CSHOPRECTITEMBUTTON_H
#define CSHOPRECTITEMBUTTON_H
#include "CShopItemButton.h"
class TextWindow;
class ImageWindow;
class CShopRectItemButton : public CShopItemButton {
public:
    CShopRectItemButton(CItem* item);
    virtual ~CShopRectItemButton();
    virtual Window* CreateInfoDialog() = 0; // Entry c4 of original vtable 4e2540 is pure virtual.
    virtual void SetPrice(int price);
    virtual void Update();
    static Window* CreateStatTextWindow(XString first, XString second);
    static Window* CreateInInventoryCountPanel(CItem* item);
    void AddInInventoryCountPanel();
    void AddStrips();
    void AddEquipStrip(int animation, const XString& text, int font);
    void AddMarkers();
    // Historical: Window* m_imageContainer; // f4, derived classes may write ImageWindow or a plain Window, so the type is not wrongly narrowed.
    // k full re-check: Grenade 15c3f2 also constructs ImageWindow; the earlier plain-Window inference was wrong.
    ImageWindow* m_imageContainer; // f4, not initialized by the original base constructor; all three leaves write ImageWindow.
    TextWindow* m_priceText;  // f8, original 15dcf4.
    CItem* m_rectItem;        // fc, original 15dc40.
};
#endif
