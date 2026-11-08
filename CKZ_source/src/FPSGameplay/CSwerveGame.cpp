// CSwerveGame.cpp - original u434; lifecycle, scene, input, shooting and helicopter actions.
#include "CSwerveGame.h"
#include "CHelicopter.h"
#include "DGHelper.h"
#include "dgCamera.h"
#include "DGWorld.h"
#include "DGCursor.h"
#include "CFpsCursor.h"
#include "SwerveHelper.h"
#include "CSwerve.h"
#include "CDH_Weapon.h"
#include "CDH_Animation.h"
#include "CDH_SoundManager.h"
#include "CSceneLighting.h"
#include "CZombieMission.h"
#include "CUnitsController.h"
#include "CScriptedSceneController.h"
#include "CVisualCharacterTypeManager.h"
#include "CBH_Player.h"
#include "CPlayerGameStats.h"
#include "CPowerUpManager.h"
#include "CPowerUp.h"
#include "CUnitInfoScreen.h"
#include "CAchievementManager.h"
#include "CLoadingScreen.h"
#include "CUnit.h"
#include "CGameAnalytics.h"
#include "CQuestZombieDialogWindow.h"
#include "CDialogQueueWindow.h"
#include "CMissionManager.h"
#include "CLocationManager.h"
#include "CSceneSetupsManager.h"
#include "BaseDialog.h"
#include "CFontMgr.h"
#include "CPopupController.h"
#include "CGrenadeManager.h"
#include "CGrenade.h"
#include "CTutorialManager.h"
#include "CApplet.h"
#include "ICCore.h"
#include "ICRenderSurface.h"
#include "ICGraphics2d.h"
#include "ICGraphics.h"
#include "CBH_PersonType.h"
#include "CSceneObjectManager.h"
#include "CDH_WeaponsManager.h"
#include "CZombieMissionManager.h"
#include "CLocation.h"
#include "CGameResultDialogWindow.h"
#include "CNotEnoughCurrencyDialogWindowWithPurchase.h"
#include "CSaveManager.h"
#include "CCommonGameConfig.h"
#include "CRandGen.h"
#include "CProgressSystem.h"
#include "CMath.h"

using namespace com::glu::platform::graphics;
using com::glu::platform::components::CStrChar;
using com::glu::platform::core::CRandGen;
typedef swerve::Camera<CSwerveAssertingErrorHandler> Camera;
typedef swerve::Object3D<CSwerveAssertingErrorHandler> Object3D;
typedef swerve::Mesh<CSwerveAssertingErrorHandler> Mesh;
typedef swerve::Fog<CSwerveAssertingErrorHandler> Fog;
typedef swerve::Transform<CSwerveAssertingErrorHandler> Transform;
typedef swerve::Group<CSwerveAssertingErrorHandler> Group;
typedef swerve::World<CSwerveAssertingErrorHandler> World;
typedef swerve::Node<CSwerveAssertingErrorHandler> Node;

static inline App* GameApp() { return (App*)WindowApp::m_instance; }
// The CHuntingInfo thin header has proven total size 78; keeps the width of the original not-yet-named 32-bit statistic fields.
static inline int& HuntValue(int offset) { return *(int*)((char*)GameApp()->m_huntingInfo+offset); }

// original __MergedGlobals@4feef0 is actually two groups of touch parameters, 8 floats in total; the source variable names are not preserved.
static float g_touchSensitivityParameters[4] = {33.0f,3.33f,10.0f,1.32f};
static float g_touchSensitivityTabletParameters[4] = {33.0f,6.66f,10.0f,2.64f};

// Each device branch interpolates separately; the helper function names were assigned during recovery.
static inline float TouchSensitivity(float fov, const float* parameters)
{
    float interpolation = (fov-parameters[2])/(parameters[0]-parameters[2]);
    if (interpolation < 0.0f) interpolation = 0.0f;
    else interpolation = com::glu::platform::math::CMath::Min(1.0f,interpolation);
    return parameters[1]*interpolation + parameters[3]*(1.0f-interpolation);
}

static inline float DefaultTouchSensitivity(float fov)
{
    return TouchSensitivity(fov,g_touchSensitivityParameters);
}

static inline float TabletTouchSensitivity(float fov)
{
    return TouchSensitivity(fov,g_touchSensitivityTabletParameters);
}

class PlayerBody : public CCollision::CKinematicBody {
public:
    PlayerBody()
    {
        m_local.m_min = vec3(-0.125f,-0.125f,-1.0f);
        m_local.m_max = vec3(0.125f,0.125f,1.0f);
        m_world = m_local;
    }
    virtual ~PlayerBody() {}
    void UpdatePlayerPos()
    {
        CSwerveGame* game = GameApp()->m_helper->m_swerveGame;
        vec3 position = game->GetCurrentCameraPos();
        CSwerveGame* current = GameApp()->m_helper->m_swerveGame;
        if (current->m_helicopterScene) {
            current->GetCurrentCameraPos();
            // Copied by value: the original loads the forward vector before calling Sqrt.
            vec3 forward = current->m_cameraForward;
            float dx = m_local.m_max.m_x-m_local.m_min.m_x;
            float dy = m_local.m_max.m_y-m_local.m_min.m_y;
            position = position + forward * MathLib::Sqrt((dx*dx+dy*dy)+0.0f);
        }
        m_world = m_local;
        m_world.m_min = m_world.m_min + position;
        m_world.m_max = m_world.m_max + position;
    }
    virtual bool Intersect(const CCollision::Line& line,vec3& point,vec3& normal)
    {
        normal = vec3(0,0,1);
        return CCollision::Intersect(line,m_world,point);
    }
    CCollision::AABB m_local;
    CCollision::AABB m_world;
};

class StadiumGoalGate : public CCollision::CKinematicBody {
public:
    StadiumGoalGate() : m_lastTrigger(0)
    {
        m_bounds.m_min = vec3(52.6565018f,-2.73799992f,3.16700006f);
        m_bounds.m_max = vec3(53.7014999f,2.63800001f,11.1680002f);
    }
    virtual ~StadiumGoalGate() {}
    virtual bool Intersect(const CCollision::Line& line,vec3& point,vec3& normal)
    {
        if (GameApp()->m_frameStartTime-m_lastTrigger < 10000) return false;
        if (GameApp()->m_grenadeManager->GetActiveGrenade() == NULL) return false;
        if (GameApp()->m_grenadeManager->GetActiveGrenade()->getId() != 3) return false;
        // Operand order matches the original cmp (hand-grenade ballistics on the left).
        if (GameApp()->m_helper->m_swerveGame->GetHandGrenadeBallistics() != Ballistics::g_CurrentlySimulatedBallistics) return false;
        if (CCollision::Intersect(line,m_bounds,point)) {
            m_lastTrigger = GameApp()->m_frameStartTime;
            CBH_Player::GetInstance()->m_gameStats.NotifySurvivalGoal();
            XString goalText(L"GOAL!!!");
            GameApp()->m_popupController->AddTextPopup(goalText,(CPopupController::EPosition)3,(CPopupController::EAnimationStyle)0,false,17);
            GameApp()->m_soundManager->Play((CDH_SoundManager::eSound)28,false,false);
        }
        return false;
    }
    CCollision::AABB m_bounds;
    int m_lastTrigger;
};

CSwerveGame::CSwerveGame()
    : Window(false), m_finishing(false), m_closingWindow(NULL), m_camera(NULL),
      m_zombieMission(NULL), m_sceneLighting(NULL), m_inverseProjection(1.0f),
      m_rayIntersection(CSwerve::GetInstance()->m_swerve),
      m_cameraTransform(CSwerve::GetInstance()->m_swerve), m_collisionDistance(-1.0f),
      m_navMeshEnabled(false), m_locationAnimation(NULL), m_resultDialog(NULL), m_resultShown(false),
      m_unitInfo(NULL), m_field464(false), m_field465(false), m_field466(false),
      m_field46c(false), m_paused(true), m_field46e(false), m_field46f(true),
      m_field470(false), m_nextWaveDialog(false), m_restoreScope(false),
      m_finishResult((EBountyResult)4), m_finishDelay(0), m_paintStage(0), m_pickupGrenade(-1),
      m_helicopterScene(false), m_goalText(NULL), m_pendingDialog(NULL), m_debugFire(false),
      m_collisionTransform(CSwerve::GetInstance()->m_swerve), m_state(0),
      m_startCountdown(2.0f), m_environmentTime(0), m_world(NULL),
      m_blurAmount(0.0f), m_field51c(0.0f), m_transition(0),
      m_transitionDuration(0), m_transitionTime(0.0f), m_blurDuration(0), m_blurTime(0.0f),
      m_fovDuration(0), m_fovTime(0.0f), m_fovBlur(false), m_resetFovBlur(false),
      m_headShotEffect(false), m_animationUpdate(false), m_field58e(false),
      m_field590(0), m_previousActiveCount(0), m_unitUpdateElapsed(0), m_field5cc(false),
      m_cursor(NULL), m_ownedCursor(NULL), m_field5d8(0), m_pendingFire(false),
      m_sceneReady(false), m_field640(false), m_interruptCountdown(-1), m_pendingInterrupt(false),
      m_field69c(false), m_field6c0(true),
      m_referenceTransform(CSwerve::GetInstance()->m_swerve), m_lightTransform(CSwerve::GetInstance()->m_swerve),
      m_cameraDirty(true), m_weaponId(-1), m_tapToShoot(false), m_field7a9(false)
{
    m_viewport.m_x = m_viewport.m_y = m_viewport.m_width = m_viewport.m_height = 0;
    m_inputX = m_inputY = m_targetAngleH = m_targetAngleV = m_velocityH = m_velocityV = m_responseTime = 0.0f;
    m_zeroInputFrames = 0;
    for (int index = 0; index < 4; ++index) (&m_cameraRight)[index] = vec3(0,0,0);
    for (int index = 0; index < 6; ++index) { m_cameraHistory[index] = vec3(0,0,0); m_cameraTimes[index] = 0.0f; }
    m_cameraSampleCount = 0;
    for (int index = 0; index < 10; ++index) {
        m_touchX[index] = 0; m_touchY[index] = 0; m_touchActive[index] = false;
    }
    CApplet::m_pCore->SetMultipleTouchEnabled(1);
    GameApp()->m_helper->initSwerveGame(this);
    m_player = CBH_Player::GetInstance();
    m_zoom = 0.0f;
    // original 0xf8b40 clears 14 raw pointers, not the whole Game object.
    m_collision = NULL; m_hitPoints = NULL; m_survivalHitPoints = NULL;
    m_debris = NULL; m_tracers = NULL; m_particles = NULL; m_smoke = NULL;
    m_flashes = NULL; m_ricochets = NULL; m_grenadeLauncher = NULL;
    m_unitBodyBallistics = NULL; m_swerveBallistics = NULL; m_handGrenades = NULL; m_helicopter = NULL;
    m_handsWorld = World(App::LoadObject3D("BIN_ZS_WEAPON_HANDS"));
    Camera eye(m_handsWorld.find(DGHelper::getSwerveID("eyecam")));
    m_handsWorld.setActiveCamera(eye);
    swerve::Background<CSwerveAssertingErrorHandler> background = m_handsWorld.getBackground();
    background.setColorClearEnable(false);
    background.setDepthClearEnable(false);
    m_handsTransform = SwerveHelper::NewTransform();
    m_eyeMatrix = SwerveHelper::NewTransform();
    m_eyeTransform = Group(m_handsWorld.find(SwerveHelper::Id("eyecamTransform")));
    m_eyeTransform.getCompositeTransform(m_eyeMatrix);
    for (int index = 0; index < 2; ++index) {
        m_blurSurfaces[index] = Surface::CreateInstance(Surface::Type_Default, Surface::Targetability_2);
        Surface::InitParam params[] = {
            {1,(unsigned int)GameApp()->m_screenWidth}, {2,(unsigned int)GameApp()->m_screenHeight},
            {4,0}, {6,0x000f4404}, {7,0x005a0102}, {0,0}
        };
        m_blurSurfaces[index]->Initialize(params);
    }
    m_background = SwerveHelper::NewBackground();
    m_blurTarget = 0;
    CBH_PersonType::GenerateGlobalSeed();
}

CSwerveGame::~CSwerveGame()
{
    if (GameApp()->m_powerUpManager->IsPowerUpRunning() == 1) GameApp()->m_powerUpManager->FinishRunningPowerUp();
    if (GameApp()->m_helper->m_gamePlayManager.GetGameData()->GetWeapon(0) != NULL)
        GameApp()->m_helper->m_gamePlayManager.GetGameData()->GetWeapon(0)->update(0);
    if (GameApp()->m_helper->m_gamePlayManager.GetGameData()->GetWeapon(1) != NULL)
        GameApp()->m_helper->m_gamePlayManager.GetGameData()->GetWeapon(1)->update(0);
    CApplet::m_pCore->SetMultipleTouchEnabled(0);
    CleanUp();
    GameApp()->m_helper->SetSwerveGamePtr(NULL);
    delete m_hitPoints; delete m_survivalHitPoints; delete m_debris; delete m_tracers;
    delete m_particles; delete m_smoke; delete m_flashes; delete m_ricochets;
    delete m_grenadeLauncher; delete m_unitBodyBallistics; delete m_swerveBallistics; delete m_handGrenades;
    for (int index = 0; index < m_kinematicBodies.size(); ++index) {
        if (m_collision != NULL) m_collision->RemoveKinematicBody(m_kinematicBodies[index]);
        // The original re-reads the pointer from the container after RemoveKinematicBody returns.
        delete m_kinematicBodies[index];
    }
    if (m_collision != NULL) delete m_collision;
}

void CSwerveGame::CleanUp()
{
    delete m_zombieMission; m_zombieMission = NULL;
    DeleteScene();
    GameApp()->m_soundManager->StopAllSoundsAndRelease();
    if (m_helicopter != NULL) { delete m_helicopter; m_helicopter = NULL; }
    // The original releases and clears the handle directly, not through the assignment operator of an empty temporary.
    m_cameraNode.Clear();
    m_rayIntersection.Clear();
    m_collisionGeometry.Clear();
    GameApp()->m_visualCharacterTypeManager->CleanupResources();
    GameApp()->m_sceneObjectManager->CleanupResources();
    for (int index = 0; index < 2; ++index) {
        if (m_blurSurfaces[index] != NULL) { delete m_blurSurfaces[index]; m_blurSurfaces[index] = NULL; }
    }
    DGHelper* helper = GameApp()->m_helper;
    CDH_WeaponsManager* weapons = GameApp()->m_weaponsManager;
    CDH_Weapon* weapon = helper->m_gamePlayManager.GetGameData()->GetWeapon(0);
    if (weapon != NULL) weapons->release3dResources(weapon->getId());
    weapon = helper->m_gamePlayManager.GetGameData()->GetWeapon(1);
    if (weapon != NULL) weapons->release3dResources(weapon->getId());
    if (m_closingWindow != NULL) m_closingWindow->CloseSafely();
}
void CSwerveGame::onStartClosing() { CleanUp(); }

