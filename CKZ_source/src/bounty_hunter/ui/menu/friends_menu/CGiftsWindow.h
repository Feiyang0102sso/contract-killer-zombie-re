// j: original 159300 complete Gifts, no new fields; the filtered Vector is only used for counting, and the original still re-queries by consecutive index afterwards.
#ifndef CGIFTSWINDOW_H
#define CGIFTSWINDOW_H
#include "WindowBase.h"
#include "Vector_ss.h"
struct SGift;
class CGiftsWindow : public Window {
public:
    CGiftsWindow();
    virtual ~CGiftsWindow() {}
    void GetGifts(Vector<const SGift*>& gifts);
    void CreateUserInterface(const Vector<const SGift*>& gifts);
    Window* CreateGiftButton(int index);
};
#endif
