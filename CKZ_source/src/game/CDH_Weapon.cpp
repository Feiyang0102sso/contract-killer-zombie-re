// CDH_Weapon.cpp - one weapon of the player: shop item, stats from weapons.xml, 3D model and its animations.
#include "CDH_Weapon.h"
#include "App.h"
#include "AppSettings.h"
#include "CSwerve.h"
#include "CXmlHelper.h"
#include "tinyxml.h"
#include "CSaveManager.h"
#include "CPowerUpManager.h"
#include "CPowerUp.h"
#include "CGrenadeManager.h"
#include "CGrenade.h"
#include "CBH_SurfaceManager.h"
#include "CBH_Player.h"
#include "CGameAnalytics.h"
#include "CHuntingInfo.h"
#include "CDH_SoundManager.h"
#include "CDH_ResourceManager.h"
#include "ResMap.h"
#include "DGHelper.h"
#include "CDH_PlayerData.h"
#include "CDH_BasicGameData.h"
#include "CSwerveGame.h"
#include "dgCamera.h"
#include "CGameplayHUD.h"
#include "SwerveHelper.h"
#include "WindowBase.h"
#include "ICMediaPlayer.h"
#include "CRandGen.h"

using com::glu::platform::graphics::ICRenderSurface;
using com::glu::platform::components::ICMediaPlayer;
using com::glu::platform::core::CRandGen;

// Sounds of CDH_SoundManager used here (names from the context).
static const CDH_SoundManager::eSound SOUND_SHOTGUN_RELOAD_START = (CDH_SoundManager::eSound)0x10;
static const CDH_SoundManager::eSound SOUND_SHOTGUN_SHELL_1 = (CDH_SoundManager::eSound)0x11;
static const CDH_SoundManager::eSound SOUND_SHOTGUN_SHELL_2 = (CDH_SoundManager::eSound)0x12;
static const CDH_SoundManager::eSound SOUND_SHOTGUN_RELOAD_END = (CDH_SoundManager::eSound)0x13;
static const CDH_SoundManager::eSound SOUND_NO_AMMO = (CDH_SoundManager::eSound)0x5a;
static const CDH_SoundManager::eSound SOUND_SCOPE_IN = (CDH_SoundManager::eSound)0x5b;
static const CDH_SoundManager::eSound SOUND_SCOPE_OUT = (CDH_SoundManager::eSound)0x5c;
static const CDH_SoundManager::eSound SOUND_MINIGUN_SPIN_START = (CDH_SoundManager::eSound)0x66;
static const CDH_SoundManager::eSound SOUND_MINIGUN_SPIN = (CDH_SoundManager::eSound)0x67;
static const CDH_SoundManager::eSound SOUND_GRENADE_THROW = (CDH_SoundManager::eSound)0x7b;

// Animation controllers of m_animation (order of addController in loadWeaponModel).
static const int CONTROLLER_SHOOT = 0;
static const int CONTROLLER_FETCH = 1;
static const int CONTROLLER_HIDE = 2;
static const int CONTROLLER_IDLE = 3;
static const int CONTROLLER_THROW = 4;
static const int CONTROLLER_RELOAD = 5;
static const int CONTROLLER_SPIN = 6;

// Weapon types ("weapon_type").
static const int WEAPON_TYPE_SHOTGUN = 1;
static const int WEAPON_TYPE_HEAVY = 8;
static const int WEAPON_TYPE_MINIGUN = 9;
// Weapon id of the mine launcher (shows the "fugas" mesh while loaded).
static const int WEAPON_ID_MINE = 0x23;

static void ChangeTexture(swerve::Node<CSwerveAssertingErrorHandler> node,
                          swerve::Image2D<CSwerveAssertingErrorHandler> image, int textureUserId);

/** The running application. */
static inline App* GetApp()
{
    return (App*)WindowApp::m_instance;
}

CDH_Weapon::CDH_Weapon(TiXmlNode* node)
    : m_modelRefCount(0),
      m_state(1),
      m_action(0),
      m_actionTimeLeft(0),
      m_reloadPending(false),
      m_throwPending(false),
      m_fetched(false),
      m_idle(false),
      m_interruptingReload(false),
      m_spinning(false),
      m_spinSoundCount(0),
      m_baseTransform(CSwerve::GetInstance()->m_swerve),
      m_inertionTransform(CSwerve::GetInstance()->m_swerve),
      m_barrelTransform(CSwerve::GetInstance()->m_swerve),
      m_cameraTransform(CSwerve::GetInstance()->m_swerve),
      m_scopeTime(0),
      m_grenadePending(false),
      m_autoShotsLeft(0),
      m_autoShot(false),
      m_zoomFovMin(52.5f),
      m_zoomFovMax(52.5f),
      m_aimZoomFov(52.5f),
      m_scopeAttached(false),
      m_countInQueue(0),
      m_reloadingStepStarted(0),
      m_gameImageId(NULL),
      m_field1b4(0),
      m_field1b8(0),
      m_spriteScopeArch(-1),
      m_lightAdded(false),
      m_spinSpeed(0.0f),
      m_spinUpTime(0.0f),
      m_powerUpApplied(false)
{
    m_field1c8 = m_field1cc = m_field1d0 = m_field1d4 = 0;
    m_animation = NULL;
    m_scopeAnimation = NULL;
    m_scopeAttached = false;
    m_available.Set(false);
    m_handsSeparated = false;
    m_shotDeferringTime = -1.0f;
    parseNode(node);
    Load();
}

void CDH_Weapon::parseNode(TiXmlNode* node)
{
    m_id = CXmlHelper::GetAttributeValue(node, "wid").ToInt();
    parseTexts(node);
    parseAnims(node);
    parseStats(node);
    parseGraphics(node);
    parseSounds(node);
    ParseOverridable(node, true);
}

void CDH_Weapon::Load()
{
    TiXmlElement* weapons = CXmlHelper::GetElement(GetApp()->m_saveManager->GetRootElement()->ToElement(),
                                                   XString("weapons"));
    if (weapons != NULL) {
        XString name = XString::Format(L"w%d", getId());
        TiXmlElement* element = CXmlHelper::GetElement(weapons, name);
        if (element != NULL) {
            m_available.Set(CXmlHelper::GetAttributeValue(element, "a").ToBool());
        }
    }
}

CDH_Weapon::~CDH_Weapon()
{
    releaseWeaponModel();
    for (int index = 0; index < m_itemsGroups.size(); index++) {
        if (m_itemsGroups[index] != NULL) {
            delete m_itemsGroups[index];
            m_itemsGroups[index] = NULL;
        }
    }
}

void CDH_Weapon::releaseWeaponModel()
{
    if (App::m_platformType != 0) {
        m_modelRefCount--;
        if (m_modelRefCount != 0) {
            return;
        }
        if (m_scopeAnimation != NULL) {
            delete m_scopeAnimation;
            m_scopeAnimation = NULL;
        }
        if (m_animation != NULL) {
            delete m_animation;
            m_animation = NULL;
        }
        m_controller.Clear();
        m_eyeCamera.Clear();
        m_menuCamera.Clear();
        m_world.Clear();
    }
}