void CSwerveGame::Init()
{
    GameApp()->m_helper->m_gamePlayManager.GetGameData()->SetActiveWeapon(0);
    const XString& missionName = GameApp()->m_huntingInfo->m_assignedData.m_missionName;
    XString location = GameApp()->m_missionManager->GetMissionByName(missionName)->m_locationName;
    location = location.Lower();
    {
        XString roof("roof");
        // Found branch first: matches the original block order.
        if (location.Find(roof,0) != -1) {
            XString helicopter("hel");
            m_helicopterScene = location.Find(helicopter,0) != -1;
        }
        else {
            m_helicopterScene = false;
        }
    }
    CMission* mission = GameApp()->m_missionManager->GetMissionByName(missionName);
    if (!*(bool*)&GameApp()->m_huntingInfo->m_field00) CBH_Player::GetInstance()->m_gameStats.NotifyMissionBegin(mission);
    if (!*(bool*)&GameApp()->m_huntingInfo->m_field00) GameApp()->m_helper->m_gamePlayManager.RestoreAmmo();
    *(bool*)&GameApp()->m_huntingInfo->m_field00 = false;
    CGameAnalytics::logMissionStarted(mission->m_name,mission->m_difficulty);
    CApplet::m_pCore->SetTickRate(1);
    ChangeState(0,(Window::eStateTransitionType)2);
    Activate();
    m_initialLevel = CBH_Player::GetInstance()->m_id;
    m_unitsController = new CUnitsController(this,GameApp()->m_huntingInfo->m_assignedData.m_missionName);
    m_scriptController = new CScriptedSceneController(this,mission);
    // original f93f2..f9410 temporary copy has its own AddRef/Release, ending right after Generate.
    // b117-sg: by-value argument temporary (manager pointer is loaded before the copy in the original).
    m_zombieMission = GameApp()->m_zombieMissionManager->Generate(mission->m_type,mission->m_difficulty,mission->m_numWaves,XString(mission->m_waveFilter));
    m_zombieMission->Init(m_scriptController->m_waveCyclesCount);
    m_zombieMission->CompleteAllWavesBeforeThis(GameApp()->m_huntingInfo->m_currentWave);
    m_hud = new CGameplayHUD(this);
    AddToFront(m_hud);
    GameApp()->m_powerUpManager->FinishRunningPowerUp();
    m_pauseWindow = new CDialogQueueWindow(NULL);
    AddToFront(m_pauseWindow);
    m_unitsController->AddListener(m_scriptController);
    m_unitsController->AddListener(m_zombieMission);
    App::LoadSGArchetype(0,0);
    // Original is fully unrolled: six presenters, animations 12..17.
    m_effectPresenters[0].SetArchetypeCharacter(0,0);
    m_effectPresenters[0].SetAnimation(12);
    m_effectPresenters[1].SetArchetypeCharacter(0,0);
    m_effectPresenters[1].SetAnimation(13);
    m_effectPresenters[2].SetArchetypeCharacter(0,0);
    m_effectPresenters[2].SetAnimation(14);
    m_effectPresenters[3].SetArchetypeCharacter(0,0);
    m_effectPresenters[3].SetAnimation(15);
    m_effectPresenters[4].SetArchetypeCharacter(0,0);
    m_effectPresenters[4].SetAnimation(16);
    m_effectPresenters[5].SetArchetypeCharacter(0,0);
    m_effectPresenters[5].SetAnimation(17);
}
void CSwerveGame::Shutdown()
{
    m_resources.removeAllKeysets();
    GameApp()->m_soundManager->StopSounds();
}
void CSwerveGame::Deactivate()
{
    bool show = m_hud->GetFlag145();
    m_pauseStack.addElement(show);
    m_hud->ClearFlags(1);
    if (m_cursor != NULL) m_cursor->PointerBreak();
}
void CSwerveGame::Activate()
{
    if (m_pauseStack.size() != 0) {
        int index = m_pauseStack.size()-1;
        bool show = m_pauseStack[index];
        m_pauseStack.removeElementAt(index);
        if (show) m_hud->Show(true,0.0f);
    }
    if (m_cursor != NULL) m_cursor->PointerBreak();
}

void CSwerveGame::OnCommand(Event& event)
{
    if (!m_sceneReady) return;
    switch (event.m_id) {
    case 0x7214ff0d: m_field7a9 = false; break;
    case 0x60042695: {
        CUnitsController* controller = m_unitsController;
        bool unharmed = true;
        int wave = m_zombieMission->m_currentWaveNumber;
        for (int index = 0; index < controller->m_units.size(); ++index) {
            CUnit* unit = controller->m_units[index];
            if (unit->IsCivilian() == true) { unharmed = unit->m_hp == unit->m_maxHp; break; }
        }
        CBH_Player* player = CBH_Player::GetInstance();
        player->m_gameStats.NotifySurvivalWave(wave);
        if (unharmed) player->m_gameStats.NotifySurvivalWaveForEvelynTakesNoDamage(wave);
        HuntValue(0x70) = wave;
        float hp = m_unitsController->GetCiviliansMeanRelativeHP();
        *(float*)((char*)GameApp()->m_huntingInfo+0x74) = hp;
        CMission* mission = GameApp()->m_missionManager->GetMissionByName(GameApp()->m_huntingInfo->m_assignedData.m_missionName);
        if ((int)mission->m_type == 6 && !m_field46f && hp > 0.0f) {
            if (wave%5 == 0) GameApp()->m_visualCharacterTypeManager->CleanupResources();
            m_field470 = true;
            CItem* item = GameApp()->m_waveManager->GetRecommendedItem(m_zombieMission->m_currentWave->m_recommendationId);
            if (item != NULL) {
                m_restoreScope = m_hud->GetFlagBc();
                if (m_hud->GetFlagBc()) m_hud->switchScope();
                GameApp()->m_huntingInfo->m_text04 = item->GetStringUId();
                m_pendingDialog = new CNextWaveZombieDialogWindow(item);
                m_nextWaveDialog = true;
                m_hud->HandleCommand(0x47b537b6,0,0,0);
            }
        }
        m_field46e = true; m_field46f = false;
        break;
    }
    case 0x5c420ec6: {
        GameApp()->m_saveManager->ResumeSaving();
        int credits = CBH_Player::GetInstance()->GetStats((PLAYER_STATS_TYPE)4);
        int attempt = HuntValue(0x54)+1;
        int cost = attempt*5;
        if (attempt > 18) cost = 95;
        if (credits < cost) {
            attempt = HuntValue(0x54)+1;
            cost = attempt*5;
            if (attempt > 18) cost = 95;
            CNotEnoughCurrencyDialogWindowWithPurchase* dialog = new CNotEnoughCurrencyDialogWindowWithPurchase(cost,0x5c420ec6);
            dialog->InitIAP();
            CDialogQueueWindow::AddDialogInQueue(dialog);
            return;
        }
        CBH_Player* player = CBH_Player::GetInstance();
        attempt = HuntValue(0x54)+1;
        cost = attempt*5;
        if (attempt > 18) cost = 95;
        player->ReduceCredits(cost,L"");
        ReviveCivilian();
        m_finishing = false; m_resultDialog = NULL;
        GameApp()->m_saveManager->PauseSaving();
        CGameAnalytics::logCivilianRevivePurchased();
        return;
    }
    case 0x3f972120: m_field640 = true; break;
    case 0x3e59479d: m_paused = true; break;
    case 0x3318da15: m_state = 1; m_startCountdown = 2.0f; break;
    case 0x2074d009: m_hud->ThrowGrenade(); break;
    case 0x1958a2e7: CGameAnalytics::logSurvivalModeShopEntered(CBH_Player::GetInstance()->m_id,false); break;
    case 0x1105a3a5: event.Clear(); m_pendingInterrupt = true; return;
    case 0x0f6a94ea: ShowArcadeMissionResultDialog(); return;
    case -0x2b7c95b: m_debugFire = false; break;
    case -0x3ba6d46:
        for (int index = 0; index < 100; ++index) m_unitsController->ShootAllZombies();
        break;
    case -0x6c6b1d9: m_paused = false; break;
    case -0x29352a5a: event.Clear(); WindowApp::HandleTunnelCommand(0x1105a3a5,0,0,0); return;
    case -0x340249bc: SetInterrupt(1); break;
    case -0x5763c5c5:
        CGameAnalytics::logSurvivalModeShopEntered(CBH_Player::GetInstance()->m_id,true);
        m_restoreScope = false;
        *(bool*)&GameApp()->m_huntingInfo->m_field00 = true;
        SetInterrupt(1);
        break;
    case -0x68dff166: m_debugFire = true; m_cursor->Fire(); break;
    case -0x68dfd73f: break;
    case -0x68df5c35: m_field7a9 = true; break;
    case -0x68df4cb6: WindowApp::HandleEvent(Event::COMMAND,(int)0x9247fd7b,0,0,0); break;
    case -0x60adcd6e: break;
    case 0x385a651b: break;
    case 0x6ef3b1d1: {
        CMission* mission = GameApp()->m_missionManager->GetMissionByName(GameApp()->m_huntingInfo->m_assignedData.m_missionName);
        CBH_Player* player = CBH_Player::GetInstance();
        int xp = HuntValue(0x0c); player->FieldAt(0x100) = xp;
        int cash = HuntValue(0x10); player->FieldAt(0xfc) = cash;
        mission->OnMissionCompleted(cash,xp);
        int id = CBH_Player::GetInstance()->m_id;
        CGameAnalytics::logSurvivalModeFinishedWave(id,HuntValue(0x70));
        CGameAnalytics::logSurvivalModeZombiesKilled(id,CBH_Player::GetInstance()->FieldAt(0x170));
        CGameAnalytics::logSurvivalModeXpEarned(id,HuntValue(0x0c));
        CGameAnalytics::logSurvivalModeSoftEarned(id,HuntValue(0x10));
        CGameAnalytics::logSurvivalModeFinalScore(id,HuntValue(8));
        SetInterrupt(1);
        break;
    }
    default: return;
    }
    event.Clear();
}

