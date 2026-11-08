// b074: complete dialog derivative; base118 + two Window pointers =120.
#ifndef CFIVEDAYSBONUSDIALOG_H
#define CFIVEDAYSBONUSDIALOG_H
#include "CZombieDialogWindow.h"
class CFiveDaysBonusDialog : public CZombieDialogWindow {
public:
    explicit CFiveDaysBonusDialog(int day);
    virtual ~CFiveDaysBonusDialog() {}
    void CreateTitle();
    void CreateMessage(int day);
    void CreateIcons(int day);
    void CreateIcon(int index, int currentDay);
    Window* m_bonusContent; // 118
    Window* m_icons;        // 11c
};
#endif
