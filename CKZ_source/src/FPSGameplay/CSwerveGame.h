// CSwerveGame.h - complete runtime layout of original u434, size 0x7b0; historical thin header kept at the end of the file.
#ifndef CSWERVEGAME_H
#define CSWERVEGAME_H
#include "WindowBase.h"
#include "tws.h"
#include "App.h"
#include "AppSettings.h"
#include "CGameplayHUD.h"
#include "CHuntingInfo.h"
#include "CQuestManager.h"
#include "SG_Presenter.h"
#include "CAuxRender.h"
#include "CSkyBox.h"
#include "MathLib.h"
#include "CCollision.h"
#include "FrustumCulling.h"
#include "Ballistics.h"
#include "CGameAIMap.h"
#include "CGameAIMap_NavMesh.h"
#include "TimeValue.h"
#include "CResBank.h"
class DGCamera;
class DGCursor;
class CFPSCursor;
class CScriptedSceneController;
class CUnitsController;
class DGWorld;
class CSceneLighting;
class CDH_Weapon;
class CGrenade;
class CUnit;
class CHelicopter;
class CZombieMission;
class Ballistics;
class Fragments;
class HitPoints;
class GrenadeLauncher;
class UnitBodyBallistics;
class SwerveBallistics;
class HandGrenadeBallistics;
class CBH_Player;
class CDH_Animation;
class CLoadingScreen;
class CUnitInfoScreen;
class TextWindow;
/** One easing run (24 bytes): curve, start/end values, duration in ms and elapsed time.
 *  b147: a sub-object in the original; the C2 clears time before duration for each of the three runs. */
struct SEaseRun {
    SEaseRun()
    {
        m_time = 0.0f;
        m_duration = 0;
    }

    EasyLib::EMethod m_method;
    float m_begin;
    float m_end;
    int m_duration;
    float m_time;
    int m_field14;
};

