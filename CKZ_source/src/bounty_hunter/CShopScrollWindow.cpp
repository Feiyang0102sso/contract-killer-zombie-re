// CShopScrollWindow.cpp: real free inertial scrolling, keeps the double HandleUpdate; unrecognized events skip TouchMove.
#include "CShopScrollWindow.h"
#include "App.h"
#include "SGImageWindow.h"
#include "SScreenAdaptingConstant.h"
#include <math.h>
#include "MathLib.h"

CShopScrollWindow::CShopScrollWindow()
    : Window(false), m_currentPage(0), m_requestedOffset(0), m_pagerPage(-1),
      m_targetX(0), m_featuredPage(-1), m_released(true), m_velocity(0.0f)
{
    FirstInit();
    ClearFlags(0x100000);
}

void CShopScrollWindow::FirstInit()
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
}

void CShopScrollWindow::Add(Window* window, bool featured, bool selected)
{
    for (int index = 0; index < m_pages.size(); ++index) {
        if (m_pages[index]->IsChildExisting(window)) {
            return;
        }
    }
    Window* page = new Window(false);
    page->SetLayoutType(Window::LAYOUT_NONE);
    int width = window->m_width;
    page->SetRect(m_targetX + m_pages.size() * width, 0, width, window->m_height, true);
    page->AddToFront(window);
    m_pages.addElement(page);
    if (featured == true) {
        m_featuredPage = m_pages.size() - 1;
    }
    if (selected == true) {
        m_requestedOffset = -((m_pages.size() - 1) * width + m_targetX / 2);
    }
    m_contents->AddToFront(page);
    m_pager->SetLayoutType(Window::LAYOUT_SIMPLE);
    m_pager->SetAlign(20);
    m_pager->SetOutsetSpacing(0, 0, SScreenAdaptingConstant(7, 15, -1, 3)(), 0);
    UpdatePager();
    CalculateContentWidthAndHeight();
}

void CShopScrollWindow::UpdatePager()
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

void CShopScrollWindow::CalculateContentWidthAndHeight()
{
    m_contentWidth = 0;
    m_contentHeight = 0;
    for (int index = 0; index < m_pages.size(); ++index) {
        m_contentWidth += m_pages[index]->m_width;
        m_contentHeight += m_pages[index]->m_height;
    }
}

void CShopScrollWindow::Update()
{
    if (m_pages.size() == 0) {
        return;
    }
    int elapsed = WindowApp::m_instance->m_updateFrameTime;
    m_dragTime += elapsed;
    int scale = App::Scale();
    m_pointer.TouchUpdate(elapsed);
    float elapsedFloat = (float)elapsed;
    int movement = scale * 30;
    PointerEventController::Position position = {0, 0};
    PointerEventController::Event event(position, 0, 0, PointerEventController::EVENT_NONE);
    while (m_pointer.GetEvent(event) == true) {
        if (event.m_type == PointerEventController::EVENT_UP) {
            return;
        }
        if (event.m_type == PointerEventController::EVENT_MOVE) {
            m_lastDeltaX = event.m_deltaX;
            m_velocity = (float)event.m_deltaX / elapsedFloat;
            if (m_velocity < -3.0f) {
                m_velocity = -3.0f;
            } else if (m_velocity > 3.0f) {
                // The vmin at original 1210e6 preserves NaN; it must not be replaced with C99 fminf.
                m_velocity = 3.0f;
            }
            MoveContents(event.m_deltaX);
        }
    }
    if (m_released) {
        if (m_pages[0]->m_x > m_targetX) {
            int distance = m_pages[0]->m_x - m_targetX;
            if (distance < movement) {
                movement = distance;
            }
            MoveContents(-movement);
            m_velocity = 0.0f;
        } else if (m_pages[m_pages.size() - 1]->m_x < m_targetX) {
            int distance = (int)fabsf((float)(m_pages[m_pages.size() - 1]->m_x - m_targetX));
            if (distance < movement) {
                movement = distance;
            }
            MoveContents(movement);
            m_velocity = 0.0f;
        } else if (fabsf(m_velocity) > 0.0f) {
            float sign = 0.0f;
            if (m_velocity < 0.0f) {
                sign = -1.0f;
            } else if (m_velocity > 0.0f) {
                sign = 1.0f;
            }
            float distance = fabsf(m_velocity) * elapsedFloat;
            // The original first takes the float min and then multiplies by the direction; distance must not be truncated before multiplying.
            float amount = (float)(scale * 30);
            if (distance < amount) {
                amount = distance;
            }
            MoveContents((int)(sign * amount));
            float previousSign = 0.0f;
            if (m_velocity < 0.0f) {
                previousSign = -1.0f;
            } else if (m_velocity > 0.0f) {
                previousSign = 1.0f;
            }
            m_velocity = m_velocity + (previousSign * elapsedFloat) * -0.002f;
            float nextSign = 0.0f;
            if (m_velocity < 0.0f) {
                nextSign = -1.0f;
            } else if (m_velocity > 0.0f) {
                nextSign = 1.0f;
            }
            if (nextSign != previousSign) {
                m_velocity = 0.0f;
            }
        }
    }
    if (m_currentPage != m_pagerPage) {
        UpdatePager();
    }
}

