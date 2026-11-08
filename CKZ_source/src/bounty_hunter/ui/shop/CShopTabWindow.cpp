// k: complete Tab owner, keeping the repeated device queries, the negative filter boundary and the original tutorial side effects.
#include "CShopTabWindow.h"
#include "ServiceWindows.h"
#include "SScreenAdaptingConstant.h"
#include "App.h"
#include "CDH_SoundManager.h"
#include "CTutorialManager.h"

CShopTabWindow::CShopTabWindow(int archetype, int character)
    : Window(false), m_archetype(archetype), m_character(character),
      m_currentTab(-1), m_currentFilter(-1), m_tabCellCount(0)
{
    Window* header = new Window(false);
    header->SetPercentWidth(100, 0, 0);
    int height;
    if (App::IsKindle() == 1) {
        height = SScreenAdaptingConstant(19, 21, 16, 20)();
    } else {
        height = SScreenAdaptingConstant(19, 19, 16, 20)();
    }
    header->SetPercentHeight(height, 0, 0);
    header->SetAlign(0xc);
    m_tabButtons = new WindowTransparent(false);
    m_tabButtons->SetWidthByContent(0, 0);
    m_tabButtons->SetHeightByContent(0, 0);
    m_tabButtons->SetAlign(0x24);
    header->AddToFront(createTD(m_tabButtons, 0, 0, 3));
    m_filterHost = new WindowTransparent(false);
    m_filterHost->SetWidthByContent(0, 0);
    m_filterHost->SetHeightByContent(0, 0);
    m_filterHost->SetAlign(0x24);
    Window* filtersTable = createTD(m_filterHost, 0, 1, 3);
    header->AddToFront(filtersTable);
    filtersTable->SetOutsetSpacing(SScreenAdaptingConstant(-2)(), 0, 0, 0);
    AddToFront(header);
    HandleUpdate(true);
    m_filterButtons = NULL;
    m_contentHost = new WindowTransparent(false);
    m_contentHost->SetPercentWidth(100, 0, 0);
    height = 80;
    if (App::IsKindle() == 1) {
        height = 84;
    }
    m_contentHost->SetPercentHeight(height, 0, 0);
    m_contentHost->SetAlign(0xc);
    m_contentHost->SetOutsetSpacing(header->m_height + header->m_y, 0, 0, 0);
    AddToFront(m_contentHost);
}

int CShopTabWindow::AddTab(CShopTabContentWindow* content, int normalAnimation, int selectedAnimation)
{
    STab tab;
    tab.m_content = content;
    int index = m_tabs.size();
    SGToggleButton* button = new SGToggleButton(m_archetype, m_character,
                                                normalAnimation, selectedAnimation);
    button->m_allowTurnOff = false;
    button->m_command = 0x82ca6886;
    button->m_commandParameter1 = index;
    button->m_commandParameter2 = 0;
    button->m_commandParameter3 = 0;
    m_tabButtons->AddToFront(button, ++m_tabCellCount, 0);
    tab.m_button = button;
    m_tabs.addElement(tab);
    content->SetLayoutType(LAYOUT_NONE);
    content->ClearFlags(0x2001);
    content->SetCorner(0, 0);
    m_contentHost->AddToFront(content);
    if (index == 0) {
        SetCurrentTab(index, -1);
    }
    return index;
}

void CShopTabWindow::SetCurrentTab(int index, int focusedItem)
{
    if (m_currentTab == index && focusedItem < 0) {
        return;
    }
    if (m_currentTab >= 0) {
        m_tabs[m_currentTab].m_content->ClearFlags(0x2001);
        SGToggleButton* old = m_tabs[m_currentTab].m_button;
        old->m_selected = false;
        old->UpdateToggle();
    }
    m_tabs[index].m_content->SetFlags(0x2001);
    SGToggleButton* current = m_tabs[index].m_button;
    current->m_selected = true;
    current->UpdateToggle();
    m_tabs[index].m_content->m_focusedItem = focusedItem;
    m_currentTab = index;
    int filter = 0;
    if (m_tabs[index].m_filters.size() < 1) {
        filter = -1;
    }
    if (focusedItem >= 0) {
        for (int position = 0; position < m_tabs[index].m_filters.size(); position++) {
            if (m_tabs[index].m_content->IsFilterFilterContainsFocusedItem(m_tabs[index].m_filters[position].m_filter)) {
                filter = position;
                break;
            }
        }
    }
    ((App*)WindowApp::m_instance)->m_soundManager->Play((CDH_SoundManager::eSound)27, false, false);
    RunEasyShow();
    UpdateFiltersLayout();
    SetCurrentFilter(filter);
    WindowApp::HandleTunnelCommand(0x6eb90413, m_currentTab, 0, 0);
}