class CSwerveGame : public Window {
public:
    typedef swerve::Group<CSwerveAssertingErrorHandler> Group;
    typedef swerve::World<CSwerveAssertingErrorHandler> World;
    typedef swerve::Node<CSwerveAssertingErrorHandler> Node;
    typedef swerve::Transform<CSwerveAssertingErrorHandler> Transform;
    typedef swerve::Graphics3D<CSwerveAssertingErrorHandler> Graphics3D;
    typedef com::glu::platform::graphics::ICGraphics2d Graphics2D;
    typedef com::glu::platform::graphics::ICRenderSurface Surface;
    CSwerveGame();
    virtual ~CSwerveGame();
    virtual void Init();
    virtual void Shutdown();
    virtual void OnCommand(Event& event);
    virtual void OnPointerEvent(Event& event);
    virtual void Update();
    virtual void Paint(Graphics2D& graphics);
    virtual void Paint3D(Graphics3D& graphics);
    virtual void OnPause();
    virtual void OnCreateState(Window** result, int state);
    virtual void OnStateInterrupt(int state, Window* window);
    virtual void Deactivate(); // Added slots bc/c0/c4, order confirmed by the original primary vtable.
    virtual void Activate();
    virtual void onStartClosing();
    void CleanUp();
    void ReviveCivilian();
    void ShowArcadeMissionResultDialog();
    void onSwitchToScope();
    void OnNewWaveStart(int wave);
    bool IsMissionCompleted();
    void onSwitchFromScope();
    void HandleTouchPressed(int x, int y, int index);
    void HandleTouchReleased(int x, int y, int index);
    void HandleTouchDragged(int x, int y, int index);
    void InitUnits();
    void SimulateAimingDispersion(float elapsed);
    void AmbientSoundsUpdate(int elapsedMs);
    void UpdateGameFinishing(int elapsedMs);
    void UpdateEnvironment(int elapsedMs, bool updateWorld);
    void CheckUnitsPreloadNeed();
    void CheckDialogConditions();
    void HuntingUpdate(int elapsedMs);
    void FinishGameWithResultAndDelayMS(EBountyResult result, int elapsedMs);
    void FinishGameWithResult(EBountyResult result, bool unused);
    void ShowFailResultDialog();
    void UpdateFovBlur(int elapsedMs);
    void ShowReturnToGpsConfirmation();
    void ShowDialogs(CQuestManager::SDialog::ECondition condition);
    void OnInterruptReloading();
    void OnScopeSwitch();
    void OnUnitKilled(CUnit* unit, bool specialKill);
    void OnSkillshot(int type);
    void SimulateAimingDispersion();
    void Shot(int x, int y);
    void UpdateUnits(int elapsedMs);
    void PaintLensFlares(Graphics2D& graphics);
    IGameAIMap* GetGameAIMap();
    void Paint3DBottom(Graphics3D& graphics);
    void Paint3DTop(Graphics3D& graphics);
    void SynchronizeGroup(Group group);
    bool DefineShotProbability(const CDH_Weapon* weapon, const vec3& position);
    const vec3& GetCurrentCameraPos();
    void SimulateAimingDispersion(int& x, int& y, float& resultX, float& resultY, float& result, bool grow);
    void AddSkillshot(Window* window);
    bool AreAnimatedHitPointsSupported() const;
    void AddFlash(const vec3& position, const vec3& velocity, float spread);
    void AddRicochet(const vec3& position, const vec3& velocity, float spread, int type);
    void AddTracer(const vec3& position, const vec3& velocity, float spread);
    void LaunchGrenade(float damage);
    const vec3& GetCurrentCameraRight();
    const vec3& GetCurrentCameraUp();
    vec3 GetCurrentCameraVel();
    void UpdateCameraVectors();
    vec3 GetLaserSightRayCastResult(bool* hit);
    float GetDistanceToCollision(float x, float y);
    void setCursor(DGCursor* cursor);
    void FixWorldPosition();
    void SwitchOffEyeCandyGeometry();
    void LoadSceneLighting();
    void DefineCollisionGeometry();
    void LoadWeapon();
    void LoadLocation();
    void LoadSkyBox();
    void LoadAIMap();
    float GetDistanceToUnit(CUnit* unit);
    void ShowUnitInfo(CUnit* unit, int amount);
    void CreateScene(bool unused);
    void SetupCamera();
    void CreateCameraAnimation();
    void DeleteScene();
    void disableSpritePicking(Group group);
    bool IsGamePaused() const;
    void FillHuntLoadingTasks(CLoadingScreen* screen);
    void InitScope();
    void InitHunting();
    void onWeaponSwitch();
    void OnNextWaveZombieDialogWindowShutdown();
    void OnShooterAttacked();
    void RunFovBlur(float multiplier, int durationMs, bool reset);
    void ShowDialogMessage(const char* title, const char* text, const char* image);
    void LaunchHandGrenade(float delay);
    void LaunchUnitWeapon(vec3& position, Group group, int type);
    void BlastGrenade(const vec3& position, const vec3& normal, float radius, const CDH_Weapon* weapon, const CGrenade* grenade);
    void Blast(const vec3& position, const vec3& normal, float radius, int damage, int power, const CDH_Weapon* weapon, const CGrenade* grenade, const CUnit* source);
    Ballistics* GetCollisionBallistics() const { return m_grenadeLauncher; }
    UnitBodyBallistics* GetUnitBodyBallistics() const { return m_unitBodyBallistics; }
    SwerveBallistics* GetSwerveBallistics() const { return m_swerveBallistics; }
    HandGrenadeBallistics* GetHandGrenadeBallistics() const { return m_handGrenades; }
    CCollision* GetCollision() const { return m_collision; }
    bool IsNavMeshEnabled() const { return m_navMeshEnabled; }
    bool IsAnimationUpdateEnabled() const { return m_animationUpdate; }
    CZombieMission* GetZombieMission() const { return m_zombieMission; }
    CHelicopter* GetHelicopter() const { return m_helicopter; }
    CSceneLighting* GetSceneLighting() const { return m_sceneLighting; }
    DGWorld* GetDGWorld() const { return m_world; }
    const Node& GetRenderReferenceNode() const { return m_referenceNode; }
    CScriptedSceneController* GetScriptController() const { return m_scriptController; }
    CUnitsController* GetUnitsController() const { return m_unitsController; }
    const swerve::Image2D<CSwerveAssertingErrorHandler>& GetLightmap() const { return m_lightmap; }
    CGameplayHUD* GetHUD() const { return m_hud; }
    bool GetDebugFireAction() const
    {
#ifdef CKZ_PC
        // PORT: closing the window frees the HUD before the final weapon update.
        if ((m_flags & FLAG_CLOSING) != 0) {
            return false;
        }
#endif
        if (((App*)WindowApp::m_instance)->m_settings->IsFireButtonEnabled() == true && GetHUD()->m_firePressed) return true;
        return m_debugFire;
    }
    bool GetFlagAc() const { return m_finishing; }
    bool IsHeadShotEffectActive() const { return m_headShotEffect; }

