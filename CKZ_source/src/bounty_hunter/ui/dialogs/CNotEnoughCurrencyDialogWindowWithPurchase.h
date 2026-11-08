// PROVISIONAL: real layout/declarations required by the Game closure; the body still belongs to the full u541 owner.
#ifndef CNOTENOUGHCURRENCYDIALOGWINDOWWITHPURCHASE_H
#define CNOTENOUGHCURRENCYDIALOGWINDOWWITHPURCHASE_H
#include "CZombieDialogWindow.h"
class CIAPCurrencyManager;
class CIAPCurrency;
class CShopItemButton;

// Original 14d24c directly calls Zombie(style2); seven own words plus the Game allocation prove size 134.
class CNotEnoughCurrencyDialogWindowWithPurchase : public CZombieDialogWindow {
public:
    CNotEnoughCurrencyDialogWindowWithPurchase(int requiredCredits, unsigned int completionCommand);
    virtual ~CNotEnoughCurrencyDialogWindowWithPurchase() {}
    virtual void OnCommand(Event& event);
    virtual void Update();
    virtual void InitIAP(); // Original 14d2d8, new slot +bc; not a Currency intermediate base class.
    void UpdateIAPs();
    void UpdateButtons();
    CShopItemButton* CreateButton(CIAPCurrency* item);
    CShopItemButton* CreateLoadingButton(bool credits);
    // Historical unsigned int m_fields118[7]; +118..130, concrete semantics await the full owner.
    CIAPCurrencyManager* m_iapManager; // 118
    int m_requiredCredits;            // 11c
    Window* m_buttonsContainer;       // 120
    int m_availableCount;             // 124
    CIAPCurrency* m_best;             // 128
    CIAPCurrency* m_next;             // 12c
    unsigned int m_completionCommand; // 130
};
#endif