void CDH_Weapon::ItemsGroup::setActiveID(int id)
{
    m_activeId = id;
    for (int itemIndex = 0; itemIndex < m_items.size(); itemIndex++) {
        ItemDesc* item = m_items[itemIndex];
        bool active = itemIndex == m_activeId;
        for (int nodeIndex = 0; nodeIndex < item->m_nodes.size(); nodeIndex++) {
            if (item->m_nodes[nodeIndex] != NULL) {
                item->m_nodes[nodeIndex].setRenderingEnable(active);
            }
        }
    }
}

void CDH_Weapon::restoreAmmo()
{
    m_ammo = m_clipSize;
    m_state = 1;
    m_action = 0;
    m_actionTimeLeft = 0;
    m_reloadPending = false;
    m_throwPending = false;
}

void CDH_Weapon::update(unsigned int deltaTime)
{
    if (GetApp()->m_powerUpManager->IsPowerUpRunning() != m_powerUpApplied) {
        ModifyDurations();
    }

    // the mine launcher shows its mine while loaded
    if (getId() == WEAPON_ID_MINE && m_fugas != NULL) {
        if (m_action != 3) {
            m_fugas.setRenderingEnable(m_ammo > 0);
        } else {
            m_fugas.setRenderingEnable(true);
        }
    }

    // minigun: the barrel slows down when not shooting
    if (m_weaponType == WEAPON_TYPE_MINIGUN && m_spinning) {
        m_spinUpTime += (float)deltaTime * 0.001f;
        if (m_spinUpTime > 0.5f) {
            float speed = m_spinSpeed + GetApp()->m_frameTimeSeconds * -0.75f;
            if (speed < 0.0f) {
                speed = 0.0f;
            }
            m_spinSpeed = speed;
        }
        if (m_spinSpeed > 0.1f && !GetApp()->m_soundManager->IsPlaying(m_spinSoundCount < 2 ? SOUND_MINIGUN_SPIN_START : SOUND_MINIGUN_SPIN)) {
            m_spinSoundCount++;
            GetApp()->m_soundManager->Play(m_spinSoundCount < 2 ? SOUND_MINIGUN_SPIN_START : SOUND_MINIGUN_SPIN, false, m_spinSoundCount > 1);
        } else if (m_spinSpeed < 0.05f) {
            if (GetApp()->m_soundManager->IsPlaying(m_spinSoundCount < 2 ? SOUND_MINIGUN_SPIN_START : SOUND_MINIGUN_SPIN)) {
                GetApp()->m_soundManager->Stop(m_spinSoundCount < 2 ? SOUND_MINIGUN_SPIN_START : SOUND_MINIGUN_SPIN);
            }
            m_spinSoundCount = 0;
        }
        if (m_animation != NULL) {
            m_animation->setControllerSpeed(CONTROLLER_SPIN, m_spinSpeed);
            if (m_animation->getControllerPosition(CONTROLLER_SPIN) >= 1.0f) {
                m_animation->setControllerPosition(CONTROLLER_SPIN, 0.0f);
            }
        }
    }

    if (App::m_platformType != 0) {
        // back to the idle animation once the current one is over
        if (m_fetched && isModelAnimationComplete() == 1 &&
            (m_scopeAnimation == NULL || m_scopeAnimation->isAnimationComplete() == true)) {
            if (m_weaponType == WEAPON_TYPE_MINIGUN && m_spinSpeed > 0.0f) {
                if (m_animation->m_currentController != CONTROLLER_SPIN) {
                    if (m_scopeAnimation != NULL) {
                        m_scopeAnimation->stopAllAnimation();
                    }
                    if (m_animation != NULL) {
                        m_animation->setCurrentController(CONTROLLER_SPIN, false);
                    }
                }
            } else {
                ResetModelAnimation();
                if (m_scopeAnimation != NULL) {
                    m_scopeAnimation->stopAllAnimation();
                }
                if (m_animation != NULL) {
                    m_animation->setCurrentController(CONTROLLER_IDLE, false);
                }
            }
            m_idle = true;
        }
        animateWeaponModel();
    }
    UpdateState(deltaTime);
}

void CDH_Weapon::ModifyDurations()
{
    CPowerUp* powerUp = GetApp()->m_powerUpManager->GetActivePowerUp();
    if (GetApp()->m_powerUpManager->IsPowerUpRunning() == true) {
        SetAnimationControllerSpeedMultiplier(CONTROLLER_SHOOT, powerUp->m_fireSpeed);
        SetAnimationControllerSpeedMultiplier(CONTROLLER_FETCH, powerUp->m_switchSpeed);
        SetAnimationControllerSpeedMultiplier(CONTROLLER_HIDE, powerUp->m_switchSpeed);
        SetAnimationControllerSpeedMultiplier(CONTROLLER_THROW, powerUp->m_fireSpeed);
        if (m_weaponType == WEAPON_TYPE_SHOTGUN) {
            for (int controller = CONTROLLER_RELOAD; controller < CONTROLLER_RELOAD + 6; controller++) {
                SetAnimationControllerSpeedMultiplier(controller, powerUp->m_reloadSpeed);
            }
        } else {
            SetAnimationControllerSpeedMultiplier(CONTROLLER_RELOAD, powerUp->m_reloadSpeed);
        }
        m_rateOfFire = (int)((float)m_rateOfFire / powerUp->m_fireSpeed);
        m_throwTime = (int)((float)m_throwTime / powerUp->m_fireSpeed);
        m_throwDelay = (int)((float)m_throwDelay / powerUp->m_fireSpeed);
        m_reloadTime = (int)((float)m_reloadTime / powerUp->m_reloadSpeed);
        m_powerUpApplied = true;
    } else {
        SetAnimationControllerSpeedMultiplier(CONTROLLER_SHOOT, 1.0f / powerUp->m_fireSpeed);
        SetAnimationControllerSpeedMultiplier(CONTROLLER_FETCH, 1.0f / powerUp->m_switchSpeed);
        SetAnimationControllerSpeedMultiplier(CONTROLLER_HIDE, 1.0f / powerUp->m_switchSpeed);
        SetAnimationControllerSpeedMultiplier(CONTROLLER_THROW, 1.0f / powerUp->m_fireSpeed);
        if (m_weaponType == WEAPON_TYPE_SHOTGUN) {
            for (int controller = CONTROLLER_RELOAD; controller < CONTROLLER_RELOAD + 6; controller++) {
                SetAnimationControllerSpeedMultiplier(controller, 1.0f / powerUp->m_reloadSpeed);
            }
        } else {
            SetAnimationControllerSpeedMultiplier(CONTROLLER_RELOAD, 1.0f / powerUp->m_reloadSpeed);
        }
        m_rateOfFire = m_baseRateOfFire;
        m_throwTime = m_baseThrowTime;
        m_throwDelay = m_baseThrowDelay;
        m_reloadTime = m_baseReloadTime;
        m_powerUpApplied = false;
    }
}

int CDH_Weapon::isModelAnimationComplete() const
{
    if (m_animation == NULL) {
        return 1;
    }
    if (m_animation->getControllerPosition(m_animation->m_currentController) >= 1.0f) {
        return 1;
    }
    return m_animation->m_currentController == -1;
}

void CDH_Weapon::ResetModelAnimation()
{
    if (m_animation != NULL) {
        if (m_animation->m_currentController != -1) {
            m_animation->setControllerPosition(m_animation->m_currentController, 1.0f);
            m_animation->animate();
        }
        m_animation->setCurrentController(CONTROLLER_IDLE, false);
        m_animation->animate();
        m_animation->setCurrentController(-1, false);
    }
    m_idle = false;
}

