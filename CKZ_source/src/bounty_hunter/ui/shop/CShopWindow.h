// b074i: real base class of the external owner (15fec6) and the e4 allocation (160c72); body not recovered.
#ifndef CSHOPWINDOW_H
#define CSHOPWINDOW_H
#include "CBaseScreen.h"
class CItem;
class CShopItemButton;
class CShopTabWindow;
class CustomWeaponButtonCreator;
class CShopWindow : public CBaseScreen {
public:
    enum ETab { TAB_0 = 0, TAB_1 = 1, TAB_2 = 2 };
    static CShopWindow* GetInstance();
    CShopWindow();
    virtual ~CShopWindow();
    void CreateEffects();
    void CreateClipingBorders();
    void CreateInfoPanel();
    virtual void Init();
    virtual void OnCommand(Event& event);
    void BuyItem(CShopItemButton* button);
    static ETab GetTabForItem(CItem* item);
    void SetTab(ETab tab, int itemId);
    void TryOpenTapJoy();
    virtual void Update();
    virtual void OnClosed();
    virtual void Paint(com::glu::platform::graphics::ICGraphics2d& graphics);
    virtual void AddModal(Window* window);
    virtual void OnGetFocus();
    virtual void OnLostFocus();
    virtual void OnEnterForeground();
    // Former: unsigned char m_unmappedC0[0xe4 - 0xc0];
    CShopTabWindow* m_tabs; // c0
    bool m_adVisible; // c4
    // Former individually named items: m_weaponTab/m_consumableTab/m_bankTab. The original SetTab confirms contiguous enum indices.
    int m_tabIds[3]; // c8/cc/d0, weapon/consumable/bank
    int m_purchasePending; // d4, zeroed by the original ctor and the two completion events.
    int m_unresolvedD8; // d8, zeroed by the original ctor; no reads within the live range.
    CustomWeaponButtonCreator* m_weaponCreator; // dc, the actual creator is owned by Content.
    CustomWeaponButtonCreator* m_consumableCreator; // e0, total e4.
    static CShopWindow* m_instance;
};
#endif
