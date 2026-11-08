// b074i: original 153312 truly inherits CBaseScreen, total c4; only declares dependencies, not counted as owner recovery.
#ifndef CACHIEVEMENTSWINDOW_H
#define CACHIEVEMENTSWINDOW_H
#include "CBaseScreen.h"
#include "SGImageWindow.h"
class CAchievement;
class CAchievementsWindow : public CBaseScreen {
public:
    CAchievementsWindow();
    virtual ~CAchievementsWindow() {}
    virtual void OnCommand(Event& event);
    // Historical int m_unresolvedC0;
    Window* m_achievementsContainer; // c0
};

// Original 153ac4 truly inherits SGImageWindow, no extra fields, allocates 10c; body belongs to the same cpp.
class AchievementReceivedWindow : public SGImageWindow {
public:
    explicit AchievementReceivedWindow(const CAchievement* achievement);
    virtual ~AchievementReceivedWindow() {}
};
class AchievementInfoWindow : public SGImageWindow {
public:
    explicit AchievementInfoWindow(const CAchievement* achievement);
    virtual ~AchievementInfoWindow() {}
};
#endif
