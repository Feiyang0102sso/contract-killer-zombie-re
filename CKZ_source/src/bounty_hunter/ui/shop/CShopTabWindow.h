// k: original 15eb24 complete owner. STab keeps its aggregate nature; the embedded Vector owns its filter item storage.
#ifndef CSHOPTABWINDOW_H
#define CSHOPTABWINDOW_H
#include "WindowBase.h"
#include "CShopTabContentWindow.h"
#include "SGButton.h"
class CShopTabWindow : public Window {
public:
    struct SFilter {
        // The original Vector copy copies 12B directly, with no memset before the placement copy; has a user-defined constructor.
        SFilter(int normal, int selected, int filter)
            : m_normalAnimation(normal), m_selectedAnimation(selected), m_filter(filter) {}
        int m_normalAnimation;
        int m_selectedAnimation;
        int m_filter;
    };
    struct STab {
        CShopTabContentWindow* m_content;
        Vector<SFilter> m_filters;
        SGToggleButton* m_button;
    };
    CShopTabWindow(int archetype, int character);
    virtual ~CShopTabWindow();
    int AddTab(CShopTabContentWindow* content, int normalAnimation, int selectedAnimation);
    void SetCurrentTab(int index, int focusedItem);
    void AddFiltersForTab(int index, Vector<SFilter>& filters);
    void UpdateFiltersLayout();
    void SetCurrentFilter(int index);
    void UpdateTab(int index);
    virtual void Paint(com::glu::platform::graphics::ICGraphics2d& graphics);
    void RunEasyShow();
    virtual void OnCommand(Event& event);
    Window* m_tabButtons; // ac
    Window* m_filterHost; // b0
    Window* m_filterButtons; // b4
    Window* m_contentHost; // b8
    int m_archetype; // bc
    int m_character; // c0
    Vector<STab> m_tabs; // c4
    Vector<SGToggleButton*> m_filters; // d4
    int m_currentTab; // e4
    int m_currentFilter; // e8
    int m_tabCellCount; // ec, total f0
};
#endif
