// CFpsCursor.cpp - cursor of the shooting game.
#include "CFpsCursor.h"
#include "App.h"
#include "AppSettings.h"
#include "DGHelper.h"
#include "CDH_BasicGameData.h"
#include "CDH_Weapon.h"
#include "CSwerveGame.h"
#include "CFontMgr.h"
#include "CFont.h"
#include "CUtility.h"
#include "SG_Presenter.h"
#include "dgCamera.h"

using namespace com::glu::platform;

/** UpdateImpl: zoom to sight factor, sight gap without and with full zoom. */
static const float ZOOM_SCALE = 0.015625f;
static const float SIGHT_GAP_NO_ZOOM = 16.0f;
static const float SIGHT_GAP_FULL_ZOOM = 4.0f;

CFPSCursor::CFPSCursor()
{
    m_shotPresenter = NULL;
    m_pressX = 0;
    m_pressY = 0;
    m_pressCamera1c = 0;
    m_pressCamera0c = 0;
    m_pressed = false;
    m_moved = false;
    m_shootRequested = false;
    m_shotAnimating = false;
    m_showCross = true;
    m_sightWindow = NULL;
    m_aimX = ((App*)WindowApp::m_instance)->m_helper->m_screenWidth / 2;
    m_aimY = ((App*)WindowApp::m_instance)->m_helper->m_screenHeight / 2;
}

CFPSCursor::~CFPSCursor()
{
    if (m_shotPresenter != NULL) {
        delete m_shotPresenter;
        m_shotPresenter = NULL;
    }
}

void CFPSCursor::ShowCross(bool show)
{
    m_showCross = show;
}

void CFPSCursor::onFire()
{
    HandleShot();
}

void CFPSCursor::HandleShot()
{
    CDH_Weapon* weapon = ((App*)WindowApp::m_instance)->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon();
    if (weapon == NULL) {
        return;
    }
    if (weapon->IsReadyForUse() == 1) {
        DGHelper* helper = ((App*)WindowApp::m_instance)->m_helper;
        if (helper->m_gamePlayManager.GetGameData()->GetActiveWeapon()->m_ammo > 0) {
            CSwerveGame* game = ((App*)WindowApp::m_instance)->m_helper->m_swerveGame;
            game->Shot(GetAimX(), GetAimY());
            m_shotAnimating = true;
        }
    }
    m_shootRequested = false;
}

void CFPSCursor::onPointerMove(int x, int y, int deltaX, int deltaY)
{
    m_moved = true;
}

void CFPSCursor::onPointerBreak()
{
    m_pressed = false;
    m_moved = false;
    m_shootRequested = false;
}

void CFPSCursor::onPointerPressed(int x, int y)
{
    m_pressed = true;
    m_pressX = x;
    m_pressY = y;
    DGCamera* camera = ((App*)WindowApp::m_instance)->m_helper->m_swerveGame->m_camera;
    if (camera != NULL) {
        m_pressCamera1c = camera->m_field1c;
        m_pressCamera0c = camera->m_field0c;
    }
}

void CFPSCursor::onPointerReleased()
{
    bool shoot = false;
    if (!((App*)WindowApp::m_instance)->m_settings->IsFireButtonEnabled() && m_pressed && !m_moved) {
        shoot = ((App*)WindowApp::m_instance)->m_helper->m_swerveGame->m_tapToShoot;
    }
    m_shootRequested = shoot;
    m_pressed = false;
    m_moved = false;
}

void CFPSCursor::DrawImpl()
{
    if (App::m_platformType == 0) {
        DrawReloadingMessage();
    }
}

void CFPSCursor::DrawReloadingMessage()
{
    CDH_Weapon* weapon = ((App*)WindowApp::m_instance)->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon();
    if (weapon == NULL || weapon->IsReloading() != 1) {
        return;
    }
    graphics::CFont* font = CFontMgr::GetInstance()->GetFont(1);
    if (font == NULL) {
        return;
    }
    components::CStrWChar text = CUtility::GetString("IDS_RELOADING");
    int x = GetAimX();
    int y = GetAimY();
    font->PaintText(text.m_string, text.m_length, x - font->MeasureTextWidth(text.m_string, -1, -1, NULL) / 2,
                    y - font->GetFontHeight() / 2, -1, -1, NULL);
}