    // All offsets cross-checked from the original constructor/destructor and the methods; scalars with unidentified semantics keep neutral offset names.
    bool m_finishing; // ac
    Window* m_closingWindow; // b0
    int m_fieldB4;
    int m_fieldB8;
    DGCamera* m_camera; // bc
    World m_cameraNode; // c0, keeps the existing consumer field name
    swerve::Light<CSwerveAssertingErrorHandler> m_dynamicLight; // c4
    CUnitsController* m_unitsController;
    CZombieMission* m_zombieMission;
    CScriptedSceneController* m_scriptController;
    CSceneLighting* m_sceneLighting;
    SG_Presenter m_effectPresenters[6]; // d8
    CAuxRender m_auxRender; // 240
    CSkyBox m_skyBox; // 268
    Mat4x4 m_inverseProjection; // 274
    swerve::RayIntersection<CSwerveAssertingErrorHandler> m_rayIntersection; // 2b4
    Transform m_cameraTransform; // 2b8
    float m_field2bc[3];
    float m_field2c8[3];
    float m_collisionDistance; // 2d4
    vec3 m_collisionPoint;
    vec3 m_collisionNormal;
    CCollision::Line m_collisionLine; // 2f0
    CCollision::Triangle m_collisionTriangle; // 308
    Node m_collisionGeometry; // 318
    CGameAIMap m_gameAIMap; // 31c
    CGameAIMap_NavMesh m_navMesh; // 37c
    bool m_navMeshEnabled; // 420
    CBH_Player* m_player; // 424
    CDH_Animation* m_locationAnimation; // 428
    Window* m_resultDialog; // 42c
    bool m_resultShown; // 430
    CUnitInfoScreen* m_unitInfo; // 434
    TimeManager m_timeManager; // 438
    bool m_field464;
    bool m_field465;
    bool m_field466;
    int m_initialLevel; // 468
    bool m_field46c;
    bool m_paused;
    bool m_field46e;
    bool m_field46f;
    bool m_field470;
    bool m_nextWaveDialog;
    bool m_restoreScope;
    Window* m_pauseWindow; // 474
    EBountyResult m_finishResult; // 478
    int m_finishDelay; // 47c
    int m_ambientSound; // 480
    swerve::Image2D<CSwerveAssertingErrorHandler> m_lightmap; // 484
    int m_paintStage; // 488
    int m_pickupGrenade; // 48c
    XString m_cameraName; // 490
    bool m_helicopterScene; // 494
    TextWindow* m_goalText; // 498
    Window* m_pendingDialog; // 49c
    float m_zoom; // 4a0, existing consumer name
    bool m_debugFire;
    Transform m_collisionTransform; // 4a8
    CCollision* m_collision;
    HitPoints* m_hitPoints; // 4b0
    HitPoints* m_survivalHitPoints;
    Fragments* m_debris;
    Fragments* m_tracers;
    Fragments* m_particles;
    Fragments* m_smoke;
    Fragments* m_flashes;
    Fragments* m_ricochets;
    GrenadeLauncher* m_grenadeLauncher;
    UnitBodyBallistics* m_unitBodyBallistics;
    SwerveBallistics* m_swerveBallistics;
    HandGrenadeBallistics* m_handGrenades;
    CHelicopter* m_helicopter;
    Vector<CCollision::CKinematicBody*> m_kinematicBodies; // 4e4
    int m_state; // 4f4, existing consumer name
    float m_startCountdown;
    int m_environmentTime;
    SViewport m_viewport; // 500
    DGWorld* m_world; // 508
    Surface* m_blurSurfaces[2]; // 50c
    int m_blurTarget; // 514
    float m_blurAmount; // 518
    int m_field51c;
    swerve::Background<CSwerveAssertingErrorHandler> m_background; // 520
    int m_transition; // 524
    SEaseRun m_transitionRun; // 528
    SEaseRun m_blurRun; // 540
    SEaseRun m_fovRun; // 558
    float m_savedFov; // 570
    bool m_fovBlur;
    bool m_resetFovBlur;
    int m_blurFrames; // 578
    float m_blendColor[4]; // 57c
    bool m_headShotEffect; // 58c
    bool m_animationUpdate;
    bool m_field58e;
    int m_field590;
    int m_previousActiveCount;
    int m_unitUpdateElapsed;
    Vector<int> m_preloadStates; // 59c
    Vector<int> m_preloadTypes; // 5ac
    Vector<Group> m_preloadGroups; // 5bc
    bool m_field5cc;
    DGCursor* m_cursor; // 5d0
    CFPSCursor* m_ownedCursor;
    int m_field5d8;
    bool m_pendingFire; // 5dc
    int m_touchX[10]; // 5e0, original touch x coordinate
    int m_touchY[10]; // 608, original touch y coordinate
    bool m_touchActive[10]; // 630
    bool m_sceneReady; // 63a
    CGameplayHUD* m_hud; // 63c
    bool m_field640;
    int m_field644;
    int m_interruptCountdown; // 648
    Vector<bool> m_pauseStack; // 64c
    bool m_pendingInterrupt; // 65c
    CResBank m_resources; // 660
    CCollision::AABB m_bounds; // 684
    bool m_field69c;
    float m_inputX; // 6a0
    float m_inputY;
    float m_targetAngleH;
    float m_targetAngleV;
    float m_velocityH;
    float m_velocityV;
    float m_responseTime;
    int m_zeroInputFrames;
    bool m_field6c0;
    swerve::Camera<CSwerveAssertingErrorHandler> m_referenceNode; // 6c4, original fe4b0 already did Query5, later calls go directly to the camera
    Transform m_referenceTransform;
    Transform m_lightTransform;
    // 6d0: right, up, forward, position. b147: an array in the original (the C2 default-constructs it
    // with a 4-iteration z/y/x store loop instead of one merged memset).
    enum { CAMERA_RIGHT = 0, CAMERA_UP = 1, CAMERA_FORWARD = 2, CAMERA_POSITION = 3 };
    vec3 m_cameraVectors[4];
    float m_field700; // 700, original dirty starts at 704
    bool m_cameraDirty; // 704
    int m_cameraSampleCount; // 708
    vec3 m_cameraHistory[6]; // 70c
    float m_cameraTimes[6]; // 754
    World m_handsWorld; // 76c
    Transform m_handsTransform;
    Vector<Group> m_syncSource; // 774
    Vector<Group> m_syncTarget;
    int m_weaponId; // 794
    swerve::Light<CSwerveAssertingErrorHandler> m_handsLight;
    Group m_handsLightGroup;
    Group m_eyeTransform;
    Transform m_eyeMatrix;
    bool m_tapToShoot; // 7a8
    bool m_field7a9;
    int m_field7ac;
};
#endif

