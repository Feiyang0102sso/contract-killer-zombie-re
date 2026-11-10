// Drives the real game inside the test program: one applet for the whole run, advanced frame by
// frame by the tests. The window exists already (Applet.cpp creates it before RunTestProgram).
#pragma once

#include <SDL3/SDL.h>

class CApplet;
class CSwerveGame;
class CUnit;

namespace ckz::test {

// Frame pacing of the tests: the window is minimized, so vertical sync does not limit the frame
// rate any more; the harness waits instead (an unlimited frame rate once crashed the GPU driver).
constexpr Uint64 TestFrameNs = 1000000000ull / 60;
// Game pixels of the "tap to continue" touches while the game starts up (lower right part of the
// 1024 x 768 screen, where the start-up screens accept a tap and no menu button sits).
constexpr int ContinueTapX = 768;
constexpr int ContinueTapY = 576;
// Seconds between two such taps.
constexpr double ContinueTapPeriod = 2.0;
// Tunnel command of the "return to map" confirmation button (CSwerveGame::HandleCommand).
constexpr int ReturnToMapCommand = 0x1105a3a5;
// Limits for reaching the map and a playable mission.
constexpr double MapTimeoutSeconds = 90.0;
constexpr double MissionTimeoutSeconds = 90.0;
// How long EnsureMissionPlayable taps at a dialog before it treats the mission as ended.
constexpr double DialogTimeoutSeconds = 15.0;

/** A condition the harness waits for; checked once per frame. */
typedef bool (*Condition)();

class GameHarness {
public:
    /** The running game; started on first use. Fails the calling test when the game cannot start. */
    static GameHarness* Get();
    /** Stop the game if it was started (once, after the last test). */
    static void Shutdown();

    /** Run `count` frames. */
    void RunFrames(int count);
    /** Run frames for `seconds` of wall clock time. */
    void RunSeconds(double seconds);
    /** Run frames until `condition` holds; false when `seconds` pass first. */
    bool RunUntil(Condition condition, double seconds);

    /** Tap the screen at game pixels `x`, `y` (a touch that begins and ends in the same frame). */
    void Tap(int x, int y);

    /** From any start-up screen to the world map. */
    bool ReachMap();
    /**
     * Into the survival mission with the HUD live: from the map, or nothing to do when a mission is
     * already playable.
     */
    bool EnterSurvival();
    /**
     * Back to the map from a mission, as the pause menu's "return to map" confirms it
     * (tunnel command ReturnToMapCommand, CSwerveGame::ShowReturnToGpsConfirmation).
     */
    bool LeaveMission();
    /**
     * A playable mission again after something stopped it: dialogs (next wave, rewards) are tapped
     * away; a mission that ended (the player lost) is left and a new survival mission started.
     */
    bool EnsureMissionPlayable();

    CApplet& Applet();

private:
    GameHarness() = default;
    bool Start();
    void Frame();

    CApplet* m_applet = nullptr;
    void* m_input = nullptr; // ckz::LoopInput, kept opaque to keep Host.h out of the tests
};

/** The mission once its scene and HUD are ready, else nullptr. */
CSwerveGame* PlayableMission();

} // namespace ckz::test
