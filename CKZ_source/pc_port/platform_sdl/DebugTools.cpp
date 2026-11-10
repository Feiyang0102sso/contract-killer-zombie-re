// Debug output of the game itself, see DebugTools.h.
#include "DebugTools.h"
#include "Config.h"
#include <stdio.h>
#include "App.h"
#include "CSwerveGame.h"
#include "CUnit.h"
#include "CUnitsController.h"
#include "DGHelper.h"

namespace ckz {

CSwerveGame* ReadyGame()
{
    auto* app = static_cast<App*>(WindowApp::m_instance);
    if (app == nullptr || app->m_helper == nullptr) {
        return nullptr;
    }
    CSwerveGame* game = app->m_helper->m_swerveGame;
    if (game == nullptr || !game->m_sceneReady || game->m_unitsController == nullptr) {
        return nullptr;
    }
    return game;
}

void UpdateUnitCountLog()
{
    static Uint64 lastRun = 0;
    if (!config.fpsLog) {
        return;
    }
    const Uint64 now = SDL_GetTicks();
    if (now - lastRun < UnitCountLogIntervalMs) {
        return;
    }
    lastRun = now;
    CSwerveGame* game = ReadyGame();
    if (game == nullptr) {
        return;
    }
    Vector<CUnit*>& units = game->m_unitsController->m_units;
    int alive = 0;
    for (int index = 0; index < units.size(); index++) {
        CUnit* unit = units[index];
        if (unit != nullptr && !unit->m_dead && unit->m_hp != 0) {
            alive++;
        }
    }
    fprintf(stderr, "Units: %d alive of %d\n", alive, units.size());
}

} // namespace ckz
