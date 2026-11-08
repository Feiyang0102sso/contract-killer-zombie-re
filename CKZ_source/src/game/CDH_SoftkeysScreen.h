// CDH_SoftkeysScreen.h - a screen window that adds its soft key bar to the parent when initialized.
#ifndef CDH_SOFTKEYSSCREEN_H
#define CDH_SOFTKEYSSCREEN_H

#include "WindowBase.h"

/** Window with an optional soft key bar (vtable: Window + PopulateSofkeys at 0xbc). */
class CDH_SoftkeysScreen : public Window {
public:
    CDH_SoftkeysScreen();
    virtual ~CDH_SoftkeysScreen();

    virtual void Init();
    /** Create the soft key bar window (NULL: no soft keys). The typo is original. */
    virtual Window* PopulateSofkeys();
};

#endif