void CShopScrollWindow::MoveContents(int x, int y)
{
    if (m_pages.size() < 1) {
        return;
    }
    int firstWidth = m_pages[0]->m_width;
    if (m_pages[0]->m_x > m_targetX + firstWidth && x > 0) {
        return;
    }
    if (m_pages[m_pages.size() - 1]->m_x < m_targetX - firstWidth && x < 0) {
        return;
    }
    for (int index = 0; index < m_pages.size(); ++index) {
        Window* page = m_pages[index];
        page->SetCorner(page->m_x + x, page->m_y);
    }
    if (m_pages.size() >= 1) {
        int index = (m_width / 2 - m_pages[0]->m_x) / firstWidth;
        if (index >= m_pages.size() - 1) {
            index = m_pages.size() - 1;
        }
        if (index <= 0) {
            index = 0;
        }
        m_currentPage = index;
    }
}

void CShopScrollWindow::OnPointerEvent(Event& event)
{
    switch (event.m_type) {
    case Event::POINTER_DRAGGED:
        if (!m_released) {
            m_pointer.TouchMove(event.m_param1, event.m_param2);
        }
        break;
    case Event::POINTER_PRESSED: {
        if (IsLocalPos(event.m_param1, event.m_param2) == true) {
            int x = event.m_param1;
            int y = event.m_param2;
            m_pointer.TouchDown(x, y);
            m_pressX = x;
            m_pressY = y;
            m_released = false;
            m_dragTime = 0;
        }
        break;
    }
    case Event::POINTER_RELEASED: {
        m_pointer.TouchUp();
        m_released = true;
        m_dragDistance = (int)(MathLib::Fabs((float)(event.m_param1 - m_pressX)) + MathLib::Fabs((float)(event.m_param2 - m_pressY)));
        App::Scale(); // Original 12141a really keeps this call; the return value is unused.
        break;
    }

    }
}

void CShopScrollWindow::OnUnhandledEvent(Event& event)
{
    // Original 121424 only returns; it does not clear the event.
}

void CShopScrollWindow::SetCurrPageIdx(int index)
{
    m_currentPage = index;
    if (index < 0 || m_pages.size() <= index) {
        m_currentPage = 0;
    }
    Window* page = NULL;
    if (m_pages.size() != 0 && m_currentPage < m_pages.size()) {
        page = m_pages[m_currentPage];
    }
    MoveContents(m_targetX - page->m_x);
}

int CShopScrollWindow::GetCurrPageIdx()
{
    return m_currentPage;
}

void CShopScrollWindow::FlushPages()
{
    for (int index = 0; index < m_pages.size(); ++index) {
        m_pages[index]->Close();
        m_pages[index]->HandleUpdate(true);
        m_pages[index]->HandleUpdate(true);
    }
    m_pages.free_all_items();
    m_featuredPage = -1;
    m_pages.m_size = 0;
    m_pages.m_capacity = 0;
    m_requestedOffset = 0;
}

CShopScrollWindow::~CShopScrollWindow()
{
    // Entities belong to the Window tree; the member vectors are destroyed naturally.
}

void CShopScrollWindow::OnCommand(Event& event)
{
    // Original 12153c only returns.
}

void CShopScrollWindow::OnLayout()
{
    if (m_pages.size() < 1) {
        return;
    }
    m_targetX = (m_width - m_pages[0]->m_width) / 2;
    if (m_pages[m_pages.size() - 1]->m_x > m_targetX) {
        MoveContents(m_targetX - m_pages[m_pages.size() - 1]->m_x + m_requestedOffset);
    }
    if (m_pages[0]->m_x < m_targetX) {
        MoveContents(m_targetX - m_pages[0]->m_x + m_requestedOffset);
    }
}