void CShopTabWindow::AddFiltersForTab(int index, Vector<SFilter>& filters)
{
    m_tabs[index].m_filters = filters;
    if (m_currentTab == index) {
        UpdateFiltersLayout();
        int filter = 0;
        if (filters.size() < 1) {
            filter = -1;
        }
        SetCurrentFilter(filter);
    }
}

void CShopTabWindow::UpdateFiltersLayout()
{
    if (m_currentTab < 0) {
        return;
    }
    if (m_filterButtons != NULL) {
        m_filterButtons->Close();
        m_filterButtons = NULL;
    }
    m_filters.removeAllElements();
    m_filterButtons = new WindowTransparent(false);
    m_filterButtons->SetHeightByContent(0, 0);
    m_filterButtons->SetWidthByContent(0, 0);
    Vector<SFilter>& filters = m_tabs[m_currentTab].m_filters;
    for (int index = 0; index < filters.size(); index++) {
        SGToggleButton* button = new SGToggleButton(m_archetype, m_character,
                                                     filters[index].m_normalAnimation,
                                                     filters[index].m_selectedAnimation);
        button->m_allowTurnOff = false;
        button->m_command = 0x87cb0ee6;
        button->m_commandParameter1 = index;
        button->m_commandParameter2 = 0;
        button->m_commandParameter3 = 0;
        button->SetCellPos(index, 0, 1, 1);
        m_filterButtons->AddToFront(button);
        m_filters.addElement(button);
    }
    m_filterHost->AddToFront(m_filterButtons);
}

void CShopTabWindow::SetCurrentFilter(int index)
{
    if (m_currentTab >= 0) {
        if (index >= 0 && index < m_filters.size()) {
            for (int position = 0; position < m_filters.size(); position++) {
                SGToggleButton* button = m_filters[position];
                button->m_selected = index == position;
                button->UpdateToggle();
            }
            m_currentFilter = index;
        }
        m_tabs[m_currentTab].m_content->SetCurrPageIdx(0);
        UpdateTab(-1);
        RunEasyShow();
    }
}

void CShopTabWindow::UpdateTab(int index)
{
    if (index <= -1) {
        index = m_currentTab;
    }
    int filter = -1;
    if (m_tabs[index].m_filters.size() >= 1) {
        filter = m_tabs[index].m_filters[m_currentFilter].m_filter;
    }
    m_tabs[index].m_content->UpdateContent(filter);
    if (((App*)WindowApp::m_instance)->m_tutorialManager->IsBuyButtonBlink() == 1) {
        SGToggleButton* button = m_tabs[0].m_button;
        if (button != NULL) {
            button->m_blinking = !button->m_selected;
        }
        if (m_filters.size() >= 1 && m_currentTab == 0) {
            button = m_filters[0];
            button->m_blinking = !button->m_selected;
        }
    }
}

void CShopTabWindow::Paint(com::glu::platform::graphics::ICGraphics2d& graphics)
{
    // Original 15f1e0 is only bx lr.
}

CShopTabWindow::~CShopTabWindow()
{
}

void CShopTabWindow::RunEasyShow()
{
    CShopTabContentWindow* content = m_tabs[m_currentTab].m_content;
    Window::EasyParams parameters(content, (EasyLib::EMethod)13);
    parameters.SetFullTime(0.5f);
    parameters.SetOpacity(0.2f, 1.0f);
    content->StartEasy(parameters, 0x100);
}

void CShopTabWindow::OnCommand(Event& event)
{
    if ((unsigned int)event.m_id == 0x82ca6886) {
        SetCurrentTab(event.m_param1, -1);
        event.Clear();
    } else if ((unsigned int)event.m_id == 0x87cb0ee6) {
        SetCurrentFilter(event.m_param1);
        event.Clear();
    }
}
