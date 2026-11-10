// Gameplay assertions in the running survival mission, with the backend of zombsniper.cfg (Engine).
// Every test drives the game's own code paths: CSwerveGame::Shot (aiming dispersion, weapon,
// damage, effects), CUnitBody::Pick, CSwerveGame::LaunchHandGrenade / LaunchGrenade and the
// projectile simulation. They need a save whose map offers the survival mission.
//
// Shots go through the game's aiming dispersion, so a single shot may miss; the tests count hits
// over several shots instead of expecting each one to land.
#include "GameHarness.h"
#include "tests_tools/TestTools.h"
#include "../CKZ_source/pc_port/platform_sdl/Config.h"
#include "../CKZ_source/pc_port/platform_sdl/Host.h"
#include <gtest/gtest.h>
// The PC copy of the Swerve API (ARM-aware calls) must come before any game header that
// includes "SwerveApi.h" next to itself (packages/swerve/code/inc, the iOS original).
#include <SwerveApi.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include "App.h"
#include "Ballistics.h"
#include "CDH_Weapon.h"
#include "CDH_WeaponsManager.h"
#include "CGrenadeManager.h"
#include "CSwerve.h"
#include "CSwerveGame.h"
#include "CUnit.h"
#include "CUnitsController.h"
#include "CVisualCharacterType.h"
#include "DGHelper.h"
#include "GamePlayManager.h"

