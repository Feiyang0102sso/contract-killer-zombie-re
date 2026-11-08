// u540, original C2 14c204/14c2a8 directly Zombie(style2), allocates 130.
#ifndef CNOTENOUGHCURRENCYDIALOGWINDOW_H
#define CNOTENOUGHCURRENCYDIALOGWINDOW_H
#include "CZombieDialogWindow.h"
class CIAPCurrencyManager;
class CIAPCurrency;
class CShopItemButton;

class CNotEnoughCurrencyDialogWindow : public CZombieDialogWindow {
public:
    explicit CNotEnoughCurrencyDialogWindow(CItem* item);
    explicit CNotEnoughCurrencyDialogWindow(CShopItemButton* button);
    virtual ~CNotEnoughCurrencyDialogWindow() {}
    virtual void OnCommand(Event& event);
    virtual void Update();
    virtual void InitIAP(); // New bc slot, not called by the constructor.
    void UpdateIAPs();
    void UpdateButtons();
    CShopItemButton* CreateButton(CIAPCurrency* item);
    CShopItemButton* CreateLoadingButton(bool credits);

    CIAPCurrencyManager* m_iapManager; // 118
    CItem* m_item;                     // 11c, borrowed
    Window* m_buttonsContainer;        // 120
    int m_availableCount;              // 124
    CIAPCurrency* m_best;              // 128, borrowed
    CIAPCurrency* m_next;              // 12c, borrowed
};
#endif