void CDH_Weapon::animateWeaponModel()
{
    m_animation->animate();
    if (m_scopeAnimation != NULL) {
        m_scopeAnimation->animate();
    }
}

void CDH_Weapon::UpdateState(int deltaTime)
{
    // shell by shell reload
    if (m_reloadingSteps.size() != 0) {
        if (m_reloadingStepStarted != 0) {
            m_reloadingSteps[0].m_time += (float)deltaTime * -0.001f;
            if (m_reloadingSteps[0].m_time <= 0.0f) {
                if (m_reloadingSteps[0].m_sound != -1) {
                    GetApp()->m_soundManager->Stop((CDH_SoundManager::eSound)m_reloadingSteps[0].m_sound);
                }
                m_reloadingSteps.removeElementAt(0);
                m_reloadingStepStarted = 0;
            }
        }
        if (m_reloadingStepStarted == 0 && m_reloadingSteps.size() != 0) {
            if (m_reloadingSteps[0].m_sound != -1) {
                GetApp()->m_soundManager->Play((CDH_SoundManager::eSound)m_reloadingSteps[0].m_sound, false, false);
            }
            int ammo = m_ammo + m_reloadingSteps[0].m_ammo;
            if (ammo > m_clipSize) {
                ammo = m_clipSize;
            }
            m_reloadingStepStarted = 1;
            m_ammo = ammo;
        }
    }

    if (!GetApp()->m_helper->m_swerveGame->GetDebugFireAction()) {
        m_autoShotsLeft = 0;
    }
    if (m_actionTimeLeft == 0) {
        CheckForReload(false);
        return;
    }
    m_actionTimeLeft -= deltaTime;
    if (m_action == 8) {
        m_throwDelayLeft -= deltaTime;
        if (m_grenadePending && m_throwDelayLeft < 1) {
            m_grenadePending = false;
            GetApp()->m_helper->m_swerveGame->LaunchHandGrenade(1.0f);
        }
    }
    if (m_actionTimeLeft < 1) {
        m_actionTimeLeft = 0;
        m_state = m_action;
        m_action = 0;
        AfterStateChanged();
    }
}

void CDH_Weapon::setDynamicLight(swerve::Light<CSwerveAssertingErrorHandler>& light)
{
    if (m_lightGroup != NULL) {
        m_world.removeChild(m_lightGroup);
    }
    m_light = swerve::Light<CSwerveAssertingErrorHandler>(light.duplicate());
    m_lightGroup = SwerveHelper::NewGroup();
    m_lightGroup.addChild(m_light);

    // ambient light of the weapon model
    swerve::Light<CSwerveAssertingErrorHandler> ambientLight =
        swerve::Light<CSwerveAssertingErrorHandler>::Create(CSwerve::GetInstance()->m_swerve);
    ambientLight.setColor(0x7f7f7f7f);
    ambientLight.setIntensity(1.0f);
    ambientLight.setMode(swerve::Light<CSwerveAssertingErrorHandler>::AMBIENT);
    ambientLight.setScope(-1);
    m_lightGroup.addChild(ambientLight);
    m_lightAdded = false;
}

void CDH_Weapon::updateDynamicLight(Transform& transform)
{
    m_lightGroup.setTransform(transform);
}

void CDH_Weapon::hideWeaponModel()
{
    if (m_scopeAnimation != NULL) {
        m_scopeAnimation->stopAllAnimation();
    }
    if (m_animation != NULL) {
        m_animation->setCurrentController(CONTROLLER_FETCH, false);
        m_animation->animate();
        m_animation->setCurrentController(-1, false);
    }
    m_fetched = false;
    m_idle = false;
}

void CDH_Weapon::fetchWeaponModel()
{
    if (m_scopeAnimation != NULL) {
        m_scopeAnimation->stopAllAnimation();
    }
    if (m_animation != NULL) {
        m_animation->setCurrentController(CONTROLLER_FETCH, false);
    }
    m_fetched = true;
    m_idle = false;
    GetApp()->m_soundManager->Play((CDH_SoundManager::eSound)m_switchSound, false, false);
    m_spinning = true;
}

void CDH_Weapon::unfetchWeaponModel()
{
    if (m_weaponType == WEAPON_TYPE_MINIGUN) {
        GetApp()->m_soundManager->Stop(m_spinSoundCount < 2 ? SOUND_MINIGUN_SPIN_START : SOUND_MINIGUN_SPIN);
        m_spinSpeed = 0.0f;
    }
    ResetModelAnimation();
    if (m_scopeAnimation != NULL) {
        m_scopeAnimation->stopAllAnimation();
    }
    if (m_animation != NULL) {
        m_animation->setCurrentController(CONTROLLER_HIDE, false);
    }
    m_fetched = false;
    m_idle = false;
    m_spinning = false;
}

void CDH_Weapon::scopeReset()
{
    if (m_animation != NULL) {
        m_animation->stopAllAnimation();
    }
    if (m_scopeAnimation != NULL) {
        m_scopeAnimation->setCurrentController(0, false);
        m_scopeAnimation->animate();
        m_scopeAnimation->setCurrentController(-1, false);
    }
    m_idle = false;
}

bool CDH_Weapon::isIdle() const
{
    return m_idle;
}

void CDH_Weapon::paintWeaponModel(swerve::Graphics3D<CSwerveAssertingErrorHandler>& graphics)
{
    if (!m_lightAdded && m_lightGroup != NULL) {
        m_world.addChild(m_lightGroup);
        m_lightAdded = true;
    }
    App::RenderWorld(graphics, m_world);
}

void CDH_Weapon::playReloadSound()
{
    bool muted = false;
    if (GetApp()->m_powerUpManager->IsPowerUpRunning() == true) {
        float soundSpeed = GetApp()->m_powerUpManager->GetActivePowerUp()->m_reloadSoundSpeed;
        CPowerUp* powerUp = GetApp()->m_powerUpManager->GetActivePowerUp();
        // The original keeps a separate "muted = true" block after the second compare.
        if (soundSpeed != 1.0f || powerUp->m_reloadSpeed != 1.0f) {
            muted = true;
        }

    }

    if (m_weaponType == WEAPON_TYPE_SHOTGUN) {
        // shotgun: start, one sound per shell, end
        GetApp()->m_soundManager->Stop(SOUND_SHOTGUN_RELOAD_START);
        GetApp()->m_soundManager->Stop(SOUND_SHOTGUN_SHELL_1);
        GetApp()->m_soundManager->Stop(SOUND_SHOTGUN_SHELL_2);
        GetApp()->m_soundManager->Stop(SOUND_SHOTGUN_RELOAD_END);
        float speed;
        if (GetApp()->m_powerUpManager->IsPowerUpRunning() == true) {
            speed = 1.0f / GetApp()->m_powerUpManager->GetActivePowerUp()->m_reloadSpeed;
        } else {
            speed = 1.0f;
        }
        float startDuration = speed * GetApp()->m_soundManager->GetSoundDuration(SOUND_SHOTGUN_RELOAD_START, 2, 22000);
        float shell1Duration = speed * GetApp()->m_soundManager->GetSoundDuration(SOUND_SHOTGUN_SHELL_1, 2, 22000);
        float shell2Duration = speed * GetApp()->m_soundManager->GetSoundDuration(SOUND_SHOTGUN_SHELL_2, 2, 22000);
        float endDuration = speed * GetApp()->m_soundManager->GetSoundDuration(SOUND_SHOTGUN_RELOAD_END, 2, 22000);

        SReloadingStep startStep(muted ? -1 : SOUND_SHOTGUN_RELOAD_START, 0, startDuration);
        m_reloadingSteps.addElement(startStep);

        unsigned int shells = m_clipSize;
        if (shells > 6) {
            shells = 6;
        }
        for (int shell = 0; shell < (int)(shells - m_ammo); shell++) {
            if ((CRandGen::GetInstance()->GetRand(99) & 1) != 0) {
                SReloadingStep step(muted ? -1 : SOUND_SHOTGUN_SHELL_1, 1, shell1Duration);
                m_reloadingSteps.addElement(step);
            } else {
                SReloadingStep step(muted ? -1 : SOUND_SHOTGUN_SHELL_2, 1, shell2Duration);
                m_reloadingSteps.addElement(step);
            }
        }

        SReloadingStep endStep(muted ? -1 : SOUND_SHOTGUN_RELOAD_END, 0, endDuration);
        m_reloadingSteps.addElement(endStep);
    } else if (!muted) {
        GetApp()->m_soundManager->Play((CDH_SoundManager::eSound)m_reloadSound, false, false);
    }
}

