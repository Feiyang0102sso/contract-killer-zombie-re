// b074i: original 14acf6/14bb14 directly use the Zombie base class; only the real external implementations are declared.
#ifndef CMISSIONINFOZOMBIEDIALOGWINDOW_H
#define CMISSIONINFOZOMBIEDIALOGWINDOW_H
#include "CZombieDialogWindow.h"
#include "CSharedPtr.h"
#include "SG_Presenter.h"
class CMission;
class TrackHandler;
class CMissionInfoZombieDialogWindow : public CZombieDialogWindow {
public:
    CMissionInfoZombieDialogWindow(const CMission* mission,
        com::glu::platform::core::CSharedPtr<TrackHandler> track, int description,
        int friendId, XString friendName);
    virtual ~CMissionInfoZombieDialogWindow();
    virtual void Init();
    virtual void OnCommand(Event& event);
    static XString ParametrizeTextWithParams(const XString& text, const CMission* mission);
    static Window* CreateRecommendedWeaponWindow(int weaponId);
    SG_Presenter m_presenter; // 118
    // Historical int m_unresolved154; original 14baa8 closes this real window.
    Window* m_avatarWindow;
    com::glu::platform::core::CSharedPtr<TrackHandler> m_track; // 158, total 160.
};
class CEndlessModeInfoZombieDialogWindow : public CZombieDialogWindow {
public:
    CEndlessModeInfoZombieDialogWindow(const CMission* mission, int description);
    virtual ~CEndlessModeInfoZombieDialogWindow() {} // Original 14c1d0/14c1e0 only destruct Zombie; no new members.
};
#endif