void CSwerveGame::ReviveCivilian()
{
    m_unitsController->DeleteAllZombies();
    m_unitsController->ReviveCivilian();
    m_zombieMission->RestartCurrentWave();
    m_field46e = true;
    ++HuntValue(0x54);
}
void CSwerveGame::onSwitchToScope()
{
    CDH_Weapon* weapon = GameApp()->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon();
    m_cursor->EnableSight(weapon->m_weaponType == 8);
    m_hud->OnScopeSwitched();
    weapon = GameApp()->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon();
    // No-scope branch first (original tests bne to the zoom-slider block at the end).
    if (!weapon->m_scopeAttached) {
        if (m_field6c0) m_cursor->ShowCross(false);
    }
    else {
        m_hud->SetZoomSliderEnable(true);
    }
}
void CSwerveGame::OnNewWaveStart(int wave)
{
    GameApp()->m_soundManager->Play((CDH_SoundManager::eSound)33,false,false);
    CMission* mission = GameApp()->m_missionManager->GetMissionByName(GameApp()->m_huntingInfo->m_assignedData.m_missionName);
    if ((int)mission->m_type == 6) {
        XString waveText = XString::Format(ResString("IDS_ARCADE_WAVE").m_data,wave);
        GameApp()->m_popupController->AddTextPopup(waveText,(CPopupController::EPosition)3,(CPopupController::EAnimationStyle)0,false,17);
        int kills = CBH_Player::GetInstance()->FieldAt(0x170);
        if (kills > 0) {
            XString format = ResString("IDS_GAME_STAT_ZOMBIES_KILLED")+L": %d";
            XString text = XString::Format(format.m_data,kills);
            GameApp()->m_popupController->AddTextPopup(text,(CPopupController::EPosition)3,(CPopupController::EAnimationStyle)0,false,17);
        }
    }
    if (CRandGen::GetInstance()->Generate()%100 > 49) return;
    XString missionName = GameApp()->m_huntingInfo->m_assignedData.m_missionName;
    XString location = GameApp()->m_missionManager->GetMissionByName(missionName)->m_locationName;
    location = location.Lower();
    if (location.Find(L"roof",0) == -1 && !(location == L"roof") && (((int)mission->m_type|2) != 3)) return;
    float delay = (float)CRandGen::GetInstance()->GetRandRange(1000,1100)*0.001f;
    GameApp()->m_soundManager->PlayRandomSoundWithDelay("KEYSET_SHOOTER_WAVE",delay,false);
}
void CSwerveGame::onSwitchFromScope()
{
    bool switchScope = false;
    if (GameApp()->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon()->IsReloading() == 1 && !m_field464) {
        m_field464 = true;
        switchScope = true;
    } else {
        CDH_Weapon* weapon = GameApp()->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon();
        if (weapon->m_action == 8 && !m_field465) { m_field465 = true; switchScope = true; }
    }
    if (switchScope && m_hud->GetFlagBc()) { m_hud->switchScope(); m_field466 = true; }
    m_camera->restoreView(true);
    m_cursor->EnableSight(true);
    m_cursor->ShowCross(true);
    m_hud->OnScopeSwitched();
}
/** Arcade continue cost: -1 when hunting state is 3, else 5 per attempt capped at 95. Name is inferred. */
static inline int ArcadeContinueCost()
{
    int cost = -1;
    if (HuntValue(0x38) != 3) {
        int attempt = HuntValue(0x54)+1;
        cost = attempt*5;
        if (attempt > 18) cost = 95;
    }
    return cost;
}
void CSwerveGame::ShowArcadeMissionResultDialog()
{
    m_resultShown = true;
    CBH_Player* player = CBH_Player::GetInstance();
    player->FieldAt(0x100) = HuntValue(0x0c);
    player->FieldAt(0xfc) = HuntValue(0x10);
    // The original computes the cost after operator new (inside the argument list).
    m_resultDialog = new CGameResultDialogWindow(HuntValue(8),ArcadeContinueCost(),true);
    CDialogQueueWindow::AddDialogInQueue(m_resultDialog);
}
void CSwerveGame::FinishGameWithResultAndDelayMS(EBountyResult result,int elapsedMs)
{
    if (GameApp()->m_powerUpManager->IsPowerUpRunning() == 1) GameApp()->m_powerUpManager->FinishRunningPowerUp();
    if (GameApp()->m_helper->m_gamePlayManager.GetGameData()->GetWeapon(0) != NULL)
        GameApp()->m_helper->m_gamePlayManager.GetGameData()->GetWeapon(0)->update(0);
    if (GameApp()->m_helper->m_gamePlayManager.GetGameData()->GetWeapon(1) != NULL)
        GameApp()->m_helper->m_gamePlayManager.GetGameData()->GetWeapon(1)->update(0);
    CDH_Weapon* weapon = GameApp()->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon();
    if ((unsigned int)((int)m_finishResult-3) <= 1) {
        // The original reads the weapon's remaining action time before storing the result.
        int actionTimeLeft = weapon->m_actionTimeLeft;
        m_finishResult = result;
        m_finishDelay = actionTimeLeft+elapsedMs;
        m_finishing = true;
    }
}
void CSwerveGame::FinishGameWithResult(EBountyResult result,bool unused)
{
    m_finishing = true;
    GameApp()->m_popupController->ClearPopups(false);
    GameApp()->m_huntingInfo->SetBountyResult(result);
    CMission* mission = GameApp()->m_missionManager->GetMissionByName(GameApp()->m_huntingInfo->m_assignedData.m_missionName);
    if ((int)mission->m_type == 6) ShowArcadeMissionResultDialog();
    else if (GameApp()->m_huntingInfo->IsPlayerWin() == true) {
        m_scriptController->OnWin();
        GameApp()->m_soundManager->Play((CDH_SoundManager::eSound)31,false,false);
        GameApp()->m_missionManager->GetMissionByName(GameApp()->m_huntingInfo->m_assignedData.m_missionName)->OnMissionCompleted(-1,-1);
        if ((int)mission->m_type == 4) {
            CPlayerGameStats& stats = CBH_Player::GetInstance()->m_gameStats;
            // Original is a tbb jump table (cases 0..3); block order is case 2, 3, then 0/1.
            switch ((int)mission->m_difficulty) {
            case 2:
                stats.AddMediumCopterMissions(1);
                break;
            case 3:
                stats.AddHardCopterMissions(1);
                break;
            case 0:
                stats.AddEasyCopterMissions(1);
                break;
            case 1:
                stats.AddEasyCopterMissions(1);
                break;
            }
        }
        m_resultShown = true;
        m_resultDialog = new CGameResultDialogWindow();
        CDialogQueueWindow::AddDialogInQueue(m_resultDialog);
    } else {
        GameApp()->m_soundManager->Play((CDH_SoundManager::eSound)32,false,false);
        ShowFailResultDialog();
        CMission* failed = GameApp()->m_missionManager->GetMissionByName(GameApp()->m_huntingInfo->m_assignedData.m_missionName);
        CBH_Player* player = CBH_Player::GetInstance();
        CGameAnalytics::logMissionFailed(failed->m_name,player->m_id,m_zombieMission->m_cycleIndex);
    }
    if (CBH_Player::GetInstance()->m_id-m_initialLevel > 0) *(bool*)((char*)GameApp()->m_huntingInfo+0x6c) = true;
}
void CSwerveGame::ShowFailResultDialog()
{
    m_resultShown = true;
    CZombieDialogWindow* dialog = new CZombieDialogWindow(3);
    dialog->SetTitle(ResString("IDS_DIALOG_TITLE_FAIL"));
    int result = HuntValue(0x38);
    XString text;
    // Written ==1 first: clang reverses the chain, giving the original's cmp #2 first.
    if (result == 1) text = ResString("IDS_RESULT_MISSION_FAILED")+XString("\n");
    else if (result == 2) text = ResString("IDS_BOUNTY_OUT_OF_AMMO")+XString("\n");
    dialog->SetText(text,false,NULL);
    dialog->AddButton((CZombieDialogWindow::EDialogButton)1,0x1105a3a5,0,0,true,20);
    CDialogQueueWindow::AddDialogInQueue(dialog);
}
void CSwerveGame::ShowReturnToGpsConfirmation()
{
    class Confirmation : public CZombieDialogWindow {
    public:
        Confirmation() : CZombieDialogWindow(3) {}
        virtual ~Confirmation() { GameApp()->m_timeScale = 1.0f; }
        virtual void Update() { GameApp()->m_timeScale = 0.0f; }
    };
    Window* parent = m_pauseWindow;
    Confirmation* dialog = new Confirmation();
    dialog->SetTitle(ResString("IDS_DIALOG_TITLE_WARNING"));
    dialog->SetText(ResString("IDS_RETURN_TO_GPS_TEXT"),false,NULL);
    dialog->AddButton((CZombieDialogWindow::EDialogButton)1,0x1105a3a5,0,0,true,20);
    dialog->AddButton((CZombieDialogWindow::EDialogButton)4,0xf9394e27,0,0,true,20);
    parent->AddModal(dialog);
    m_paused = true;
}
void CSwerveGame::ShowDialogs(CQuestManager::SDialog::ECondition condition)
{
    int questId = HuntValue(0x18);
    if (questId >= 0) {
        CQuestManager::SQuest* quest = GameApp()->m_questManager->GetQuestById(questId);
        int count = quest->m_gameDialogs.size();
        for (int index = 0; index < count; ++index) {
            if (quest->m_gameDialogs[index].m_condition == condition) {
                CDialogQueueWindow::AddDialogInQueue(new CQuestZombieDialogWindow(quest->m_gameDialogs[index]));
                count = quest->m_gameDialogs.size();
            }
        }
        int* command = (int*)&static_cast<CDialogQueueWindow*>(m_pauseWindow)->m_finishCommand;
        command[0] = (int)0xf9394e27; command[1] = 0; command[2] = 0; command[3] = 0;
        m_paused = true;
    }
}
void CSwerveGame::OnInterruptReloading() { m_field466 = false; m_pendingFire = false; }
void CSwerveGame::OnScopeSwitch() { m_savedFov = m_camera->GetFOV(); }
void CSwerveGame::OnUnitKilled(CUnit* unit,bool specialKill)
{
    CMission* mission = GameApp()->m_missionManager->GetMissionByName(GameApp()->m_huntingInfo->m_assignedData.m_missionName);
    if ((int)mission->m_type == 6) {
        CBH_PersonType* type = unit->m_body.m_personType;
        int score = (int)type->m_arcadeScore;
        float multiplier = m_zombieMission->m_currentWave->m_cashXpMultiplier;
        int cash = (int)(multiplier*type->m_arcadeCash);
        int xp = (int)(multiplier*type->m_arcadeXp);
        GameApp()->m_huntingInfo->AddArcadePoints(score,cash,xp);
        m_goalText->SetText(XString::Format(ResString("IDS_ARCADE_SCORE").m_data,HuntValue(8)));
        vec3 normal(0,0,1);
        vec3 hit = unit->m_body.GetHitPosition();
        if (m_survivalHitPoints != NULL) {
            TextWindow* text = new TextWindow(XString::Format(L"+%d",score),CFontMgr::GetInstance()->GetFont(29));
            text->SetWidthByContent(0,0); text->SetHeightByContent(0,0);
            m_survivalHitPoints->AddWindow(text,hit,normal);
        }
        if (specialKill) OnSkillshot(GameApp()->m_commonGameConfig->m_headshotScore);
    }
}
void CSwerveGame::OnSkillshot(int score)
{
    CMission* mission = GameApp()->m_missionManager->GetMissionByName(GameApp()->m_huntingInfo->m_assignedData.m_missionName);
    if ((int)mission->m_type == 6) {
        GameApp()->m_huntingInfo->AddArcadePoints(score,0,0);
        // One full expression: the original loads m_goalText before ResString and
        // destroys the three temporaries in reverse order right after SetText.
        m_goalText->SetText(XString::Format(XString::Format(ResString("IDS_ARCADE_SCORE").m_data,HuntValue(8)).m_data));
        if (m_survivalHitPoints != NULL) {
            TextWindow* text = new TextWindow(XString::Format(L"+%d",score),CFontMgr::GetInstance()->GetFont(27));
            text->SetWidthByContent(0,0); text->SetHeightByContent(0,0);
            m_survivalHitPoints->AddWindow(text);
        }
    }
}
void CSwerveGame::SimulateAimingDispersion()
{
    int elapsedMs = GameApp()->m_updateFrameTime;
    float fov = m_camera->GetFOV();
    if (!(fov > 0.0f)) return;
    short width = m_width;
    float currentFov = m_camera->GetFOV();
    float baseFov = m_camera->m_defaultFov;
    float minimum = GameApp()->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon()->m_minAimDispersion;
    float minimumSpeed = GameApp()->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon()->m_minAimRecoverySpeed;
    float maximum = GameApp()->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon()->m_maxAimDispersion;
    float maximumSpeed = GameApp()->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon()->m_maxAimRecoverySpeed;
    float minimumAgain = GameApp()->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon()->m_minAimDispersion;
    float screenScale = (float)width/fov;
    float maximumAgain = GameApp()->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon()->m_maxAimDispersion;
    float fovScale = currentFov/baseFov;
    float value = m_zoom/screenScale;
    float amount = (value-fovScale*minimum)/(fovScale*maximum-fovScale*minimum);
    if (amount > 1.0f) amount = 1.0f;
    if (amount < 0.0f) amount = 0.0f;
    float recovery = (fovScale*minimumSpeed)*(1.0f-amount)+(fovScale*maximumSpeed)*amount;
    value = value+(recovery*(float)elapsedMs)*-0.001f;
    if (value < fovScale*minimumAgain) value = fovScale*minimumAgain;
    if (GameApp()->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon()->IsReloading() == 1) {
        value += ((float)elapsedMs*15.0f)*0.001f;
        if (value > fovScale*maximumAgain) value = fovScale*maximumAgain;
    }
    m_zoom = value*screenScale;
}
void CSwerveGame::SimulateAimingDispersion(int& x,int& y,float& resultX,float& resultY,float& unused,bool grow)
{
    float radius = m_zoom;
    int first = (int)(1000u-CRandGen::GetInstance()->GetRand(2000));
    float radiusAgain = m_zoom;
    float dx = (radius*0.001f)*(float)first;
    int second = (int)(1000u-CRandGen::GetInstance()->GetRand(2000));
    float dy = (radiusAgain*0.001f)*(float)second;
    float square = dx*dx+dy*dy;
    float length = MathLib::InvSqrt(square)*square;
    if (length > m_zoom) {
        float scale = m_zoom/length;
        dy *= scale; dx *= scale;
    }
    x += (int)dx; y += (int)dy;
    float firstFov = m_camera->GetFOV();
    short width = m_width;
    float secondFov = m_camera->GetFOV();
    float baseFov = m_camera->m_defaultFov;
    float perShot = GameApp()->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon()->m_aimDispersionPerShoot;
    float maxSpread = GameApp()->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon()->m_maxAimDispersion;
    float screenScale = (float)width/firstFov;
    float fovScale = secondFov/baseFov;
    if (grow) {
        float value = m_zoom/screenScale+perShot*fovScale;
        if (value > maxSpread*fovScale) value = maxSpread*fovScale;
        m_zoom = value*screenScale;
    }
    resultX = (float)x;
    resultY = (float)y;
}
void CSwerveGame::UpdateUnits(int elapsedMs)
{
    m_unitsController->Update(elapsedMs);
    int unit = m_unitsController->m_updateCounter58;
    // Same unit as last frame: accumulate time; the reset branch comes last in the original.
    if (unit != 0 && unit == m_previousActiveCount) {
        int elapsed = m_unitUpdateElapsed+elapsedMs;
        m_unitUpdateElapsed = elapsed;
        if (elapsed > GameApp()->m_progressSystem->GetTaggingTimeMS()) {
            m_unitUpdateElapsed = 0;
            ShowUnitInfo((CUnit*)unit,0);
        }
    }
    else {
        m_unitUpdateElapsed = 0;
    }
    m_previousActiveCount = unit;
}
void CSwerveGame::ShowDialogMessage(const char* title,const char* text,const char* image)
{
    CQuestManager::SDialog dialog;
    // Original stores the image pointer before the two ResString calls.
    dialog.m_image = image;
    dialog.m_text = ResString(text);
    dialog.m_title = ResString(title);
    CDialogQueueWindow::AddDialogInQueue(new CQuestZombieDialogWindow(dialog));
}
void ShowDialogMessage(const char* title,const char* text,const char* image)
{
    CSwerveGame* game = GameApp()->m_helper->m_swerveGame;
    if (game != NULL) game->ShowDialogMessage(title,text,image);
}

void CSwerveGame::HandleTouchPressed(int x,int y,int index)
{
    int result = m_hud->PointerPressed(x,y,index);
    // The original compares against 2 first, so the -1 case is written first here (if-chain is reversed by clang).
    if (result == -1) {
        if (m_cursor != NULL) {
            if (index >= 1) {
                if (!GameApp()->m_settings->IsFireButtonEnabled() && GameApp()->m_helper->m_swerveGame->m_tapToShoot) m_pendingFire = true;
            }
            else {
                m_cursor->PointerPressed(x,y);
            }
        }
    } else if (result == 2) {
        if (GameApp()->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon()->m_shootType != 2) m_cursor->Fire();
    }
    m_touchActive[index] = true;
    m_touchX[index] = x;
    m_touchY[index] = y;
}
void CSwerveGame::HandleTouchReleased(int x,int y,int index)
{
    m_pendingFire = false;
    m_hud->PointerReleased(x,y,index);
    m_hud->CheckContollStateOnPointerReleased(x,y,index);
    m_pendingFire = false;
    m_touchActive[index] = false;
    m_touchX[index] = 0; m_touchY[index] = 0;
    if (m_cursor != NULL) m_cursor->PointerReleased();
}
void CSwerveGame::HandleTouchDragged(int x,int y,int index)
{
    if (y > m_height) y = m_touchY[index];
    if (y <= -1) y = m_touchY[index];
    if (x > m_width) x = m_touchX[index];
    if (x <= -1) x = m_touchX[index];
    m_hud->PointerDragged(x,y,index);
    m_hud->CheckContollStateOnPointerMove(x,y,m_touchX[index],m_touchY[index],index);
    if (m_touchActive[index]) {
        // The two groups of four floats in original MergedGlobals0x4feef0; source and wrong-address correction recorded in data.
        unsigned int device = App::GetDeviceType();
        float sensitivity;
        // The original computes the two groups of device parameters separately; GetFOV is called before the parameters are read.
        if (device <= 12 && ((1u<<device)&0x1802u) != 0) {
            sensitivity = TabletTouchSensitivity(m_camera->GetFOV());
        }
        else {
            sensitivity = DefaultTouchSensitivity(m_camera->GetFOV());
        }
        int oldX = m_touchX[index];
        int oldY = m_touchY[index];
        float setting = (float)GameApp()->m_settings->m_field38;
        float fov = m_camera->GetFOV();
        float horizontalScale = (float)m_width/fov;
        float verticalScale = (float)m_height/(fov*(float)m_height/(float)m_width);
        float horizontal = (sensitivity*(setting*0.01f))*(float)(x-oldX);
        float vertical = (sensitivity*(setting*0.01f))*(float)(y-oldY);
        m_targetAngleH -= horizontal/horizontalScale;
        m_targetAngleV -= vertical/verticalScale;
        m_inputX -= (horizontal/horizontalScale)*0.25f;
        m_inputY -= (vertical/horizontalScale)*0.25f;
        if (m_cursor != NULL && !m_headShotEffect) m_cursor->PointerMove(x,y,(int)horizontal,(int)vertical);
        m_touchX[index] = x; m_touchY[index] = y;
    }
}
void CSwerveGame::OnPointerEvent(Event& event)
{
    if (m_sceneReady && !m_paused && !m_finishing) {
        int x = event.m_param1;
        int y = event.m_param2;
        unsigned int index = event.m_param3;
        if (index > 9) return;
        // Three-case switch: the original compares RELEASED, DRAGGED, then PRESSED (first case falls through last).
        switch (event.m_type) {
        case Event::POINTER_PRESSED:
            HandleTouchPressed(x,y,index);
            break;
        case Event::POINTER_RELEASED:
            HandleTouchReleased(x,y,index);
            break;
        case Event::POINTER_DRAGGED:
            HandleTouchDragged(x,y,index);
            break;
        }
    }
    event.Clear();
}
void CSwerveGame::InitUnits()
{
    m_unitsController->InitUnits();
    m_zombieMission->Preload(-1,false);
    m_scriptController->PreLoad();
}