void CDH_Weapon::loadWeaponModel()
{
    if (App::m_platformType == 0) {
        m_scopeTime = 1000;
        return;
    }
    int users = m_modelRefCount;
    m_modelRefCount = users + 1;
    if (users > 0) {
        return;
    }

    m_world = swerve::World<CSwerveAssertingErrorHandler>(App::LoadObject3D(m_modelId));
    if (getId() == WEAPON_ID_MINE) {
        m_fugas = swerve::Mesh<CSwerveAssertingErrorHandler>(m_world.find(SwerveHelper::Id("fugas")));
    }
    if (m_textureId != NULL) {
        swerve::Image2D<CSwerveAssertingErrorHandler> image(App::LoadObject3D(m_textureId));
        ChangeTexture(m_world, image, m_textureNodeId);
    }
    m_world.getBackground().setColorClearEnable(false);

    // cameras and transforms of the model
    m_eyeCamera = swerve::Camera<CSwerveAssertingErrorHandler>(m_world.find(DGHelper::getSwerveID("eyecam")));
    m_eyecamTransform = Group(m_world.find(SwerveHelper::Id("eyecamTransform")));
    m_eyecamTransform.getCompositeTransform(m_baseTransform);
    if (m_weaponType == WEAPON_TYPE_HEAVY) {
        m_barrelCount = 2;
    } else {
        m_barrelCount = 1;
    }
    for (int barrel = 0; barrel < m_barrelCount; barrel++) {
        XString name = XString::Format(L"dummyBarrel%02dTransform", barrel + 1);
        m_barrels[barrel] = Group(m_world.find(SwerveHelper::Id(XString::AnsiString(name.m_data))));
        if (m_barrels[barrel] == NULL) {
            m_barrels[barrel] = Group(m_world.find(SwerveHelper::Id("weaponControllerTransform")));
        }
    }
    Transform cameraTransform(CSwerve::GetInstance()->m_swerve);
    m_eyeCamera.getCompositeTransform(cameraTransform);
    float offset[3];
    offset[0] = -1e-05f;
    offset[1] = 0.0f;
    offset[2] = 0.0f;
    cameraTransform.postTranslate(offset);
    m_eyeCamera.setTransform(cameraTransform);
    m_menuCamera = swerve::Camera<CSwerveAssertingErrorHandler>(m_world.find(DGHelper::getSwerveID("menu_cam")));
    m_world.setActiveCamera(m_eyeCamera);
    m_controller = swerve::Transformable<CSwerveAssertingErrorHandler>(
        m_world.find(DGHelper::getSwerveID("weaponControllerTransform")));

    // animations of the weapon
    m_animation = new CDH_Animation(m_controller);
    int weaponType = m_weaponType;
    m_animation->addController(m_shootAnimation);
    m_animation->addController(m_fetchAnimation);
    m_animation->addController(m_hideAnimation);
    m_animation->addController(m_unusedAnimation);
    m_animation->addController(m_throwAnimation);
    if (weaponType == WEAPON_TYPE_SHOTGUN) {
        // one reload animation per number of missing shells
        for (int shells = 1; shells != 7; shells++) {
            XString name = m_reloadAnimation + XString::Format(L"_%d", shells);
            m_animation->addController(name);
        }
    } else {
        m_animation->addController(m_reloadAnimation);
        if (m_weaponType == WEAPON_TYPE_MINIGUN) {
            // Named local: original builds the string before loading m_animation.
            XString twistName("twist");
            m_animation->addController(twistName);
        }
    }

    // aim animation: on the eye camera's parent, else on the weapon controller
    XString::AnsiString aimName(m_aimAnimation.m_data);
    if (CDH_Animation::HaveController(m_eyeCamera.getParent(), aimName)) {
        m_scopeAnimation = new CDH_Animation(m_eyeCamera.getParent());
        m_scopeAnimation->addController(m_aimAnimation);
        m_scopeTime = m_scopeAnimation->getControllerBaseDuration(0);
    }
    if (m_scopeAnimation == NULL) {
        if (CDH_Animation::HaveController(m_controller, aimName) == true) {
            m_scopeAnimation = new CDH_Animation(m_controller);
            m_scopeAnimation->addController(m_aimAnimation);
            m_scopeTime = m_scopeAnimation->getControllerBaseDuration(0);
        }
    }

    m_throwTime = m_baseThrowTime = m_animation->getControllerBaseDuration(CONTROLLER_THROW);
    if (m_rateOfFire < m_animation->getControllerBaseDuration(CONTROLLER_SHOOT)) {
        m_animation->setControllerDuration(CONTROLLER_SHOOT, m_rateOfFire);
    }
    int reloadDuration = m_animation->getControllerBaseDuration(CONTROLLER_RELOAD);
    if (m_reloadTime < reloadDuration) {
        m_reloadTime = reloadDuration;
        m_baseReloadTime = reloadDuration;
    }
    m_animation->setControllerDuration(CONTROLLER_RELOAD, m_reloadTime);

    // optional model parts
    for (int groupIndex = 0; groupIndex < m_itemsGroups.size(); groupIndex++) {
        ItemsGroup* group = m_itemsGroups[groupIndex];
        for (int itemIndex = 0; itemIndex < group->m_items.size(); itemIndex++) {
            ItemDesc* item = group->m_items[itemIndex];
            item->m_nodes.removeAllElements();
            for (int nodeIndex = 0; nodeIndex < item->m_nodeNames.size(); nodeIndex++) {
                item->m_nodes.addElement(
                    Node(m_world.find(DGHelper::getSwerveID(XString::AnsiString(item->m_nodeNames[nodeIndex].m_data)))));
            }
            item->m_resourceData = GetApp()->m_resourceManager->getResourceData(item->m_resourceId, false);
        }
        group->m_items[0]->m_active = true;
        group->setActiveID(0);
    }
    hideWeaponModel();
    if (m_scopeAnimation != NULL) {
        scopeReset();
    }
}

