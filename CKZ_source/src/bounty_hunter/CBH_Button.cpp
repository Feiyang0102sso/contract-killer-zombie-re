// b074: complete original button implementation, including its original coordinate quirk.
#include "CBH_Button.h"
#include "SScreenAdaptingConstant.h"
#include "WindowApp.h"
#include "App.h"
#include "CDH_SoundManager.h"
#include "MathLib.h"

static SScreenAdaptingConstant STILL_FIRE_DRAG_DELTA(10);

CBH_Button::CBH_Button() : Window(false)
{
    SetLayoutType(LAYOUT_NONE);
    m_commandParameter3 = 0;
    m_commandParameter2 = 0;
    m_commandParameter1 = 0;
    m_toggle = false;
    m_pressed = false;
    m_expandDragArea = true;
    m_command = 0;
    m_hitPaddingX = 0;
    m_hitPaddingY = 0;
}

void CBH_Button::OnPointerEvent(Event& event)
{
    bool hit = false;
    // Only drag/release calculate the hit. Press first records the screen position.
    if ((unsigned int)(event.m_type - Event::POINTER_DRAGGED) <= 1) {
        float factor = 1.0f;
        if (m_expandDragArea) {
            factor = 1.5f;
        }
        float extension = factor - 1.0f;
        float x = event.m_param1;
        float y = event.m_param2;
        if (x >= -(m_width * extension) && x < m_width * factor &&
            y >= -(m_height * extension) && y < m_height * factor) {
            float delta = m_pressGlobalX - m_parent->LocalToGlobalX(m_x);
            if (MathLib::Fabs(delta) < STILL_FIRE_DRAG_DELTA()) {
                delta = m_pressGlobalY - m_parent->LocalToGlobalX(m_y);
                hit = MathLib::Fabs(delta) < STILL_FIRE_DRAG_DELTA();
            }
        }
    }
    // b147: original block order is dragged, pressed, released.
    if (event.m_type == Event::POINTER_DRAGGED) {
        if (!m_toggle) {
            m_pressed = hit;
        }
        event.Clear();
    } else if (event.m_type == Event::POINTER_PRESSED) {
        if (m_toggle) {
            m_pressed = !m_pressed;
            if (hit == 1) {
                OnFire();
                if (m_command != 0) {
                    WindowApp::HandleTunnelCommand(m_command, m_pressed, m_commandParameter1, m_commandParameter2);
                }
                event.Clear();
            }
        } else {
            m_pressed = true;
        }
        ((App*)WindowApp::m_instance)->m_soundManager->Play((CDH_SoundManager::eSound)27, false, false);
        m_pressGlobalX = m_parent->LocalToGlobalX(m_x);
        m_pressGlobalY = m_parent->LocalToGlobalX(m_y);
        event.Clear();
    } else if (event.m_type == Event::POINTER_RELEASED) {
        if (!m_toggle) {
            m_pressed = false;
            if (hit != 1) {
                return;
            }
            FireCommand();
            event.Clear();
        }
    }
}

void CBH_Button::FireCommand()
{
    OnFire();
    if (m_command != 0) {
        WindowApp::HandleTunnelCommand(m_command, m_commandParameter1, m_commandParameter2, m_commandParameter3);
    }
}

void CBH_Button::SetToggle(bool toggle)
{
    m_toggle = toggle;
}

bool CBH_Button::IsLocalPos(int x, int y) const
{
    return x >= -m_hitPaddingX && x < m_width + m_hitPaddingX &&
           y >= -m_hitPaddingY && y < m_height + m_hitPaddingY;
}
