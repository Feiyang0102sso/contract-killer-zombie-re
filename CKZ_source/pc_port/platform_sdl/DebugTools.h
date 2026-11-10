// Debug output of the game itself, switched on in zombsniper.cfg (Config.h).
// Tools for automated checks live in tests/ and are only compiled into the test build.
#pragma once

#include <SDL3/SDL.h>

class CSwerveGame;

namespace ckz {

// FpsLog (see Graphics.cpp) also prints the number of living units once per second, so the
// frame rate can be matched with how crowded the mission is.
constexpr Uint64 UnitCountLogIntervalMs = 1000;

/** The running mission once its scene is ready, else nullptr. */
CSwerveGame* ReadyGame();

/** Call once per frame; prints the living unit count when FpsLog is on. */
void UpdateUnitCountLog();

} // namespace ckz
