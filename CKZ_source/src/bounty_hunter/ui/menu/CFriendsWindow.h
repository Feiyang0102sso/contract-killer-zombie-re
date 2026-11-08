// b074i: original 15624a truly inherits CBaseScreen, total 12c; declares dependencies only, not counted as owner recovery.
#ifndef CFRIENDSWINDOW_H
#define CFRIENDSWINDOW_H
#include "CBaseScreen.h"
#include "CVector.h"
#include "CStrWChar.h"
class CTabWindow;
class CFriendListWindow;
class CNewsFeedWindow;
class CImageButton;
// Original 156878 element stride is 20h; the destructor frees only the first XString; the other seven words have no owning operations.
// No producer found in this game; no business meaning is invented for the unused words.
struct SFriendInfo {
    XString m_name;
    unsigned int m_unresolvedWords[7];
};
class CFriendsWindow : public CBaseScreen {
public:
    CFriendsWindow();
    virtual inline ~CFriendsWindow() {}
    virtual void OnCommand(Event& event);
    virtual void Update();
    virtual void RefreshFriendTab(); // new in original vtable: bc
    // Historical thin header: unsigned char m_unresolvedC0[0x12c - 0xc0];
    void CreateUserInteface(); // Keeps the original symbol spelling.
    void CreateLoginButtons();
    Window* CreateNewsFeedTab();
    com::glu::platform::core::CVector<SFriendInfo> m_friendInfo; // c0
    CTabWindow* m_tabs; // d8
    int m_unresolvedDc;
    com::glu::platform::core::CVector<com::glu::platform::components::CStrWChar> m_friendNames; // e0
    Vector<Window*> m_windows; // f8, this owner inserts no elements.
    int m_selectedGift; // 108
    CFriendListWindow* m_friends; // 10c
    CNewsFeedWindow* m_news; // 110
    CImageButton* m_facebookLogin; // 114
    CImageButton* m_gameCenterLogin; // 118
    int m_unresolved11c;
    int m_field120;
    bool m_flag124;
    bool m_flag125;
    int m_unresolved128;
};
#endif