// The old thin header and all historical comments are kept below, only for tracing the recovery process.
#if 0
// PROVISIONAL (created by b048): owner src/FPSGameplay/CSwerveGame.cpp not written yet
// CSwerveGame.h - the running 3D game (a window); only what src/dh3d uses is declared.
#ifndef CSWERVEGAME_H
#define CSWERVEGAME_H

#include "WindowBase.h"
#include "tws.h"
#include "App.h"  // added by b055x (GetDebugFireAction)
#include "AppSettings.h"  // added by b055x
#include "CGameplayHUD.h"  // added by b055x
#include "CHuntingInfo.h"

class DGCamera;
class CScriptedSceneController;  // added by b079
class CUnitsController;  // added by b080
class CGameplayHUD;  // added by b055x
class DGWorld;
class CSceneLighting;
class CDH_Weapon;
class CGrenade;
class CUnit;
class vec3;
class CHelicopter;
class CZombieMission;
class IGameAIMap;
class Ballistics;
class UnitBodyBallistics;
class SwerveBallistics;
class HandGrenadeBallistics;
class CCollision;

class CSwerveGame : public Window {
public:
    CSwerveGame();  // added by b055x
    /** The player switched weapons. */
    void onWeaponSwitch();
    /** Shoot at the screen position (x, y). */
    void Shot(int x, int y);

