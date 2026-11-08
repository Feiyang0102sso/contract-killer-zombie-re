// b074i: original 154204 truly inherits Window, allocates cc; only declares dependencies, not counted as owner recovery.
#ifndef CCAREERWINDOW_H
#define CCAREERWINDOW_H
#include "WindowBase.h"
#include "BaseDialog.h"
#include "Vector_ss.h"
class CCareerWindow : public Window {
public:
    CCareerWindow();
    virtual inline ~CCareerWindow() {}
    virtual void OnClosed();
    virtual void OnCommand(Event& event);
    virtual void Update();
    virtual void Paint(com::glu::platform::graphics::ICGraphics2d& graphics);
    struct StatItemInfo {
        // The original Vector copies 3 words at a time with no preceding memset, supporting a user constructor rather than a zeroed aggregate.
        StatItemInfo(const char* name, int value, bool time, bool percent, bool gameCenter)
            : m_name(name), m_value(value), m_time(time), m_percent(percent), m_gameCenter(gameCenter) {}
        const char* m_name;
        int m_value;
        bool m_time;
        bool m_percent;
        bool m_gameCenter;
    };
    void FillStats();
    // Historical unsigned char m_unresolvedAc[0xcc - 0xac], now real members.
    Window* m_content;           // ac
    ImageRes m_background;       // b0
    int m_elapsedMS;             // b8
    Vector<StatItemInfo> m_stats; // bc
};

class VertScrollerSimpleRed : public ScrollingContainer::VertScroller {
public:
    VertScrollerSimpleRed(ScrollingContainer* container);
    virtual inline ~VertScrollerSimpleRed() {}
    virtual void Paint(com::glu::platform::graphics::ICGraphics2d& graphics);
};

class SimpleGreyProgressBar : public Window {
public:
    SimpleGreyProgressBar(float percent);
    virtual inline ~SimpleGreyProgressBar() {}
    virtual void Paint(com::glu::platform::graphics::ICGraphics2d& graphics);
    ImageRes m_background;
    ImageRes m_center;
    ImageRes m_left;
    ImageRes m_right;
    float m_percent;
};

class StatItem : public Window {
public:
    StatItem(const CCareerWindow::StatItemInfo& info, int index);
    virtual ~StatItem() {}
    virtual void OnStartupEasy();
    int m_index;
    bool m_flagb0;
};
#endif
