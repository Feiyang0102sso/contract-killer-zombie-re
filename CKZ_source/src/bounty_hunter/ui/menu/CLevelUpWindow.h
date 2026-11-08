// b074i: original 159684 calls Window(false) directly, GPSGame allocates d4; complete owner to be recovered.
#ifndef CLEVELUPWINDOW_MENU_FORWARD_H
#define CLEVELUPWINDOW_MENU_FORWARD_H
// j: the real owner lives in friends_menu; the old partial definition is kept as history and no longer forms a second, different class definition.
/*
#include "WindowBase.h"
class CLevelUpWindow : public Window {
public:
    CLevelUpWindow();
    virtual ~CLevelUpWindow();
    unsigned char m_unmappedAc[0xd4 - 0xac];
};
*/
#include "friends_menu/CLevelUpWindow.h"
#endif
