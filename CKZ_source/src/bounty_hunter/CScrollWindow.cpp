// CScrollWindow.cpp: restored per the original paging state machine/pointer events/dynamic size reads; the original boundary behavior is not patched.
#include "CScrollWindow.h"
#include "App.h"
#include "SGImageWindow.h"
#include "SGButton.h"
#include "SScreenAdaptingConstant.h"
#include <math.h>
#include "MathLib.h"

// The real PC-relative table of the original Init is at 4ff3b0, not 4f07b0, which ref misattributed by nearest symbol.
static const SScreenAdaptingConstant SCROLL_ARROW_BOTTOM(50, 100, 120, 100);
static const SScreenAdaptingConstant SCROLL_ARROW_SIDE(64, -1, -1, -1);

void CScrollWindow::Add(Window* window, bool featured, bool selected)
{
    for (int index = 0; index < m_pages.size(); ++index) {
        if (m_pages[index]->IsChildExisting(window)) {
            return;
        }
    }
    Window* page = new Window(false);
    page->SetLayoutType(Window::LAYOUT_NONE);
    page->SetRect(m_targetX + m_pages.size() * window->m_width, 0,
                  window->m_width, window->m_height, true);
    page->AddToFront(window);
    m_pages.addElement(page);
    if (featured == true) {
        m_featuredPage = m_pages.size() - 1;
    }
    if (selected == true) {
        m_currentPage = m_pages.size() - 1;
    }
    m_contents->AddToFront(page);
    m_pager->SetLayoutType(Window::LAYOUT_SIMPLE);
    m_pager->SetAlign(20);
    m_pager->SetOutsetSpacing(0, 0, SScreenAdaptingConstant(7, 15, -1, 3)(), 0);
    if (m_showArrows) {
        ShowArrows();
    }
    UpdatePager();
}

void CScrollWindow::ShowArrows()
{
    HideArrows();
    if (m_leftArrow != NULL && m_currentPage >= 1) {
        m_leftArrow->SetFlags(1);
    }
    if (m_rightArrow != NULL && m_currentPage < m_pages.size() - 1) {
        m_rightArrow->SetFlags(1);
    }
}

void CScrollWindow::UpdatePager()
{
    m_pager->CloseChilds();
    m_pager->HandleUpdate(true);
    for (int index = 0; index < m_pages.size(); ++index) {
        SGImageWindow* image = new SGImageWindow();
        image->SetArchetypeCharacter(1, 0);
        int animation = 28;
        if (index == m_currentPage) {
            animation = 29;
        }
        image->SetAnimation(animation, 1);
        image->SetCellPos(index, 0, 1, 1);
        if (index < m_pages.size() - 1) {
            image->SetOutsetSpacing(0, 5, 0, 0);
        }
        image->SetAlign(36);
        m_pager->AddToFront(image);
    }
    m_pager->SetWidthByContent(0, 0);
    m_pagerPage = m_currentPage;
}

void CScrollWindow::Update()
{
    if (m_pages.size() == 0) {
        return;
    }
    int elapsed = WindowApp::m_instance->m_updateFrameTime;
    m_dragTime += elapsed;
    int scale = App::Scale();
    m_pointer.TouchUpdate(elapsed);
    int movement = scale * 30;
    PointerEventController::Event event;
    event.m_position.x = 0;
    event.m_position.y = 0;
    event.m_deltaX = 0;
    event.m_deltaY = 0;
    event.m_type = PointerEventController::EVENT_NONE;
    while (m_pointer.GetEvent(event) == true) {
        if (event.m_type == PointerEventController::EVENT_UP) {
            return;
        }
        if (event.m_type == PointerEventController::EVENT_MOVE) {
            m_lastDeltaX = event.m_deltaX;
            MoveContents(event.m_deltaX);
        }
    }
    if (m_state == 2) {
        Window* page = NULL;
        if (m_pages.size() != 0 && m_currentPage < m_pages.size()) {
            page = m_pages[m_currentPage];
        }
        int x = page->m_x;
        if (x - movement > m_targetX) {
            MoveContents(-movement);
        } else {
            MoveContents(m_targetX - x);
            SetState(0);
        }
    } else if (m_state == 3) {
        Window* page = NULL;
        if (m_pages.size() != 0 && m_currentPage < m_pages.size()) {
            page = m_pages[m_currentPage];
        }
        int x = page->m_x;
        if (x + movement < m_targetX) {
            MoveContents(movement);
        } else {
            MoveContents(m_targetX - x);
            SetState(0);
        }
    } else if (m_snapToPage) {
        Window* page = NULL;
        if (m_pages.size() != 0 && m_currentPage < m_pages.size()) {
            page = m_pages[m_currentPage];
        }
        if (page->m_x < m_targetX) {
            float distance = MathLib::Fabs((float)(page->m_x - m_targetX));
            if ((float)movement < distance) {
                distance = (float)movement;
            }
            MoveContents((int)distance);
        }
        if (page->m_x > m_targetX) {
            float distance = MathLib::Fabs((float)(page->m_x - m_targetX));
            if ((float)movement < distance) {
                distance = (float)movement;
            }
            MoveContents((int)-distance);
        }
    }
    if (m_currentPage != m_pagerPage) {
        UpdateEventBlock();
        UpdatePager();
    }
    m_leftArrow->m_blinking = false;
    m_rightArrow->m_blinking = false;
    if (m_featuredPage >= 0) {
        if (m_pagerPage > m_featuredPage) {
            m_leftArrow->m_blinking = true;
        } else if (m_pagerPage < m_featuredPage) {
            m_rightArrow->m_blinking = true;
        }
    }
}