// The three-component translation helper inlined in the original; the helper function name was assigned during recovery.
static inline void ResetWorldTranslation(swerve::Transformable<CSwerveAssertingErrorHandler>& world)
{
    float zero[3];
    zero[0] = 0.0f;
    zero[1] = 0.0f;
    zero[2] = 0.0f;
    world.setTranslation(zero);
}

void CSwerveGame::FixWorldPosition()
{
    // Original queries interface 0x1b (Transformable), not 0x12 (Node).
    swerve::Transformable<CSwerveAssertingErrorHandler> world(m_cameraNode.find(DGHelper::getSwerveID("worldTransform")));
    ResetWorldTranslation(world);
}
void CSwerveGame::SwitchOffEyeCandyGeometry()
{
    if (m_cameraNode.m_interface == NULL) return;
    Group light(m_cameraNode.find(DGHelper::getSwerveID("LightSourceTransform")));
    Group candy(m_cameraNode.find(DGHelper::getSwerveID("EyeCandyTransform")));
    if (light.m_interface != NULL) {
        Group parent(light.getParent());
        if (parent.m_interface != NULL) parent.removeChild(light);
        // The original explicitly releases and nulls the parent node handle before the end of scope.
        if (parent.m_interface != NULL) parent.m_interface->Release();
        parent.m_interface = NULL;
    }
    if (candy.m_interface != NULL) {
        Group parent(candy.getParent());
        if (parent.m_interface != NULL) parent.removeChild(candy);
        // The original explicitly releases and nulls the parent node handle before the end of scope.
        if (parent.m_interface != NULL) parent.m_interface->Release();
        parent.m_interface = NULL;
    }
    // original feb60..feb7a explicitly releases in lookup order, not the default reverse order of locals.
    light.Clear();
    candy.Clear();
}
void CSwerveGame::LoadSceneLighting()
{
    if (m_cameraNode.m_interface == NULL) return;
    Group lights(m_cameraNode.find(DGHelper::getSwerveID("dynamic_lightsTransform")));
    if (lights.m_interface != NULL) { lights.setRenderingEnable(false); lights.setPickingEnable(false); }
    m_sceneLighting = new CSceneLighting(m_cameraNode, lights);
}
void CSwerveGame::DefineCollisionGeometry()
{
    if (m_cameraNode.m_interface == NULL) return;
    m_collisionGeometry = Group(m_cameraNode.find(DGHelper::getSwerveID("firegeometryTransform")));
    if (m_collisionGeometry.m_interface != NULL) {
        m_collisionGeometry.setPickingEnable(true);
        m_collisionGeometry.setRenderingEnable(false);
    }
}
void CSwerveGame::SynchronizeGroup(Group group)
{
    int userId = group.getUserID();
    if (userId != 0) {
        Group matching(World(GameApp()->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon()->m_world).find(userId));
        if (matching.m_interface != NULL) {
            m_syncSource.addElement(group);
            m_syncTarget.addElement(matching);
        }
    }
    int count = group.getChildCount();
    for (int index = 0; index < count; ++index) {
        Group child(group.getChild(index));
        if (child.m_interface != NULL) SynchronizeGroup(child);
        else {
            swerve::SkinnedMesh<CSwerveAssertingErrorHandler> skin(group.getChild(index));
            if (skin.m_interface != NULL) SynchronizeGroup(skin.getSkeleton());
        }
    }
}
void CSwerveGame::UpdateCameraVectors()
{
    for (int index = 4; index >= 0; --index) {
        m_cameraHistory[index+1] = m_cameraHistory[index];
        m_cameraTimes[index+1] = m_cameraTimes[index];
    }
    ++m_cameraSampleCount;
    if ((unsigned int)m_cameraSampleCount > 6) m_cameraSampleCount = 6;
    m_cameraHistory[0] = m_cameraPosition;
    m_cameraTimes[0] = GameApp()->m_timeSeconds;
    m_cameraDirty = false;
    m_cameraRight = vec3(1,0,0); m_cameraUp = vec3(0,1,0);
    m_cameraForward = vec3(0,0,1); m_cameraPosition = vec3(0,0,0);
    if (m_referenceNode.m_interface == NULL) m_referenceNode = Camera(m_cameraNode.find(SwerveHelper::Id(XString::AnsiString(m_cameraName.m_data))));
    if (m_referenceNode.m_interface == NULL) return;
    m_referenceNode.getTransformTo(m_cameraNode, m_referenceTransform);
    float matrix[16];
    m_referenceTransform.get(matrix);
    m_cameraRight = vec3(matrix[0],matrix[4],matrix[8]);
    m_cameraUp = vec3(matrix[1],matrix[5],matrix[9]);
    m_cameraForward = vec3(matrix[2],matrix[6],matrix[10]);
    m_cameraPosition = vec3(matrix[3],matrix[7],matrix[11]);
    m_referenceNode.getProjection(m_cameraTransform);
    SwerveHelper::SetMatrix(m_inverseProjection, m_cameraTransform);
    m_inverseProjection.Invert();
}
vec3 CSwerveGame::GetCurrentCameraVel()
{
    vec3 result;
    if (!m_helicopterScene) return result;
    float timeScale = 1.0f;
    if (GameApp()->m_powerUpManager->IsPowerUpRunning() == 1)
        timeScale = GameApp()->m_powerUpManager->GetActivePowerUp()->m_reloadSoundSpeed;
    int intervals = m_cameraSampleCount - 1;
    if (intervals < 1) return result;
    float total = 0.0f;
    for (int index = 0; index < intervals; ++index) total += timeScale * (m_cameraTimes[index] - m_cameraTimes[index+1]);
    for (int index = 0; index < m_cameraSampleCount-1; ++index) {
        float elapsed = timeScale * (m_cameraTimes[index] - m_cameraTimes[index+1]);
        vec3 velocity;
        if (elapsed > 0.0f) velocity = (m_cameraHistory[index] - m_cameraHistory[index+1]) * (1.0f / elapsed);
        result = result + velocity * (elapsed / total);
    }
    return result;
}
vec3 CSwerveGame::GetLaserSightRayCastResult(bool* hit)
{
    if (hit != NULL) *hit = false;
    if (m_collision != NULL) {
        int width = GameApp()->m_helper->m_screenWidth;
        int height = GameApp()->m_helper->m_screenHeight;
        float distance = GetDistanceToCollision((float)(width/2), (float)(height/2));
        if (distance > 0.0f) {
            if (hit != NULL) *hit = true;
            vec3 position = GetCurrentCameraPos();
            if (m_cameraDirty) UpdateCameraVectors();
            return position - m_cameraForward * distance;
        }
    }
    vec3 position = GetCurrentCameraPos();
    return position + vec3(0.0f,0.0f,10000.0f);
}
float CSwerveGame::GetDistanceToCollision(float x, float y)
{
    Camera camera = m_cameraNode.getActiveCamera();
    m_collisionDistance = -1.0f;
    m_collisionPoint = vec3(0,0,0); m_collisionNormal = vec3(0,0,0);
    float width = (float)GameApp()->m_helper->m_screenWidth;
    float height = (float)GameApp()->m_helper->m_screenHeight;
    vec3 nearPoint = m_inverseProjection.TransformFast(vec3(2.0f*x/width-1.0f,-(2.0f*y/height-1.0f),1.0f));
    vec3 origin(m_inverseProjection[0][3],m_inverseProjection[1][3],m_inverseProjection[2][3]);
    camera.getTransformTo(m_cameraNode, m_cameraTransform);
    float direction[4] = {nearPoint.m_x,nearPoint.m_y,nearPoint.m_z,0};
    m_cameraTransform.transform(4, direction);
    vec3 ray(direction[0],direction[1],direction[2]);
    ray = ray * MathLib::InvSqrt(ray.LengthSqr());
    float position[4] = {origin.m_x,origin.m_y,origin.m_z,1.0f};
    m_cameraTransform.transform(4, position);
    origin = vec3(position[0],position[1],position[2]);
    m_collisionLine.m_from = origin;
    m_collisionLine.m_to = origin + ray * 1000.0f;
    if (m_collision->IntersectNearest(m_collisionLine,m_collisionPoint,m_collisionNormal,m_collisionTriangle,false))
        m_collisionDistance = MathLib::Sqrt((m_collisionPoint-origin).LengthSqr());
    return m_collisionDistance;
}
void CSwerveGame::DeleteScene()
{
    m_sceneReady = false;
    if (m_unitsController != NULL) { delete m_unitsController; m_unitsController = NULL; }
    if (m_scriptController != NULL) { delete m_scriptController; m_scriptController = NULL; }
    if (m_ownedCursor != NULL) { delete m_ownedCursor; m_ownedCursor = NULL; }
    m_cursor = NULL;
    if (m_camera != NULL) { delete m_camera; m_camera = NULL; }
    if (m_world != NULL) { delete m_world; m_world = NULL; }
    if (m_locationAnimation != NULL) { delete m_locationAnimation; m_locationAnimation = NULL; }
    if (m_sceneLighting != NULL) { delete m_sceneLighting; m_sceneLighting = NULL; }
}

void CSwerveGame::CreateScene(bool unused)
{
    m_world = new DGWorld(m_cameraNode,NULL);
    CMission* mission = GameApp()->m_missionManager->GetMissionByName(GameApp()->m_huntingInfo->m_assignedData.m_missionName);
    int capacity = GameApp()->m_locationManager->GetLocation(mission->m_locationName)->m_sectorCapacity;
    CGrenade* grenade = GameApp()->m_grenadeManager->GetActiveGrenade();
    m_collision = new CCollision(m_collisionGeometry,capacity);
    m_collisionGeometry.getTransformTo(m_cameraNode,m_collisionTransform);
    for (int index = 0; index < m_kinematicBodies.size(); ++index) m_collision->AddKinematicBody(m_kinematicBodies[index]);
    if ((int)mission->m_type == 6) {
        m_survivalHitPoints = new HitPoints(32,3.0f,NULL);
        m_goalText = new TextWindow(L"",CFontMgr::GetInstance()->GetFont(26));
        m_goalText->SetAlign(12); m_goalText->SetOutsetSpacing(8);
        AddToFront(m_goalText);
    }
    unsigned int device = App::GetDeviceType();
    int tracerCount = 32;
    int debrisCount = 128;
    int particleCount = 128;
    int smokeCount = 64;
    float particleAge = 2.0f;
    float smokeAge = 1.5f;
    float ricochetAge = 2.5f;
    bool smokeCollision = true;
    if (device <= 8 && ((1u<<device)&0x19cu) != 0) {
        tracerCount = 16; debrisCount = 32; particleCount = 42; smokeCount = 32;
        particleAge = 1.5f; smokeAge = 1.0f; ricochetAge = 1.5f; smokeCollision = false;
    }
    m_tracers = new Fragments(tracerCount,0.22f,vec3(0,0,0),NULL,(Fragments::EParticleType)2,vec3(100,100,25),"BIN_M3G_EFFECT_TRACER_SYSTEM");
    m_debris = new Fragments(debrisCount,0.11f,vec3(0,0,-9.8f),NULL,(Fragments::EParticleType)1,vec3(100,100,25),"BIN_M3G_EFFECT_TRACER_SYSTEM");
    m_particles = new Fragments(particleCount,particleAge,vec3(0,0,-9.8f),m_collision,(Fragments::EParticleType)3,vec3(10,10,10),"BIN_M3G_EFFECT_DEBRIS_SYSTEM");
    CCollision* smokeGeometry = NULL;
    if (smokeCollision) smokeGeometry = m_collision;
    m_smoke = new Fragments(smokeCount,smokeAge,vec3(0,0,-1),smokeGeometry,(Fragments::EParticleType)3,vec3(100,100,100),"BIN_M3G_EFFECT_SMOKE_SYSTEM");
    m_flashes = new Fragments(tracerCount,0.15f,vec3(0,0,0),NULL,(Fragments::EParticleType)3,vec3(100,100,100),"BIN_M3G_EFFECT_FLASH_SYSTEM");
    m_ricochets = new Fragments(smokeCount,ricochetAge,vec3(0,0,-9.8f),m_collision,(Fragments::EParticleType)0,vec3(100,100,40),"BIN_M3G_EFFECT_TRACER_SYSTEM");
    float bounce1 = 0.25f;
    float bounce2 = 0.5f;
    if (grenade == NULL) m_ricochets->SetDrag(0.95f);
    else { m_ricochets->SetDrag(grenade->m_shardDrag); bounce1 = grenade->m_shardBounce; bounce2 = grenade->m_shardFriction; }
    m_ricochets->SetBouncing(bounce1,bounce2);
    m_ricochets->SetCollisionFlags(3);
    m_grenadeLauncher = new GrenadeLauncher(m_collision,"BIN_M3G_GRENADE","BIN_ZS_WEAPON_ROCKET");
    m_grenadeLauncher->SetCollisionFlags(7);
    m_unitBodyBallistics = new UnitBodyBallistics(m_collision,&m_auxRender);
    m_swerveBallistics = new SwerveBallistics(tracerCount,3.0f,m_collision);
    m_handGrenades = new HandGrenadeBallistics(m_collision);
    m_handGrenades->SetCollisionFlags(3);
    m_viewport.m_x = 0; m_viewport.m_y = 0;
    m_viewport.m_width = GameApp()->m_helper->m_screenWidth;
    m_viewport.m_height = GameApp()->m_helper->m_screenHeight;
    SetupCamera();
    {
    Transform transform(CSwerve::GetInstance()->m_swerve);
    vec3 first;
    vec3 second;
    Node low(m_cameraNode.find(DGHelper::getSwerveID("dummy_size_1Transform")));
    Node high(m_cameraNode.find(DGHelper::getSwerveID("dummy_size_2Transform")));
    if (low != Object3D() && high != Object3D()) {
        low.getTransformTo(m_cameraNode,transform);
        DGHelper::TransformPoint(transform,first);
        high.getTransformTo(m_cameraNode,transform);
        DGHelper::TransformPoint(transform,second);
        m_bounds.m_min.m_x = second.m_x;
        if (first.m_x < second.m_x) m_bounds.m_min.m_x = first.m_x;
        m_bounds.m_min.m_y = second.m_y;
        if (first.m_y < second.m_y) m_bounds.m_min.m_y = first.m_y;
        m_bounds.m_max.m_x = second.m_x;
        if (first.m_x > second.m_x) m_bounds.m_max.m_x = first.m_x;
        m_bounds.m_max.m_y = second.m_y;
        if (first.m_y > second.m_y) m_bounds.m_max.m_y = first.m_y;
    } else {
        m_bounds.m_min.m_x = -100.0f; m_bounds.m_min.m_y = -100.0f;
        m_bounds.m_max.m_x = 100.0f; m_bounds.m_max.m_y = 100.0f;
    }
    }
    m_sceneReady = true;
}

