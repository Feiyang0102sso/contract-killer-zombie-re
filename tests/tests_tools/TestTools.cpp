// Tools for automated checks of the PC port, see TestTools.h.
#include "TestTools.h"
#include "ArmProfiler.h"
#include "../../CKZ_source/pc_port/platform_sdl/DebugTools.h"
#include "../../CKZ_source/pc_port/platform_sdl/Host.h"
#include <stdio.h>
#include <string.h>
// The PC copy of the Swerve API first (see GameplayTest.cpp).
#include <SwerveApi.h>
#include "App.h"
#include "CApplet.h"
#include "CGPSMapGame.h"
#include "CGPSMapScreen.h"
#include "CMapLocation.h"
#include "CMissionManager.h"
#include "CSwerve.h"
#include "CSwerveGame.h"
#include "CUnit.h"
#include "CUnitsController.h"
#include "CVisualCharacterType.h"
#include "DGHelper.h"

namespace ckz::test {
namespace {

typedef swerve::Camera<CSwerveAssertingErrorHandler> Camera;
typedef swerve::Group<CSwerveAssertingErrorHandler> Group;
typedef swerve::Node<CSwerveAssertingErrorHandler> Node;
typedef swerve::Transform<CSwerveAssertingErrorHandler> Transform;

/** Hit type names for the log. */
const char* HitTypeName(EHitType hitType)
{
    switch (hitType) {
    case HIT_BODY: return "body";
    case HIT_HEAD: return "head";
    case HIT_WEAPON: return "weapon";
    case HIT_SPECIAL: return "special";
    }
    return "?";
}

} // namespace

bool ProjectHead(CUnit* unit, Camera& camera, HeadOnScreen& head, bool onScreen)
{
    if (unit == nullptr || unit->m_dead || unit->m_hp == 0) {
        return false;
    }
    CUnitBody& body = unit->m_body;
    if (body.m_lodCount == 0) {
        return false;
    }
    Group lod = body.m_lods[body.m_currentLod];
    Node hitZone(lod.find(CVisualCharacterType::GetHitzoneGroupUserId()));
    if (hitZone.m_interface == nullptr) {
        return false;
    }
    // Origin of the hit zone group = centre of the head sphere.
    Transform toCamera(CSwerve::GetInstance()->m_swerve);
    hitZone.getTransformTo(camera, toCamera);
    float point[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    toCamera.transform(4, point);
    Transform projection(CSwerve::GetInstance()->m_swerve);
    camera.getProjection(projection);
    projection.transform(4, point);
    if (point[3] <= 0.0f) {
        return false;
    }
    auto* app = static_cast<App*>(WindowApp::m_instance);
    const float screenWidth = static_cast<float>(app->m_helper->m_screenWidth);
    const float screenHeight = static_cast<float>(app->m_helper->m_screenHeight);
    head.x = (point[0] / point[3] + 1.0f) * 0.5f * screenWidth;
    head.y = (1.0f - point[1] / point[3]) * 0.5f * screenHeight;
    head.depth = point[3];
    if (!onScreen) {
        return true;
    }
    if (head.x < 0.0f || head.y < 0.0f || head.x >= screenWidth || head.y >= screenHeight) {
        return false;
    }
    return true;
}

namespace {

/** F11: shoot once at the nearest head on screen through the game's own shot path. */
void ShootNearestHead()
{
    CSwerveGame* game = ReadyGame();
    if (game == nullptr) {
        fprintf(stderr, "Test shot: no mission\n");
        return;
    }
    Camera camera = game->m_cameraNode.getActiveCamera();
    Vector<CUnit*>& units = game->m_unitsController->m_units;
    CUnit* target = nullptr;
    HeadOnScreen targetHead = {};
    for (int index = 0; index < units.size(); index++) {
        HeadOnScreen head = {};
        if (!ProjectHead(units[index], camera, head)) {
            continue;
        }
        if (target == nullptr || head.depth < targetHead.depth) {
            target = units[index];
            targetHead = head;
        }
    }
    if (target == nullptr) {
        fprintf(stderr, "Test shot: no head on screen\n");
        return;
    }
    const int hpBefore = target->m_hp;
    game->Shot(static_cast<int>(targetHead.x), static_cast<int>(targetHead.y));
    EHitType hitType = HIT_BODY;
    const bool picked = target->GetPickInfo(&hitType);
    const char* pickedName = "not picked";
    if (picked) {
        pickedName = HitTypeName(hitType);
    }
    fprintf(stderr, "Test shot: head at %.1f,%.1f (depth %.1f), hp %d -> %d, %s\n", targetHead.x, targetHead.y,
            targetHead.depth, hpBefore, target->m_hp, pickedName);
}

} // namespace

void UpdatePickTest()
{
    static const bool enabled = SDL_getenv(PickTestVariable) != nullptr;
    static Uint64 lastRun = 0;
    if (!enabled) {
        return;
    }
    const Uint64 now = SDL_GetTicks();
    if (now - lastRun < PickTestIntervalMs) {
        return;
    }
    lastRun = now;
    CSwerveGame* game = ReadyGame();
    if (game == nullptr) {
        return;
    }
    Camera camera = game->m_cameraNode.getActiveCamera();
    Vector<CUnit*>& units = game->m_unitsController->m_units;
    int tested = 0;
    int heads = 0;
    for (int index = 0; index < units.size(); index++) {
        HeadOnScreen head = {};
        if (!ProjectHead(units[index], camera, head)) {
            continue;
        }
        CUnitBody& body = units[index]->m_body;
        EHitType hitType = HIT_BODY;
        const bool hit = body.Pick(head.x, head.y, -1.0f, camera, hitType);
        tested++;
        const char* outcome = "miss";
        if (hit) {
            outcome = HitTypeName(hitType);
            if (hitType == HIT_HEAD) {
                heads++;
            }
        }
        fprintf(stderr, "Pick test: unit %d lod %u head at %.1f,%.1f -> %s\n", index, body.m_currentLod, head.x,
                head.y, outcome);
    }
    if (tested > 0) {
        fprintf(stderr, "Pick test: %d of %d head picks are head hits\n", heads, tested);
    }
}

/** True when `wanted` is `root` or one of its descendants (without touching `wanted` itself). */
bool InWindowTree(const Window* root, const Window* wanted)
{
    for (const Window* child = root; child != nullptr; child = child->m_nextSibling) {
        if (child == wanted) {
            return true;
        }
        if (InWindowTree(child->m_firstChild, wanted)) {
            return true;
        }
    }
    return false;
}

bool IsMapOpen()
{
    auto* app = static_cast<App*>(WindowApp::m_instance);
    if (app == nullptr || app->m_helper == nullptr) {
        return false;
    }
    CGPSMapScreen* mapScreen = app->m_helper->m_gpsScreen;
    if (app->m_helper->m_swerveGame != nullptr || mapScreen == nullptr) {
        return false;
    }
    // DGHelper keeps m_gpsScreen after the map screen is deleted for a mission (the original never
    // clears it); until the next map screen exists the pointer dangles. Only use it once it is
    // part of the live window tree again.
    if (!InWindowTree(app->m_desktop, mapScreen)) {
        return false;
    }
    if (mapScreen->m_mapGame == nullptr) {
        return false;
    }
    return mapScreen->m_mapGame->m_parent == mapScreen;
}

bool StartSurvivalMission()
{
    if (!IsMapOpen()) {
        return false;
    }
    auto* app = static_cast<App*>(WindowApp::m_instance);
    // The survival ("endless") mission sits on the map location of type MISSION_EM.
    CGPSMapGame* map = app->m_helper->m_gpsScreen->m_mapGame;
    for (int index = 0; index < map->m_locations.size(); index++) {
        CMapLocation* location = map->m_locations[index];
        if (location->m_missionType != CMission::MISSION_EM || !location->HasMission()) {
            continue;
        }
        const CMission* mission = app->m_missionManager->GetMissionByName(location->GetCurrentMission());
        if (mission == nullptr) {
            continue;
        }
        fprintf(stderr, "Auto play: starting the survival mission\n");
        map->StartMission(mission);
        return true;
    }
    return false;
}

void UpdateAutoPlay()
{
    static bool started = false;
    if (started) {
        return;
    }
    const char* mode = SDL_getenv(AutoPlayVariable);
    if (mode == nullptr || strcmp(mode, AutoPlaySurvival) != 0) {
        return;
    }
    started = StartSurvivalMission();
}

bool HandleTestKey(CApplet& applet, SDL_Keycode key, bool down)
{
    // F8 has its own switch: the profiler only exists with CKZ_ARM_PROFILE.
    if (key == SDLK_F8) {
        if (down) {
            ToggleArmProfile();
        }
        return true;
    }
    static const bool enabled = SDL_getenv(TestKeysVariable) != nullptr;
    if (!enabled) {
        return false;
    }
    switch (key) {
    case SDLK_F10:
        SetFireHeld(applet, down);
        return true;
    case SDLK_F11:
        if (down) {
            ShootNearestHead();
        }
        return true;
    case SDLK_F7:
        if (down) {
            TriggerAction(applet, GameAction::Aim);
        }
        return true;
    case SDLK_LEFT:
        if (down) {
            TurnView(applet, -TestTurnCounts, 0.0f);
        }
        return true;
    case SDLK_RIGHT:
        if (down) {
            TurnView(applet, TestTurnCounts, 0.0f);
        }
        return true;
    case SDLK_UP:
        if (down) {
            TurnView(applet, 0.0f, -TestTurnCounts);
        }
        return true;
    case SDLK_DOWN:
        if (down) {
            TurnView(applet, 0.0f, TestTurnCounts);
        }
        return true;
    default:
        return false;
    }
}

} // namespace ckz::test