void CFPSCursor::UpdateImpl(int elapsedMs)
{
    if (m_shootRequested && !((App*)WindowApp::m_instance)->m_settings->IsFireButtonEnabled()) {
        HandleShot();
    }
    int zoom = (int)((App*)WindowApp::m_instance)->m_helper->m_swerveGame->m_zoom;
    float factor = zoom * ZOOM_SCALE;
    // ?: with the constant on the left gives the original NEON vmax / vmin
    factor = (factor < 0.0f) ? 0.0f : factor;
    factor = (1.0f < factor) ? 1.0f : factor;
    float gap = (1.0f - factor) * SIGHT_GAP_NO_ZOOM + factor * SIGHT_GAP_FULL_ZOOM;
    int length = (int)gap;
    if (m_sightWindow != NULL) {
        m_sightWindow->m_length = length;
        int size = length * 2 + zoom * 2;
        m_sightWindow->SetDesiredHeight(size);
        m_sightWindow->SetDesiredWidth(size);
    }
    if (m_shotPresenter == NULL) {
        return;
    }
    if (m_shotAnimating && m_shotPresenter->HasFinished()) {
        m_shotAnimating = false;
    }
    m_shotPresenter->Update(WindowApp::m_instance->m_updateFrameTime);
}

void CFPSCursor::InitImpl()
{
    ((App*)WindowApp::m_instance)->m_helper->m_swerveGame->m_camera->SetWanderngByActiveWeapon();
    CreateSightWindow();
}

void CFPSCursor::CreateSightWindow()
{
    m_sightWindow = new SightWindow();
    m_sightWindow->SetAlign(0x24);
    m_sightWindow->SetDesiredHeight(0x80);
    m_sightWindow->SetDesiredWidth(0x80);
    ((App*)WindowApp::m_instance)->m_helper->m_swerveGame->AddToFront(m_sightWindow);
}

void CFPSCursor::DeInitImpl()
{
}

void CFPSCursor::onWeaponSwitch()
{
    ((App*)WindowApp::m_instance)->m_helper->m_swerveGame->m_camera->SetWanderngByActiveWeapon();
}

void CFPSCursor::ShowSight(bool show)
{
    if (m_sightWindow == NULL) {
        return;
    }
    int visibleFlag = Window::FLAG_VISIBLE;
    if (show) {
        m_sightWindow->SetFlags(visibleFlag);
    } else {
        m_sightWindow->ClearFlags(visibleFlag);
    }
}

void CFPSCursor::EnableSight(bool enable)
{
    if (enable) {
        if (m_sightWindow == NULL) {
            CreateSightWindow();
        }
    } else {
        if (m_sightWindow != NULL) {
            m_sightWindow->Close();
            m_sightWindow = NULL;
        }
    }
}

void CFPSCursor::SightWindow::Paint(graphics::ICGraphics2d& graphics)
{
    // center dot
    FillRect(graphics, m_width / 2 - m_thickness / 2, m_height / 2 - m_thickness / 2, m_thickness, m_thickness,
             0xffffffff);
    // bottom, top, right and left bars
    FillRect(graphics, m_width / 2 - m_thickness / 2, m_height - m_length, m_thickness, m_length, 0xffffffff);
    FillRect(graphics, m_width / 2 - m_thickness / 2, 0, m_thickness, m_length, 0xffffffff);
    FillRect(graphics, m_width - m_length, m_height / 2 - m_thickness / 2, m_length, m_thickness, 0xffffffff);
    FillRect(graphics, 0, m_height / 2 - m_thickness / 2, m_length, m_thickness, 0xffffffff);
}
