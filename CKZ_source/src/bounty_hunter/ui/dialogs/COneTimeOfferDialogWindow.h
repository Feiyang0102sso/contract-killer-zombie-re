// PROVISIONAL (created by b078c): owner src/bounty_hunter/ui/dialogs/COneTimeOfferDialogWindow.cpp not written yet
#ifndef CONETIMEOFFERDIALOGWINDOW_H
#define CONETIMEOFFERDIALOGWINDOW_H
#include "CZombieDialogWindow.h"
class CIAPCurrency;
class CShopItemButton;
/** Only Window conversion, constructor signature and 0x128 allocation size are mapped. */
// b084k: original 14e18c directly Zombie(style3); the historical Window view was only for the old consumer closure.
class COneTimeOfferDialogWindow : public CZombieDialogWindow {
public:
    COneTimeOfferDialogWindow(int packId);
    virtual ~COneTimeOfferDialogWindow() {}
    virtual void Update();
    virtual void OnCommand(Event& event);
    void UpdateIAPLayout();
    // Historical unsigned char m_dataAc[0x128 - 0xac];
    CIAPCurrency* m_currency;    // 118
    Window* m_inner;            // 11c
    CShopItemButton* m_button;   // 120
    int m_packId;               // 124
};
#endif
