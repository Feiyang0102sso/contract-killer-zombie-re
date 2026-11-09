// CTabWindow: original 1215ac..122640, full tab window, total 0x130.
#ifndef CTABWINDOW_H
#define CTABWINDOW_H
#include "WindowBase.h"
class CBH_ProgressBar;
class CTabWindow : public Window {
public:
    struct TTab {
        XString m_name;          // 00
        Window* m_window;        // 04, owned by the Window tree
        struct Bounds {
            Bounds()
            {
                m_width = 0;
                m_height = 0;
                m_x = 0;
                m_y = 0;
            }

            short m_x;              // 08
            short m_y;              // 0a
            short m_width;          // 0c
            short m_height;         // 0e
        } m_bounds;
        ImageRes m_image;        // 10
        bool m_locked;           // 18, total 1c
    };
    explicit CTabWindow(bool showInfo);
    virtual ~CTabWindow();
    virtual void OnGetFocus();
    virtual void OnPointerEvent(Event& event);
    virtual void Paint(com::glu::platform::graphics::ICGraphics2d& graphics);
    void CreateInfoTab();
    void AddTab(const XString& name, Window* window, int padding);
    void AddTab(const XString& name, Window* window, const char* image, bool locked, int padding);
    int GetCaptionHeight();
    int GetTabWidth();
    void SetCurrTab(int index, bool fromPointer);
    void DrawInfoTab(com::glu::platform::graphics::ICGraphics2d& graphics);
    void UpdateInfoTab();
    void SetTabsWidthByWindowWidth();

    Vector<TTab*> m_tabs;        // ac
    TTab* m_infoTab;             // bc
    bool m_showInfo;             // c0
    Window* m_infoWindow;        // c4
    CBH_ProgressBar* m_moneyBar; // c8
    CBH_ProgressBar* m_creditsBar; // cc
    ImageRes m_images[11];       // d0
    int m_currentTab;            // 128
    com::glu::platform::graphics::CFont* m_font; // 12c
};
#endif
