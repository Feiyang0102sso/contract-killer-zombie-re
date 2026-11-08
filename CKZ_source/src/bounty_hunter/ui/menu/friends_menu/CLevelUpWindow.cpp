// j: complete level-up animation state machine, keeping the state0 early return and the strict unsigned timing boundary.
#include "CLevelUpWindow.h"
#include "CDH_SoundManager.h"
#include "CPopupController.h"
#include "App.h"

CLevelUpWindow::CLevelUpWindow()
    : Window(false), m_world(App::LoadObject3D("BIN_M3G_OBJECT_HEAD_WAS_SHOT")),
      m_remainingMS(0), m_state(0)
{
    m_world.getBackground().setColorClearEnable(false);
    m_animations.LoadFrom(m_world);
    m_currentAnimation = NULL;
    m_remainingMS = 1500;
}

void CLevelUpWindow::NextState()
{
    int nextState = m_state + 1;
    if (m_state == 0) {
        ((App*)WindowApp::m_instance)->m_soundManager->Play((CDH_SoundManager::eSound)29, false, false);
        m_currentAnimation = m_animations.Anim(SwerveHelper::Id("start"));
        if (m_currentAnimation != NULL) {
            m_currentAnimation->Start(0.0f, 1.0f, 0.0f, 0.0f, 1.0f);
        }
    } else if (m_state == 1) {
        m_currentAnimation = NULL;
        CPopupController* popups = ((App*)WindowApp::m_instance)->m_popupController;
        popups->AddTextPopup(ResString("IDS_POPUP_LEVELUP"), CPopupController::Position4,
            CPopupController::AnimationStyle5, true, 30);
        m_remainingMS = 2000;
    } else if (m_state == 2) {
        m_currentAnimation = m_animations.Anim(SwerveHelper::Id("end"));
        if (m_currentAnimation != NULL) {
            m_currentAnimation->Start(0.0f, 1.0f, 0.0f, 0.0f, 1.0f);
        }
    }
    m_state = nextState;
}

void CLevelUpWindow::Update()
{
    if (m_state == 0) {
        unsigned int elapsed = WindowApp::m_instance->m_updateFrameTime;
        if (elapsed > m_remainingMS) {
            NextState();
        } else {
            m_remainingMS -= elapsed;
        }
        return;
    }
    if (m_state == 2) {
        unsigned int elapsed = WindowApp::m_instance->m_updateFrameTime;
        if (elapsed > m_remainingMS) {
            NextState();
        } else {
            m_remainingMS -= elapsed;
        }
    } else if (m_currentAnimation != NULL && !m_currentAnimation->IsPlaying()) {
        if (m_state == 3) {
            Close();
        } else {
            NextState();
        }
    }
    m_world.animate(WindowApp::m_instance->m_updateTime);
    m_animations.Update();
}

void CLevelUpWindow::Paint3D(swerve::Graphics3D<CSwerveAssertingErrorHandler>& graphics)
{
    graphics.render(m_world.m_interface);
}

void CLevelUpWindow::Paint(com::glu::platform::graphics::ICGraphics2d& graphics)
{
    RunPaint3D(graphics);
}