/** Put `image` into texture 0 of the appearances of `node` (and below) whose texture has user id `textureUserId`. */
static void ChangeTexture(swerve::Node<CSwerveAssertingErrorHandler> node,
                          swerve::Image2D<CSwerveAssertingErrorHandler> image, int textureUserId)
{
    swerve::Mesh<CSwerveAssertingErrorHandler> mesh(node);
    if (mesh != NULL) {
        int submeshCount = mesh.getSubmeshCount();
        for (int index = 0; index < submeshCount; index++) {
            bool found = false;
            {
                swerve::Appearance<CSwerveAssertingErrorHandler> appearance = mesh.getAppearance(index);
                swerve::Texture2D<CSwerveAssertingErrorHandler> texture = appearance.getTexture(0);
                if (texture != NULL && mesh.getAppearance(index).getTexture(0).getUserID() == textureUserId) {
                    found = true;
                }
            }
            if (found) {
                mesh.getAppearance(index).getTexture(0).setImage(image);
            }
        }
    }
    swerve::Group<CSwerveAssertingErrorHandler> group(node);
    if (group != NULL) {
        int childCount = group.getChildCount();
        for (int index = 0; index < childCount; index++) {
            ChangeTexture(group.getChild(index), image, textureUserId);
        }
    }
}

void CDH_Weapon::loadResources()
{
    restoreAmmo();
}

void CDH_Weapon::parseTexts(TiXmlNode* node)
{
    XString nameString = CXmlHelper::GetAttributeValueOptional(node, "nameStr");
    // ?: keeps one temporary for both sources (as the original)
    m_name = (nameString.ByteSize() == 0) ? Window::ResStringSafe(CXmlHelper::GetAttributeValue(node, "name")) : nameString;
    XString descriptionString = CXmlHelper::GetAttributeValueOptional(node, "descStr");
    m_description = (descriptionString.ByteSize() == 0) ? Window::ResStringSafe(CXmlHelper::GetAttributeValue(node, "desc"))
                                                         : descriptionString;
}

void CDH_Weapon::parseAnims(TiXmlNode* node)
{
    m_shootAnimation = CXmlHelper::GetAttributeValueOrUseDefault(node, "shootAnimation", "shoot");
    m_reloadAnimation = CXmlHelper::GetAttributeValueOrUseDefault(node, "reloadAnimation", "reload");
    m_fetchAnimation = CXmlHelper::GetAttributeValueOrUseDefault(node, "fetchAnimation", "fetch");
    m_hideAnimation = CXmlHelper::GetAttributeValueOrUseDefault(node, "hideAnimation", "hide");
    m_aimAnimation = CXmlHelper::GetAttributeValueOrUseDefault(node, "aimAnimation", "scope");
    m_unusedAnimation = CXmlHelper::GetAttributeValueOrUseDefault(node, "unusedAnimation", "");
    m_throwAnimation = CXmlHelper::GetAttributeValueOrUseDefault(node, "throwAnimation", "");
}

void CDH_Weapon::parseStats(TiXmlNode* node)
{
    m_handsSeparated = CXmlHelper::GetAttributeValueOrUseDefault(node, "hands_are_separated", "0").ToBool();
    m_shotDeferringTime = CXmlHelper::GetAttributeValueOrUseDefault(node, "shot_deferring_time", "-1").ToFloat();
    m_projectileType = CXmlHelper::GetAttributeValueOrUseDefault(node, "projectile_type", "0").ToInt();
    m_shootType = CXmlHelper::GetAttributeValue(node, "shoot_type").ToInt();
    m_clipSize = CXmlHelper::GetAttributeValue(node, "num_of_shots").ToInt();
    m_baseRateOfFire = m_rateOfFire = CXmlHelper::GetAttributeValue(node, "rate_of_fire_ms").ToInt();
    m_baseReloadTime = m_reloadTime = CXmlHelper::GetAttributeValue(node, "reloading_time_ms").ToInt();
    m_bonusXp = CXmlHelper::GetAttributeValue(node, "bonus_xp").ToInt();
    m_scopeAttached = CXmlHelper::GetAttributeValue(node, "scopeAttached").ToBool();
    m_zoomFovMin = CXmlHelper::GetAttributeValueOrUseDefault(node, "zoom_fov_min", "52.5").ToFloat();
    m_zoomFovMax = CXmlHelper::GetAttributeValueOrUseDefault(node, "zoom_fov_max", "52.5").ToFloat();
    m_aimZoomFov = CXmlHelper::GetAttributeValueOrUseDefault(node, "aim_zoom_fov", "52.5").ToFloat();
    m_scopeHudModel = CXmlHelper::GetAttributeValue(node, "scope_hud_model");
    m_countInQueue = CXmlHelper::GetAttributeValue(node, "count_in_queue").ToInt();
    m_minAimDispersion = CXmlHelper::GetAttributeValueOrUseDefault(node, "minAimDispersion", "0").ToFloat();
    m_maxAimDispersion = CXmlHelper::GetAttributeValueOrUseDefault(node, "maxAimDispersion", "15").ToFloat();
    m_aimDispersionPerShoot = CXmlHelper::GetAttributeValueOrUseDefault(node, "aimDispersionPerShoot", "5").ToFloat();
    m_aimDispersionPerTurnAngle = CXmlHelper::GetAttributeValueOrUseDefault(node, "aimDispersionPerTurnAngle", "1").ToFloat();
    m_minAimRecoverySpeed = CXmlHelper::GetAttributeValueOrUseDefault(node, "minAimRecoverySpeed", "0").ToFloat();
    m_maxAimRecoverySpeed = CXmlHelper::GetAttributeValueOrUseDefault(node, "maxAimRecoverySpeed", "15").ToFloat();
    m_accuracyDesc = CXmlHelper::GetAttributeValue(node, "accuracy_desc");
    m_damageDesc = CXmlHelper::GetAttributeValue(node, "damage_desc");
    m_headshotMultiplier = CXmlHelper::GetAttributeValue(node, "headshot_mult").ToFloat();
    m_baseThrowDelay = m_throwDelay = CXmlHelper::GetAttributeValue(node, "throw_delay").ToInt();

    // the dispersion range must not be empty
    if (m_maxAimDispersion <= m_minAimDispersion) {
        m_maxAimDispersion = m_minAimDispersion + 0.01f;
    }

    m_unlockLevel = CXmlHelper::GetAttributeValue(node, "unlockLevel").ToInt();
    m_fetched = false;
    m_idle = false;
    m_damageMin = CXmlHelper::GetAttributeValue(node, "damageMin").ToInt();
    m_damageMax = CXmlHelper::GetAttributeValue(node, "damageMax").ToInt();
    m_firePower[0] = 100;
    m_firePower[1] = CXmlHelper::GetAttributeValue(node, "firePower10").ToInt();
    m_firePower[2] = CXmlHelper::GetAttributeValue(node, "firePower20").ToInt();
    m_firePower[3] = CXmlHelper::GetAttributeValue(node, "firePower30").ToInt();
    m_firePower[4] = CXmlHelper::GetAttributeValue(node, "firePower40").ToInt();
    m_firePower[5] = CXmlHelper::GetAttributeValue(node, "firePower50").ToInt();
    m_weaponType = CXmlHelper::GetAttributeValue(node, "weapon_type").ToInt();
    switch (m_weaponType) {
    case 2:
    case 7:
    case 8:
    case 9:
        // always automatic
        m_shootType = 2;
        break;
    }
}