void ApplyFog(Mesh mesh, Fog fog)
{
    // original 0xffe28 does not Release, and the return code of the first get(0) is not checked; keeps this special ownership boundary.
    swerve::IAppearanceInterface* appearance = NULL;
    mesh.getAppearance(0, &appearance);
    int index = 1;
    do {
        appearance->setFog(static_cast<swerve::IObject3DInterface*>(fog.m_interface));
    } while (mesh.getAppearance(index++, &appearance) == 0);
}
void CSwerveGame::disableSpritePicking(Group group)
{
    if (group.m_interface == NULL) return;
    for (int index = 0; index < group.getChildCount(); ++index) {
        swerve::Sprite3D<CSwerveAssertingErrorHandler> sprite(group.getChild(index));
        if (sprite.m_interface != NULL) sprite.setPickingEnable(false);
        else disableSpritePicking(Group(group.getChild(index)));
    }
}
void SetChildsFog(Group group, Fog fog)
{
    if (group.m_interface == NULL) return;
    for (int index = 0; index < group.getChildCount(); ++index) {
        Group child(group.getChild(index));
        if (child.m_interface != NULL) SetChildsFog(child, fog);
        else {
            Mesh mesh(group.getChild(index));
            if (mesh.m_interface != NULL) ApplyFog(mesh, fog);
        }
    }
}

void CSwerveGame::SetupCamera()
{
    Group root(m_cameraNode.find(SwerveHelper::Id("RootNode")));
    disableSpritePicking(root);
    swerve::Appearance<CSwerveAssertingErrorHandler> appearance(m_cameraNode.find(SwerveHelper::Id("fog")));
    if (appearance.m_interface != NULL) {
        Fog fog = appearance.getFog();
        if (root.m_interface != NULL && fog.m_interface != NULL) SetChildsFog(root,fog);
    }
    Camera camera(m_cameraNode.find(SwerveHelper::Id(XString::AnsiString(m_cameraName.m_data))));
    m_cameraNode.setActiveCamera(camera);
    GameApp()->m_helper->SetCameraSize(camera,GameApp()->m_screenWidth,GameApp()->m_screenHeight,(camera_size_update_type_t)0);
}
void CSwerveGame::LoadLocation()
{
    CMission* firstMission = GameApp()->m_missionManager->GetMissionByName(GameApp()->m_huntingInfo->m_assignedData.m_missionName);
    CLocation* location = GameApp()->m_locationManager->GetLocation(firstMission->m_locationName);
    if (location == NULL) return;
    m_cameraName = location->m_cameraName;
    if (App::m_platformType != 0) m_cameraNode = World(App::LoadObject3D(XString::AnsiString(location->m_bigModel.m_data)));
    else m_cameraNode = World(App::LoadObject3D(XString::AnsiString(location->m_smallModel.m_data)));
    if (m_helicopterScene) {
        m_locationAnimation = new CDH_Animation(m_cameraNode);
        m_locationAnimation->addController(XString("__DefaultAnimation__"));
        m_locationAnimation->setCurrentController(0,false);
    }
    CMission* mission = GameApp()->m_missionManager->GetMissionByName(GameApp()->m_huntingInfo->m_assignedData.m_missionName);
    CSceneSetupsManager::SSceneSetup* setup = GameApp()->m_sceneSetupsManager->GetSceneSetupByName(mission->m_sceneSetupName);
    if (!setup->m_helicopterHidden) m_helicopter = new CHelicopter(m_cameraNode,setup->m_helicopterModelId);
    for (int index = 0; index < location->m_parts.size(); ++index) {
        const CLocation::SOptionalPart& part = location->m_parts[index];
        if (setup->m_optionalParts.indexOf(part.m_name,0) == -1 && part.m_hidden) {
            Group hidden(m_cameraNode.find(DGHelper::getSwerveID(XString::AnsiString(part.m_group.m_data))));
            hidden.setRenderingEnable(false);
        }
    }
    FixWorldPosition();
    DefineCollisionGeometry();
    LoadSceneLighting();
    if (location->m_lightingNode.ByteSize() >= 4) {
        XString::AnsiString lightName(location->m_lightingNode.m_data);
        m_dynamicLight = swerve::Light<CSwerveAssertingErrorHandler>(m_cameraNode.find(SwerveHelper::Id(lightName)));
        if (GameApp()->m_helper->m_gamePlayManager.GetGameData()->GetWeapon(0) != NULL)
            GameApp()->m_helper->m_gamePlayManager.GetGameData()->GetWeapon(0)->setDynamicLight(m_dynamicLight);
        if (GameApp()->m_helper->m_gamePlayManager.GetGameData()->GetWeapon(1) != NULL)
            GameApp()->m_helper->m_gamePlayManager.GetGameData()->GetWeapon(1)->setDynamicLight(m_dynamicLight);
        m_handsLight = swerve::Light<CSwerveAssertingErrorHandler>(m_dynamicLight.duplicate());
        m_handsLightGroup = SwerveHelper::NewGroup();
        m_handsLightGroup.addChild(m_handsLight);
        swerve::Light<CSwerveAssertingErrorHandler> ambient = swerve::Light<CSwerveAssertingErrorHandler>::Create(CSwerve::GetInstance()->m_swerve);
        ambient.setColor(0x7f7f7f7f); ambient.setIntensity(1.0f); ambient.setMode(128); ambient.setScope(-1);
        m_handsLightGroup.addChild(ambient);
        m_handsWorld.addChild(m_handsLightGroup);
    }
    if (m_sceneLighting != NULL) m_sceneLighting->m_ambient = location->m_baseLuminance;
    if (App::m_platformType == 0) SwitchOffEyeCandyGeometry();
    m_ambientSound = CDH_SoundManager::GetSoundFromResourceID(location->m_ambientResource);
    GameApp()->m_soundManager->PrecacheSound((CDH_SoundManager::eSound)m_ambientSound);
    Camera camera(m_cameraNode.find(DGHelper::getSwerveID(XString::AnsiString(m_cameraName.m_data))));
    m_cameraNode.setActiveCamera(camera);
    GameApp()->m_helper->SetCameraSize(camera,GameApp()->m_helper->m_screenWidth,GameApp()->m_helper->m_screenHeight,(camera_size_update_type_t)0);
    m_camera = new DGCamera(camera,location->m_cameraMaxAngleH,location->m_cameraMaxAngleV);
    m_targetAngleH = m_camera->m_field0c; m_targetAngleV = m_camera->m_field1c;
    XString lightResource = location->m_light;
    if (lightResource.Length() != 0) m_lightmap = swerve::Image2D<CSwerveAssertingErrorHandler>(App::LoadObject3D(XString::AnsiString(lightResource.m_data)));
    CCollision::CKinematicBody* body = new PlayerBody();
    m_kinematicBodies.addElement(body);
    if ((int)mission->m_type == 6) {
        body = new StadiumGoalGate();
        m_kinematicBodies.addElement(body);
    }
}

void CSwerveGame::LaunchGrenade(float spread)
{
    if (m_grenadeLauncher == NULL) return;
    vec3 direction = m_camera->GetViewDirection();
    direction = direction * MathLib::InvSqrt(direction.LengthSqr());
    CDH_Weapon* weapon = GameApp()->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon();
    vec3 emitter = weapon->GetTracerEmitterPos(0,m_referenceNode,m_cameraNode);
    vec3 velocity = direction * -110.0f;
    if (GameApp()->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon()->m_clipSize >= 3)
        velocity = direction * -90.0f;
    int id = GameApp()->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon()->getId();
    vec3 aimDirection;
    float radius = 5.0f;
    bool alternate = false;
    if (id == 35 || id == 36) {
        velocity = direction * -5.0f;
        if (id == 35) { radius = 4.5f; aimDirection = direction * -200.0f; }
        else {
            radius = 3.5f;
            float speed = -150.0f;
            if (m_hud->GetFlagBc()) speed = -75.0f;
            aimDirection = direction * speed;
            alternate = m_hud->GetFlagBc();
        }
    }
    velocity = velocity + GetCurrentCameraVel();
    m_grenadeLauncher->SetExplosionRadius(radius);
    m_grenadeLauncher->AddProjectile(emitter,velocity,spread,aimDirection,alternate);
    if (id == 35 || id == 36) {
        m_grenadeLauncher->SetDrag(0.0f);
        m_grenadeLauncher->SetGravity(vec3(0,0,0));
        m_grenadeLauncher->SetTrail(0.33f,0.66f,22.0f);
    } else {
        m_grenadeLauncher->SetDrag(0.9f);
        m_grenadeLauncher->SetGravity(vec3(0,0,-9.8f));
        m_grenadeLauncher->SetTrail(0.5f,0.5f,15.0f);
    }
}
void CSwerveGame::LaunchHandGrenade(float delay)
{
    if (m_handGrenades == NULL) return;
    vec3 direction = m_camera->GetViewDirection();
    direction = direction * MathLib::InvSqrt(direction.LengthSqr());
    vec3 up = GetCurrentCameraUp();
    vec3 right = GetCurrentCameraRight();
    vec3 position = m_camera->GetViewPosition();
    vec3 velocity = direction * -25.0f + up * 5.5f;
    position = position + up * -0.15f + right * -0.25f;
    velocity = velocity + GetCurrentCameraVel();
    HandGrenadeBallistics::FragGrenade* grenade = new HandGrenadeBallistics::FragGrenade();
    m_handGrenades->AddProjectile(position,velocity,delay,grenade);
    CBH_Player::GetInstance()->m_gameStats.NotifyThrow();
}
void CSwerveGame::LaunchUnitWeapon(vec3& position, Group group, int type)
{
    if (m_handGrenades == NULL) return;
    const vec3& camera = GetCurrentCameraPos();
    vec3 destination = camera + vec3(0,0,0.2f);
    vec3 velocity = destination - position - m_handGrenades->m_gravity * 0.5f;
    HandGrenadeBallistics::StoneGrenade* grenade = new HandGrenadeBallistics::StoneGrenade(group,type);
    m_handGrenades->AddProjectile(position,velocity,0.0f,grenade);
}
void CSwerveGame::BlastGrenade(const vec3& position,const vec3& normal,float radius,const CDH_Weapon* weapon,const CGrenade* grenade)
{
    if (weapon != NULL) Blast(position,normal,radius,weapon->m_damageMin,weapon->m_damageMax,weapon,grenade,NULL);
    else Blast(position,normal,radius,0,1000,NULL,grenade,NULL);
    GameApp()->m_soundManager->Play((CDH_SoundManager::eSound)0,false,false);
}
void CSwerveGame::Blast(const vec3& position,const vec3& normal,float radius,int damage,int power,
                      const CDH_Weapon* weapon,const CGrenade* grenade,const CUnit* source)
{
    m_world->blast(position,normal);
    if (radius > 5.0f) {
        vec3 offset = GetCurrentCameraRight() * 1.25f;
        m_world->blast(position-offset,normal);
        m_world->blast(position+offset,normal);
    }
    CBH_Player::GetInstance()->m_gameStats.NotifyShotBegin(weapon,grenade,1);
    if (weapon != NULL) m_unitsController->Blast(position,radius,weapon,NULL,source);
    else m_unitsController->Blast(position,radius,damage,power,NULL,grenade,source);
    CBH_Player::GetInstance()->m_gameStats.NotifyShotEnd();
    vec3 camera = m_camera->GetViewPosition();
    vec3 delta = camera-position;
    if (!m_helicopterScene) {
        XString missionName = GameApp()->m_huntingInfo->m_assignedData.m_missionName;
        XString location = GameApp()->m_missionManager->GetMissionByName(missionName)->m_locationName;
        bool stadium = location == L"Stadium_Cam2";
        float scale = (delta.LengthSqr()-1.0f)/1599.0f;
        if (scale < 0.0f) scale = 0.0f;
        else if (scale > 1.0f) scale = 1.0f;
        float strength = (1.0f-scale)+(1.0f-scale)+scale*4.0f;
        float nearAmplitude = 0.0625f;
        float farAmplitude = 0.25f;
        if (stadium) { nearAmplitude = 0.015625f; farAmplitude = 0.03125f; }
        m_camera->SetShake(0.8f,strength,farAmplitude*(1.0f-scale)+nearAmplitude*scale);
    }
}

bool CSwerveGame::IsMissionCompleted() { return m_zombieMission->m_field54; }

void CSwerveGame::SimulateAimingDispersion(float elapsed)
{
    float fov = m_camera->GetFOV();
    if (fov > 0.0f) {
        float scale = (float)m_width / fov;
        float increase = GameApp()->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon()->m_aimDispersionPerTurnAngle;
        CDH_Weapon* weapon = GameApp()->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon();
        float value = m_zoom / scale + increase * elapsed;
        if (value > weapon->m_maxAimDispersion) value = weapon->m_maxAimDispersion;
        m_zoom = value * scale;
    }
}

void CHelicopter::Update(int elapsedMs)
{
    if (m_original.m_interface == NULL) return;
    Transform transform = SwerveHelper::NewTransform();
    m_original.getTransformTo(m_world, transform);
    m_model.setTransform(transform);
    if (m_actions.Size() != 0) {
        SAction& action = m_actions[0];
        action.m_elapsed += elapsedMs;
        if (action.m_sound != 0xa0 && GameApp()->m_soundManager->IsPlaying((CDH_SoundManager::eSound)action.m_sound) == 0)
            GameApp()->m_soundManager->Play((CDH_SoundManager::eSound)action.m_sound, false, true);
        int bodyDuration = m_bodyAnimation->getControllerBaseDuration(action.m_bodyController);
        int positionDuration = m_positionAnimation->getControllerBaseDuration(action.m_positionController);
        // Duration on the left: matches the original cmp operand order.
        if ((!action.m_bodyLoop && bodyDuration <= action.m_elapsed) ||
            (!action.m_positionLoop && positionDuration <= action.m_elapsed)) {
            if (action.m_sound != 0xa0) GameApp()->m_soundManager->Stop((CDH_SoundManager::eSound)action.m_sound);
            if (action.m_event.GetLength() != 0)
                GameApp()->m_helper->m_swerveGame->m_scriptController->OnSceneEvent(action.m_event);
            m_actions.RemoveRange(0, 1);
            if (m_actions.Size() != 0) {
                m_bodyAnimation->setCurrentController(m_actions[0].m_bodyController, false);
                m_positionAnimation->setCurrentController(m_actions[0].m_positionController, false);
            }
        }
    }
}

void CSwerveGame::AmbientSoundsUpdate(int elapsedMs)
{
    if (GameApp()->m_soundManager->IsPlaying((CDH_SoundManager::eSound)m_ambientSound) == 0)
        GameApp()->m_soundManager->Play((CDH_SoundManager::eSound)m_ambientSound, false, false);
}

void CSwerveGame::UpdateGameFinishing(int elapsedMs)
{
    if ((int)m_finishResult != 4) {
        m_finishDelay -= elapsedMs;
        if (m_finishDelay > 0) return;
        FinishGameWithResult(m_finishResult, false);
        m_finishResult = (EBountyResult)4;
    }
}
void CSwerveGame::UpdateFovBlur(int elapsedMs)
{
    if (!m_fovBlur) return;
    float blurDuration = (float)m_blurDuration;
    if (!(m_blurTime >= blurDuration)) m_blurTime += (float)elapsedMs;
    float fovDuration = (float)m_fovDuration;
    if (!(m_fovTime >= fovDuration)) m_fovTime += (float)elapsedMs;
    if (m_blurTime >= blurDuration && m_fovTime >= fovDuration) {
        m_blurAmount = 0.0f;
        m_camera->SetFOV(m_savedFov);
        m_fovBlur = false;
        return;
    }
    if (m_blurFrames < 3) { m_blurAmount = 0.01f; ++m_blurFrames; }
    else m_blurAmount = EasyLib::Run((EasyLib::EDirType)1,m_blurMethod,m_blurTime,m_blurBegin,m_blurEnd-m_blurBegin,blurDuration,1.70158f);
    DGCamera* camera = m_camera;
    float base = m_savedFov;
    float scale = EasyLib::Run((EasyLib::EDirType)1,m_fovMethod,m_fovTime,m_fovBegin,m_fovEnd-m_fovBegin,(float)m_fovDuration,1.70158f);
    camera->SetFOV(base*scale);
    m_fovBlur = true;
}

