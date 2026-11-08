// DGCursor.cpp - aiming cursor of the 3D game.
#include "DGCursor.h"
#include "CDH_Accelerometer.h"
#include "App.h"
#include "AppSettings.h"
#include "DGHelper.h"
#include "CSwerveGame.h"

/** Scale of the tilt movement added to the pointer movement (12 bit fixed point). */
static const float TILT_TO_FIXED = 4096.0f;

DGCursor::DGCursor()
{
    m_visible = true;
    m_accelerometer = new CDH_Accelerometer();
    m_lastTiltX = 0;
    m_lastTiltY = 0;
}

DGCursor::~DGCursor()
{
    if (m_accelerometer != NULL) {
        delete m_accelerometer;
        m_accelerometer = NULL;
    }
}

void DGCursor::PointerMove(int x, int y, int deltaX, int deltaY)
{
    if (!m_visible) {
        return;
    }
    if (((App*)WindowApp::m_instance)->m_settings->IsAccelerationEnabled() == true) {
        float tiltX = m_accelerometer->m_positionX;
        float tiltY = m_accelerometer->m_positionY;
        onPointerMove(x, y, (deltaX << 12) + (int)((tiltX - m_lastTiltX) * TILT_TO_FIXED),
                      (deltaY << 12) + (int)((tiltY - m_lastTiltY) * TILT_TO_FIXED));
        m_lastTiltX = tiltX;
        m_lastTiltY = tiltY;
    } else {
        onPointerMove(x, y, deltaX, deltaY);
    }
}

void DGCursor::update(int elapsedMs)
{
    if (!m_visible) {
        return;
    }
    MovingEntity::update(elapsedMs);
    if (((App*)WindowApp::m_instance)->m_settings->IsAccelerationEnabled() == true) {
        m_accelerometer->update(elapsedMs);
    }
    UpdateImpl(elapsedMs);
}

void DGCursor::Init()
{
    onPointerBreak();
    InitImpl();
}

void DGCursor::DeInit()
{
    DeInitImpl();
}

void DGCursor::PointerBreak()
{
    onPointerBreak();
}

void DGCursor::draw()
{
    if (m_visible) {
        DrawImpl();
    }
}

void DGCursor::Hide()
{
    m_visible = false;
    onPointerReleased();
}

void DGCursor::PointerPressed(int x, int y)
{
    if (m_visible) {
        onPointerPressed(x, y);
    }
}

void DGCursor::PointerReleased()
{
    if (m_visible) {
        onPointerReleased();
    }
}

void DGCursor::Fire()
{
    if (((App*)WindowApp::m_instance)->m_helper->m_swerveGame->m_state == 2) {
        if (m_visible) {
            onFire();
        }
    }
}
