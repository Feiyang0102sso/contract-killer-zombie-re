// Debug tools for automated checks of the Swerve backends (PORT_PLAN.md P2, M4/M5).
// Everything here is off unless its environment variable is set.
#pragma once

#include <SDL3/SDL.h>

class CApplet;

namespace ckz {

// CKZ_PICK_TEST: every PickTestIntervalMs during a mission each living unit is picked through the
// screen position of its head hit zone and the result is printed. Run it with both backends
// (CKZ_SWERVE=arm for the original) and compare the logs.
constexpr const char* PickTestVariable = "CKZ_PICK_TEST";
constexpr Uint64 PickTestIntervalMs = 1500;

// CKZ_DEBUG_KEYS: keys for driving a mission with posted window messages. A captured mouse reads
// raw input, so posted mouse messages do not reach the game; posted keys do.
//   F10 (held)  fire, like holding the left mouse button
//   arrows      turn the view by DebugTurnCounts mouse counts
//   F11         shoot once at the head of the nearest unit on screen and print its hit points
//   F7          aim (scope in / out), like the right mouse button
constexpr const char* DebugKeysVariable = "CKZ_DEBUG_KEYS";
constexpr float DebugTurnCounts = 40.0f;

// CKZ_FPS_LOG (see Graphics.cpp) also prints the number of living units once per second, so the
// frame rate can be matched with how crowded the mission is.
constexpr const char* UnitCountLogVariable = "CKZ_FPS_LOG";
constexpr Uint64 UnitCountLogIntervalMs = 1000;

/** Call once per frame; runs the head pick self-test when it is enabled. */
void UpdatePickTest();

/** Call once per frame; prints the living unit count when CKZ_FPS_LOG is set. */
void UpdateUnitCountLog();

/** Handle a debug key press or release; true when the key was a debug key (CKZ_DEBUG_KEYS only). */
bool HandleDebugKey(CApplet& applet, SDL_Keycode key, bool down);

} // namespace ckz