    void LaunchHandGrenade(float delay);  // added by b055x
    void OnScopeSwitch();  // added by b055x
    void onSwitchToScope();  // added by b055x
    void onSwitchFromScope();  // added by b055x
    // Real owner methods; camera accessors may first call UpdateCameraVectors (+0x704).
    const vec3& GetCurrentCameraPos();    // 0xfdba4, storage +0x6f4
    const vec3& GetCurrentCameraRight();  // 0xfe224, storage +0x6d0
    const vec3& GetCurrentCameraUp();     // 0xfe240, storage +0x6dc
    vec3 GetLaserSightRayCastResult(bool* hit); // 0xfe610, hidden structure return
    void AddFlash(const vec3& position, const vec3& velocity, float spread); // 0xfdde8
    void AddRicochet(const vec3& position, const vec3& velocity, float spread, int type); // 0xfde60
    void BlastGrenade(const vec3& position, const vec3& normal, float radius,
                      const CDH_Weapon* weapon, const CGrenade* grenade); // 0x100850
    void Blast(const vec3& position, const vec3& normal, float radius, int damage, int power,
               const CDH_Weapon* weapon, const CGrenade* grenade, const CUnit* source); // 0x1008b4
    void OnShooterAttacked(); // 0x1003f0
    IGameAIMap* GetGameAIMap(); // original 0xfd0bc, selects the real embedded map by +420
    void OnUnitKilled(CUnit* unit, bool specialKill); // original 0xfbb58
    void ShowUnitInfo(CUnit* unit, int amount); // original 0xff4b4; the result of the final call is not used as the return value
    void LaunchUnitWeapon(vec3& position, swerve::Group<CSwerveAssertingErrorHandler> group, int type); // original 0x100750
    float GetDistanceToCollision(float x, float y); // original 0xfe748
    bool DefineShotProbability(const CDH_Weapon* weapon, const vec3& position); // original 0xfdb70
    /** Below are named views of original direct Unit/Body/Target field reads; the complete owner is still pending b067. */
    Ballistics* GetCollisionBallistics() const
    {
        return *(Ballistics* const*)(m_data4a4 + (0x4d0 - 0x4a4));
    }
    UnitBodyBallistics* GetUnitBodyBallistics() const
    {
        return *(UnitBodyBallistics* const*)(m_data4a4 + (0x4d4 - 0x4a4));
    }
    SwerveBallistics* GetSwerveBallistics() const
    {
        return *(SwerveBallistics* const*)(m_data4a4 + (0x4d8 - 0x4a4));
    }
    HandGrenadeBallistics* GetHandGrenadeBallistics() const
    {
        return *(HandGrenadeBallistics* const*)(m_data4a4 + (0x4dc - 0x4a4));
    }
    CCollision* GetCollision() const
    {
        return *(CCollision* const*)(m_data4a4 + (0x4ac - 0x4a4));
    }
    bool IsNavMeshEnabled() const
    {
        return *(const bool*)(m_datac4 + (0x420 - 0xc4));
    }
    bool IsAnimationUpdateEnabled() const
    {
        return *(const bool*)(m_data4f8 + (0x58d - 0x4f8));
    }
    void FinishGameWithResultAndDelayMS(EBountyResult result, int milliseconds); // 0xfb4d8
    CZombieMission* GetZombieMission() const
    {
        return *(CZombieMission* const*)(m_datac4 + (0xcc - 0xc4));
    }
    CHelicopter* GetHelicopter() const
    {
        return *(CHelicopter* const*)(m_data4a4 + (0x4e0 - 0x4a4));
    }