void CScrollWindow::MoveContents(int x, int y)
{
    if (m_pages.size() >= 1) {
        if (m_pages[0]->m_x > m_width && x > 0) {
            return;
        }
        if (m_pages[m_pages.size() - 1]->m_x < -(m_width / 2) && x < 0) {
            return;
        }
    }
    for (int index = 0; index < m_pages.size(); ++index) {
        Window* page = m_pages[index];
        int pageX = page->m_x;
        int pageY = page->m_y;
        page->SetCorner(pageX + x, pageY);
    }
}

void CScrollWindow::SetState(int state)
{
    if (m_showArrows && (state == 2 || state == 3)) {
        HideArrows();
    }
    if (m_showArrows && state == 0 && (unsigned int)(m_state - 2) <= 1) {
        ShowArrows();
    }
    m_state = state;
}

void CScrollWindow::UpdateEventBlock()
{
    for (int index = 0; index < m_pages.size(); ++index) {
        m_pages[index]->ClearFlags(0x2000);
    }
    Window* page = NULL;
    if (m_pages.size() != 0 && m_currentPage < m_pages.size()) {
        page = m_pages[m_currentPage];
    }
    page->SetFlags(0x2000);
}

void CScrollWindow::OnPointerEvent(Event& event)
{
    if (event.m_type == Event::POINTER_DRAGGED) {
        m_pointer.TouchMove(event.m_param1, event.m_param2);
    } else if (event.m_type == Event::POINTER_PRESSED) {
        int x = event.m_param1;
        int y = event.m_param2;
        m_pointer.TouchDown(x, y);
        m_pressX = x;
        m_pressY = y;
        m_snapToPage = false;
        m_dragTime = 0;
    } else if (event.m_type == Event::POINTER_RELEASED) {
        m_pointer.TouchUp();
        m_snapToPage = true;
        m_dragDistance = (int)(MathLib::Fabs((float)(event.m_param1 - m_pressX)) + MathLib::Fabs((float)(event.m_param2 - m_pressY)));
        if (m_dragDistance > App::Scale() * 20) {
            PointerReleased();
        }
    }
}

void CScrollWindow::PointerReleased()
{
    float distance = (float)m_dragDistance;
    m_speed = MathLib::Fabs(distance / (float)m_dragTime);
    if (m_speed < 0.0f) {
        m_speed = 0.0f;
    } else {
        // The NEON vmin at original 11fcfa preserves NaN; C99 fminf would swallow a single NaN.
        m_speed = (2.0f < m_speed) ? 2.0f : m_speed;
    }
    if (MathLib::Fabs(distance) < (float)(WindowApp::m_instance->m_screenWidth / 2) && m_speed < 0.3f) {
        return;
    }
    if (m_lastDeltaX <= -1) {
        ScrollLeftRight();
    } else if (m_lastDeltaX >= 1) {
        ScrollRightLeft();
    }
}

void CScrollWindow::OnUnhandledEvent(Event& event)
{
    if (((unsigned int)event.m_type | 2) == (unsigned int)Event::POINTER_RELEASED) {
        event.Clear();
    }
}

CScrollWindow::CScrollWindow(bool showArrows)
    : Window(false), m_currentPage(0), m_field10c(0), m_field110(0),
      m_pagerPage(-1), m_targetX(0), m_listener(NULL), m_featuredPage(-1),
      m_snapToPage(true), m_showArrows(showArrows)
{
    Init(false);
}

