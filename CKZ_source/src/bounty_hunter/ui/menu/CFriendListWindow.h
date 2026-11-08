// j: original 155f18 complete FriendList, cc; b4/b8 not written by the original constructor, their purpose not guessed.
#ifndef CFRIENDLISTWINDOW_H
#define CFRIENDLISTWINDOW_H
#include "WindowBase.h"
#include "Vector_ss.h"
class CFriendListWindow : public Window {
public:
    CFriendListWindow();
    virtual inline ~CFriendListWindow() {}
    virtual void OnCommand(Event& event);
    Window* CreateInviteButton();
    Window* CreateFriendsWindow();
    Window* m_windowAc;
    Window* m_friends;
    int m_fieldb4;
    int m_fieldb8;
    // This original path inserts no elements; the destructor only frees storage and does not own child windows.
    Vector<Window*> m_friendWindows;
};
#endif