void CDH_Weapon::parseGraphics(TiXmlNode* node)
{
    XString model = CXmlHelper::GetAttributeValue(node, "model");
    XString texture;
    texture = CXmlHelper::GetAttributeValueOrUseDefault(node, "texture", "");
    m_textureNodeId = SwerveHelper::Id(
        XString::AnsiString(CXmlHelper::GetAttributeValueOrUseDefault(node, "textureNode", "").m_data));
    XString imageGame = CXmlHelper::GetAttributeValue(node, "image_game");
    XString imageShop = CXmlHelper::GetAttributeValue(node, "image_shop");
    XString imageBulletOn = CXmlHelper::GetAttributeValue(node, "image_bullet_on");
    XString imageBulletOff = CXmlHelper::GetAttributeValue(node, "image_bullet_off");
    XString imageRecommended = CXmlHelper::GetAttributeValue(node, "image_rec");
    XString imageBig = CXmlHelper::GetAttributeValue(node, "image_rec");
    m_spriteScopeArch = CXmlHelper::GetAttributeValue(node, "sprite_scope_arch").ToInt();

    m_modelId = GetApp()->m_resMap->getResourceID(model);
    m_textureId = NULL;
    if (texture.Length() != 0) {
        m_textureId = GetApp()->m_resMap->getResourceID(texture);
    }
    m_gameImageId = GetApp()->m_resMap->getResourceID(imageGame);
    m_shopImageId = GetApp()->m_resMap->getResourceID(imageShop);
    m_recommendedImageId = GetApp()->m_resMap->getResourceID(imageRecommended);
    m_bigImageId = GetApp()->m_resMap->getResourceID(imageBig);
}

void CDH_Weapon::parseSounds(TiXmlNode* node)
{
    XString shootSound = CXmlHelper::GetAttributeValue(node, "shootSound");
    XString reloadSound = CXmlHelper::GetAttributeValue(node, "reloadSound");
    XString noAmmoSound = CXmlHelper::GetAttributeValue(node, "noAmmoSound");
    XString switchSound = CXmlHelper::GetAttributeValue(node, "switchSound");
    m_shootSound = CDH_SoundManager::GetSoundFromResourceID(GetApp()->m_resMap->getResourceID(shootSound));
    m_reloadSound = CDH_SoundManager::GetSoundFromResourceID(GetApp()->m_resMap->getResourceID(reloadSound));
    m_noAmmoSound = CDH_SoundManager::GetSoundFromResourceID(GetApp()->m_resMap->getResourceID(noAmmoSound));
    m_switchSound = CDH_SoundManager::GetSoundFromResourceID(GetApp()->m_resMap->getResourceID(switchSound));
}

CDH_Weapon::ItemDesc::~ItemDesc()
{
    GetApp()->m_resourceManager->releaseResource(m_resourceId);
}

CDH_Weapon::ItemsGroup::~ItemsGroup()
{
    for (int index = 0; index < m_items.size(); index++) {
        if (m_items[index] != NULL) {
            delete m_items[index];
            m_items[index] = NULL;
        }
    }
}

void CDH_Weapon::setAvailable(bool available)
{
    m_available.Set(available);
    Save(true);
}

void CDH_Weapon::Save(bool saveNow)
{
    TiXmlElement* weapons = CXmlHelper::ObtainElement(GetApp()->m_saveManager->GetRootElement()->ToElement(),
                                                      XString("weapons"));
    XString name = XString::Format(L"w%d", getId());
    TiXmlElement* element = CXmlHelper::ObtainElement(weapons, name);
    element->SetAttribute("a", isAvailable());
    if (saveNow) {
        GetApp()->m_saveManager->SaveOnNextUpdate(false);
    }
}

void CDH_Weapon::releaseResources()
{
    GetApp()->m_surfaceManager->ReleaseSurface(m_gameImageId);
    GetApp()->m_surfaceManager->ReleaseSurface(m_shopImageId);
    GetApp()->m_surfaceManager->ReleaseSurface(m_recommendedImageId);
    GetApp()->m_surfaceManager->ReleaseSurface(m_bigImageId);
}

ICRenderSurface* CDH_Weapon::gameImage() const
{
    ICRenderSurface* surface = GetApp()->m_surfaceManager->GetFromCache(m_gameImageId);
    if (surface == NULL) {
        surface = GetApp()->m_surfaceManager->CreateSurface(m_gameImageId, true, false);
    }
    return surface;
}

ICRenderSurface* CDH_Weapon::shopImage() const
{
    ICRenderSurface* surface = GetApp()->m_surfaceManager->GetFromCache(m_shopImageId);
    if (surface == NULL) {
        surface = GetApp()->m_surfaceManager->CreateSurface(m_shopImageId, true, false);
    }
    return surface;
}

ICRenderSurface* CDH_Weapon::recommendedImage() const
{
    ICRenderSurface* surface = GetApp()->m_surfaceManager->GetFromCache(m_recommendedImageId);
    if (surface == NULL) {
        surface = GetApp()->m_surfaceManager->CreateSurface(m_recommendedImageId, true, false);
    }
    return surface;
}

ICRenderSurface* CDH_Weapon::bigImage() const
{
    ICRenderSurface* surface = GetApp()->m_surfaceManager->GetFromCache(m_bigImageId);
    if (surface == NULL) {
        surface = GetApp()->m_surfaceManager->CreateSurface(m_bigImageId, true, false);
    }
    return surface;
}

int CDH_Weapon::CheckForReload(bool force)
{
    if (!force && m_ammo > 0) {
        return 1;
    }
    if (m_projectileType == 2) {
        return 0;
    }
    if (m_state == 2) {
        // scope out first, reload afterwards
        m_reloadPending = true;
        ScopeOut();
    } else {
        PlayReloadAnimation(m_ammo);
        StartAction(3, m_reloadTime);
        playReloadSound();
    }
    return 1;
}

void CDH_Weapon::AfterStateChanged()
{
    switch (m_state) {
    case 8:
        // grenade thrown: back to the scope when it was on
        if (m_throwPending) {
            ScopeIn();
        } else {
            m_state = 1;
        }
        return;
    case 3:
        // reload finished
        if (m_weaponType != WEAPON_TYPE_SHOTGUN) {
            m_ammo = m_clipSize;
        }
        m_autoShotsLeft = 0;
        m_autoShot = false;
        if (m_interruptingReload) {
            m_reloadPending = false;
            m_throwPending = false;
            m_state = 1;
            return;
        }
        if (m_reloadPending) {
            ScopeIn();
        } else {
            m_state = 1;
        }
        return;
    case 4:
        m_state = 1;
        CheckForReload(false);
        return;
    case 5:
        m_state = 2;
        CheckForReload(false);
        return;
    case 6:
        m_state = 2;
        GetApp()->m_helper->m_swerveGame->onSwitchToScope();
        return;
    case 7:
        if (m_reloadPending) {
            PlayReloadAnimation(m_ammo);
            StartAction(3, m_reloadTime);
            playReloadSound();
        } else {
            bool throwPending = m_throwPending;
            m_state = 1;
            if (throwPending) {
                GetApp()->m_helper->m_swerveGame->GetHUD()->ThrowGrenade();
            }
        }
        GetApp()->m_helper->m_swerveGame->onSwitchFromScope();
        m_fetched = true;
        return;
    }
}

