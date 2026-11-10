// Tools for driving the game by hand or from scripts (PORT_PLAN.md P2, M4/M5).
// Compiled only into the test program (vs/ckz_tests.vcxproj, zombsniper_tests.exe --play); each tool
// is off unless its environment variable is set. The automated assertions are the gtest suites in tests/.
#pragma once

#include <SDL3/SDL.h>

class CApplet;
class CUnit;
namespace swerve {
template <class ErrorHandler> class Camera;
}
class CSwerveAssertingErrorHandler;

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

/** Call once per frame; runs the head pick self-test when it is enabled. */
void UpdatePickTest();

/** Head of a unit on screen: game pixels and the distance along the view (clip w). */
struct HeadOnScreen {
    float x;
    float y;
    float depth;
};

/**
 * Project the head hit zone (CVisualCharacterType::LoadModel) of a living unit to the screen.
 * False when the unit has no hit zone, the head is behind the camera, or (with `onScreen`) the
 * head is outside the screen.
 */
bool ProjectHead(CUnit* unit, swerve::Camera<CSwerveAssertingErrorHandler>& camera, HeadOnScreen& head,
                 bool onScreen = true);

/** True when the world map is the open screen (no mission running). */
bool IsMapOpen();

/** Start the survival mission from the open map. False when the map is not open or has none. */
bool StartSurvivalMission();

/** Call once per frame; starts the auto play mission once. */
void UpdateAutoPlay();

/** Handle a test key press or release; true when the key was a test key. */
bool HandleTestKey(CApplet& applet, SDL_Keycode key, bool down);

} // namespace ckz::test
