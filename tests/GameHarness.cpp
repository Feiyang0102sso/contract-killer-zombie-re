// Drives the real game inside the test program, see GameHarness.h.
#include "GameHarness.h"
#include "tests_tools/TestTools.h"
#include "../CKZ_source/pc_port/platform_sdl/DebugTools.h"
#include "../CKZ_source/pc_port/platform_sdl/Host.h"
#include <gtest/gtest.h>
#include <stdio.h>
#include <stdlib.h>
#include <SwerveApi.h>
#include "CApplet.h"
#include "CSwerveGame.h"
#include "WindowApp.h"

namespace ckz::test {
namespace {

GameHarness* instance = nullptr;
bool startFailed = false;

/** Encode a touch position like the platform layer does (x in bits 0-13, y in bits 14-27). */
unsigned int TouchPosition(int x, int y)
{
    return static_cast<unsigned int>(x) | (static_cast<unsigned int>(y) << 14);
}

} // namespace

CSwerveGame* PlayableMission()
{
    CSwerveGame* game = ckz::ReadyGame();
    if (game == nullptr || !ckz::IsMissionInputActive()) {
        return nullptr;
    }
    // The loading screen ("tap to continue") and the pause menu stop the mission.
    if (game->IsGamePaused()) {
        return nullptr;
    }
    return game;
}

GameHarness* GameHarness::Get()
{
    if (instance != nullptr) {
        return instance;
    }
    if (startFailed) {
        ADD_FAILURE() << "the game could not start (see an earlier test)";
        return nullptr;
    }
    GameHarness* harness = new GameHarness();
    if (!harness->Start()) {
        startFailed = true;
        delete harness;
        ADD_FAILURE() << "the game could not start; check zombsniper_tests.log";
        return nullptr;
    }
    instance = harness;
    return instance;
}

void GameHarness::Shutdown()
{
    if (instance == nullptr) {
        return;
    }
    instance->m_applet->Stop();
    delete instance->m_applet;
    delete static_cast<ckz::LoopInput*>(instance->m_input);
    delete instance;
    instance = nullptr;
}

bool GameHarness::Start()
{
    fprintf(stderr, "Test harness: starting the game\n");
    m_applet = new CApplet();
    if (!m_applet->Initialize()) {
        fprintf(stderr, "Test harness: CApplet::Initialize failed\n");
        return false;
    }
    m_input = new ckz::LoopInput();
    ckz::running = true;
    m_applet->Start();
    return true;
}

void GameHarness::Frame()
{
    const Uint64 start = SDL_GetTicksNS();
    ckz::RunFrame(*m_applet, *static_cast<ckz::LoopInput*>(m_input));
    if (!ckz::running) {
        // The window was closed from the taskbar (or the game quit): stop the whole run here.
        fprintf(stderr, "Test harness: the game window was closed, stopping the test run\n");
        fflush(stderr);
        _exit(3);
    }
    const Uint64 elapsed = SDL_GetTicksNS() - start;
    if (elapsed < TestFrameNs) {
        SDL_DelayPrecise(TestFrameNs - elapsed);
    }
}

void GameHarness::RunFrames(int count)
{
    for (int index = 0; index < count; index++) {
        Frame();
    }
}

void GameHarness::RunSeconds(double seconds)
{
    const Uint64 end = SDL_GetTicksNS() + static_cast<Uint64>(seconds * 1e9);
    while (SDL_GetTicksNS() < end) {
        Frame();
    }
}

bool GameHarness::RunUntil(Condition condition, double seconds)
{
    const Uint64 end = SDL_GetTicksNS() + static_cast<Uint64>(seconds * 1e9);
    while (!condition()) {
        if (SDL_GetTicksNS() >= end) {
            return false;
        }
        Frame();
    }
    return true;
}

void GameHarness::Tap(int x, int y)
{
    const unsigned int position = TouchPosition(x, y);
    m_applet->QueueSystemEvent(ckz::TouchBegan, 1, position | (1u << 28));
    Frame();
    m_applet->QueueSystemEvent(ckz::TouchEnded, 1, position);
    Frame();
}

bool GameHarness::ReachMap()
{
    if (IsMapOpen()) {
        return true;
    }
    fprintf(stderr, "Test harness: going to the map\n");
    const Uint64 end = SDL_GetTicksNS() + static_cast<Uint64>(MapTimeoutSeconds * 1e9);
    Uint64 nextTap = SDL_GetTicksNS() + static_cast<Uint64>(ContinueTapPeriod * 1e9);
    while (!IsMapOpen()) {
        if (SDL_GetTicksNS() >= end) {
            fprintf(stderr, "Test harness: the map did not open\n");
            return false;
        }
        // The intro movie ends on the first tap; the start-up screens wait for one too.
        ckz::SkipMovie();
        if (SDL_GetTicksNS() >= nextTap) {
            Tap(ContinueTapX, ContinueTapY);
            nextTap = SDL_GetTicksNS() + static_cast<Uint64>(ContinueTapPeriod * 1e9);
        }
        Frame();
    }
    // Let the map finish its opening animation.
    RunSeconds(1.0);
    return true;
}

bool GameHarness::EnterSurvival()
{
    if (PlayableMission() != nullptr) {
        return true;
    }
    if (!ReachMap()) {
        return false;
    }
    if (!StartSurvivalMission()) {
        fprintf(stderr, "Test harness: the map has no survival mission\n");
        return false;
    }
    // The loading screen ends with "tap to continue", which a press of the fire button also does.
    const Uint64 end = SDL_GetTicksNS() + static_cast<Uint64>(MissionTimeoutSeconds * 1e9);
    Uint64 nextTap = SDL_GetTicksNS() + static_cast<Uint64>(ContinueTapPeriod * 1e9);
    while (PlayableMission() == nullptr) {
        if (SDL_GetTicksNS() >= end) {
            fprintf(stderr, "Test harness: the mission did not become playable\n");
            return false;
        }
        if (SDL_GetTicksNS() >= nextTap) {
            // A press of the HUD fire button passes "tap to continue" (like F10 of TestTools.h);
            // before the HUD exists, a plain tap.
            if (ckz::IsMissionInputActive()) {
                ckz::SetFireHeld(*m_applet, true);
                Frame();
                ckz::SetFireHeld(*m_applet, false);
            } else {
                Tap(ContinueTapX, ContinueTapY);
            }
            nextTap = SDL_GetTicksNS() + static_cast<Uint64>(ContinueTapPeriod * 1e9);
        }
        Frame();
    }
    fprintf(stderr, "Test harness: survival mission playable\n");
    return true;
}

bool GameHarness::LeaveMission()
{
    if (IsMapOpen()) {
        return true;
    }
    // Only a running mission understands the command; before the first one, just go to the map.
    if (ckz::ReadyGame() == nullptr) {
        return ReachMap();
    }
    fprintf(stderr, "Test harness: leaving the mission\n");
    WindowApp::HandleTunnelCommand(ReturnToMapCommand, 0, 0, 0);
    // The map may greet the player with dialogs (rewards, level up); the taps close them.
    const Uint64 end = SDL_GetTicksNS() + static_cast<Uint64>(MapTimeoutSeconds * 1e9);
    Uint64 nextTap = SDL_GetTicksNS() + static_cast<Uint64>(ContinueTapPeriod * 1e9);
    while (!IsMapOpen()) {
        if (SDL_GetTicksNS() >= end) {
            fprintf(stderr, "Test harness: the map did not open after the mission\n");
            return false;
        }
        if (SDL_GetTicksNS() >= nextTap && ckz::ReadyGame() == nullptr) {
            Tap(ContinueTapX, ContinueTapY);
            nextTap = SDL_GetTicksNS() + static_cast<Uint64>(ContinueTapPeriod * 1e9);
        }
        Frame();
    }
    RunSeconds(1.0);
    return true;
}

bool GameHarness::EnsureMissionPlayable()
{
    if (PlayableMission() != nullptr) {
        return true;
    }
    CSwerveGame* game = ckz::ReadyGame();
    if (game != nullptr && !game->m_finishing) {
        // Paused by a dialog: the fire button or a tap closes it, as on the loading screen.
        fprintf(stderr, "Test harness: the mission is paused, closing the dialog\n");
        const Uint64 end = SDL_GetTicksNS() + static_cast<Uint64>(DialogTimeoutSeconds * 1e9);
        while (SDL_GetTicksNS() < end) {
            if (PlayableMission() != nullptr) {
                return true;
            }
            if (ckz::IsMissionInputActive()) {
                ckz::SetFireHeld(*m_applet, true);
                Frame();
                ckz::SetFireHeld(*m_applet, false);
            } else {
                Tap(ContinueTapX, ContinueTapY);
            }
            RunSeconds(ContinueTapPeriod);
        }
    }
    fprintf(stderr, "Test harness: the mission ended, starting a new one\n");
    if (!LeaveMission()) {
        return false;
    }
    return EnterSurvival();
}

CApplet& GameHarness::Applet()
{
    return *m_applet;
}

} // namespace ckz::test
