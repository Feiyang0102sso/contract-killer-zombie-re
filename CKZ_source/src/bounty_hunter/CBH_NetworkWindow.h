// b074: network/safehouse screen, BaseScreen c0 plus an original unused word and focus flag.
#ifndef CBH_NETWORKWINDOW_H
#define CBH_NETWORKWINDOW_H
#include "CBaseScreen.h"
class CBH_NetworkWindow : public CBaseScreen {
public:
    CBH_NetworkWindow();
    virtual ~CBH_NetworkWindow();
    virtual void OnStartupEasy();
    virtual void Init();
    virtual void Shutdown();
    virtual void OnClosed();
    virtual void OnGetFocus();
    virtual void OnCommand(Event& event);
    virtual void Update();
    virtual void ReleaseResources(); // bc
    Window* CreateMenuBar();
    int m_unresolvedC0; // c0, no initialization in original ctor
    bool m_logOnFocus; // c4, original 1183a0 writes only this bool.
    // The original GPSGame 11aa9a/11b8a6 allocates 0xcc; the old c8 was only a lower bound from this owner's maximum access.
    int m_unresolvedC8; // c8, not initialized by the original ctor, semantics not yet proven; full size cc.
};
#endif