    /** Ballistics reads these existing handles; accessor names describe the observed fields. */
    CSceneLighting* GetSceneLighting() const
    {
        return *(CSceneLighting* const*)(m_datac4 + (0xd4 - 0xc4));
    }
    DGWorld* GetDGWorld() const
    {
        return *(DGWorld* const*)(m_data4f8 + (0x508 - 0x4f8));
    }
    const swerve::Node<CSwerveAssertingErrorHandler>& GetRenderReferenceNode() const
    {
        return *(const swerve::Node<CSwerveAssertingErrorHandler>*)(m_data4f8 + (0x6c4 - 0x4f8));
    }
    /** FOV effect used by power ups (0x10041c). */  // added by b078a
    void RunFovBlur(float multiplier, int durationMs, bool reset);
    /** Script controller at +0xd0 (CScriptManager 0x133ae8, tutorial 0x13c004). */  // added by b079
    CScriptedSceneController* GetScriptController() const  // added by b079
    {
        return *(CScriptedSceneController* const*)(m_datac4 + (0xd0 - 0xc4));
    }
    /** Unit controller read by CZombieMission at +0xc8. */  // added by b080
    CUnitsController* GetUnitsController() const  // added by b080
    {
        return *(CUnitsController* const*)(m_datac4 + (0xc8 - 0xc4));
    }
    /** Shared lightmap handle read by both Visual ReplaceTexture overloads at +0x484. */  // added by b079e
    const swerve::Image2D<CSwerveAssertingErrorHandler>& GetLightmap() const  // added by b079e
    {
        return *(const swerve::Image2D<CSwerveAssertingErrorHandler>*)(m_datac4 + (0x484 - 0xc4));
    }
    /** The gameplay HUD (0x63c). */  // added by b055x
    CGameplayHUD* GetHUD() const  // added by b055x
    {  // added by b055x
        return *(CGameplayHUD* const*)(m_data4f8 + (0x63c - 0x4f8));  // added by b055x
    }  // added by b055x
    /** Fire held: fire button on the HUD, or the debug fire flag at 0x4a4 (weak copy in CDH_Weapon.o). */  // added by b055x
    bool GetDebugFireAction() const  // added by b055x
    {  // added by b055x
        if (((App*)WindowApp::m_instance)->m_settings->IsFireButtonEnabled() == true && GetHUD()->m_firePressed) {  // added by b055x
            return true;  // added by b055x
        }  // added by b055x
        return *(const bool*)m_data4a4;  // added by b055x
    }  // added by b055x

    /** Flag at 0xac (a bool; when set the pause menu gets `false` as its second argument). */  // added by b055x
    bool GetFlagAc() const  // added by b055x
    {  // added by b055x
        return *(const bool*)m_dataac;  // added by b055x
    }  // added by b055x
    /** Head-shot zoom flag read at 0x168b70. */  // added by b049
    bool IsHeadShotEffectActive() const  // added by b049
    {
        return *(const bool*)(m_data4f8 + (0x58c - 0x4f8));
    }

    unsigned char m_dataac[0xbc - 0xac];    // layout not known yet
    DGCamera* m_camera;                     // 0xbc
    swerve::World<CSwerveAssertingErrorHandler> m_cameraNode;    // 0xc0 node the camera transform is taken from (FrustumCulling)
    unsigned char m_datac4[0x4a0 - 0xc4];
    float m_zoom;                           // 0x4a0 sight zoom (CFPSCursor::UpdateImpl)
    unsigned char m_data4a4[0x4f4 - 0x4a4];
    int m_state;                            // 0x4f4 game state (2: shooting allowed, from DGCursor::Fire)
    unsigned char m_data4f8[0x7a8 - 0x4f8];
    bool m_tapToShoot;                      // 0x7a8 a released tap shoots (CFPSCursor::onPointerReleased)
    unsigned char m_data7a9[0x7b0 - 0x7a9];    // size 0x7b0 from CGameScreen::OnCreateState  // added by b055x
};

#endif

#endif







