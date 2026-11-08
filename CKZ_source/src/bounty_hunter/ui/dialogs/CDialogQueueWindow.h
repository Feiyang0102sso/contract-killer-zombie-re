// PROVISIONAL (created by b079): owner src/bounty_hunter/ui/dialogs/CDialogQueueWindow.cpp not written yet
// Static insertion entry point only; no construction or object layout is claimed here.
#ifndef CDIALOGQUEUEWINDOW_H
#define CDIALOGQUEUEWINDOW_H

#include "WindowBase.h"
#include "Vector_ss.h"
// b067h: original 149e9a constructs Window directly, Game allocates 0xd4; the body is left to the full owner.
class CDialogQueueWindow : public Window {
public:
    explicit CDialogQueueWindow(Window* parent);
    virtual ~CDialogQueueWindow();
    virtual void Init();
    virtual void Update();
    void ShowNextDialog();
    static void AddDialogInQueue(Window* dialog);
    static CDialogQueueWindow* m_activeQueue; // Original 50a530, 4-byte zero initial value
    Vector<Window*> m_dialogs; // +ac, original 149ee0 ensureCapacity(8).
    // Historical m_dataBc[0xd0 - 0xbc] zeroed by the original ctor; first field now confirmed per 149ecc/14a006.
    Window* m_currentDialog; // +bc, the currently shown window, zeroed by ShowNextDialog.
    // Historical unsigned char m_dataC0[0xd0 - 0xc0]; command and parameters, full owner to be recovered.
    unsigned int m_finishCommand; // c0, GPSGame11b87a writes the command directly.
    int m_finishParameters[3];    // c4/c8/cc, zeroed together by the original.
    Window* m_parent; // +d0: the original uses this when parent is null.
};

#endif