void CScrollWindow::Init(bool unused)
{
    SetLayoutType(Window::LAYOUT_NONE);
    m_contents = new Window(false);
    AddToFront(m_contents);
    m_pager = new Window(false);
    m_pager->SetLayoutType(Window::LAYOUT_NONE);
    m_pager->SetWidthByContent(0, 0);
    m_pager->SetHeightByContent(0, 0);
    AddToFront(m_pager);
    ClearFlags(0x200000);
    ClearFlags(0x20);
    m_speed = 0.0f;
    if (!m_showArrows) {
        return;
    }
    m_leftArrow = new SGButton(1, 0, 0, 1);
    m_leftArrow->SetLayoutType(Window::LAYOUT_SIMPLE);
    m_leftArrow->SetAlign(9);
    m_leftArrow->m_command = 0x441b9a28;
    m_leftArrow->m_commandParameter1 = 0;
    m_leftArrow->m_commandParameter2 = 0;
    m_leftArrow->m_commandParameter3 = 0;
    int side = SCROLL_ARROW_SIDE();
    int bottom = SCROLL_ARROW_BOTTOM();
    m_leftArrow->SetOutsetSpacing(side, 0, 0, bottom);
    int xPadding = SScreenAdaptingConstant(10)();
    int yPadding = SScreenAdaptingConstant(20)();
    m_leftArrow->m_hitPaddingX = xPadding;
    m_leftArrow->m_hitPaddingY = yPadding;
    m_leftArrow->ClearFlags(1);
    AddToFront(m_leftArrow);
    m_rightArrow = new SGButton(1, 0, 2, 3);
    m_rightArrow->SetLayoutType(Window::LAYOUT_SIMPLE);
    m_rightArrow->SetAlign(10);
    m_rightArrow->m_command = (int)0xb4c8a72a;
    m_rightArrow->m_commandParameter1 = 0;
    m_rightArrow->m_commandParameter2 = 0;
    m_rightArrow->m_commandParameter3 = 0;
    side = SCROLL_ARROW_SIDE();
    bottom = SCROLL_ARROW_BOTTOM();
    m_rightArrow->SetOutsetSpacing(side, bottom, 0, 0);
    xPadding = SScreenAdaptingConstant(10)();
    yPadding = SScreenAdaptingConstant(20)();
    m_rightArrow->m_hitPaddingX = xPadding;
    m_rightArrow->m_hitPaddingY = yPadding;
    m_rightArrow->ClearFlags(1);
    AddToFront(m_rightArrow);
}

void CScrollWindow::ScrollLeftRight()
{
    if (m_currentPage == m_pages.size() - 1) {
        SendEventToListener(0x236889e6);
    }
    if (m_currentPage < m_pages.size() - 1 && m_state != 2) {
        Window* current = NULL;
        if (m_pages.size() != 0 && m_currentPage < m_pages.size()) {
            current = m_pages[m_currentPage];
        }
        m_previousPageX = current->m_x - m_x;
        ++m_currentPage;
        m_pages[m_currentPage]->SetFlags(0x2001);
        SetState(2);
        SendEventToListener(0x087412a4);
    }
}

void CScrollWindow::ScrollRightLeft()
{
    if (m_currentPage >= 1 && m_state != 3) {
        --m_currentPage;
        m_pages[m_currentPage]->SetFlags(0x2001);
        SetState(3);
        SendEventToListener(0xfea612a5);
    }
}

void CScrollWindow::SendEventToListener(unsigned int command)
{
    if (m_listener != NULL) {
        Event event(Event::NONE, 0, 0, 0, 0);
        event.m_id = command;
        m_listener->OnCommand(event);
    }
}

CScrollWindow::~CScrollWindow()
{
    // Page entities belong to the Window tree; only the pointer vector and PointerEventController's internal vectors are destructed naturally.
}

void CScrollWindow::OnCommand(Event& event)
{
    if (event.m_id == 0x441b9a28) {
        ScrollRightLeft();
    } else if (event.m_id == (int)0xb4c8a72a) {
        ScrollLeftRight();
    }
}

void CScrollWindow::HideArrows()
{
    if (m_leftArrow != NULL) {
        m_leftArrow->ClearFlags(1);
    }
    if (m_rightArrow != NULL) {
        m_rightArrow->ClearFlags(1);
    }
}

void CScrollWindow::OnLayout()
{
    if (m_pages.size() >= 1) {
        m_targetX = (m_width - m_pages[0]->m_width) / 2;
    }
}
