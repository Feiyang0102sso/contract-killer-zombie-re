// Desktop key map: the single place that decides which input triggers which game action.
// To rebind a control, edit the tables below; no other file needs to change.
#pragma once

#include <SDL3/SDL.h>

namespace ckz {

/** Game actions a key, mouse button or wheel step can trigger during a mission. */
enum class GameAction {
    Reload,
    SwitchWeapon,
    Aim,
    UseGrenade,
    UsePowerUp,
    Pause, // on the main map with nothing open it quits the game instead
};

struct KeyBinding {
    SDL_Keycode key;
    GameAction action;
};

struct MouseButtonBinding {
    Uint8 button;
    GameAction action;
};

// During a mission the mouse is captured: moving it turns the view, like in any PC shooter.
// Game pixels of touch drag per mouse count; raise it to turn faster.
// Measured: 1.0 turns the unscoped view about 0.14 degrees per count.
constexpr float MouseLookSensitivity = 0.5f;

// Held to fire during a mission (automatic weapons keep firing). In menus it is the pointer.
constexpr Uint8 FireButton = SDL_BUTTON_LEFT;

constexpr KeyBinding KeyBindings[] = {
    { SDLK_R, GameAction::Reload },
    { SDLK_Q, GameAction::SwitchWeapon },
    { SDLK_E, GameAction::SwitchWeapon },
    { SDLK_1, GameAction::UseGrenade },
    { SDLK_2, GameAction::UsePowerUp },
    { SDLK_ESCAPE, GameAction::Pause },
};

constexpr MouseButtonBinding MouseButtonBindings[] = {
    { SDL_BUTTON_RIGHT, GameAction::Aim },
};

// The mouse wheel zooms the scope of a sniper rifle: degrees of field of view per wheel notch.
// The scoped rifles go from 10 degrees (scope in) down to 6 degrees (fully zoomed).
constexpr float ScopeZoomFovStep = 1.0f;

} // namespace ckz
