// CScrollWindow: original 11f698..12020c complete owner, paging drag window, total 0x13c.
#ifndef CSCROLLWINDOW_H
#define CSCROLLWINDOW_H
#include "WindowBase.h"
#include "PointerEventController.h"
class SGButton;

class CScrollWindow : public Window {
public:
    explicit CScrollWindow(bool showArrows);
    virtual ~CScrollWindow();
    virtual void Update();
    virtual void OnPointerEvent(Event& event);
    virtual void OnUnhandledEvent(Event& event);
    virtual void OnCommand(Event& event);
    virtual void OnLayout();
    void Add(Window* window, bool featured, bool selected);
    void ShowArrows();
    void HideArrows();
    void UpdatePager();
    void MoveContents(int x, int y = 0);
    void SetState(int state);
    void UpdateEventBlock();
    void PointerReleased();
    void Init(bool unused);
    void ScrollLeftRight();
    void ScrollRightLeft();
    void SendEventToListener(unsigned int command);

    int m_previousPageX;         // 0xac
    int m_state;                 // 0xb0, not initialized by the original constructor
    int m_currentPage;           // 0xb4
    int m_lastDeltaX;            // 0xb8
    int m_dragDistance;          // 0xbc
    int m_dragTime;              // 0xc0
    float m_speed;               // 0xc4
    PointerEventController m_pointer; // 0xc8
    Vector<Window*> m_pages;      // 0xf0, holds only pointers, entities owned by the Window tree
    Window* m_contents;          // 0x100
    int m_unresolved104[2];      // 0x104/108
    int m_field10c;              // 0x10c
    int m_field110;              // 0x110
    int m_pressX;                // 0x114
    int m_pressY;                // 0x118
    Window* m_pager;             // 0x11c
    int m_pagerPage;             // 0x120
    int m_targetX;               // 0x124
    Window* m_listener;          // 0x128, borrowed
    int m_featuredPage;          // 0x12c
    bool m_snapToPage;           // 0x130
    bool m_showArrows;           // 0x131
    SGButton* m_leftArrow;       // 0x134, the original Init does not write it when false
    SGButton* m_rightArrow;      // 0x138
};
#endif