void CDH_Weapon::ScopeIn()
{
    switch (m_state) {
    case 1:
    case 3:
    case 8:
        PlayScopeAnimation(false);
        StartAction(6, m_scopeTime);
        if (m_scopeAttached) {
            GetApp()->m_soundManager->Play(SOUND_SCOPE_IN, false, false);
        }
        if (m_reloadPending) {
            m_reloadPending = false;
        }
        if (m_throwPending) {
            m_throwPending = false;
        }
        GetApp()->m_helper->m_swerveGame->m_camera->SetFOV(m_aimZoomFov);
        GetApp()->m_helper->m_swerveGame->OnScopeSwitch();
        break;
    }
}

bool CDH_Weapon::IsReadyForAutoShot()
{
    return m_shootType == 2 && (m_state == 1 || m_state == 2) && m_action == 0 && m_autoShot;
}

int CDH_Weapon::IsReadyForUse()
{
    return (m_state == 1 || m_state == 2) && m_action == 0;
}

int CDH_Weapon::IsReloading()
{
    if (m_reloadPending) {
        return 1;
    }
    return m_action == 3;
}

int CDH_Weapon::MakeShot()
{
    if (m_weaponType == WEAPON_TYPE_MINIGUN) {
        // the minigun spins up before it shoots
        m_spinUpTime = 0.0f;
        float speed = m_spinSpeed + GetApp()->m_frameTimeSeconds * 0.75f;
        m_spinSpeed = (1.0f < speed) ? 1.0f : speed;
        if (m_spinSpeed < 1.0f) {
            if (m_animation->m_currentController == CONTROLLER_SPIN) {
                return 0;
            }
            ResetModelAnimation();
            if (m_scopeAnimation != NULL) {
                m_scopeAnimation->stopAllAnimation();
            }
            if (m_animation != NULL) {
                m_animation->setCurrentController(CONTROLLER_SPIN, false);
            }
            return 0;
        }
        ICMediaPlayer::GetInstance()->Vibrate(m_rateOfFire, 0);
        DGCamera* camera = GetApp()->m_helper->m_swerveGame->m_camera;
        if (camera->GetShakeTimeLeft() <= 0.0f) {
            GetApp()->m_helper->m_swerveGame->m_camera->SetShake((float)m_rateOfFire * 0.001f, 50.0f, 0.025f);
        }
    }

    if (m_state == 1 || m_state == 2) {
        if (m_ammo < 1) {
            GetApp()->m_soundManager->Play(SOUND_NO_AMMO, false, false);
            m_autoShotsLeft = 0;
            m_autoShot = false;
            CheckForReload(false);
            return 0;
        }
        if (!GetApp()->m_settings->IsFireButtonEnabled()) {
            HandleAutoShot();
        }
        int ammo = m_ammo;
        m_ammo = ammo - 1;
        if (m_weaponType == WEAPON_TYPE_MINIGUN || m_weaponType == WEAPON_TYPE_HEAVY) {
            m_ammo = ammo - 2;
        }
        PlayShootAnimation();
        GetApp()->m_soundManager->Play((CDH_SoundManager::eSound)m_shootSound, false, false);
        m_actionTimeLeft = m_rateOfFire;
        if (m_state == 1) {
            m_action = 4;
        } else {
            m_action = 5;
        }
        return 1;
    }
    return 0;
}

void CDH_Weapon::ResetAutoShot()
{
    m_autoShotsLeft = 0;
    m_autoShot = false;
}

void CDH_Weapon::HandleAutoShot()
{
    if (m_shootType == 2) {
        if (!m_autoShot) {
            m_autoShot = true;
            m_autoShotsLeft = m_ammo;
        }
        if (m_autoShot) {
            m_autoShotsLeft--;
            if (m_autoShotsLeft < 1) {
                m_autoShotsLeft = 0;
                m_autoShot = false;
            }
        }
    }
}

void CDH_Weapon::PlayShootAnimation()
{
    if (m_scopeAnimation != NULL) {
        m_scopeAnimation->stopAllAnimation();
    }
    if (m_animation != NULL) {
        m_animation->setCurrentController(CONTROLLER_SHOOT, false);
    }
    m_idle = false;
}

void CDH_Weapon::PlayScopeAnimation(bool out)
{
    ResetModelAnimation();
    if (m_animation != NULL) {
        m_animation->stopAllAnimation();
    }
    if (m_scopeAnimation != NULL) {
        m_scopeAnimation->setCurrentController(0, out);
    }
    m_idle = false;
    m_fetched = !out;
}

void CDH_Weapon::ScopeOut()
{
    if (m_state == 2) {
        if (m_action == 7) {
            return;
        }
        GetApp()->m_helper->m_swerveGame->m_camera->restoreView(true);
        GetApp()->m_helper->m_swerveGame->OnScopeSwitch();
        PlayScopeAnimation(true);
        StartAction(7, m_scopeTime);
        if (m_scopeAttached) {
            GetApp()->m_soundManager->Play(SOUND_SCOPE_OUT, false, false);
        }
        GetApp()->m_helper->m_swerveGame->m_camera->restoreView(true);
        GetApp()->m_helper->m_swerveGame->OnScopeSwitch();
    }
}

int CDH_Weapon::Throw(int& throwTime)
{
    if (m_state == 2) {
        // scope out first, throw afterwards
        m_throwPending = true;
        ScopeOut();
        return 0;
    }
    if (m_action == 8) {
        return 0;
    }
    CGrenade* grenade = GetApp()->m_grenadeManager->GetActiveGrenade();
    if (grenade == NULL) {
        return 0;
    }
    if (grenade->GetCount() <= 0) {
        return 0;
    }
    int count = grenade->m_count.Get();
    count--;
    grenade->m_count.Set(count);
    int time = m_throwTime;
    if (grenade->m_throwTime > m_throwTime) {
        time = grenade->m_throwTime;
    }
    throwTime = time;
    PlayThrowAnimation();
    m_actionTimeLeft = m_throwTime;
    int delay = m_throwDelay;
    if (m_throwDelay > m_throwTime - 1) {
        delay = m_throwTime - 1;
    }
    m_throwDelayLeft = delay;
    m_action = 8;
    m_grenadePending = true;
    GetApp()->m_soundManager->Play(SOUND_GRENADE_THROW, false, false);
    return 1;
}

void CDH_Weapon::PlayThrowAnimation()
{
    ResetModelAnimation();
    if (m_scopeAnimation != NULL) {
        m_scopeAnimation->stopAllAnimation();
    }
    if (m_animation != NULL) {
        m_animation->setCurrentController(CONTROLLER_THROW, false);
    }
    m_idle = false;
}

