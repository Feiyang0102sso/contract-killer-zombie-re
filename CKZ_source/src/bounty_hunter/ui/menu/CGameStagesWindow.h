// b074i: original 1568b4 truly inherits SGImageWindowCommand, allocates 168; the complete owner is not yet written.
#ifndef CGAMESTAGESWINDOW_H
#define CGAMESTAGESWINDOW_H
#include "SGImageWindowCommand.h"
#include "Vector_ss.h"
class CPlayerGameResourceWindow;
class CGameStagesWindow : public SGImageWindowCommand {
public:
    CGameStagesWindow();
    virtual inline ~CGameStagesWindow() {}
    virtual void OnLayout();
    virtual void Update();
    // Historical unsigned char m_unresolved130[0x168 - 0x130]; non-owning tail fields kept at their original offsets.
    Vector<CPlayerGameResourceWindow*> m_resources; // 130
    int m_field140;
    int m_field144;
    int m_field148;
    int m_field14c;
    bool m_flag150;
    int m_field154;
    int m_field158;
    int m_field15c;
    int m_field160;
    int m_field164; // Not written by the original ctor.
};
#endif
