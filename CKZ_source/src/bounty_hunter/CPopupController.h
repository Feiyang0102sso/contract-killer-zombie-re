// PROVISIONAL (created by b057x): owner src/bounty_hunter/CPopupController.cpp not written yet
// CPopupController.h - popups (a window); only what src/general/App.cpp uses is declared.
// b074i: the thin declaration above is historical; now restores the complete 140 layout of WindowTransparent and the 7 queue groups.
#ifndef CPOPUPCONTROLLER_H
#define CPOPUPCONTROLLER_H

#include <stddef.h>
#include "WindowBase.h"
#include "xString.h"
#include "Vector_ss.h"

class AnimatedWindow;
class CAchievement;  // added by b077

class CPopupController : public WindowTransparent {
public:
    CPopupController();
    virtual ~CPopupController();
    void ClearPopups(bool immediate); // b067h: original 0x11e424, a real external call from Game.
    /** Queue the "achievement unlocked" popup. */  // added by b077
    void AddAchievementPopup(const CAchievement* achievement);  // added by b077
    // Partial enum values observed in CScriptedSceneController; labels are descriptive.
    enum EPosition { Position0=0, Position1=1, Position2=2, Position3=3, Position4=4, Position5=5, Position6=6 };
    enum EAnimationStyle { AnimationStyle0=0, AnimationStyle1=1, AnimationStyle2=2, AnimationStyle3=3, AnimationStyle4=4, AnimationStyle5=5 };
    void AddTextPopup(const XString& text, EPosition position, EAnimationStyle animation,
                      bool flag, int style); // 0x11e3e8

    virtual void OnCommand(Event& event);
    virtual void Update();
    void AddPopup(const char* icon, XString text, EPosition position, EAnimationStyle animation, bool flag, int fontId);
    void SetupStyle(AnimatedWindow* window, EAnimationStyle animation);
    void SetupPosition(AnimatedWindow* window, EPosition position);
    void AddPopupInQueue(Window* window, EPosition position);
    static const unsigned int POPUP_TIME_INTERVAL_MS[7]; // original 45ddf4, full 28B.
    static const int POPUP_STACK_MAX_SIZE[7]; // Original 45de10 is only 28B, followed by another owner's UTF32; not a 1288B array.

    // unsigned char m_dataac[0x140 - 0xac];    // layout not known yet (size from App.cpp)
    Vector<Window*> m_queues[7]; // +ac..11b
    unsigned int m_lastPopupMS[7]; // +11c..137
    int m_warningDelayMS; // +138
    int m_skillshotCount; // +13c
};

#endif