void CSwerveGame::UpdateEnvironment(int elapsedMs, bool updateWorld)
{
    // original fae84 compares elapsedMs; the third argument bool is not read.
    if (elapsedMs > 0 && !m_paused) GameApp()->m_achievementManager->Update(true);
    m_world->update();
    m_camera->Update(elapsedMs);
    if (m_sceneLighting != NULL) m_sceneLighting->Update();
}

void CSwerveGame::CheckUnitsPreloadNeed()
{
    if (m_field470) {
        if ((int)m_finishResult != 4) return;
        if (m_zombieMission->Preload(5, true) > 5) {
            GameApp()->m_visualCharacterTypeManager->CleanupResources();
            m_zombieMission->Preload(-1, false);
        }
        m_field470 = false;
    }
}

void CSwerveGame::CheckDialogConditions()
{
    // The original only spills the condition value right before addElement.
    if (m_preloadStates.indexOf(1, 0) != -1) return;
    if (m_environmentTime > 3000) {
        int condition = 1;
        m_preloadStates.addElement(condition);
        ShowDialogs(CQuestManager::SDialog::CONDITION_START);
    }
}
void CSwerveGame::HuntingUpdate(int elapsedMs)
{
    UpdateFovBlur(elapsedMs);
    if (m_field46e && !m_nextWaveDialog) { OnNewWaveStart(m_zombieMission->m_currentWaveNumber); m_field46e = false; }
    int simulationMs = elapsedMs;
    if (GameApp()->m_powerUpManager->IsPowerUpRunning() == 1) {
        CPowerUp* power = GameApp()->m_powerUpManager->GetActivePowerUp();
        if (power->m_reloadSoundSpeed > 0.0f) simulationMs = (int)((float)elapsedMs*power->m_reloadSoundSpeed);
    }
    if (m_locationAnimation != NULL) {
        float speed = 1.0f;
        if (GameApp()->m_powerUpManager->IsPowerUpRunning() == 1) speed = GameApp()->m_powerUpManager->GetActivePowerUp()->m_reloadSoundSpeed;
        m_locationAnimation->setControllerSpeed(0,speed);
        m_locationAnimation->animate();
    }
    m_scriptController->Update(elapsedMs);
    if (m_helicopterScene && GameApp()->m_soundManager->IsPlaying((CDH_SoundManager::eSound)20) == 0)
        GameApp()->m_soundManager->Play((CDH_SoundManager::eSound)20,false,true);
    if (m_hitPoints != NULL) { m_hitPoints->Simulate((float)simulationMs*0.001f); m_hitPoints->PostSimulate(); }
    if (m_survivalHitPoints != NULL) { m_survivalHitPoints->Simulate((float)simulationMs*0.001f); m_survivalHitPoints->PostSimulate(); }
    if (m_debris != NULL) m_debris->Simulate((float)simulationMs*0.001f);
    if (m_tracers != NULL) m_tracers->Simulate((float)simulationMs*0.001f);
    if (m_particles != NULL) m_particles->Simulate((float)simulationMs*0.001f);
    if (m_smoke != NULL) m_smoke->Simulate((float)simulationMs*0.001f);
    if (m_flashes != NULL) m_flashes->Simulate((float)simulationMs*0.001f);
    if (m_ricochets != NULL) m_ricochets->Simulate((float)simulationMs*0.001f);
    if (m_grenadeLauncher != NULL) {
        m_grenadeLauncher->Simulate((float)simulationMs*0.001f);
        if (m_handGrenades->m_activeCount != 0) m_grenadeLauncher->IntersectProjectiles(m_handGrenades);
    }
    if (m_unitBodyBallistics != NULL) m_unitBodyBallistics->Simulate((float)simulationMs*0.001f);
    if (m_swerveBallistics != NULL) { m_swerveBallistics->Simulate((float)simulationMs*0.001f); m_swerveBallistics->PostSimulate(); }
    if (m_handGrenades != NULL) {
        m_handGrenades->Simulate((float)simulationMs*0.001f);
        m_handGrenades->PostSimulate();
        m_pickupGrenade = -1;
        if (GameApp()->m_grenadeManager->GetActiveGrenade() != NULL && GameApp()->m_grenadeManager->GetActiveGrenade()->m_isBeacon) {
            for (int index = 0; index < m_handGrenades->m_capacity; ++index) {
                Ballistics::Projectile* projectile = m_handGrenades->m_projectiles+index;
                if (projectile->m_active && ((HandGrenadeBallistics::HandGrenade*)projectile->m_userData)->CanBeBeacon() == true) {
                    m_pickupGrenade = index; break;
                }
            }
        }
    }
    if (m_camera != NULL) SimulateAimingDispersion();
    if (GameApp()->m_settings->IsFireButtonEnabled() == true && m_hud->m_firePressed) {
        if (GameApp()->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon()->m_shootType == 2 &&
            GameApp()->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon()->IsReadyForUse() == 1) m_cursor->Fire();
    }
    if (GameApp()->m_settings->IsFireButtonEnabled() == true && m_cursor != NULL &&
        GameApp()->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon()->IsReadyForAutoShot() == true) {
        int x = m_cursor->GetAimX();
        int y = m_cursor->GetAimY();
        Shot(x,y);
    }
    bool switchScope = false;
    if (GameApp()->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon()->IsReloading() == 1 && !m_field464) {
        m_field464 = true; switchScope = true;
    } else {
        CDH_Weapon* weapon = GameApp()->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon();
        if (weapon->m_action == 8 && !m_field465) { m_field465 = true; switchScope = true; }
        else if (GameApp()->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon()->IsReloading() == 0 && m_field464) {
            if (!m_hud->GetFlagBc() && m_field466) m_field466 = false;
            m_field464 = false;
        }
    }
    if (switchScope && m_hud->GetFlagBc()) { m_hud->switchScope(); m_field466 = true; }
    if (GameApp()->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon()->m_action != 8 && m_field465) {
        if (!m_hud->GetFlagBc() && m_field466) m_field466 = false;
        m_field465 = false;
    }
    if (m_field5cc && GameApp()->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon()->IsReadyForUse() == 1) {
        m_hud->switchScope(); m_field5cc = false;
    }
    m_environmentTime += elapsedMs;
    bool shotEffect = m_camera->IsShotEffectInProgress();
    UpdateUnits(simulationMs);
    UpdateEnvironment(simulationMs,false);
    if (!shotEffect) {
        GameApp()->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon()->update(elapsedMs);
        if (m_cursor != NULL && m_currentState == 2) m_cursor->update(elapsedMs);
    }
}

IGameAIMap* CSwerveGame::GetGameAIMap()
{
    if (m_navMeshEnabled) return &m_navMesh;
    return &m_gameAIMap;
}

const vec3& CSwerveGame::GetCurrentCameraPos()
{
    if (m_cameraDirty) UpdateCameraVectors();
    return m_cameraPosition;
}
const vec3& CSwerveGame::GetCurrentCameraRight()
{
    if (m_cameraDirty) UpdateCameraVectors();
    return m_cameraRight;
}
const vec3& CSwerveGame::GetCurrentCameraUp()
{
    if (m_cameraDirty) UpdateCameraVectors();
    return m_cameraUp;
}
bool CSwerveGame::DefineShotProbability(const CDH_Weapon* weapon, const vec3& position)
{
    if (weapon == NULL) return false;
    GameApp()->m_helper->m_swerveGame->GetCurrentCameraPos();
    return true;
}
void CSwerveGame::AddSkillshot(Window* window)
{
    if (m_hitPoints != NULL) m_hitPoints->AddWindow(window);
}
bool CSwerveGame::AreAnimatedHitPointsSupported() const
{
    return m_hitPoints != NULL && m_survivalHitPoints == NULL;
}
void CSwerveGame::AddFlash(const vec3& position, const vec3& velocity, float spread)
{
    if (m_flashes != NULL && MathLib::Sqrt(velocity.LengthSqr()) > 0.0f) m_flashes->AddProjectile(position, velocity, spread, 0);
}
void CSwerveGame::AddRicochet(const vec3& position, const vec3& velocity, float spread, int type)
{
    if (m_ricochets != NULL && MathLib::Sqrt(velocity.LengthSqr()) > 0.0f) m_ricochets->AddProjectile(position, velocity, spread, type);
}
void CSwerveGame::AddTracer(const vec3& position, const vec3& velocity, float spread)
{
    if (m_tracers != NULL && MathLib::Sqrt(velocity.LengthSqr()) > 0.0f) m_tracers->AddProjectile(position, velocity, spread, 0);
}
void CSwerveGame::setCursor(DGCursor* cursor)
{
    DGCursor* previous = NULL;
    if (m_cursor != NULL) {
        m_cursor->DeInit();
        previous = m_cursor;
    }
    if (previous == cursor) return;
    m_cursor = cursor;
    cursor->Init();
}
void CSwerveGame::LoadWeapon()
{
    DGHelper* helper = GameApp()->m_helper;
    CDH_Weapon* weapon = helper->m_gamePlayManager.GetGameData()->GetWeapon(0);
    if (weapon != NULL) weapon->loadWeaponModel();
    weapon = helper->m_gamePlayManager.GetGameData()->GetWeapon(1);
    if (weapon != NULL) weapon->loadWeaponModel();
}
void CSwerveGame::LoadSkyBox() { m_skyBox.Init(m_cameraNode); }
void CSwerveGame::LoadAIMap()
{
    if (m_navMesh.Init(m_cameraNode) == true) { m_navMeshEnabled = true; return; }
    m_gameAIMap.Init(m_cameraNode);
}
float CSwerveGame::GetDistanceToUnit(CUnit* unit)
{
    if (unit == NULL) return 0.0f;
    vec3 delta = GetCurrentCameraPos() - unit->GetPosition();
    return DGHelper::sqrt(delta.m_x * delta.m_x + delta.m_y * delta.m_y + delta.m_z * delta.m_z);
}
void CSwerveGame::ShowUnitInfo(CUnit* unit, int amount)
{
    if (m_unitInfo != NULL && m_unitInfo->GetStateValue() != 3) {
        if (m_unitInfo->GetUnit() == unit) { m_unitInfo->FlushBeforeCloseTimer(); return; }
        m_unitInfo->Close();
    }
    GetDistanceToUnit(unit);
}
void CSwerveGame::CreateCameraAnimation() {} // original 0xffd78 bx lr, no body side effects.
bool CSwerveGame::IsGamePaused() const { return m_currentState != 2; }
void CSwerveGame::InitScope()
{
    GameApp()->m_helper->m_gamePlayManager.GetGameData()->GetWeapon(0);
    GameApp()->m_helper->m_gamePlayManager.GetGameData()->GetWeapon(1);
}
void CSwerveGame::InitHunting() { CreateScene(false); }
void CSwerveGame::FillHuntLoadingTasks(CLoadingScreen* screen)
{
    class StepFn : public CLoadingScreen::IStep {
    public:
        typedef void (CSwerveGame::*Function)();
        StepFn(Function function,CSwerveGame* owner) : m_function(function),m_owner(owner) {}
        virtual ~StepFn() {}
        virtual bool Load(CLoadingScreen* screen) { (m_owner->*m_function)(); return true; }
        Function m_function;
        CSwerveGame* m_owner;
    };
    screen->AddLoadingStep(new StepFn(&CSwerveGame::LoadWeapon,this),"LoadWeapon");
    screen->AddLoadingStep(new StepFn(&CSwerveGame::LoadLocation,this),"LoadLocation");
    screen->AddLoadingStep(new StepFn(&CSwerveGame::LoadSkyBox,this),"LoadSkyBox");
    screen->AddLoadingStep(new StepFn(&CSwerveGame::LoadAIMap,this),"LoadAIMap");
    screen->AddLoadingStep(new StepFn(&CSwerveGame::CreateCameraAnimation,this),"CreateCameraAnimation");
    screen->AddLoadingStep(new StepFn(&CSwerveGame::InitUnits,this),"InitUnits");
    screen->AddLoadingStep(new StepFn(&CSwerveGame::InitScope,this),"InitScope");
    screen->AddLoadingStep(new StepFn(&CSwerveGame::InitHunting,this),"InitHunting");
    if (GameApp()->m_tutorialManager != NULL && !GameApp()->m_tutorialManager->FirstMissionCompleted()) screen->SetRange(50,100,false);
    else screen->SetRange(20,100,true);
}
void CSwerveGame::OnCreateState(Window** result,int state)
{
    m_interruptCountdown = -1;
    Window* window = NULL;
    if (state == 1) {
        m_ownedCursor = new CFPSCursor();
        setCursor(m_ownedCursor);
        m_scriptController->OnStart();
        m_unitsController->SetCiviliansMeanRelativeHP(*(float*)((char*)GameApp()->m_huntingInfo+0x74));
        if (m_scriptController->m_showWeapon) GameApp()->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon()->fetchWeaponModel();
        m_interruptCountdown = 1000;
    } else if (state == 2) m_hud->Show(true,0.0f);
    else if (state == 0) {
        CLoadingScreen* loading = new CLoadingScreen(true);
        FillHuntLoadingTasks(loading);
        window = loading;
    }
    if (window == NULL) window = new Window(false);
    *result = window;
}
void CSwerveGame::OnStateInterrupt(int state,Window* window)
{
    switch (state) {
    case 0: ChangeState(1,(Window::eStateTransitionType)5); break;
    case 1: ChangeState(2,(Window::eStateTransitionType)5); break;
    case 2: m_hud->ClearFlags(1); break;
    case 3: break;
    case 4:
        if (window->GetInterrupt() == 2) PopState();
        Activate();
        break;
    }
}
void CSwerveGame::onWeaponSwitch() { if (m_cursor != NULL) m_cursor->onWeaponSwitch(); }
void CSwerveGame::OnNextWaveZombieDialogWindowShutdown()
{
    m_nextWaveDialog = false;
    if (m_restoreScope) m_hud->switchScope();
}
void CSwerveGame::OnShooterAttacked()
{
    RunFovBlur(1.0f, 3000, true);
    m_camera->SetShake(1.0f, 4.0f, 0.25f);
}
void CSwerveGame::RunFovBlur(float multiplier, int durationMs, bool reset)
{
    m_field51c = 0.0f;
    m_blurTime = 0.0f;
    m_blurBegin = 0.98f;
    m_blurEnd = 0.8f;
    m_blurMethod = (EasyLib::EMethod)22;
    m_blurDuration = durationMs;
    m_fovTime = 0.0f;
    m_fovBegin = multiplier;
    m_fovEnd = 1.0f;
    m_fovMethod = (EasyLib::EMethod)27;
    m_fovDuration = (int)((float)durationMs * 0.7f);
    if (!m_fovBlur) m_savedFov = m_camera->GetFOV();
    m_blurFrames = 0;
    m_resetFovBlur = reset;
    m_fovBlur = true;
}
void CSwerveGame::OnPause()
{
    if (m_currentState != 0) WindowApp::HandleEvent(Event::COMMAND, (int)0x9247fd7b, 0, 0, 0);
}

