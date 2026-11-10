// Tools for automated checks of the PC port (PORT_PLAN.md P2, M4/M5).
// Compiled only into the test build (CkzTests=true); each tool is off unless its environment
// variable is set.
#pragma once

#include <SDL3/SDL.h>

class CApplet;

namespace ckz::test {

// CKZ_PICK_TEST: every PickTestIntervalMs during a mission each living unit is picked through the
// screen position of its head hit zone and the result is printed. Run it with both backends
// (Engine=swerve in zombsniper.cfg for the original) and compare the logs.
constexpr const char* PickTestVariable = "CKZ_PICK_TEST";
constexpr Uint64 PickTestIntervalMs = 1500;

// CKZ_DEBUG_KEYS: keys for driving a mission with posted window messages. A captured mouse reads
// raw input, so posted mouse messages do not reach the game; posted keys do.
//   F10 (held)  fire, like holding the left mouse button
//   arrows      turn the view by TestTurnCounts mouse counts
//   F11         shoot once at the head of the nearest unit on screen and print its hit points
//   F7          aim (scope in / out), like the right mouse button
// F8 (start / stop the ARM profiler, see ArmProfiler.h) works without this variable.
constexpr const char* TestKeysVariable = "CKZ_DEBUG_KEYS";
constexpr float TestTurnCounts = 40.0f;

// CKZ_AUTOPLAY=survival: the survival mission starts as soon as the map is open
// (automated performance and rendering checks).
constexpr const char* AutoPlayVariable = "CKZ_AUTOPLAY";
constexpr const char* AutoPlaySurvival = "survival";

// CKZ_HIDDEN: run without showing the window and without the missing-file dialog.
constexpr const char* HiddenVariable = "CKZ_HIDDEN";

/** Call once per frame; runs the head pick self-test when it is enabled. */
void UpdatePickTest();

/** Call once per frame; starts the auto play mission once. */
void UpdateAutoPlay();

/** Handle a test key press or release; true when the key was a test key. */
bool HandleTestKey(CApplet& applet, SDL_Keycode key, bool down);

/** True when the game runs hidden (CKZ_HIDDEN). */
bool RunHidden();

} // namespace ckz::test
