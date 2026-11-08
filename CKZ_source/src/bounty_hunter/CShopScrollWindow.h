// CShopScrollWindow: full owner of original 120d2c..1215ac, an independent Window derivative that does not inherit CScrollWindow.
#ifndef CSHOPSCROLLWINDOW_H
#define CSHOPSCROLLWINDOW_H
#include "WindowBase.h"
#include "PointerEventController.h"
class CShopScrollWindow : public Window {
public:
    CShopScrollWindow();
    virtual ~CShopScrollWindow();
    virtual void Update();
    virtual void OnPointerEvent(Event& event);
    virtual void OnUnhandledEvent(Event& event);
    virtual void OnCommand(Event& event);
    virtual void OnLayout();
    void FirstInit();
    void Add(Window* window, bool featured, bool selected);
    void UpdatePager();
    void CalculateContentWidthAndHeight();
    void MoveContents(int x, int y = 0);
    void SetCurrPageIdx(int index);
    int GetCurrPageIdx();
    void FlushPages();

    int m_unresolvedAc;              // 0xac
    int m_currentPage;               // 0xb0
    int m_lastDeltaX;                // 0xb4
    int m_dragDistance;              // 0xb8
    int m_dragTime;                  // 0xbc
    float m_speed;                   // 0xc0, zeroed by FirstInit, not read by Update
    int m_requestedOffset;           // 0xc4
    PointerEventController m_pointer; // 0xc8
    Vector<Window*> m_pages;          // 0xf0, entities belong to the Window tree
    Window* m_contents;              // 0x100
    int m_contentWidth;              // 0x104
    int m_contentHeight;             // 0x108
    int m_pressX;                    // 0x10c
    int m_pressY;                    // 0x110
    Window* m_pager;                 // 0x114
    int m_pagerPage;                 // 0x118
    int m_targetX;                   // 0x11c
    int m_unresolved120;             // 0x120
    int m_featuredPage;              // 0x124
    bool m_released;                 // 0x128
    float m_velocity;                // 0x12c, total 0x130
};
#endif
