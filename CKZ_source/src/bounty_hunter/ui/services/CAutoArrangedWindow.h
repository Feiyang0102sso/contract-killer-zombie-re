// b081j: original 159ce8..159fac auto-arranged window; item storage does not separately own the windows.
#ifndef CAUTOARRANGEDWINDOW_H
#define CAUTOARRANGEDWINDOW_H
#include "WindowBase.h"
class CAutoArrangedWindow : public Window {
public:
    CAutoArrangedWindow(int width, int height);
    virtual ~CAutoArrangedWindow();
    bool UpdateContentLayout();
    bool Add(Window* window);
    Vector<Window*> m_windows; // ac
    bool m_singleRow;          // bc
    int m_maxRows;             // c0, 0 means unlimited
    int m_maxColumns;          // c4, total c8
};
#endif