CHelicopter::CHelicopter(World world, XString resource)
    : m_world(world), m_bodyAnimation(NULL), m_positionAnimation(NULL)
{
    m_original = Group(m_world.find(SwerveHelper::Id("helicopterTransform")));
    if (m_original.m_interface != NULL) {
        Group box(m_original.find(SwerveHelper::Id("Box01Transform")));
        if (box.m_interface != NULL) box.setRenderingEnable(false);
        Group loaded;
        // Original 0x100c58 tests stored byte size directly, before conversion.
        if (resource.ByteSize() != 0) loaded = Group(App::LoadObject3D(XString::AnsiString(resource.m_data)));
        else loaded = Group(App::LoadObject3D("BIN_M3G_OBJECT_HELICOPTER"));
        Group original = loaded;
        m_model = SwerveHelper::FullObjectDuplicate(original, 0);
        m_world.addChild(m_model);
        m_bodyAnimation = new CDH_Animation(m_model);
        m_bodyAnimation->addController(XString("heli_start"));
        m_bodyAnimation->addController(XString("heli_idle"));
        m_bodyAnimation->addController(XString("heli_speedup"));
        m_bodyAnimation->addController(XString("heli_fly"));
        m_bodyAnimation->addController(XString("heli_slowdown"));
        m_positionAnimation = new CDH_Animation(m_original);
        m_positionAnimation->addController(XString("heli_land"));
        m_positionAnimation->addController(XString("heli_idle"));
        m_positionAnimation->addController(XString("heli_takeoff"));
        if (CDH_Animation::HaveController(m_original, "special")) m_positionAnimation->addController(XString("special"));
        m_bodyAnimation->setCurrentController(0, false);
        m_positionAnimation->setCurrentController(1, false);
    }
    m_landed = false;
    m_flying = true;
    m_engineOn = true;
}
CHelicopter::~CHelicopter()
{
    if (m_bodyAnimation != NULL) { delete m_bodyAnimation; m_bodyAnimation = NULL; }
    if (m_positionAnimation != NULL) { delete m_positionAnimation; m_positionAnimation = NULL; }
}
bool CHelicopter::IsLanded()
{
    if (!m_engineOn) return true;
    // Written ==0 first: clang reverses the chain, giving the original's cmp #1 first.
    if (m_actions.Size() == 0) return m_landed;
    if (m_actions.Size() == 1) return m_actions[0].m_bodyController == 1 && m_actions[0].m_positionController == 1;
    return false;
}
bool CHelicopter::IsFlying() { return m_actions.Size() == 0 && m_flying; }
void CHelicopter::Land()
{
    if (m_original.m_interface == NULL) return;
    m_landed = true;
    m_flying = false;
    SAction first = {0x14,"HelicopterLanded",true,3,false,0,0};
    SAction second = {0x14,"",false,4,true,1,0};
    SAction third = {0x14,"",false,1,true,1,0};
    m_actions.Clear();
    m_actions.Add(first); m_actions.Add(second); m_actions.Add(third);
    m_bodyAnimation->setCurrentController(m_actions[0].m_bodyController, false);
    m_positionAnimation->setCurrentController(m_actions[0].m_positionController, false);
}
void CHelicopter::ForceLand()
{
    if (m_original.m_interface == NULL) return;
    m_landed = true; m_flying = false; m_engineOn = false;
    SAction action = {0xa0,"",true,0,true,1,0};
    m_actions.Clear(); m_actions.Add(action);
    m_bodyAnimation->setCurrentController(m_actions[0].m_bodyController, false);
    m_positionAnimation->setCurrentController(m_actions[0].m_positionController, false);
    m_bodyAnimation->animate(); m_positionAnimation->animate();
    m_bodyAnimation->stopAllAnimation(); m_positionAnimation->stopAllAnimation();
}
void CHelicopter::Takeoff()
{
    if (m_original.m_interface == NULL) return;
    m_landed = false; m_flying = true;
    SAction first = {0xa0,"",false,0,false,1,0};
    SAction second = {0x14,"",false,2,true,1,0};
    SAction third = {0x14,"",false,3,true,1,0};
    SAction fourth = {0x14,"HelicopterTookOff",true,3,false,2,0};
    m_actions.Clear();
    if (!m_engineOn) m_actions.Add(first);
    m_actions.Add(second); m_actions.Add(third); m_actions.Add(fourth);
    m_engineOn = true;
    m_bodyAnimation->setCurrentController(m_actions[0].m_bodyController, false);
    m_positionAnimation->setCurrentController(m_actions[0].m_positionController, false);
}
void CHelicopter::EngineOn()
{
    if (m_original.m_interface == NULL) return;
    if (IsLanded() && m_engineOn) return;
    SAction first = {0xa0,"",false,0,false,1,0};
    SAction second = {0x14,"",true,1,true,1,0};
    m_actions.Clear(); m_actions.Add(first); m_actions.Add(second);
    m_engineOn = true;
    m_bodyAnimation->setCurrentController(m_actions[0].m_bodyController, false);
    m_positionAnimation->setCurrentController(m_actions[0].m_positionController, false);
}











void CSwerveGame::Update()
{
    Window::Update();
    int elapsedMs = GameApp()->m_updateFrameTime;
    if (m_pendingFire) {
        if (!m_tapToShoot || GameApp()->m_settings->IsFireButtonEnabled() == 1) {
            m_pendingFire = false;
            GameApp()->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon()->ResetAutoShot();
        } else {
            m_cursor->Fire();
            if (GameApp()->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon()->m_shootType != 2) m_pendingFire = false;
        }
    }
    float absoluteX = MathLib::Fabs(m_inputX);
    float absoluteY = MathLib::Fabs(m_inputY);
    float amountX = absoluteX;
    float amountY = absoluteY;
    if (amountX > 1.0f) amountX = 1.0f;
    if (amountY > 1.0f) amountY = 1.0f;
    float signX = 0.0f;
    float signY = 0.0f;
    if (m_inputX < 0.0f) signX = -1.0f;
    if (m_inputX > 0.0f) signX = 1.0f;
    if (m_inputY < 0.0f) signY = -1.0f;
    if (m_inputY > 0.0f) signY = 1.0f;
    float stepX = ((((1.0f-amountX)*2.5f+amountX*15.0f)*signX)*(float)elapsedMs)*-0.001f;
    float stepY = ((((1.0f-amountY)*2.5f+amountY*15.0f)*signY)*(float)elapsedMs)*-0.001f;
    if (absoluteX >= MathLib::Fabs(stepX)) m_inputX += stepX;
    else m_inputX = 0.0f;
    if (absoluteY >= MathLib::Fabs(stepY)) m_inputY += stepY;
    else m_inputY = 0.0f;
    if (m_inputX < -15.0f) m_inputX = -15.0f;
    else if (m_inputX > 15.0f) m_inputX = 15.0f;
    if (m_inputY < -1.1f) m_inputY = -1.1f;
    else if (m_inputY > 1.1f) m_inputY = 1.1f;
    if (m_state == 1) {
        m_startCountdown -= GameApp()->m_frameTimeSeconds;
        if (m_startCountdown <= 0.0f) m_state = 2;
    }
    if (m_camera != NULL && GameApp()->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon() != NULL) {
        float elapsed = GameApp()->m_frameTimeSeconds;
        if (elapsed > 0.0f) {
            float deltaH = m_targetAngleH-m_camera->m_field0c;
            float deltaV = m_targetAngleV-m_camera->m_field1c;
            float amountH = MathLib::Fabs(deltaH)/15.0f;
            float amountV = MathLib::Fabs(deltaV)/15.0f;
            if (amountH > 1.0f) amountH = 1.0f;
            if (amountV > 1.0f) amountV = 1.0f;
            float powerH = MathLib::Pow(amountH,0.875f);
            float powerV = MathLib::Pow(amountV,0.875f);
            float directionH = 0.0f;
            float directionV = 0.0f;
            if (deltaH < 0.0f) directionH = -1.0f;
            if (deltaH > 0.0f) directionH = 1.0f;
            if (deltaV < 0.0f) directionV = -1.0f;
            if (deltaV > 0.0f) directionV = 1.0f;
            m_velocityH = directionH*((1.0f-powerH)*0.0f+powerH*90.0f);
            m_velocityV = (directionV*((float)m_width/(float)m_height))*((1.0f-powerV)*0.0f+powerV*90.0f);
            float blend = m_responseTime*2.0f;
            if (blend < 0.0f) blend = 0.0f;
            else if (blend > 1.0f) blend = 1.0f;
            m_velocityH = m_velocityH*(1.0f-blend)+(deltaH/elapsed)*blend;
            m_velocityV = m_velocityV*(1.0f-blend)+(deltaV/elapsed)*blend;
            float moveH = m_velocityH*elapsed;
            float moveV = m_velocityV*elapsed;
            float distanceSquared = moveH*moveH+moveV*moveV;
            SimulateAimingDispersion(MathLib::InvSqrt(distanceSquared)*distanceSquared);
            float newH = m_velocityH*elapsed;
            if (MathLib::Fabs(newH) > MathLib::Fabs(moveH)) newH = moveH;
            float expectedH = m_camera->m_field0c+newH;
            float currentV = m_camera->m_field1c;
            float velocityV = m_velocityV;
            m_camera->SetAngleH(expectedH,false);
            float newV = velocityV*elapsed;
            if (MathLib::Fabs(newV) > MathLib::Fabs(moveV)) newV = moveV;
            float expectedV = currentV+newV;
            m_camera->SetAngleV(expectedV,false);
            if (m_camera->m_field0c != expectedH) m_targetAngleH = m_camera->m_field0c;
            if (m_camera->m_field1c != expectedV) m_targetAngleV = m_camera->m_field1c;
            m_responseTime += elapsed;
            if (deltaH == 0.0f && deltaV == 0.0f) {
                ++m_zeroInputFrames;
                if (m_zeroInputFrames >= 4) m_responseTime = 0.0f;
            } else m_zeroInputFrames = 0;
        }
    }
    m_cameraDirty = true;
    if (m_kinematicBodies.size() != 0) ((PlayerBody*)m_kinematicBodies[0])->UpdatePlayerPos();
    if (m_interruptCountdown >= 0) {
        m_interruptCountdown -= elapsedMs;
        if (m_interruptCountdown <= -1) fireInterrupt();
    }
    if (!m_sceneReady) return;
    if (m_helicopter != NULL) m_helicopter->Update(GameApp()->m_updateFrameTime);
    AmbientSoundsUpdate(elapsedMs);
    if (m_pendingInterrupt) { SetInterrupt(1); return; }
    UpdateGameFinishing(elapsedMs);
    if (m_pauseWindow->HasModalChild() == 0 && elapsedMs != 0) {
        m_hud->SetFlags(0x2000);
        m_paused = false;
        m_tapToShoot = m_scriptController->m_showHUD;
    } else {
        m_hud->ClearFlags(0x2000);
        m_paused = true;
        m_tapToShoot = false;
    }
    if (m_finishing) m_tapToShoot = false;
    float fade = 0.0f;
    if (m_fovBlur && m_resetFovBlur) { fade = 0.3f; m_tapToShoot = false; }
    m_hud->Show(m_tapToShoot,fade);
    if (m_ownedCursor != NULL) m_ownedCursor->ShowSight(m_tapToShoot);
    if (m_paused) { UpdateEnvironment(elapsedMs,false); CheckUnitsPreloadNeed(); return; }
    switch (m_currentState) {
    case 0:
    case 3: break;
    case 2:
        if (m_resultDialog != NULL) break;
        CheckDialogConditions();
        // fall through: the original shares the HuntingUpdate call with default
    default: HuntingUpdate(elapsedMs); break;
    case 4: UpdateEnvironment(elapsedMs,false); break;
    }
    if (m_pendingDialog != NULL && !m_hud->GetFlagBc()) {
        CDialogQueueWindow::AddDialogInQueue(m_pendingDialog);
        m_pendingDialog = NULL;
    }
    m_timeManager.update(elapsedMs);
}

