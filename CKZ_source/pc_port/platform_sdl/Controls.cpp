// Translate desktop keys to the same HUD touch targets as the iOS controls.
// Which input maps to which action lives in KeyBindings.h.
#include "Host.h"
#include <stdio.h>
#include "App.h"
#include "AppSettings.h"
#include "CApplet.h"
#include "CGameplayHUD.h"
#include "CSwerveGame.h"
#include "CDH_BasicGameData.h"
#include "CDH_Weapon.h"
#include "dgCamera.h"
#include "DGHelper.h"
#include "CGPSMapGame.h"
#include "CGPSMapScreen.h"

namespace ckz {
namespace {

// Each kind of synthetic touch has its own finger, so releasing one never releases another.
constexpr unsigned int LookTouchId = 1;   // the view drag; in menus the mouse pointer is touch 1
constexpr unsigned int ActionTouchId = 3; // one-shot taps on HUD controls
constexpr unsigned int FireTouchId = 4;   // the held fire button

/** Pack a game pixel position the way touch events carry it. */
unsigned int EncodePosition(int x, int y)
{
    return (x & 0x3fff) | ((y & 0x3fff) << 14);
}

/** Return the HUD of the running mission, or nullptr outside of gameplay. */
CGameplayHUD* GetActiveHud()
{
    auto* app = static_cast<App*>(WindowApp::m_instance);
    if (app == nullptr || app->m_helper == nullptr) {
        return nullptr;
    }
    CSwerveGame* game = app->m_helper->m_swerveGame;
    if (game == nullptr || !game->m_sceneReady || !game->m_tapToShoot) {
        return nullptr;
    }
    return game->GetHUD();
}

/** Return the HUD control that performs the action; nullptr when the HUD has none. */
Window* GetActionControl(CGameplayHUD& hud, GameAction action)
{
    switch (action) {
    case GameAction::Reload: return hud.m_weaponInfo;
    case GameAction::SwitchWeapon: return hud.m_switchButton;
    case GameAction::Aim: return hud.m_scopeButton;
    case GameAction::UseGrenade: return hud.m_grenade;
    case GameAction::UsePowerUp: return hud.m_powerUp;
    case GameAction::Pause: return hud.m_pauseButton;
    }
    return nullptr;
}

/**
 * True when the map (the game's main menu) is shown and nothing covers it. Shops, dialogs and
 * settings open as modal windows in front of the map, at whatever level of the window tree.
 */
bool IsMainMapOnTop()
{
    auto* app = static_cast<App*>(WindowApp::m_instance);
    if (app == nullptr || app->m_helper == nullptr || app->m_helper->m_swerveGame != nullptr) {
        return false;
    }
    CGPSMapScreen* mapScreen = app->m_helper->m_gpsScreen;
    if (mapScreen == nullptr || mapScreen->m_mapGame == nullptr) {
        return false;
    }
    Window* window = static_cast<Window*>(mapScreen->m_mapGame);
    // While the map loads, the loading screen stands in for it.
    if (window->m_parent != mapScreen) {
        return false;
    }
    // Up to the desktop: a visible modal window in front of the map at any level covers it.
    while (window->m_parent != nullptr) {
        for (Window* sibling = window->m_parent->m_firstChild; sibling != window; sibling = sibling->m_nextSibling) {
            const bool modal = (sibling->m_flags & Window::FLAG_MODAL) != 0;
            const bool visible = (sibling->m_flags & Window::FLAG_VISIBLE) != 0;
            if (modal && visible) {
                return false;
            }
        }
        window = window->m_parent;
    }
    return true;
}

} // namespace

bool TriggerAction(CApplet& applet, GameAction action)
{
    CGameplayHUD* hud = GetActiveHud();
    if (hud == nullptr) {
        return false;
    }
    // The grenade and power-up controls only exist when the player owns the item.
    Window* control = GetActionControl(*hud, action);
    if (control == nullptr || !(control->m_flags & Window::FLAG_VISIBLE)) {
        return false;
    }
    int x = control->m_width / 2;
    int y = control->m_height / 2;
    control->LocalToGlobal(x, y);
    const unsigned int position = EncodePosition(x, y);
    // A full tap in one go: none of the actions needs the control to stay held.
    applet.QueueSystemEvent(TouchBegan, ActionTouchId, position | (1u << 28));
    applet.QueueSystemEvent(TouchEnded, ActionTouchId, position);
    return true;
}

void HandleKeyDown(CApplet& applet, SDL_Keycode key)
{
    for (const KeyBinding& binding : KeyBindings) {
        if (binding.key != key) {
            continue;
        }
        // On the main map there is nothing to pause: the pause key quits, the only way out of full screen.
        if (binding.action == GameAction::Pause && IsMainMapOnTop()) {
            fprintf(stderr, "Pause key on the main map: quitting\n");
            ckz::running = false;
            return;
        }
        TriggerAction(applet, binding.action);
        return;
    }
}

bool HandleMouseButtonDown(CApplet& applet, Uint8 button)
{
    for (const MouseButtonBinding& binding : MouseButtonBindings) {
        if (binding.button == button) {
            TriggerAction(applet, binding.action);
            return true;
        }
    }
    return false;
}

void AdjustScopeZoom(float wheelY)
{
    CGameplayHUD* hud = GetActiveHud();
    if (hud == nullptr) {
        return;
    }
    // Only while looking through the scope, not during the scope in / out animation.
    if (!hud->GetFlagBc() || hud->GetValue148() != 0) {
        return;
    }
    auto* app = static_cast<App*>(WindowApp::m_instance);
    CDH_Weapon* weapon = app->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon();
    // The weapon table gives a zoom range (zoom_fov_min .. zoom_fov_max) to the scoped sniper rifles only.
    // The original HUD never created its zoom slider, so this wheel is the only way to use that range.
    if (weapon == nullptr || !weapon->m_scopeAttached || weapon->m_zoomFovMax <= weapon->m_zoomFovMin) {
        return;
    }
    DGCamera* camera = app->m_helper->m_swerveGame->m_camera;
    // Wheel up narrows the field of view, which zooms in.
    float fov = camera->GetFOV() - wheelY * ScopeZoomFovStep;
    fov = SDL_clamp(fov, weapon->m_zoomFovMin, weapon->m_zoomFovMax);
    camera->SetFOV(fov);
    fprintf(stderr, "Scope zoom: fov %.1f (range %.1f - %.1f)\n", fov, weapon->m_zoomFovMin, weapon->m_zoomFovMax);
}

void ApplyDesktopFireMode()
{
    auto* app = static_cast<App*>(WindowApp::m_instance);
    if (app == nullptr || app->m_settings == nullptr) {
        return;
    }
    // With the HUD fire button on, holding it fires (and keeps automatic weapons firing),
    // which is what the left mouse button does through SetFireHeld.
    app->m_settings->SetFireButtonEnabled(true);
}

bool IsMissionInputActive()
{
    return GetActiveHud() != nullptr;
}

void TurnView(CApplet& applet, float mouseX, float mouseY)
{
    // Whole pixels go to the game now, the fraction waits for the next frame.
    static float pendingX = 0.0f;
    static float pendingY = 0.0f;
    pendingX += mouseX * MouseLookSensitivity;
    pendingY += mouseY * MouseLookSensitivity;
    int moveX = static_cast<int>(pendingX);
    int moveY = static_cast<int>(pendingY);
    if (moveX == 0 && moveY == 0) {
        return;
    }
    pendingX -= static_cast<float>(moveX);
    pendingY -= static_cast<float>(moveY);

    // The game turns the view by how far a finger drags, so each frame is one short drag
    // that starts at the screen centre. A drag may not leave the screen.
    const int centerX = WindowWidth / 2;
    const int centerY = WindowHeight / 2;
    moveX = SDL_clamp(moveX, 1 - centerX, centerX - 2);
    moveY = SDL_clamp(moveY, 1 - centerY, centerY - 2);
    const unsigned int start = EncodePosition(centerX, centerY);
    const unsigned int end = EncodePosition(centerX + moveX, centerY + moveY);
    applet.QueueSystemEvent(TouchBegan, LookTouchId, start | (1u << 28));
    applet.QueueSystemEvent(TouchMoved, LookTouchId, end);
    applet.QueueSystemEvent(TouchEnded, LookTouchId, end);
}

void SetFireHeld(CApplet& applet, bool held)
{
    static bool fireHeld = false;
    static unsigned int firePosition = 0;
    if (held == fireHeld) {
        return;
    }
    if (!held) {
        applet.QueueSystemEvent(TouchEnded, FireTouchId, firePosition);
        fireHeld = false;
        return;
    }
    CGameplayHUD* hud = GetActiveHud();
    if (hud == nullptr) {
        return;
    }
    Window* button = hud->m_fireButton;
    int x = button->m_width / 2;
    int y = button->m_height / 2;
    button->LocalToGlobal(x, y);
    firePosition = EncodePosition(x, y);
    applet.QueueSystemEvent(TouchBegan, FireTouchId, firePosition | (1u << 28));
    fireHeld = true;
}

} // namespace ckz
