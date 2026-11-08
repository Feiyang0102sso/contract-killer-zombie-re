// j: original 153d60 complete stats table window, total b4.
#ifndef CAFTERGAMESTATSWINDOW_H
#define CAFTERGAMESTATSWINDOW_H
#include "WindowBase.h"
#include "CFontMgr.h"
class CAfterGameStatsWindow : public Window {
public:
    CAfterGameStatsWindow();
    virtual ~CAfterGameStatsWindow() {}
    void AddTableStr(XString name, const XString& value, const XString& score, bool hideValue, eGameFont font);
    void AddStat(XString name, int value, int score, bool hideValue);
    Window* m_table;
    int m_row;
};
#endif