void CSwerveGame::PaintLensFlares(Graphics2D& graphics)
{
    static const float coeff[6] = {-1.0f,1.0f,-0.2f,0.33f,0.75f,-0.5f};
    int x = 0;
    int y = 0;
    DGCamera* camera = m_camera;
    Transform transform = SwerveHelper::NewTransform();
    m_dynamicLight.getTransformTo(m_cameraNode,transform);
    int visible = camera->GetScreenPosition(&x,&y,SwerveHelper::GetTranslation(transform));
    if (!m_helicopterScene && visible == 1 && x >= 0 && y >= 0 && x < GameApp()->m_helper->m_screenWidth && y < GameApp()->m_helper->m_screenHeight) {
        int centerX = GameApp()->m_helper->m_screenWidth/2;
        int centerY = GameApp()->m_helper->m_screenHeight/2;
        for (int index = 0; index < 6; ++index) {
            float coefficient = coeff[index];
            if (m_effectPresenters[index].IsViable()) {
                m_effectPresenters[index].Draw(centerX+(int)((float)(centerX-x)*coefficient),centerY+(int)((float)(centerY-y)*coefficient));
            }
        }
    }
}
void CSwerveGame::Paint(Graphics2D& graphics)
{
    if (!m_sceneReady) return;
    // Switch on the state: original tests 1/4 first, then range 2..3, with the 2..3 body laid out first.
    switch (m_currentState) {
    case 2:
    case 3:
        m_paintStage = 0;
        RunPaint3D(graphics);
        if (m_blurAmount > 0.0f && m_field51c == 0) Graphics2D::GetInstance()->Draw(*m_blurSurfaces[m_blurTarget],NULL,NULL,Graphics2D::Flip_None);
        m_hud->HandleBottomLayerPaint(*Graphics2D::GetInstance());
        RunPaint3D(graphics);
        if (m_blurAmount > 0.0f && m_field51c == 1) Graphics2D::GetInstance()->Draw(*m_blurSurfaces[m_blurTarget],NULL,NULL,Graphics2D::Flip_None);
        if (m_cursor != NULL && (m_hud->GetFlag145() || m_hud->GetFlagBc())) {
            if (m_field69c && m_cursor == m_ownedCursor) { m_cursor->Hide(); m_hud->ClearFlags(1); }
            else if (m_hud->GetValue148() == 0) m_cursor->draw();
        }
        break;
    case 1:
    case 4:
        RunPaint3D(graphics);
        if (m_blurAmount > 0.0f && m_field51c == 0) Graphics2D::GetInstance()->Draw(*m_blurSurfaces[m_blurTarget],NULL,NULL,Graphics2D::Flip_None);
        RunPaint3D(graphics);
        if (m_blurAmount > 0.0f && m_field51c == 1) Graphics2D::GetInstance()->Draw(*m_blurSurfaces[m_blurTarget],NULL,NULL,Graphics2D::Flip_None);
        break;
    }
    PaintLensFlares(graphics);
}
void CSwerveGame::Paint3D(Graphics3D& graphics)
{
    if (m_paintStage == 0) { Paint3DBottom(graphics); ++m_paintStage; }
    else if (m_paintStage == 1) { Paint3DTop(graphics); m_paintStage = 0; }
}
void CSwerveGame::Paint3DBottom(Graphics3D& graphics)
{
    if (m_blurAmount > 0.0f && m_field51c == 0) {
        m_blurTarget = m_blurTarget == 0;
        graphics.m_graphics->ReleaseTarget();
        graphics.m_graphics->BindTarget(m_blurSurfaces[m_blurTarget],NULL,0x40);
    } else if (m_transition == 1) {
        graphics.m_graphics->ReleaseTarget();
        graphics.m_graphics->BindTarget(m_blurSurfaces[0],NULL,0x40);
    }
    m_skyBox.Update(); m_skyBox.Render(graphics);
    App::RenderWorld(graphics,m_cameraNode);
    m_unitsController->UpdateHudUnitsInfo();
    m_world->paintHitEffects(graphics);
    if (m_debris != NULL) m_debris->Paint3D(graphics);
    if (m_tracers != NULL) m_tracers->Paint3D(graphics);
    if (m_particles != NULL) m_particles->Paint3D(graphics);
    if (m_smoke != NULL) m_smoke->Paint3D(graphics);
    if (m_flashes != NULL) m_flashes->Paint3D(graphics);
    if (m_ricochets != NULL) m_ricochets->Paint3D(graphics);
    if (m_grenadeLauncher != NULL) { m_grenadeLauncher->Paint3D(graphics); m_unitBodyBallistics->Paint3D(graphics); }
    if (m_handGrenades != NULL) m_handGrenades->Paint3D(graphics);
    if (m_blurAmount > 0.0f && m_field51c == 0) {
        ICGraphics::GetInstance()->Sync();
        int previous = m_blurTarget == 0;
        float alpha = m_blurAmount;
        if (alpha < 0.0f) alpha = 0.0f;
        else if (alpha > 0.98f) alpha = 0.98f;
        com::glu::platform::components::Color_RGBA color = {1.0f,1.0f,1.0f,alpha};
        Graphics2D::GetInstance()->Draw(*m_blurSurfaces[previous],NULL,&color,Graphics2D::Flip_None);
    } else if (m_transition == 1) m_transition = 2;
    else if (m_transition == 2) {
        ICGraphics::GetInstance()->Sync();
        m_blendColor[3] = EasyLib::Run((EasyLib::EDirType)1,m_transitionMethod,m_transitionTime,m_transitionBegin,m_transitionEnd-m_transitionBegin,(float)m_transitionDuration,1.70158f);
        Graphics2D::GetInstance()->Draw(*m_blurSurfaces[0],NULL,(const com::glu::platform::components::Color_RGBA*)m_blendColor,Graphics2D::Flip_None);
        if (!(m_transitionTime >= (float)m_transitionDuration)) m_transitionTime += (float)GameApp()->m_updateFrameTime;
        if (m_transitionTime >= (float)m_transitionDuration) m_transition = 0;
    }
}
void CSwerveGame::Paint3DTop(Graphics3D& graphics)
{
    struct Helper {
        static void Convert(Transform transform,Mat4x4& matrix)
        {
            float values[16];
            transform.get(values);
            for (int row = 0; row < 4; ++row) {
                for (int column = 0; column < 4; ++column) matrix.ToFloatPtr()[row*4+column] = values[row*4+column];
            }
        }
    };
    if (m_blurAmount > 0.0f && m_field51c == 1) {
        m_blurTarget = m_blurTarget == 0;
        graphics.m_graphics->ReleaseTarget();
        graphics.m_graphics->BindTarget(m_blurSurfaces[m_blurTarget],NULL,0x40);
    }
    CDH_Weapon* weapon = GameApp()->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon();
    if (App::m_platformType != 0 && m_field6c0 && weapon != NULL && (m_hud->isWeaponModelVisible() || !weapon->m_scopeAttached) && !m_headShotEffect) {
        weapon->SetInertionAngle(m_inputX,m_inputY);
        float values[16];
        m_eyeMatrix.get(values); m_handsTransform.set(values);
        float yaw[4] = {m_inputX,0.0f,1.0f,0.0f};
        m_handsTransform.postRotate(yaw);
        float pitch[4] = {m_inputY,1.0f,0.0f,0.0f};
        m_handsTransform.postRotate(pitch);
        m_eyeTransform.setTransform(m_handsTransform);
        if (m_dynamicLight.m_interface != NULL && m_referenceNode.m_interface != NULL) {
            Group parent(m_dynamicLight.getParent());
            m_referenceNode.getTransformTo(m_cameraNode,m_referenceTransform);
            Mat4x4 cameraMatrix(1.0f);
            Helper::Convert(m_referenceTransform,cameraMatrix);
            parent.getTransformTo(m_cameraNode,m_lightTransform);
            Mat4x4 lightMatrix(1.0f);
            Helper::Convert(m_lightTransform,lightMatrix);
            Mat4x4 inverse = cameraMatrix;
            inverse.Invert();
            Mat4x4 relative = lightMatrix;
            relative.Multiply(inverse);
            Group eye(m_handsWorld.find(SwerveHelper::Id("eyecamTransform")));
            eye.getTransformTo(m_handsWorld,m_referenceTransform);
            Mat4x4 eyeMatrix(1.0f);
            Helper::Convert(m_referenceTransform,eyeMatrix);
            Mat4x4 result = relative;
            result.Multiply(eyeMatrix);
            {
                Transform output = m_lightTransform;
                float copied[16];
                for (int row = 0; row < 4; ++row) {
                    for (int column = 0; column < 4; ++column) copied[row*4+column] = result.ToFloatPtr()[row*4+column];
                }
                output.set(copied);
            }
            weapon->updateDynamicLight(m_lightTransform);
            m_handsLightGroup.setTransform(m_lightTransform);
        }
        if (m_scriptController->m_showWeapon) weapon->paintWeaponModel(graphics);
        if (weapon->m_handsSeparated) {
            if (m_weaponId != weapon->getId()) {
                m_syncSource.removeAllElements(); m_syncTarget.removeAllElements();
                m_weaponId = weapon->getId();
                Group controller(m_handsWorld.find(SwerveHelper::Id("weaponControllerTransform")));
                SynchronizeGroup(controller);
            }
            int count = m_syncSource.size();
            for (int index = 0; index < count; ++index) {
                m_syncTarget[index].getCompositeTransform(m_handsTransform);
                float translation[3]; float orientation[4]; float scale[3];
                m_syncTarget[index].getTranslation(translation);
                m_syncTarget[index].getOrientation(orientation);
                m_syncTarget[index].getScale(scale);
                m_syncSource[index].setTranslation(translation);
                m_syncSource[index].setOrientation(orientation);
                m_syncSource[index].setScale(scale);
            }
            graphics.render(m_handsWorld.m_interface);
        }
    }
    if (m_blurAmount > 0.0f && m_field51c == 1) {
        ICGraphics::GetInstance()->Sync();
        float alpha = (50.0f*m_blurAmount)/(float)GameApp()->m_updateFrameTime;
        if (alpha < 0.0f) alpha = 0.0f;
        else if (alpha > 1.0f) alpha = 1.0f;
        com::glu::platform::components::Color_RGBA color = {1.0f,1.0f,1.0f,alpha};
        int previous = m_blurTarget == 0;
        Graphics2D::GetInstance()->Draw(*m_blurSurfaces[previous],NULL,&color,Graphics2D::Flip_None);
    }
}
void CSwerveGame::Shot(int x,int y)
{
    if (m_finishing) return;
    if (m_field69c) { GameApp()->m_soundManager->Play((CDH_SoundManager::eSound)90,false,false); return; }
    CDH_Weapon* weapon = GameApp()->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon();
    if (weapon == NULL || weapon->IsReadyForUse() != 1 || m_field5cc) return;
    if (weapon->MakeShot() != 1) return;
    int aimX = x;
    int aimY = y;
    float rayX;
    float rayY;
    float unused;
    float originalDispersion = m_zoom;
    SimulateAimingDispersion(aimX,aimY,rayX,rayY,unused,true);
    if (GameApp()->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon()->m_projectileType == 1) { LaunchGrenade(0.0f); return; }
    CDH_Weapon* tracerWeapon = GameApp()->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon();
    int weaponType = tracerWeapon->m_weaponType;
    unsigned int shotCount = 1;
    unsigned int tracerCount = 1;
    switch (weaponType) {
    case 1:
    case 7: shotCount = 7; tracerCount = 7; break;
    case 8: shotCount = 2; tracerCount = 2; break;
    case 9: shotCount = 3; tracerCount = 1; break;
    }
    GetCurrentCameraRight(); GetCurrentCameraUp(); GetCurrentCameraPos();
    if (m_cameraDirty) UpdateCameraVectors();
    CBH_Player* player = CBH_Player::GetInstance();
    player->m_gameStats.NotifyShotBegin(GameApp()->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon(),NULL,shotCount);
    for (unsigned int shot = 0; shot < shotCount; ++shot) {
        int hit = m_unitsController->Shot(rayX,rayY,weapon,m_collisionLine);
        vec3 point = m_collisionPoint;
        vec3 normal = m_collisionNormal;
        if (m_handGrenades != NULL) m_handGrenades->Pick((float)aimX,(float)aimY,m_cameraNode.getActiveCamera());
        if (shot < tracerCount && m_tracers != NULL) {
            int barrel = (int)shot;
            if (weaponType == 1 || weaponType == 7) barrel = 0;
            vec3 emitter = tracerWeapon->GetTracerEmitterPos(barrel,m_referenceNode,m_cameraNode);
            vec3 cameraVelocity = GetCurrentCameraVel();
            emitter = emitter+cameraVelocity*GameApp()->m_frameTimeSeconds;
            float spread = 0.0f;
            float pointSquared = (point.m_x*point.m_x+point.m_y*point.m_y)+point.m_z*point.m_z;
            if (MathLib::InvSqrt(pointSquared)*pointSquared == 0.0f) {
                point = emitter-m_cameraForward*100.0f;
                float fov = m_camera->GetFOV();
                if (fov > 0.0f) spread = m_zoom/((float)m_width/fov);
                else spread = 0.0f;
            }
            int speed = CRandGen::GetInstance()->GetRandRange(10000,15000);
            vec3 direction = point-emitter;
            float inverseLength = MathLib::InvSqrt((direction.m_x*direction.m_x+direction.m_y*direction.m_y)+direction.m_z*direction.m_z);
            vec3 velocity = (direction*inverseLength)*((float)speed*0.01f);
            AddTracer(emitter,velocity,spread);
        }
        if (hit == 0) {
            float normalSquared = (normal.m_x*normal.m_x+normal.m_y*normal.m_y)+normal.m_z*normal.m_z;
            if (MathLib::InvSqrt(normalSquared)*normalSquared > 0.0f) {
                CSwerveGame* game = GameApp()->m_helper->m_swerveGame;
                const vec3& cameraPosition = game->GetCurrentCameraPos();
                vec3 direction = point-cameraPosition;
                float inverseLength = MathLib::InvSqrt((direction.m_x*direction.m_x+direction.m_y*direction.m_y)+direction.m_z*direction.m_z);
                CRandGen* random = CRandGen::GetInstance();
                direction = direction*inverseLength;
                float probability = (1.0f-MathLib::Fabs((direction.m_x*normal.m_x+direction.m_y*normal.m_y)+direction.m_z*normal.m_z))*0.125f;
                if ((float)random->GetRandRange(0,1000)*0.001f <= probability) {
                    vec3 incoming = direction*250.0f;
                    float projection = (normal.m_x*incoming.m_x+normal.m_y*incoming.m_y)+normal.m_z*incoming.m_z;
                    vec3 perpendicular = normal*projection;
                    vec3 tangent = incoming-perpendicular;
                    vec3 velocity = perpendicular*-0.75f+tangent*0.5f;
                    AddRicochet(point,velocity,5.0f,2);
                }
            }
        }
        if (m_debris != NULL) {
            float normalSquared = (normal.m_x*normal.m_x+normal.m_y*normal.m_y)+normal.m_z*normal.m_z;
            if (MathLib::InvSqrt(normalSquared)*normalSquared > 0.0f && hit == 0) {
                float inverseLength = MathLib::InvSqrt(normalSquared);
                CDH_Weapon* current = GameApp()->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon();
                unsigned int type = (unsigned int)current->m_weaponType;
                if (current->m_projectileType == 2) type = 5;
                if (type <= 9 && type != 6) {
                    normal = normal*inverseLength;
                    unsigned int debris = 0;
                    unsigned int particles = 0;
                    unsigned int smoke = 4;
                    int flashes = 0;
                    switch (type) {
                    case 0:
                        particles = CRandGen::GetInstance()->Generate()%7+1;
                        debris = 1;
                        smoke = CRandGen::GetInstance()->Generate()%3+2;
                        break;
                    case 1:
                        debris = CRandGen::GetInstance()->Generate()%3+2;
                        particles = 1;
                        smoke = CRandGen::GetInstance()->Generate()%3+2;
                        break;
                    case 2:
                        debris = CRandGen::GetInstance()->Generate()%7+1;
                        particles = 1;
                        smoke = CRandGen::GetInstance()->Generate()%3+2;
                        break;
                    case 3:
                        particles = CRandGen::GetInstance()->Generate()%3;
                        smoke = CRandGen::GetInstance()->Generate()%3;
                        flashes = 1;
                        break;
                    case 7:
                        debris = CRandGen::GetInstance()->Generate()%3;
                        particles = CRandGen::GetInstance()->Generate()&1;
                        smoke = CRandGen::GetInstance()->Generate()%3;
                        break;
                    case 8:
                    case 9:
                        smoke = CRandGen::GetInstance()->Generate()%3;
                        flashes = 1;
                        break;
                    }
                    for (unsigned int index = 0; index < debris; ++index) {
                        float speed = (float)CRandGen::GetInstance()->GetRandRange(5,10);
                        vec3 velocity = normal*speed;
                        m_debris->AddProjectile(point,velocity,60.0f,0);
                    }
                    for (unsigned int index = 0; index < particles; ++index) {
                        float speed = (float)CRandGen::GetInstance()->GetRandRange(2,6);
                        vec3 velocity = normal*speed;
                        m_particles->AddProjectile(point,velocity,60.0f,0);
                    }
                    for (unsigned int index = 0; index < smoke; ++index) {
                        float speed = (float)CRandGen::GetInstance()->GetRandRange(0,9)*0.1f+0.1f;
                        vec3 velocity = normal*speed;
                        m_smoke->AddProjectile(point,velocity,60.0f,0);
                    }
                    if (flashes != 0) {
                        vec3 velocity = normal*0.01f;
                        do {
                            const vec3& position = GetCurrentCameraPos();
                            vec3 direction = position-point;
                            float scale = MathLib::InvSqrt((direction.m_x*direction.m_x+direction.m_y*direction.m_y)+direction.m_z*direction.m_z);
                            vec3 origin = point+(direction*scale)*0.1f;
                            m_flashes->AddProjectile(origin,velocity,0.0f,0);
                            --flashes;
                        } while (flashes != 0);
                    }
                }
            }
        }
        m_zoom = originalDispersion;
        aimX = x; aimY = y;
        SimulateAimingDispersion(aimX,aimY,rayX,rayY,unused,true);
    }
    CBH_Player::GetInstance()->m_gameStats.NotifyShotEnd();
}
