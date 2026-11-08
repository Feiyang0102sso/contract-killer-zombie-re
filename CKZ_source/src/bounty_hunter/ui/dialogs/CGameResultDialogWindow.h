// PROVISIONAL: real allocation/base class/virtual declarations of original u538; no speculative body is provided.
#ifndef CGAMERESULTDIALOGWINDOW_H
#define CGAMERESULTDIALOGWINDOW_H
#include "CZombieDialogWindow.h"

class CGameResultDialogWindow : public CZombieDialogWindow {
public:
    CGameResultDialogWindow(); // C2 14a048 passes Zombie(style3).
    CGameResultDialogWindow(int first, int second, bool flag); // C2 14a5d4 passes style2.
    virtual ~CGameResultDialogWindow() {}
    virtual void Update();
    // Historical unsigned int m_fields118[3]; original size 124, +11c zeroed/+120=NULL; the +118 default initial value is not guessed.
    int m_countdown;             // 118, written as 30000 only when there is a revive button.
    SGButton* m_reviveButton;    // 11c
    TextWindow* m_timer;         // 120
};
#endif
