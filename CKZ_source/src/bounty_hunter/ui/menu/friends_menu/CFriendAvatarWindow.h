// j: original 158bac complete avatar window, d8; the original Update indeed has no network/async body.
#ifndef CFRIENDAVATARWINDOW_H
#define CFRIENDAVATARWINDOW_H
#include "WindowBase.h"
#include "CStrWChar.h"
class ImageWindow;
class CWaitWindow;
extern const int EventId_FriendAvatarDone;

class CFriendAvatarWindow : public Window {
public:
    CFriendAvatarWindow(int unusedFriendId);
    CFriendAvatarWindow(com::glu::platform::components::CStrWChar name);
    virtual ~CFriendAvatarWindow();
    void Init(com::glu::platform::components::CStrWChar name);
    virtual void Update();
    int m_fieldac; // Not written by the original ctor/Init.
    bool m_flagb0;
    bool m_flagb1;
    bool m_flagb2;
    int m_fieldb4;
    ImageWindow* m_image;
    ImageWindow* m_placeholder;
    CWaitWindow* m_wait;
    com::glu::platform::components::CStrWChar m_name; // c4
    const char* m_surfaceName; // d4; resources are released only via SurfaceManager.
};
#endif
