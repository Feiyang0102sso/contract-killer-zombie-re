// PROVISIONAL (created by b078c): owner src/bounty_hunter/ui/dialogs/CDailyDealDialogWindow.cpp not written yet
#ifndef CDAILYDEALDIALOGWINDOW_H
#define CDAILYDEALDIALOGWINDOW_H
#include "CZombieDialogWindow.h"
struct SDailyDealOffer;
/** Only Window conversion, constructor signature and 0x128 allocation size are mapped. */
// b084k: original 14938c constructs Zombie(style3) directly; adds the SetupLayout slot bc.
class CDailyDealDialogWindow : public CZombieDialogWindow {
public:
    CDailyDealDialogWindow(SDailyDealOffer* offer);
    virtual ~CDailyDealDialogWindow() {}
    virtual void Update();
    virtual void OnClosed();
    virtual void OnCommand(Event& event);
    virtual void SetupLayout();
    // Historical unsigned char m_dataAc[0x128 - 0xac];
    CItem* m_item;                // 118, borrowed
    SDailyDealOffer* m_offer;      // 11c, borrowed
    TextWindow* m_timer;           // 120
    int m_lastTime;               // 124
};
#endif