namespace ckz::test {
namespace {

typedef swerve::Camera<CSwerveAssertingErrorHandler> Camera;
typedef swerve::Group<CSwerveAssertingErrorHandler> Group;

// How long a test waits for a zombie to walk into view.
constexpr double TargetTimeoutSeconds = 40.0;
// Shots spent on one target before the test gives up on it.
constexpr int ShotsPerTarget = 40;
// Longest wait for the weapon to be ready (reload included).
constexpr double WeaponReadySeconds = 8.0;
// A hand grenade explodes on contact plus the grenade's delay; a launcher shell on contact.
constexpr double ProjectileSeconds = 12.0;
// AimAt: steps of the view turn loop, the screen error that counts as aimed (game pixels) and the
// share of the remaining error corrected per step.
constexpr int AimSteps = 40;
constexpr float AimTolerance = 12.0f;
constexpr float AimDamping = 0.7f;
// AimAt: size of the probe turn and the largest turn per step, in mouse counts.
constexpr float AimProbeCounts = 5.0f;
constexpr float AimMaxCounts = 60.0f;
// AimAt: a probe response below AimMinimumGain pixels per count is replaced by AimDefaultGain
// (measured: about -0.5 on both axes).
constexpr float AimMinimumGain = 0.05f;
constexpr float AimDefaultGain = -0.5f;
// LauncherShellExplodesOnZombie: zombies shot at, and how close (metres, ground plane) the shell
// must end to the zombie it was aimed at.
constexpr int ShellTargets = 6;
constexpr int ShellAttempts = 10;
constexpr float ShellRangeMetres = 25.0f;
constexpr float ShellOnTargetMetres = 2.5f;
constexpr float ShellMinHeightMetres = 0.3f;
// Mission.*: weapon ids put into slots 0 and 1 before the survival mission (a one-shot rifle and
// the minigun), so the tests do not depend on what the save holds.
const int MissionWeapons[2] = { 18, 37 };
// Weapons.EveryWeaponHitsZombies: environment variable with a comma list of weapon ids to test.
constexpr const char* WeaponListVariable = "CKZ_WEAPONS";
// Weapons.EveryWeaponHitsZombies: shots per weapon before it counts as doing no damage.
constexpr int WeaponShots = 12;
// Longest time a held trigger may take to fire (the minigun spins up first).
constexpr double SpinUpSeconds = 4.0;
// ExplosivesFromManyAngles: directions, and the view turn between them in mouse counts.
constexpr int ExplosiveAngles = 16;
constexpr float ExplosiveTurnCounts = 120.0f;
constexpr float ExplosiveTurnDownCounts = 25.0f;
// Torso point between the feet (0) and the head centre (1) on screen, see BodyScreenPoint.
constexpr float TorsoHeight = 0.62f;

App* GameApp()
{
    return static_cast<App*>(WindowApp::m_instance);
}

CDH_Weapon* ActiveWeapon()
{
    return GameApp()->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon();
}

/** Units of the mission that are alive and not civilians. */
std::vector<CUnit*> LivingZombies(CSwerveGame* game)
{
    std::vector<CUnit*> zombies;
    Vector<CUnit*>& units = game->m_unitsController->m_units;
    for (int index = 0; index < units.size(); index++) {
        CUnit* unit = units[index];
        if (unit == nullptr || unit->m_dead || unit->m_hp <= 0 || unit->m_removed || unit->m_blowing) {
            continue;
        }
        if (unit->IsCivilian()) {
            continue;
        }
        zombies.push_back(unit);
    }
    return zombies;
}

/** The nearest zombie whose head is on screen, or nullptr. */
CUnit* NearestHeadOnScreen(HeadOnScreen& head)
{
    CSwerveGame* game = PlayableMission();
    if (game == nullptr) {
        return nullptr;
    }
    Camera camera = game->m_cameraNode.getActiveCamera();
    CUnit* nearest = nullptr;
    for (CUnit* unit : LivingZombies(game)) {
        HeadOnScreen candidate = {};
        if (!ProjectHead(unit, camera, candidate)) {
            continue;
        }
        if (nearest == nullptr || candidate.depth < head.depth) {
            nearest = unit;
            head = candidate;
        }
    }
    return nearest;
}

bool AnyHeadOnScreen()
{
    HeadOnScreen head = {};
    return NearestHeadOnScreen(head) != nullptr;
}

bool WeaponReady()
{
    CDH_Weapon* weapon = ActiveWeapon();
    return weapon != nullptr && weapon->IsReadyForUse() == 1;
}

/** Screen position (game pixels) of the origin of `node`; false when it is behind the camera. */
bool ProjectOrigin(Group& node, Camera& camera, float& x, float& y)
{
    swerve::Transform<CSwerveAssertingErrorHandler> toCamera(CSwerve::GetInstance()->m_swerve);
    node.getTransformTo(camera, toCamera);
    float point[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    toCamera.transform(4, point);
    swerve::Transform<CSwerveAssertingErrorHandler> projection(CSwerve::GetInstance()->m_swerve);
    camera.getProjection(projection);
    projection.transform(4, point);
    if (point[3] <= 0.0f) {
        return false;
    }
    const float screenWidth = static_cast<float>(GameApp()->m_helper->m_screenWidth);
    const float screenHeight = static_cast<float>(GameApp()->m_helper->m_screenHeight);
    x = (point[0] / point[3] + 1.0f) * 0.5f * screenWidth;
    y = (1.0f - point[1] / point[3]) * 0.5f * screenHeight;
    return true;
}

/**
 * A point on the torso: TorsoHeight of the way from the feet (the model's origin) to the head
 * centre on screen. False when the feet cannot be projected.
 */
bool BodyScreenPoint(CUnit* unit, Camera& camera, const HeadOnScreen& head, float& x, float& y)
{
    float feetX = 0.0f;
    float feetY = 0.0f;
    if (!ProjectOrigin(unit->m_body.m_model, camera, feetX, feetY)) {
        return false;
    }
    x = feetX + (head.x - feetX) * TorsoHeight;
    y = feetY + (head.y - feetY) * TorsoHeight;
    return true;
}

/** Puts the weapon slots of the save back when it goes out of scope (also after a failed ASSERT). */
class LoadoutGuard {
public:
    LoadoutGuard()
    {
        CDH_BasicGameData* data = GameApp()->m_helper->m_gamePlayManager.GetGameData();
        m_slots[0] = data->m_weaponIds[0];
        m_slots[1] = data->m_weaponIds[1];
    }
    ~LoadoutGuard()
    {
        CDH_BasicGameData* data = GameApp()->m_helper->m_gamePlayManager.GetGameData();
        data->SetWeapon(0, m_slots[0]);
        data->SetWeapon(1, m_slots[1]);
        data->SetActiveWeapon(0);
    }

private:
    int m_slots[2];
};

class Mission : public testing::Test {
protected:
    void SetUp() override
    {
        m_harness = GameHarness::Get();
        ASSERT_NE(m_harness, nullptr);
        if (PlayableMission() == nullptr) {
            // The same weapons for every run, whatever the save holds (MissionWeapons).
            ASSERT_TRUE(m_harness->LeaveMission());
            CDH_BasicGameData* data = GameApp()->m_helper->m_gamePlayManager.GetGameData();
            data->SetWeapon(0, MissionWeapons[0]);
            data->SetWeapon(1, MissionWeapons[1]);
            data->SetActiveWeapon(0);
        }
        ASSERT_TRUE(m_harness->EnterSurvival()) << "the survival mission did not become playable";
    }

    /** Wait until a zombie's head is on screen; false after TargetTimeoutSeconds. */
    bool WaitForTarget()
    {
        if (m_harness->RunUntil(AnyHeadOnScreen, TargetTimeoutSeconds)) {
            return true;
        }
        // An earlier test may have left the view turned away or the wave over: a new mission
        // starts with the view on the zombies' way in.
        fprintf(stderr, "Test: no zombie in view, restarting the mission\n");
        if (!m_harness->LeaveMission() || !m_harness->EnterSurvival()) {
            return false;
        }
        return m_harness->RunUntil(AnyHeadOnScreen, TargetTimeoutSeconds);
    }

    /** Fire the active weapon once at game pixels x, y (waits for it to be ready). */
    bool Shoot(float x, float y, bool precise = false)
    {
        if (!m_harness->RunUntil(WeaponReady, WeaponReadySeconds)) {
            return false;
        }
        // Shoot every frame until a round actually leaves the weapon, like holding the fire button:
        // the minigun only fires once it has spun up (CDH_Weapon::MakeShot).
        CDH_Weapon* weapon = ActiveWeapon();
        if (weapon->m_projectileType == 2) {
            // Healing guns use no ammo, so there is nothing to wait for.
            CSwerveGame* game = PlayableMission();
            if (game == nullptr) {
                return false;
            }
            if (precise) {
                game->m_zoom = 0.0f;
            }
            game->Shot(static_cast<int>(x), static_cast<int>(y));
            m_harness->RunFrames(1);
            return true;
        }
        const int ammoBefore = weapon->m_ammo;
        const Uint64 end = SDL_GetTicksNS() + static_cast<Uint64>(SpinUpSeconds * 1e9);
        while (SDL_GetTicksNS() < end) {
            CSwerveGame* game = PlayableMission();
            if (game == nullptr) {
                return false;
            }
            if (precise) {
                // No aiming dispersion: the shot goes exactly through x, y (SimulateAimingDispersion).
                game->m_zoom = 0.0f;
            }
            game->Shot(static_cast<int>(x), static_cast<int>(y));
            // One frame lets the hit effects and the unit's reaction run.
            m_harness->RunFrames(1);
            if (weapon->m_ammo != ammoBefore) {
                return true;
            }
        }
        fprintf(stderr, "Test shot: weapon %d fired nothing for %.1f s (ammo %d)\n", weapon->m_id, SpinUpSeconds,
                weapon->m_ammo);
        return false;
    }

    GameHarness* m_harness = nullptr;
};

TEST_F(Mission, HeadPickFindsHeadHitZone)
{
    ASSERT_TRUE(WaitForTarget()) << "no zombie came into view";
    CSwerveGame* game = PlayableMission();
    ASSERT_NE(game, nullptr);
    Camera camera = game->m_cameraNode.getActiveCamera();
    int tested = 0;
    int heads = 0;
    for (CUnit* unit : LivingZombies(game)) {
        HeadOnScreen head = {};
        if (!ProjectHead(unit, camera, head)) {
            continue;
        }
        EHitType hitType = HIT_BODY;
        const bool hit = unit->m_body.Pick(head.x, head.y, -1.0f, camera, hitType);
        tested++;
        if (hit && hitType == HIT_HEAD) {
            heads++;
        } else if (hit) {
            ADD_FAILURE() << "head at " << head.x << "," << head.y << " (depth " << head.depth << ") picked as hit type "
                          << static_cast<int>(hitType);
        } else {
            ADD_FAILURE() << "head at " << head.x << "," << head.y << " (depth " << head.depth << ") picked nothing";
        }
    }
    EXPECT_GT(tested, 0);
    fprintf(stderr, "Head pick: %d of %d head centres hit the head zone\n", heads, tested);
}

TEST_F(Mission, BodyPickFindsBody)
{
    ASSERT_TRUE(WaitForTarget()) << "no zombie came into view";
    CSwerveGame* game = PlayableMission();
    ASSERT_NE(game, nullptr);
    Camera camera = game->m_cameraNode.getActiveCamera();
    int tested = 0;
    int bodies = 0;
    for (CUnit* unit : LivingZombies(game)) {
        HeadOnScreen head = {};
        if (!ProjectHead(unit, camera, head)) {
            continue;
        }
        float x = 0.0f;
        float y = 0.0f;
        if (!BodyScreenPoint(unit, camera, head, x, y)) {
            continue;
        }
        EHitType hitType = HIT_HEAD;
        const bool hit = unit->m_body.Pick(x, y, -1.0f, camera, hitType);
        tested++;
        if (hit && hitType != HIT_HEAD) {
            bodies++;
        }
    }
    EXPECT_GT(tested, 0);
    // A torso point can miss a thin or turned zombie, but most must be body hits, never heads.
    EXPECT_GE(bodies * 2, tested) << bodies << " of " << tested << " torso points hit the body";
    fprintf(stderr, "Body pick: %d of %d torso points hit the body\n", bodies, tested);
}

TEST_F(Mission, HeadshotKillsAndBeheads)
{
    // Original rule (CUnit::ReceivedShot): a head hit that brings the hit points to 0 beheads.
    // Shots go exactly through the head centre (no aiming dispersion). A zombie can still turn or
    // stagger between frames, so the test needs one beheading among a few kills.
    int beheaded = 0;
    int kills = 0;
    for (int target = 0; target < 4 && beheaded == 0; target++) {
        ASSERT_TRUE(WaitForTarget()) << "no zombie came into view";
        HeadOnScreen head = {};
        CUnit* unit = NearestHeadOnScreen(head);
        ASSERT_NE(unit, nullptr);
        for (int shot = 0; shot < ShotsPerTarget && unit->m_hp > 0 && !unit->m_dead; shot++) {
            Camera camera = PlayableMission()->m_cameraNode.getActiveCamera();
            if (!ProjectHead(unit, camera, head)) {
                break;
            }
            const int hpBefore = unit->m_hp;
            ASSERT_TRUE(Shoot(head.x, head.y, true)) << "the weapon never fired";
            fprintf(stderr, "Headshot: target %d shot %d at %.0f,%.0f: hp %d -> %d, beheaded %d\n", target, shot, head.x,
                    head.y, hpBefore, unit->m_hp, unit->m_body.m_beheaded);
        }
        if (unit->m_hp == 0) {
            kills++;
        }
        if (unit->m_body.m_beheaded) {
            beheaded++;
        }
    }
    EXPECT_GT(beheaded, 0) << kills << " zombies killed by shots at the head, none beheaded";
    fprintf(stderr, "Headshot: %d kills, %d beheaded\n", kills, beheaded);
}

TEST_F(Mission, BodyShotsDamage)
{
    ASSERT_TRUE(WaitForTarget()) << "no zombie came into view";
    HeadOnScreen head = {};
    CUnit* unit = NearestHeadOnScreen(head);
    ASSERT_NE(unit, nullptr);
    const int hpBefore = unit->m_hp;
    int bodyHits = 0;
    for (int shot = 0; shot < 6 && unit->m_hp > 0; shot++) {
        Camera camera = PlayableMission()->m_cameraNode.getActiveCamera();
        if (!ProjectHead(unit, camera, head)) {
            break;
        }
        float x = 0.0f;
        float y = 0.0f;
        if (!BodyScreenPoint(unit, camera, head, x, y)) {
            break;
        }
        // The shot has no dispersion, so the pick at the aim point is what it hits.
        EHitType aimed = HIT_HEAD;
        const bool onBody = unit->m_body.Pick(x, y, -1.0f, camera, aimed) && aimed != HIT_HEAD;
        const int hp = unit->m_hp;
        ASSERT_TRUE(Shoot(x, y, true));
        // (An overkill body shot may still take the head off: CUnitBody::Blow beheads too.)
        if (onBody && unit->m_hp < hp) {
            bodyHits++;
        }
    }
    EXPECT_GT(bodyHits, 0) << "no body hit in 6 shots";
    EXPECT_LT(unit->m_hp, hpBefore);
    fprintf(stderr, "Body shots: %d body hits, hp %d -> %d\n", bodyHits, hpBefore, unit->m_hp);
}

/** A thrown hand grenade, followed frame by frame until it explodes. */
struct GrenadeTrack {
    bool launched;   // LaunchHandGrenade added a projectile
    bool exploded;   // the projectile ended within ProjectileSeconds
    int collisions;  // contacts with the level before the explosion
    float age;       // seconds of flight
    vec3 position;   // last position (the explosion point)
};

/** A launcher shell, followed frame by frame until it is gone. */
struct ShellTrack {
    bool launched;
    bool gone;       // the projectile ended within ProjectileSeconds
    float age;
    vec3 position;
};

/** Throw a hand grenade the way the HUD does (CSwerveGame::LaunchHandGrenade) and follow it. */
GrenadeTrack ThrowHandGrenade(GameHarness* harness)
{
    GrenadeTrack track = {};
    CSwerveGame* game = PlayableMission();
    if (game == nullptr || game->m_handGrenades == nullptr) {
        return track;
    }
    HandGrenadeBallistics* ballistics = game->m_handGrenades;
    // Remember the grenades flying already: the new one is the projectile that appears.
    std::vector<void*> before;
    for (int index = 0; index < ballistics->m_capacity; index++) {
        if (ballistics->m_projectiles[index].m_active) {
            before.push_back(ballistics->m_projectiles[index].m_userData);
        }
    }
    game->LaunchHandGrenade(1.0f);
    int projectile = -1;
    HandGrenadeBallistics::FragGrenade* grenade = nullptr;
    for (int index = 0; index < ballistics->m_capacity; index++) {
        const Ballistics::Projectile& candidate = ballistics->m_projectiles[index];
        if (!candidate.m_active) {
            continue;
        }
        bool known = false;
        for (void* old : before) {
            if (old == candidate.m_userData) {
                known = true;
            }
        }
        if (!known) {
            projectile = index;
            grenade = static_cast<HandGrenadeBallistics::FragGrenade*>(candidate.m_userData);
        }
    }
    if (projectile < 0) {
        return track;
    }
    track.launched = true;
    const Uint64 end = SDL_GetTicksNS() + static_cast<Uint64>(ProjectileSeconds * 1e9);
    while (ballistics->m_projectiles[projectile].m_active && ballistics->m_projectiles[projectile].m_userData == grenade) {
        if (SDL_GetTicksNS() >= end) {
            return track;
        }
        track.collisions = grenade->m_collisionCount;
        track.age = ballistics->m_projectiles[projectile].m_age;
        track.position = ballistics->m_projectiles[projectile].m_position;
        harness->RunFrames(1);
    }
    track.exploded = true;
    fprintf(stderr, "Hand grenade: %d contacts, exploded after %.2f s at %.1f,%.1f,%.1f\n", track.collisions, track.age,
            track.position.m_x, track.position.m_y, track.position.m_z);
    return track;
}

/** Fire a launcher shell from the active weapon (CSwerveGame::LaunchGrenade) and follow it. */
ShellTrack FireShell(GameHarness* harness)
{
    ShellTrack track = {};
    CSwerveGame* game = PlayableMission();
    if (game == nullptr || game->m_grenadeLauncher == nullptr) {
        return track;
    }
    GrenadeLauncher* launcher = game->m_grenadeLauncher;
    std::vector<bool> activeBefore;
    for (int index = 0; index < launcher->m_capacity; index++) {
        activeBefore.push_back(launcher->m_projectiles[index].m_active);
    }
    game->LaunchGrenade(0.0f);
    int shell = -1;
    for (int index = 0; index < launcher->m_capacity; index++) {
        if (launcher->m_projectiles[index].m_active && !activeBefore[index]) {
            shell = index;
        }
    }
    if (shell < 0) {
        return track;
    }
    track.launched = true;
    const Uint64 end = SDL_GetTicksNS() + static_cast<Uint64>(ProjectileSeconds * 1e9);
    while (launcher->m_projectiles[shell].m_active) {
        if (SDL_GetTicksNS() >= end) {
            return track;
        }
        track.age = launcher->m_projectiles[shell].m_age;
        track.position = launcher->m_projectiles[shell].m_position;
        harness->RunFrames(1);
    }
    track.gone = true;
    fprintf(stderr, "Launcher shell: gone after %.2f s at %.1f,%.1f,%.1f\n", track.age, track.position.m_x,
            track.position.m_y, track.position.m_z);
    return track;
}

TEST_F(Mission, HandGrenadeHitsGroundAndExplodes)
{
    if (GameApp()->m_grenadeManager->GetActiveGrenade() == nullptr) {
        GTEST_SKIP() << "the save has no hand grenade";
    }
    const GrenadeTrack track = ThrowHandGrenade(m_harness);
    ASSERT_TRUE(track.launched) << "LaunchHandGrenade added no projectile";
    ASSERT_TRUE(track.exploded) << "the grenade never exploded";
    // Out of the level bounds the game makes the grenade explode where it is (PostSimulate): that
    // is the "vanished without a bang" case. It must have touched the level first.
    EXPECT_GT(track.collisions, 0) << "the grenade exploded without touching the level, last at " << track.position.m_x
                                   << "," << track.position.m_y << "," << track.position.m_z << " age " << track.age;
}

TEST_F(Mission, LauncherShellHitsSomething)
{
    const ShellTrack track = FireShell(m_harness);
    ASSERT_TRUE(track.launched) << "LaunchGrenade added no projectile";
    ASSERT_TRUE(track.gone) << "the shell is still flying";
    // A shell that hits nothing just disappears when it reaches the maximum age (no explosion).
    const float maxAge = PlayableMission()->m_grenadeLauncher->m_maxAge;
    EXPECT_LT(track.age, maxAge - 0.1f) << "the shell vanished at its maximum age without hitting anything, last at "
                                        << track.position.m_x << "," << track.position.m_y << "," << track.position.m_z;
}

/** True when the nearest zombie with its head on screen is within ShellRangeMetres. */
bool ZombieInShellRange()
{
    HeadOnScreen head = {};
    if (NearestHeadOnScreen(head) == nullptr) {
        return false;
    }
    return head.depth < ShellRangeMetres;
}

/** Aim point of `unit` on screen now: its torso, or its head when the torso is off screen. */
bool TorsoOnScreen(CUnit* unit, float& x, float& y)
{
    CSwerveGame* game = PlayableMission();
    if (game == nullptr) {
        return false;
    }
    Camera camera = game->m_cameraNode.getActiveCamera();
    HeadOnScreen head = {};
    if (!ProjectHead(unit, camera, head, false)) {
        return false;
    }
    const float screenWidth = static_cast<float>(GameApp()->m_helper->m_screenWidth);
    const float screenHeight = static_cast<float>(GameApp()->m_helper->m_screenHeight);
    if (BodyScreenPoint(unit, camera, head, x, y) && x >= 0.0f && y >= 0.0f && x < screenWidth && y < screenHeight) {
        return true;
    }
    // Close in front of the camera the torso point can leave the screen; the head is a target too.
    x = head.x;
    y = head.y;
    return x >= 0.0f && y >= 0.0f && x < screenWidth && y < screenHeight;
}

/**
 * Turn the view until the torso of `unit` sits under the crosshair (screen centre), like a player
 * moving the mouse. The turn per mouse count is measured on the way, so its sign and size need not
 * be known. Returns false when the unit leaves the screen or the view does not settle.
 */
bool AimAt(GameHarness* harness, CUnit* unit)
{
    const float centreX = static_cast<float>(GameApp()->m_helper->m_screenWidth) * 0.5f;
    const float centreY = static_cast<float>(GameApp()->m_helper->m_screenHeight) * 0.5f;
    // Probe: a small turn tells how far (and which way) the target moves per mouse count.
    float startX = 0.0f;
    float startY = 0.0f;
    if (!TorsoOnScreen(unit, startX, startY)) {
        fprintf(stderr, "Aim: no aim point on screen (mission %d, hp %d)\n", PlayableMission() != nullptr, unit->m_hp);
        return false;
    }
    ckz::TurnView(harness->Applet(), AimProbeCounts, AimProbeCounts);
    harness->RunFrames(2);
    float probeX = 0.0f;
    float probeY = 0.0f;
    if (!TorsoOnScreen(unit, probeX, probeY)) {
        fprintf(stderr, "Aim: the probe turn lost the target\n");
        return false;
    }
    float gainX = (probeX - startX) / AimProbeCounts;
    float gainY = (probeY - startY) / AimProbeCounts;
    fprintf(stderr, "Aim: %.2f, %.2f screen pixels per mouse count\n", gainX, gainY);
    // An axis that did not move (the view at its pitch limit, a turn smaller than a pixel) gets
    // the usual response; the loop corrects what is left.
    if (fabsf(gainX) < AimMinimumGain) {
        gainX = AimDefaultGain;
    }
    if (fabsf(gainY) < AimMinimumGain) {
        gainY = AimDefaultGain;
    }
    for (int step = 0; step < AimSteps; step++) {
        float x = 0.0f;
        float y = 0.0f;
        if (!TorsoOnScreen(unit, x, y)) {
            fprintf(stderr, "Aim: step %d, target not on screen\n", step);
            return false;
        }
        const float errorX = x - centreX;
        const float errorY = y - centreY;
        if (fabsf(errorX) < AimTolerance && fabsf(errorY) < AimTolerance) {
            fprintf(stderr, "Aim: on target after %d steps\n", step);
            return true;
        }
        // Move a share of the way, never more than AimMaxCounts per step.
        float countsX = -errorX / gainX * AimDamping;
        float countsY = -errorY / gainY * AimDamping;
        countsX = SDL_clamp(countsX, -AimMaxCounts, AimMaxCounts);
        countsY = SDL_clamp(countsY, -AimMaxCounts, AimMaxCounts);
        ckz::TurnView(harness->Applet(), countsX, countsY);
        harness->RunFrames(2);
    }
    fprintf(stderr, "Aim: did not settle in %d steps\n", AimSteps);
    return false;
}

TEST_F(Mission, LauncherShellExplodesOnZombie)
{
    // A shell that reaches a zombie's bounding box is tested exactly against its body
    // (Ballistics::Simulate -> CUnitBody::Pick(line), Group slot 83). The native ray pick used to
    // send that ray the wrong way, and shells flew through zombies to explode somewhere behind.
    // Far zombies are out of reach: the shell drops to the ground short of them on both backends,
    // so the test waits for one within ShellRangeMetres.
    int tries = 0;
    int onTarget = 0;
    for (int attempt = 0; attempt < ShellAttempts && tries < ShellTargets; attempt++) {
        ASSERT_TRUE(m_harness->RunUntil(ZombieInShellRange, TargetTimeoutSeconds)) << "no zombie came close enough";
        HeadOnScreen head = {};
        CUnit* unit = NearestHeadOnScreen(head);
        ASSERT_NE(unit, nullptr);
        if (!AimAt(m_harness, unit)) {
            fprintf(stderr, "Shell on zombie: could not aim at the target\n");
            continue;
        }
        const vec3 target = unit->GetPosition();
        const ShellTrack shell = FireShell(m_harness);
        ASSERT_TRUE(shell.launched);
        tries++;
        const float dx = shell.position.m_x - target.m_x;
        const float dy = shell.position.m_y - target.m_y;
        const float distance = sqrtf(dx * dx + dy * dy);
        fprintf(stderr, "Shell on zombie: target at %.1f,%.1f, shell gone after %.2f s at %.1f,%.1f,%.1f (%.1f m away)\n",
                target.m_x, target.m_y, shell.age, shell.position.m_x, shell.position.m_y, shell.position.m_z, distance);
        // On the body: near the zombie and above the ground (a shell that passed through it ends
        // on the ground behind).
        if (shell.gone && distance < ShellOnTargetMetres && shell.position.m_z > ShellMinHeightMetres) {
            onTarget++;
        }
    }
    ASSERT_GT(tries, 0) << "never got a zombie under the crosshair";
    // The original lets a shell through now and then too (1 of 2 on 2026-11-10); with the old
    // native ray pick none of them stopped on the body.
    fprintf(stderr, "Shell on zombie: %d of %d on the body\n", onTarget, tries);
    // Only asserted on the native backend. The emulated original runs at a lower frame rate, so its
    // shells move in longer steps and the flights cannot be compared one to one (2026-11-10: native
    // 8 of 10 on the body, original 2 of 6, native before the fix 0 of 2).
    if (ckz::config.engine != ckz::EngineSwerve) {
        EXPECT_GE(onTarget * 3, tries) << onTarget << " of " << tries << " shells aimed at a zombie exploded on it";
    }
}

TEST_F(Mission, ExplosivesFromManyAngles)
{
    // Throw grenades and fire shells in ExplosiveAngles directions (turning the view like the mouse
    // does) and count the ones that vanish: a grenade that explodes without touching the level, a
    // shell that expires without hitting anything. Run it with CKZ_ENGINE=swerve to compare.
    //
    // Grenades without contact are reported, not failed: the level's collision mesh only covers
    // the area in front of the player, a grenade landing behind or beside the player falls through
    // the ground and the out-of-bounds check (HandGrenadeBallistics::PostSimulate) sets it off under
    // the ground, unseen. The original does the same (7 of 16 native, 8 of 16 original, 2026-11-10).
    const bool haveGrenade = GameApp()->m_grenadeManager->GetActiveGrenade() != nullptr;
    int grenades = 0;
    int grenadesWithoutContact = 0;
    int grenadesNotExploded = 0;
    int shells = 0;
    int shellsExpired = 0;
    const float maxAge = PlayableMission()->m_grenadeLauncher->m_maxAge;
    float totalTurnX = 0.0f;
    float totalTurnY = 0.0f;
    for (int angle = 0; angle < ExplosiveAngles; angle++) {
        // Alternate left and right, a little lower each time.
        float turnX = ExplosiveTurnCounts;
        if (angle % 2 == 1) {
            turnX = -2.0f * ExplosiveTurnCounts;
        }
        ckz::TurnView(m_harness->Applet(), turnX, ExplosiveTurnDownCounts);
        totalTurnX += turnX;
        totalTurnY += ExplosiveTurnDownCounts;
        m_harness->RunSeconds(0.3);
        // A long run may end in a lost mission or stop at a wave dialog: carry on in a playable one.
        ASSERT_TRUE(m_harness->EnsureMissionPlayable()) << "no playable mission";
        if (haveGrenade) {
            const GrenadeTrack grenade = ThrowHandGrenade(m_harness);
            if (grenade.launched) {
                grenades++;
                if (!grenade.exploded) {
                    grenadesNotExploded++;
                }
                if (grenade.exploded && grenade.collisions == 0) {
                    grenadesWithoutContact++;
                    fprintf(stderr, "Explosives: grenade %d exploded without contact at %.1f,%.1f,%.1f\n", angle,
                            grenade.position.m_x, grenade.position.m_y, grenade.position.m_z);
                }
            }
        }
        const ShellTrack shell = FireShell(m_harness);
        if (shell.launched) {
            shells++;
            if (!shell.gone || shell.age >= maxAge - 0.1f) {
                shellsExpired++;
            }
        }
    }
    fprintf(stderr, "Explosives: %d of %d grenades exploded without touching the level, %d of %d shells expired\n",
            grenadesWithoutContact, grenades, shellsExpired, shells);
    RecordProperty("grenades", grenades);
    RecordProperty("grenadesWithoutContact", grenadesWithoutContact);
    EXPECT_EQ(grenadesNotExploded, 0) << "a grenade never exploded";
    EXPECT_EQ(shellsExpired, 0) << "a shell vanished at its maximum age";
    // Face the zombies again for the tests that follow.
    ckz::TurnView(m_harness->Applet(), -totalTurnX, -totalTurnY);
    m_harness->RunSeconds(0.5);
}

TEST_F(Mission, EveryEquippedWeaponDamages)
{
    CDH_BasicGameData* data = GameApp()->m_helper->m_gamePlayManager.GetGameData();
    int tested = 0;
    for (int slot = 0; slot < 2; slot++) {
        CDH_Weapon* weapon = data->GetWeapon(slot);
        if (weapon == nullptr) {
            continue;
        }
        if (ActiveWeapon() != weapon) {
            // The HUD's switch button: the game data and the weapon view change together.
            GameApp()->m_helper->m_gamePlayManager.SwitchWeapons();
        }
        m_harness->RunSeconds(1.5);
        ASSERT_EQ(ActiveWeapon(), weapon);
        int damagingShots = 0;
        int shots = 0;
        for (int attempt = 0; attempt < 12 && damagingShots == 0; attempt++) {
            ASSERT_TRUE(WaitForTarget()) << "no zombie came into view";
            if (ActiveWeapon() != weapon) {
                // A restarted mission begins with slot 0 again.
                GameApp()->m_helper->m_gamePlayManager.SwitchWeapons();
                m_harness->RunSeconds(1.5);
            }
            HeadOnScreen head = {};
            CUnit* unit = NearestHeadOnScreen(head);
            ASSERT_NE(unit, nullptr);
            const int hp = unit->m_hp;
            ASSERT_TRUE(Shoot(head.x, head.y)) << "weapon " << weapon->m_id << " never became ready";
            shots++;
            if (weapon->m_projectileType == 1) {
                // Launcher: the damage comes with the explosion.
                m_harness->RunSeconds(1.5);
            }
            if (unit->m_hp < hp) {
                damagingShots++;
            }
        }
        tested++;
        EXPECT_GT(damagingShots, 0) << "weapon " << weapon->m_id << " (type " << weapon->m_weaponType
                                    << ", projectile " << weapon->m_projectileType << ") did no damage in " << shots
                                    << " shots";
        fprintf(stderr, "Weapon %d (type %d): %d damaging of %d shots\n", weapon->m_id, weapon->m_weaponType,
                damagingShots, shots);
    }
    EXPECT_GT(tested, 0);
}

/** Weapons.*: every weapon of the game, each in its own survival start (about 15 s per weapon). */
class Weapons : public Mission {
protected:
    void SetUp() override
    {
        m_harness = GameHarness::Get();
        ASSERT_NE(m_harness, nullptr);
    }
};

TEST_F(Weapons, EveryWeaponHitsZombies)
{
    ASSERT_TRUE(m_harness->LeaveMission());
    App* app = GameApp();
    CDH_BasicGameData* data = app->m_helper->m_gamePlayManager.GetGameData();
    LoadoutGuard restoreLoadout;
    std::vector<int> ids;
    Vector<CDH_Weapon*>& weapons = app->m_weaponsManager->GetWeapons();
    for (int index = 0; index < weapons.size(); index++) {
        ids.push_back(weapons[index]->m_id);
    }
    // CKZ_WEAPONS=3,26 tests only those weapon ids.
    const char* only = SDL_getenv(WeaponListVariable);
    if (only != nullptr) {
        ids.clear();
        std::string list = only;
        size_t start = 0;
        while (start < list.size()) {
            size_t comma = list.find(',', start);
            if (comma == std::string::npos) {
                comma = list.size();
            }
            ids.push_back(atoi(list.substr(start, comma - start).c_str()));
            start = comma + 1;
        }
    }
    fprintf(stderr, "Weapons: %zu weapons\n", ids.size());
    for (int id : ids) {
        SCOPED_TRACE("weapon " + std::to_string(id));
        ASSERT_TRUE(m_harness->LeaveMission());
        data->SetWeapon(0, id);
        data->SetActiveWeapon(0);
        ASSERT_TRUE(m_harness->EnterSurvival());
        CDH_Weapon* weapon = ActiveWeapon();
        ASSERT_NE(weapon, nullptr);
        EXPECT_EQ(weapon->m_id, id) << "the mission did not take the weapon of slot 0";
        if (!WaitForTarget()) {
            ADD_FAILURE() << "no zombie came into view";
            continue;
        }
        // Healing guns (projectile type 2) only heal civilians: zombies keep their hit points.
        const bool heals = weapon->m_projectileType == 2;
        int damagingShots = 0;
        int shots = 0;
        for (int attempt = 0; attempt < WeaponShots && damagingShots == 0; attempt++) {
            if (!WaitForTarget()) {
                break;
            }
            // Wait for the weapon first (reload, bolt), then take the target's position and fire at
            // once: a zombie keeps walking while a sniper rifle is being worked.
            if (!m_harness->RunUntil(WeaponReady, WeaponReadySeconds)) {
                ADD_FAILURE() << "weapon " << id << " never became ready";
                break;
            }
            HeadOnScreen head = {};
            CUnit* unit = NearestHeadOnScreen(head);
            if (unit == nullptr) {
                break;
            }
            if (weapon->m_weaponType == 1 || weapon->m_weaponType == 7) {
                // Shotgun damage falls off with distance (CDH_Weapon::GetShotDamage): close in.
                if (!m_harness->RunUntil(ZombieInShellRange, TargetTimeoutSeconds)) {
                    break;
                }
                unit = NearestHeadOnScreen(head);
                if (unit == nullptr) {
                    continue;
                }
            }
            if (weapon->m_projectileType == 1) {
                // Launchers fire along the view (CSwerveGame::LaunchGrenade), not through the
                // touched point: wait for a zombie in range and turn the crosshair onto it.
                if (!m_harness->RunUntil(ZombieInShellRange, TargetTimeoutSeconds)) {
                    break;
                }
                unit = NearestHeadOnScreen(head);
                if (unit == nullptr || !AimAt(m_harness, unit)) {
                    continue;
                }
            }
            const int hp = unit->m_hp;
            if (!Shoot(head.x, head.y, true)) {
                ADD_FAILURE() << "weapon " << id << " never fired";
                break;
            }
            shots++;
            if (weapon->m_projectileType == 1) {
                // Launcher: the damage comes with the explosion.
                m_harness->RunSeconds(1.5);
            }
            if (unit->m_hp < hp) {
                damagingShots++;
            }
        }
        fprintf(stderr, "Weapons: %d (type %d, projectile %d, clip %d): %d damaging of %d shots\n", id,
                weapon->m_weaponType, weapon->m_projectileType, weapon->m_clipSize, damagingShots, shots);
        if (heals) {
            EXPECT_EQ(damagingShots, 0) << "a healing gun hurt a zombie";
        } else {
            EXPECT_GT(damagingShots, 0) << "weapon " << id << " (type " << weapon->m_weaponType << ") did no damage in "
                                        << shots << " shots";
        }
    }
    // Back on the map; LoadoutGuard puts the save's weapons back.
    EXPECT_TRUE(m_harness->LeaveMission());
}

} // namespace
} // namespace ckz::test
