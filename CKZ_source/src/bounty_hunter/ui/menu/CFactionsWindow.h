// b074i: original 1553d2 truly inherits CBaseScreen, no new members, total c0; body awaits its owning owner.
#ifndef CFACTIONSWINDOW_H
#define CFACTIONSWINDOW_H
#include "CBaseScreen.h"
class CFactionsWindow : public CBaseScreen {
public:
    CFactionsWindow();
    virtual ~CFactionsWindow();
};
#endif
