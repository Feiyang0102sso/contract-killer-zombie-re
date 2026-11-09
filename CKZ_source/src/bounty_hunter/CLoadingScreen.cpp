// CLoadingScreen.cpp: original 11d07c..11d7a4, complete owner of the loading tasks and the tips window.
#include "CLoadingScreen.h"
#include "App.h"
#include "BaseDialog.h"
#include "CBH_SurfaceManager.h"
#include "CDH_LoadingBarWidget.h"
#include "DGHelper.h"
#include "CFontMgr.h"
#include "CTipManager.h"
#include "CTip.h"

CLoadingScreen::CLoadingScreen(bool showTips)
    : Window(false), m_loadingBar(NULL), m_infoWindow(NULL), m_tipWindow(NULL),
      m_tip(NULL), m_initialized(false), m_rangeFirst(0), m_rangeLast(100),
      m_currentStep(0), m_completed(0), m_stepFinished(true), m_remaining(100.0f),
      m_finishCommand(0), m_finished(false), m_waitForTap(showTips), m_field102(true)
{
    m_steps.ensureCapacity(10);
}

CLoadingScreen::~CLoadingScreen()
{
    {
        CBH_SurfaceManager* manager = ((App*)WindowApp::m_instance)->m_surfaceManager;
        XString::AnsiString name(m_background.c_str());
        manager->ReleaseSurface(name);
    }
    ClearSteps();
    WindowApp::HandleTunnelCommand(0x3318da15, 0, 0, 0);
}

void CLoadingScreen::ClearSteps()
{
    for (int index = 0; index < m_steps.size(); ++index) {
        if (m_steps[index]->IsDestructible() == true) {
            IStep* step = m_steps[index];
            if (step != NULL) {
                delete step;
                m_steps[index] = NULL;
            }
        }
    }
    m_steps.setSize(0);
}

void CLoadingScreen::SetInfoWindow(Window* window)
{
    m_infoWindow = window;
    if (m_initialized) {
        AddToFront(window);
    }
}

void CLoadingScreen::AddLoadingStep(IStep* step, const char* name)
{
    m_stepNames.addElement(name);
    m_steps.addElement(step);
    UpdateSteps();
}

void CLoadingScreen::UpdateSteps()
{
    m_total = 0;
    for (int index = 0; index < m_steps.size(); ++index) {
        m_total += m_steps[index]->GetTotal();
    }
}

void CLoadingScreen::Init()
{
    ImageWindow* background = new ImageWindow();
    if (m_background.ByteSize() == 0) {
        m_background = ((App*)WindowApp::m_instance)->m_helper->GetRandomBackgroundResId();
    }
    com::glu::platform::graphics::ICRenderSurface* surface;
    {
        CBH_SurfaceManager* manager = ((App*)WindowApp::m_instance)->m_surfaceManager;
        XString::AnsiString name(m_background.c_str());
        surface = manager->CreateSurface(name, true, false);
    }
    background->SetImage(surface);
    background->SetLayoutType(Window::LAYOUT_NONE);
    background->SetAlign(9);
    AddToFront(background);
    m_loadingBar = new CDH_LoadingBarWidget();
    m_loadingBar->m_progress = m_rangeFirst;
    AddToFront(m_loadingBar);
    com::glu::platform::graphics::CFont* font = CFontMgr::GetInstance()->GetFont(29);
    if (font == NULL) {
        return;
    }
    WindowTransparent* tips = new WindowTransparent(false);
    tips->SetAlign(18);
    tips->SetPercentWidth(50, 0, 0);
    tips->SetPercentHeight(32, 0, 0);
    m_tipWindow = new TextWindow();
    m_tipWindow->SetFont(font);
    m_tipWindow->SetAlign(36);
    m_tipWindow->SetPercentWidth(80, 0, 0);
    tips->AddToFront(m_tipWindow, 0, 0);
    WindowTransparent* spacer = new WindowTransparent(false);
    spacer->SetLayoutType(Window::LAYOUT_TABLE);
    spacer->SetPercentHeight(10, 0, 0);
    tips->AddToFront(spacer, 0, 1);
    AddToFront(tips);
    UpdateSteps();
    if (m_infoWindow != NULL && !m_infoWindow->IsChildOf(this)) {
        AddToFront(m_infoWindow);
    }
    m_initialized = true;
}

void CLoadingScreen::SetFinishCmd(unsigned int id)
{
    m_finishCommand = id;
}

void CLoadingScreen::Update()
{
    if (m_finished) {
        return;
    }
    if (m_tip == NULL && m_tipWindow != NULL) {
        CTipManager* manager = ((App*)WindowApp::m_instance)->m_tipManager;
        if (manager != NULL) {
            m_tip = manager->GetRandomTip();
            if (m_tip != NULL && m_field102) {
                m_tipWindow->SetText(m_tip->m_description);
            }
        }
    }
    if (m_currentStep >= (unsigned int)m_steps.size()) {
        m_loadingBar->setState(CDH_LoadingBarWidget::STATE_TAP);
        m_finished = true;
        ClearSteps();
        if (!m_waitForTap) {
            SetInterrupt(1);
        }
        if (m_finishCommand != 0) {
            WindowApp::HandleTunnelCommand(m_finishCommand, 0, 0, 0);
        }
        return;
    }
    IStep* step = m_steps[m_currentStep];
    bool loaded = step->Load(this);
    unsigned int current = step->GetStep();
    m_stepFinished = false;
    if (loaded) {
        m_stepFinished = true;
        ++m_currentStep;
        m_completed += step->GetTotal();
        current = 0;
    }
    // The original multiplication wraps around in the 32-bit unsigned integer domain first, and the division is also unsigned; must not convert to float first.
    unsigned long remaining = (m_rangeLast - m_rangeFirst) * (m_total - current - m_completed) / m_total;
    m_remaining = (float)remaining;
    m_loadingBar->m_progress = (int)((float)m_rangeLast - m_remaining);
}

void CLoadingScreen::SetRange(unsigned long first, unsigned long last, bool flag)
{
    m_remaining = (float)(last - first);
    m_rangeFirst = first;
    m_rangeLast = last;
    m_waitForTap = flag;
}

void CLoadingScreen::OnPointerPressed(Event& event)
{
    if (m_finished) {
        SetInterrupt(1);
        event.Clear();
    }
}