void CDH_Weapon::PlayReloadAnimation(int ammo)
{
    ResetModelAnimation();
    if (m_scopeAnimation != NULL) {
        m_scopeAnimation->stopAllAnimation();
    }
    if (m_animation != NULL) {
        if (m_weaponType == WEAPON_TYPE_SHOTGUN) {
            // the reload animation for the number of missing shells
            unsigned int shells = m_clipSize;
            if (shells > 6) {
                shells = 6;
            }
            int controller = shells - ammo + 4;
            if ((int)(shells - ammo - 1) < 0) {
                controller = CONTROLLER_RELOAD;
            }
            m_animation->setCurrentController(controller, false);
            if (GetApp()->m_powerUpManager->IsPowerUpRunning() == true) {
                float reloadSpeed = GetApp()->m_powerUpManager->GetActivePowerUp()->m_reloadSpeed;
                m_baseReloadTime = m_reloadTime =
                    (int)(1.0f / reloadSpeed * (float)m_animation->getControllerBaseDuration(controller));
            } else {
                m_baseReloadTime = m_reloadTime = m_animation->getControllerBaseDuration(controller);
            }
            m_animation->setControllerDuration(controller, m_reloadTime);
        } else {
            m_animation->setCurrentController(CONTROLLER_RELOAD, false);
        }
    }
    m_idle = false;
}

void CDH_Weapon::InterruptReloading()
{
    if (!m_reloadPending && m_action != 3) {
        return;
    }
    m_interruptingReload = true;
    int ammo = m_ammo;
    UpdateState(m_actionTimeLeft);
    UpdateState(0);
    m_animation->setRelativePosition(1.0f);
    m_animation->animate();
    m_ammo = ammo;
    UpdateState(0);
    m_action = 0;
    m_interruptingReload = false;
    if (m_weaponType == WEAPON_TYPE_SHOTGUN && m_reloadingSteps.size() != 0) {
        GetApp()->m_soundManager->Stop((CDH_SoundManager::eSound)m_reloadingSteps[0].m_sound);
        m_reloadingSteps.removeAllElements();
        m_reloadingStepStarted = 0;
    } else {
        GetApp()->m_soundManager->Stop((CDH_SoundManager::eSound)m_reloadSound);
    }
}

int CDH_Weapon::Buy()
{
    if (m_available.Get()) {
        return 1;
    }
    int bought = CItem::Buy();
    if (bought != 1) {
        return 0;
    }
    XString name = GetName();
    int pricePoints = getPricePoints();
    int priceCredits = getPriceCredits();
    // the survival weapon check is repeated in both branches (as the original)
    if (pricePoints != 0) {
        CGameAnalytics::logCurrencyChange(name, CBH_Player::GetInstance()->GetStats(PLAYER_STATS_SOFT_CURRENCY),
                                          -pricePoints, CGameAnalytics::CURRENCY_SOFT);
        CGameAnalytics::logItemPurchased(name, pricePoints, CGameAnalytics::CURRENCY_SOFT);
        const XString& survivalWeapon = GetApp()->m_huntingInfo->m_text04;
        const XString& itemId = GetStringUId();
        if (XString::CmpData(survivalWeapon.m_data, survivalWeapon.ByteSize(), itemId.m_data, itemId.ByteSize(), 0x7fffffff) == 0) {
            CGameAnalytics::logSurvivalModeWeaponPurchased(CBH_Player::GetInstance()->m_id, getId());
        }
    } else if (priceCredits != 0) {
        CGameAnalytics::logCurrencyChange(name, CBH_Player::GetInstance()->GetStats(PLAYER_STATS_SOFT_CURRENCY),
                                          -priceCredits, CGameAnalytics::CURRENCY_HARD);
        CGameAnalytics::logItemPurchased(name, priceCredits, CGameAnalytics::CURRENCY_HARD);
        const XString& survivalWeapon = GetApp()->m_huntingInfo->m_text04;
        const XString& itemId = GetStringUId();
        if (XString::CmpData(survivalWeapon.m_data, survivalWeapon.ByteSize(), itemId.m_data, itemId.ByteSize(), 0x7fffffff) == 0) {
            CGameAnalytics::logSurvivalModeWeaponPurchased(CBH_Player::GetInstance()->m_id, getId());
        }
    }
    CBH_Player::GetInstance()->m_gameStats.AddWeaponsEarned(1);
    WindowApp::HandleTunnelCommand(0x9a412292, m_id, 0, 0);
    setAvailable(true);
    return bought;
}

int CDH_Weapon::GetShotDamage(float distance) const
{
    int damage = CRandGen::GetInstance()->GetRandRange(m_damageMin, m_damageMax);
    int range;
    if (distance >= 50.0f) {
        range = 5;
    } else if (distance >= 40.0f) {
        range = 4;
    } else if (distance >= 30.0f) {
        range = 3;
    } else if (distance >= 20.0f) {
        range = 2;
    } else if (distance >= 10.0f) {
        range = 1;
    } else {
        range = 0;
    }
    if (GetApp()->m_powerUpManager->IsPowerUpRunning() == true) {
        damage = (int)((float)damage * GetApp()->m_powerUpManager->GetActivePowerUp()->m_damageMultiplier);
    }
    return (int)((float)damage * ((float)m_firePower[range] / 100.0f));
}

bool CDH_Weapon::CheckFilter(int filter)
{
    int projectileType = m_projectileType;
    if (filter == 5 && projectileType == 2) {
        return true;
    }
    if (filter == 2 && (m_weaponType == 8 || m_weaponType == 9)) {
        return true;
    }
    if (filter == 1 && (m_weaponType == 0 || m_weaponType == 7)) {
        return projectileType != 2;
    }
    if (projectileType == 2) {
        return false;
    }
    if ((projectileType == 2) & (filter == 5)) {
        return true;
    }
    if (filter < 0) {
        return true;
    }
    return m_weaponType == filter;
}

bool CDH_Weapon::IsEquiped() const
{
    DGHelper* helper = GetApp()->m_helper;
    if (helper->m_gamePlayManager.GetGameData()->GetWeapon(0) == this) {
        return true;
    }
    return GetApp()->m_helper->m_gamePlayManager.GetGameData()->GetWeapon(1) == this;
}

bool CDH_Weapon::IsOwned() const
{
    return isAvailable();
}

void CDH_Weapon::SetAnimationControllerSpeedMultiplier(int controller, float multiplier)
{
    float speed = m_animation->getControllerSpeed(controller) * multiplier;
    m_animation->setControllerSpeed(controller, speed);
    if (m_animation->m_currentController == controller) {
        m_actionTimeLeft = (int)((float)m_actionTimeLeft / multiplier);
    }
}

void CDH_Weapon::SetInertionAngle(float yaw, float pitch)
{
    float pitchRotation[4];
    float yawRotation[4];
    float matrix[16];
    m_baseTransform.get(matrix);
    m_inertionTransform.set(matrix);
    yawRotation[0] = yaw;
    yawRotation[1] = 0.0f;
    yawRotation[2] = 1.0f;
    yawRotation[3] = 0.0f;
    m_inertionTransform.postRotate(yawRotation);
    pitchRotation[0] = pitch;
    pitchRotation[1] = 1.0f;
    pitchRotation[2] = 0.0f;
    pitchRotation[3] = 0.0f;
    m_inertionTransform.postRotate(pitchRotation);
    m_eyecamTransform.setTransform(m_inertionTransform);
}

vec3 CDH_Weapon::GetTracerEmitterPos(int barrel, swerve::Camera<CSwerveAssertingErrorHandler> camera,
                                     swerve::World<CSwerveAssertingErrorHandler> world)
{
    m_barrels[barrel].getTransformTo(m_eyeCamera, m_barrelTransform);
    camera.getTransformTo(world, m_cameraTransform);
    m_cameraTransform.postMultiply(m_barrelTransform);
    float matrix[16];
    m_cameraTransform.get(matrix);
    return vec3(matrix[3], matrix[7], matrix[11]);
}
