// PROVISIONAL (created by b077): owner src/bounty_hunter/ui/dialogs/CWaitPurchaseWindow.cpp not written yet
// CWaitPurchaseWindow.h - modal "please wait" window shown during a store purchase; only what CIAPCurrency uses.
#ifndef CWAITPURCHASEWINDOW_H
#define CWAITPURCHASEWINDOW_H
#include "CZombieDialogWindow.h"

// b084k promoted to real: original 14e934 directly Zombie(style3), allocates 118, no extra instance fields.
class CWaitPurchaseWindow : public CZombieDialogWindow {
public:
    CWaitPurchaseWindow();
    virtual ~CWaitPurchaseWindow() {} // Original 14ea18/14ea28 only do Zombie destruct/free.
    virtual void OnCommand(Event& event);
    /** Show the window over everything. */
    static void ShowModal();
    static void SetClose();
    static bool m_close; // Original 50a534 is written with strb; this owner has no read or auto-close logic.
};

#endif
