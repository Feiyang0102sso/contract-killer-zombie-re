// k: original 150a60..1527e4 complete GameplayHUD; recovered following the original re-read timing and tree/resource ownership.
#include "CGameplayHUD.h"
#include "App.h"
#include "AppSettings.h"
#include "CSwerveGame.h"
#include "CUnitsController.h"
#include "CUnit.h"
#include "DGHelper.h"
#include "dgCamera.h"
#include "CFpsCursor.h"
#include "CDH_Weapon.h"
#include "CDH_SoundManager.h"
#include "CGrenadeManager.h"
#include "CGrenade.h"
#include "CPowerUpManager.h"
#include "CPowerUp.h"
#include "CTutorialManager.h"
#include "SScreenAdaptingConstant.h"
#include "CFontMgr.h"
#include "MathLib.h"
#include <math.h>
#include <string.h>
using namespace com::glu::platform::graphics;
using namespace com::glu::platform::components;

// The original actually uses 4ff990, the complete six groups of 96B; must not adopt 4f07b0, the same-named MergedGlobals from ref.
static SScreenAdaptingConstant HUD_MARGIN_TOP(5);
static SScreenAdaptingConstant HUD_MARGIN_SIDE(10);
static SScreenAdaptingConstant HUD_FIRE_MARGIN(10);
static SScreenAdaptingConstant HUD_SCOPE_MARGIN(10);
static SScreenAdaptingConstant HUD_ARROW_X(60);
static SScreenAdaptingConstant HUD_ARROW_Y(40);

CGameplayHUD::CGameplayHUD(CSwerveGame* game) : Window(false), m_flagBc(false),
    m_unresolvedBd(false), m_scopePointerId(-1), m_unresolvedC4(0), m_unresolvedC8(0),
    m_canSwitchWeapons(true), m_switchAfterScope(false), m_grenadeTimeLeft(0),
    m_grenadeDuration(1), m_grenade(NULL), m_powerUp(NULL), m_directionTime(0),
    m_game(game), m_directionUnitId(-1), m_slider(NULL), m_firePointerId(-1),
    m_firePressed(false), m_hideWeaponModel(false), m_zoomedScope(NULL),
    m_tutorialArrow(NULL), m_unresolved13c(-1), m_unresolved140(-1),
    m_steadyAim(false), m_flag145(false), m_value148(0)
{
    m_controlsLayer = new WindowTransparent(false);
    AddToFront(m_controlsLayer);
    m_unitBarsLayer = new WindowTransparent(false);
    m_unitBarsLayer->SetFlags(0x10000000);
    m_unitBarsLayer->ClearFlags(1);
    AddToFront(m_unitBarsLayer);
    InitCommon();
    InitFireButton();
    InitWeaponButton();
    InitScopeButton();
    m_hpBar = NULL; // The original writes f0 only after all four Inits complete.
    m_directionArrow = new ArrowMarkerWin;
    AddToFront(m_directionArrow);
}

void CGameplayHUD::InitCommon()
{
    m_pauseButton = new SGImageWindowCommand;
    m_pauseButton->SetArchetypeCharacter(0, 0);
    m_pauseButton->SetAnimation(6, 1);
    m_pauseButton->SetAlign(9);
    SGImageWindowCommand* commandButton = m_pauseButton;
    commandButton->m_command = 0x9247fd7b;
    commandButton->m_commandParameter1 = 0;
    commandButton->m_commandParameter2 = 0;
    commandButton->m_commandParameter3 = 0;
    SGImageWindowCommand* pause = m_pauseButton;
    int paddingX = SScreenAdaptingConstant(20)();
    int paddingY = SScreenAdaptingConstant(20)();
    pause->m_hitPaddingX = paddingX;
    pause->m_hitPaddingY = paddingY;
    pause = m_pauseButton;
    int top = HUD_MARGIN_TOP();
    int left = HUD_MARGIN_SIDE();
    pause->SetOutsetSpacing(top, 0, 0, left);
    AddToFront(m_pauseButton);
    m_flash = new FlashingEffectWindow;
    m_flash->SetArchetypeCharacter(0, 0);
    m_flash->SetLayoutType(LAYOUT_NONE);
    AddToFront(m_flash);
    m_zoomedScope = new CZoomedScopeWindow;
}

void CGameplayHUD::InitFireButton()
{
    m_fireButton = new SGHudImageWindow;
    m_fireButton->SetArchetypeCharacter(0, 0);
    m_fireButton->SetAnimation(5, 1);
    m_fireButton->SetAlign(18);
    SGHudImageWindow* fire = m_fireButton;
    int right = HUD_FIRE_MARGIN();
    int bottom = HUD_SCOPE_MARGIN();
    fire->SetOutsetSpacing(0, right, bottom, 0);
    m_controlsLayer->AddToFront(m_fireButton);
    CGrenade* grenade = ((App*)WindowApp::m_instance)->m_grenadeManager->GetActiveGrenade();
    if (grenade != NULL && grenade->GetCount() >= 1) {
        m_grenade = new ConsumableHudImageWindow(grenade->m_hudImage);
        m_grenade->SetAlign(18);
        ConsumableHudImageWindow* target = m_grenade;
        int inset = (int)(WindowApp::m_instance->m_screenWidth * 0.33f);
        int lower = HUD_SCOPE_MARGIN();
        target->SetOutsetSpacing(0, inset, lower, 0);
        ProgressFillHudImageWindow* fill = m_grenade->m_fill;
        fill->m_blendGreen = 0.0f;
        fill->m_blendBlue = 0.0f;
        fill->m_blendAlpha = 0.0f;
        m_controlsLayer->AddToFront(m_grenade);
    }
    CPowerUp* powerUp = ((App*)WindowApp::m_instance)->m_powerUpManager->GetActivePowerUp();
    if (powerUp != NULL && powerUp->GetCount() >= 1) {
        m_powerUp = new ConsumableHudImageWindow(powerUp->m_hudImageResource);
        m_powerUp->SetAlign(10);
        ConsumableHudImageWindow* target = m_powerUp;
        int inset = (int)(WindowApp::m_instance->m_screenHeight * 0.33f);
        int side = HUD_MARGIN_SIDE();
        target->SetOutsetSpacing(inset, side, 0, 0);
        ProgressFillHudImageWindow* fill = m_powerUp->m_fill;
        fill->m_blendGreen = 1.0f;
        fill->m_blendBlue = 1.0f;
        fill->m_blendAlpha = 0.0f;
        m_controlsLayer->AddToFront(m_powerUp);
    }
}

void CGameplayHUD::InitWeaponButton()
{
    CDH_Weapon* first = ((App*)WindowApp::m_instance)->m_helper->m_gamePlayManager.GetGameData()->GetWeapon(0);
    CDH_Weapon* second = ((App*)WindowApp::m_instance)->m_helper->m_gamePlayManager.GetGameData()->GetWeapon(1);
    if (first == second || first == NULL || second == NULL) {
        m_canSwitchWeapons = false;
    }
    m_switchButton = new SGHudImageWindow;
    m_switchButton->SetArchetypeCharacter(0, 0);
    m_switchButton->SetAnimation(3, 1);
    m_switchButton->SetAlign(18);
    int lower = (int)(WindowApp::m_instance->m_screenHeight * 0.29f);
    SGHudImageWindow* button = m_switchButton;
    int right = HUD_MARGIN_SIDE();
    button->SetOutsetSpacing(0, right, lower, 0);
    m_controlsLayer->AddToFront(m_switchButton);
    m_weaponInfo = new CGameplayWeaponInfoWindow;
    m_weaponInfo->SetAlign(10);
    CGameplayWeaponInfoWindow* info = m_weaponInfo;
    int top = HUD_MARGIN_TOP();
    int side = HUD_MARGIN_SIDE();
    info->SetOutsetSpacing(top, side, 0, 0);
    m_controlsLayer->AddToFront(m_weaponInfo);
}

void CGameplayHUD::InitScopeButton()
{
    m_scopeButton = new SGImageWindowCommand;
    SGImageWindowCommand* commandButton = m_scopeButton;
    commandButton->m_command = 0x31577148;
    commandButton->m_commandParameter1 = 0;
    commandButton->m_commandParameter2 = 0;
    commandButton->m_commandParameter3 = 0;
    m_scopeButton->SetArchetypeCharacter(0, 0);
    m_scopeButton->SetAnimation(2, 1);
    m_scopeButton->SetAlign(17);
    SGImageWindowCommand* button = m_scopeButton;
    int bottom = HUD_SCOPE_MARGIN();
    int left = HUD_FIRE_MARGIN();
    button->SetOutsetSpacing(0, 0, bottom, left);
    m_controlsLayer->AddToFront(m_scopeButton);
}

CGameplayHUD::~CGameplayHUD()
{
    if (m_zoomedScope != NULL) {
        delete m_zoomedScope;
        m_zoomedScope = NULL;
    }
    CDH_Weapon* first = ((App*)WindowApp::m_instance)->m_helper->m_gamePlayManager.GetGameData()->GetWeapon(0);
    CDH_Weapon* second = ((App*)WindowApp::m_instance)->m_helper->m_gamePlayManager.GetGameData()->GetWeapon(1);
    if (first != NULL) {
        first->releaseResources();
    }
    if (second != NULL) {
        second->releaseResources();
    }
    // Vector only frees its own buffer; all civilian bars and ordinary controls belong to the Window tree.
}

void CGameplayHUD::UpdateHUDsVisibility()
{
    if (!m_flag145) {
        return;
    }
    Window* layer = m_controlsLayer;
    if (((App*)WindowApp::m_instance)->m_settings->m_field1e) {
        layer->SetFlags(1);
    } else {
        layer->ClearFlags(1);
    }
}

// Original clamp keeps NaN on the upper-bound selection path.
static inline float ClampHPBarScale(float scale)
{
    if (scale < 0.4f) {
        return 0.4f;
    }
    return (1.0f < scale) ? 1.0f : scale; // Original conditional VMIN, as in ClampOpacity.
}

void CGameplayHUD::UpdateUnitHPBars()
{
    CUnitsController* controller = m_game->m_unitsController;
    while (m_civilianBars.size() - controller->m_hudUnits.size() > 0) {
        m_civilianBars[0]->Close();
        m_civilianBars.removeElementAt(0);
    }
    int added = controller->m_hudUnits.size() - m_civilianBars.size();
    for (int index = 0; index < added; index++) {
        // The original empty-name local lives until after the whole bar is constructed before being released; the NameBar argument lifetime is not ended early.
        CCivilianBar* bar = new CCivilianBar(new CCivilianHPBar, new CCivilianNameBar(XString(L"")));
        bar->SetLayoutType(LAYOUT_NONE);
        m_unitBarsLayer->AddToFront(bar);
        m_civilianBars.addElement(bar);
    }
    for (int index = 0; index < controller->m_hudUnits.size(); index++) {
        CUnitsController::SHudUnitInfo* items = controller->m_hudUnits.m_items;
        CUnitsController::SHudUnitInfo* item = &items[index];
        float scale = 15.0f / item->m_cameraDistance;
        scale = ClampHPBarScale(scale);
        m_civilianBars[index]->GetHPBar()->SetScale(scale);
        m_civilianBars[index]->GetHPBar()->SetPercent((float)item->m_hp / (float)item->m_maxHp);
        float green = 0.0f;
        if (item->m_civilian) {
            green = 1.0f;
        }
        Color_RGBA color = {1.0f, green, 1.0f, 1.0f};
        m_civilianBars[index]->GetHPBar()->SetColor(color);
        m_civilianBars[index]->SetCenter(items[index].m_screenX, item->m_screenY);
    }
}

void CGameplayHUD::Update()
{
    int elapsed = WindowApp::m_instance->m_updateFrameTime;
    ((App*)WindowApp::m_instance)->m_powerUpManager->Update(elapsed);
    if (m_grenadeTimeLeft >= 1) {
        m_grenadeTimeLeft -= elapsed;
    }
    SGHudImageWindow* fire = m_fireButton;
    if (((App*)WindowApp::m_instance)->m_settings->IsFireButtonEnabled() == 1) {
        fire->SetFlags(1);
    } else {
        fire->ClearFlags(1);
    }
    fire = m_fireButton;
    if (((App*)WindowApp::m_instance)->m_settings->IsFireButtonEnabled() == 1) {
        fire->SetFlags(0x2000);
    } else {
        fire->ClearFlags(0x2000);
    }
    m_unitBarsLayer->HandleUpdate(true);
    UpdateHUDsVisibility();
    UpdateUnitHPBars();
    UpdateUnitDirectionArrow();
    if (((App*)WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial() == 1) {
        UpdateTutorialBlinking();
    }
    if (m_slider != NULL && m_zoomedScope != NULL && m_zoomedScope->m_enabled) {
        if (!m_zoomedScope->m_shooting) {
            float ratio = 1.0f - m_slider->GetValue();
            if (ratio < 0.0f) {
                ratio = 0.0f;
            }
            m_zoomedScope->m_zoomPosition = (m_zoomedScope->m_zoom->m_durationSec * ratio) * 0.5f;
        }
        m_zoomedScope->Update();
    }
    CGameplayHPBar* hp = m_hpBar;
    if (hp != NULL) {
        CUnitsController* controller = m_game->m_unitsController;
        if (controller->m_civiliansMaxHp >= 1) {
            hp->SetPercent((float)controller->m_civiliansHp / (float)controller->m_civiliansMaxHp);
        }
    }
    if (m_powerUp != NULL) {
        float progress = ((App*)WindowApp::m_instance)->m_powerUpManager->GetPowerUpLeftPercent();
        m_powerUp->m_fill->m_progress = progress;
        m_powerUp->m_fill->SetIsAnimating(progress > 0.0f);
        ConsumableHudImageWindow* target = m_powerUp;
        target->SetCount(((App*)WindowApp::m_instance)->m_powerUpManager->GetActivePowerUp()->GetCount());
    }
    if (m_grenade != NULL) {
        // This result is unused in the original, but the external call to GetActiveWeapon is kept.
        ((App*)WindowApp::m_instance)->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon();
        m_grenade->m_fill->m_progress = 1.0f - (float)m_grenadeTimeLeft / (float)m_grenadeDuration;
        m_grenade->m_fill->SetIsAnimating(m_grenadeTimeLeft > 0);
        ConsumableHudImageWindow* target = m_grenade;
        target->SetCount(((App*)WindowApp::m_instance)->m_grenadeManager->GetActiveGrenade()->GetCount());
    }
    if (m_value148 != 0) {
        processModelAnimation();
    }
    if (((App*)WindowApp::m_instance)->m_helper->m_swerveGame->IsGamePaused()) {
        return;
    }
    CDH_Weapon* weapon = ((App*)WindowApp::m_instance)->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon();
    if (weapon == NULL || weapon->IsReadyForUse() != 1 || weapon->m_ammo != 0 ||
        !m_canSwitchWeapons || m_value148 != 0) {
        return;
    }
    if (weapon->CheckForReload(false)) {
        return;
    }
    if (m_flagBc) {
        switchScope();
    } else {
        switchWeapons();
    }
}

void CGameplayHUD::UpdateUnitDirectionArrow()
{
    if (m_directionUnitId < 0) {
        return;
    }
    if (m_directionTime <= 0) {
        m_directionArrow->ClearFlags(1);
        return;
    }
    m_directionTime -= WindowApp::m_instance->m_updateFrameTime;
    CUnit* unit = m_game->m_unitsController->GetUnitByUniqueId(m_directionUnitId);
    if (unit == NULL) {
        return; // The original NULL path keeps the old display; do not hide on our own.
    }
    int x;
    int y;
    unit->m_body.GetScreenPos(x, y, 1.0f);
    float markerX;
    float markerY;
    int angle;
    if (x >= 1 && x < WindowApp::m_instance->m_screenWidth) {
        if (y >= 1 && y < WindowApp::m_instance->m_screenHeight) {
            m_directionArrow->ClearFlags(1);
            return;
        }
        SScreenAdaptingConstant margin(80);
        int minimum = margin();
        int width = WindowApp::m_instance->m_screenWidth;
        int right = width - margin();
        int clamped = x;
        if (clamped < minimum) {
            clamped = minimum;
        } else if (clamped > right) {
            clamped = right;
        }
        markerX = (float)clamped;
        if (y <= 0) {
            markerY = (float)HUD_ARROW_Y();
            angle = 90;
        } else {
            int height = WindowApp::m_instance->m_screenHeight;
            markerY = (float)(height - HUD_ARROW_Y());
            angle = 270;
        }
    } else {
        SScreenAdaptingConstant margin(90);
        int minimum = margin();
        int height = WindowApp::m_instance->m_screenHeight;
        int bottom = height - margin();
        int clamped = y;
        if (clamped < minimum) {
            clamped = minimum;
        } else if (clamped > bottom) {
            clamped = bottom;
        }
        markerY = (float)clamped;
        if (x <= 0) {
            markerX = (float)HUD_ARROW_X();
            angle = 0;
        } else {
            int width = WindowApp::m_instance->m_screenWidth;
            markerX = (float)(width - HUD_ARROW_X());
            angle = 180;
        }
    }
    m_directionArrow->m_angle = angle;
    m_directionArrow->m_markerX = (int)markerX;
    m_directionArrow->m_markerY = (int)markerY;
    m_directionArrow->SetFlags(1);
}

void CGameplayHUD::UpdateTutorialBlinking()
{
    bool blinking = ((App*)WindowApp::m_instance)->m_tutorialManager->IsSwitchWeaponButtonBlink();
    if (m_switchButton->m_blinking != blinking) {
        m_switchButton->SetBlink(blinking);
        int animation = 3;
        if (blinking) {
            animation = 4;
        }
        m_switchButton->SetAnimation(animation, 1);
        if (blinking == 1) {
            if (m_tutorialArrow == NULL) {
                ArrowMarkerWin* arrow = new ArrowMarkerWin;
                m_tutorialArrow = arrow;
                int x = m_switchButton->m_x;
                int width = m_switchButton->m_width;
                int offset = SScreenAdaptingConstant(40)();
                int y = m_switchButton->m_y;
                int height = m_switchButton->m_height;
                arrow->m_markerX = x + width / 2 - offset;
                arrow->m_markerY = y + height / 2;
                arrow->m_angle = 180;
                m_tutorialArrow->SetFlags(1);
                AddToFront(m_tutorialArrow);
            }
        } else if (m_tutorialArrow != NULL) {
            m_tutorialArrow->Close();
            m_tutorialArrow = NULL;
        }
    }
    SGHudImageWindow* button = m_switchButton;
    if (((App*)WindowApp::m_instance)->m_tutorialManager->IsShowSwitchWeaponButton() == 1) {
        button->SetFlags(1);
    } else {
        button->ClearFlags(1);
    }
}

void CGameplayHUD::processModelAnimation()
{
    if (m_value148 == 1) {
        CDH_Weapon* weapon = ((App*)WindowApp::m_instance)->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon();
        bool complete = weapon->isModelAnimationComplete();
        if (!complete) {
            weapon = ((App*)WindowApp::m_instance)->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon();
            if (weapon->isIdle() != 1) {
                return;
            }
        }
        ((App*)WindowApp::m_instance)->m_helper->m_gamePlayManager.SwitchWeapons();
        weapon = ((App*)WindowApp::m_instance)->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon();
        if (weapon != NULL) {
            weapon->fetchWeaponModel();
            m_value148 = 2;
        }
    } else if (m_value148 == 2) {
        CDH_Weapon* weapon = ((App*)WindowApp::m_instance)->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon();
        if (weapon->isIdle() == 1) {
            m_value148 = 0;
        }
    }
}

void CGameplayHUD::switchWeapons()
{
    if (((App*)WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial() == 1) {
        WindowApp::HandleTunnelCommand(0x807f0f43, 0, 0, 0);
    }
    ((App*)WindowApp::m_instance)->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon()->unfetchWeaponModel();
    m_value148 = 1;
    ((App*)WindowApp::m_instance)->m_soundManager->Play((CDH_SoundManager::eSound)2, false, false);
}

void CGameplayHUD::switchScope()
{
    CDH_Weapon* weapon = ((App*)WindowApp::m_instance)->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon();
    if (m_flagBc) {
        if (!weapon->m_scopeAttached) {
            ((App*)WindowApp::m_instance)->m_helper->m_swerveGame->m_camera->restoreView(true);
        } else {
            if (m_slider != NULL) {
                m_slider->Enable(false);
            }
            m_zoomedScope->m_enabled = false;
        }
        weapon->ScopeOut();
        CSwerveGame* game = ((App*)WindowApp::m_instance)->m_helper->m_swerveGame;
        game->setCursor(game->m_ownedCursor);
    } else {
        weapon->ScopeIn();
    }
    m_value148 = 3;
}

void CGameplayHUD::HandleBottomLayerPaint(ICGraphics2d& graphics)
{
    m_unitBarsLayer->SetFlags(1);
    m_unitBarsLayer->HandlePaint(graphics);
    m_unitBarsLayer->ClearFlags(1);
}

void CGameplayHUD::Paint(ICGraphics2d& graphics)
{
    if (m_flag145) {
        m_zoomedScope->RunPaint3D(graphics);
        ((App*)WindowApp::m_instance)->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon();
        CFontMgr::GetInstance()->GetFont(1); // The original still does the lookup, even though no text is drawn afterwards.
    }
}

void CGameplayHUD::StartFlashingEffect(int image, int intervalMs)
{
    m_flash->SetAnimation(image, 1);
    m_flash->StartFlashing(intervalMs);
    FlashingEffectWindow* flash = m_flash;
    flash->SetCorner((m_width - flash->m_imageWidth) / 2, (m_height - flash->m_imageHeight) / 2);
}

void CGameplayHUD::StopFlashingEffect() { m_flash->m_stopRequested = true; }

int CGameplayHUD::PointerPressed(int x, int y, int pointerId)
{
    if (m_value148 != 0) {
        return 5;
    }
    if (!m_flag145) {
        return -1;
    }
    if (m_canSwitchWeapons && m_switchButton->CheckCollision(x, y, 1.0f) == 1) {
        CDH_Weapon* weapon = ((App*)WindowApp::m_instance)->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon();
        if (weapon == NULL) {
            return -1;
        }
        if (weapon->IsReadyForUse() == 1) {
            if (m_flagBc) {
                switchScope();
                m_switchAfterScope = true;
            } else {
                switchWeapons();
            }
            return 5;
        }
        if (weapon->IsReadyForUse() != 0 || weapon->IsReloading() != 1) {
            return -1;
        }
        ((App*)WindowApp::m_instance)->m_helper->m_swerveGame->OnInterruptReloading();
        weapon->InterruptReloading();
        weapon->ResetAutoShot();
        switchWeapons();
        return 1;
    }
    if (((App*)WindowApp::m_instance)->m_settings->IsFireButtonEnabled() == 1 &&
        m_fireButton->CheckCollision(x, y, 1.5f) == 1) {
        m_firePressed = true;
        m_firePointerId = pointerId;
        return 2;
    }
    if (m_grenade != NULL && m_grenade->CheckCollision(x, y, 1.5f) == 1) {
        return ThrowGrenade();
    }
    if (m_powerUp != NULL && m_powerUp->CheckCollision(x, y, 1.5f) == 1) {
        if (((App*)WindowApp::m_instance)->m_powerUpManager->IsPowerUpAvailable() == 1) {
            ((App*)WindowApp::m_instance)->m_powerUpManager->RunPowerUp();
            return 1;
        }
        if (((App*)WindowApp::m_instance)->m_powerUpManager->GetActivePowerUp() != NULL) {
            if (!((App*)WindowApp::m_instance)->m_powerUpManager->IsPowerUpRunning()) {
                CPowerUp* active = ((App*)WindowApp::m_instance)->m_powerUpManager->GetActivePowerUp();
                if (active->GetCount() <= 0) {
                    WindowApp::HandleTunnelCommand(0x2e501085, 0, 0, 0);
                }
            }
        }
        return 1;
    }
    if (m_weaponInfo->CheckCollision(x, y) != 1) {
        return -1;
    }
    CDH_Weapon* weapon = ((App*)WindowApp::m_instance)->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon();
    if (weapon == NULL) {
        return -1;
    }
    int capacity = weapon->m_clipSize;
    int ammo = weapon->m_ammo;
    if (weapon->IsReadyForUse() != 1 || weapon->IsReloading() != 0) {
        return 1;
    }
    if (ammo < capacity) {
        weapon->CheckForReload(true);
    }
    return 1;
}

int CGameplayHUD::ThrowGrenade()
{
    CGrenade* grenade = ((App*)WindowApp::m_instance)->m_grenadeManager->GetActiveGrenade();
    if (grenade == NULL || grenade->GetCount() <= 0) {
        WindowApp::HandleTunnelCommand(0x2b46d225, 0, 0, 0);
        return 4;
    }
    if (m_grenadeTimeLeft > 0) {
        return 4;
    }
    CDH_Weapon* weapon = ((App*)WindowApp::m_instance)->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon();
    if (weapon == NULL) {
        return -1;
    }
    int ready = weapon->IsReadyForUse();
    if (ready != 1) {
        if (ready != 0) {
            return -1;
        }
        if (weapon->IsReadyForUse() != 0 || weapon->IsReloading() != 1) {
            return -1;
        }
        ((App*)WindowApp::m_instance)->m_helper->m_swerveGame->OnInterruptReloading();
        weapon->InterruptReloading();
    }
    int duration;
    if (weapon->Throw(duration) != 1) {
        return -1;
    }
    m_grenadeTimeLeft = duration;
    m_grenadeDuration = duration;
    return 4;
}

int CGameplayHUD::PointerReleased(int, int, int pointerId)
{
    if (!m_flag145 || m_scopePointerId != pointerId) {
        return -1;
    }
    CDH_Weapon* weapon = ((App*)WindowApp::m_instance)->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon();
    if (weapon == NULL || weapon->IsReadyForUse() != 1) {
        return -1;
    }
    switchScope();
    return 5;
}

int CGameplayHUD::PointerDragged(int, int, int) { return -1; } // Original 151e8c is the only place returning -1.

void CGameplayHUD::CheckContollStateOnPointerReleased(int, int, int pointerId)
{
    if (((App*)WindowApp::m_instance)->m_settings->IsFireButtonEnabled() != 1) {
        return;
    }
    if (m_firePointerId == pointerId) {
        m_firePressed = false;
        m_firePointerId = -1;
    }
}

void CGameplayHUD::CheckContollStateOnPointerMove(int x, int y, int previousX, int previousY, int pointerId)
{
    if (((App*)WindowApp::m_instance)->m_settings->IsFireButtonEnabled() != 1) {
        return;
    }
    if (m_fireButton->CheckCollision(x, y, 1.5f)) {
        return;
    }
    if (m_fireButton->CheckCollision(previousX, previousY, 1.5f) == 1 && m_firePointerId == pointerId) {
        m_firePressed = false;
        m_firePointerId = -1;
    }
}

void CGameplayHUD::Show(bool visible, float hiddenOpacity)
{
    float opacity = hiddenOpacity;
    if (visible) {
        opacity = 1.0f;
    }
    m_controlsLayer->SetOpacity(opacity);
    m_controlsLayer->SetFlags(1);
    if (visible == 1) {
        m_controlsLayer->SetFlags(0x2000);
    } else {
        m_controlsLayer->ClearFlags(0x2000);
        m_firePressed = false;
    }
    m_flag145 = visible;
}

void CGameplayHUD::SetZoomSliderEnable(bool enabled)
{
    if (m_slider != NULL) {
        m_slider->Enable(enabled);
    }
    m_zoomedScope->m_enabled = enabled;
}

bool CGameplayHUD::isWeaponModelVisible() const
{
    if (m_flagBc && m_value148 == 0) {
        return false;
    }
    return !m_hideWeaponModel;
}

void CGameplayHUD::OnScopeSwitched()
{
    m_flagBc = !m_flagBc;
    m_value148 = 0;
    if (m_switchAfterScope) {
        switchWeapons();
        m_switchAfterScope = false;
    }
}

void CGameplayHUD::OnCommand(Event& event)
{
    switch (event.m_id) {
    case 0x03412c74:
        ((App*)WindowApp::m_instance)->m_helper->m_swerveGame->RunFovBlur(1.0f, 3000, true);
        break;
    case 0x58611016:
        m_directionUnitId = event.m_param1;
        UpdateUnitDirectionArrow();
        m_directionTime = 1500; // The original first runs Update with the old timer, then writes the new duration.
        break;
    case 0x47b537b6:
        m_firePressed = false;
        m_firePointerId = -1;
        break;
    case 0x31577148:
        {
            CDH_Weapon* weapon = ((App*)WindowApp::m_instance)->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon();
            if (weapon != NULL && weapon->IsReadyForUse() == 1 && m_value148 == 0) {
                switchScope();
            }
        }
        break;
    default:
        return; // In the original, unknown commands are not forwarded to Window::OnCommand.
    }
    event.Clear();
}

CGameplayHUD::CZoomedScopeWindow::CZoomedScopeWindow() : ViewWindow(swerve::World<CSwerveAssertingErrorHandler>())
{
    // The original only clears c4..e9; the trailing alignment bytes are not initialized.
    __builtin_memset(&m_shoot, 0, 0x26); // The original calls libc _memset; must not be turned into _np_memset via the prefix macro.
    CDH_Weapon* weapon = ((App*)WindowApp::m_instance)->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon();
    if (weapon != NULL) {
        InitResources(weapon);
    }
}

void CGameplayHUD::CZoomedScopeWindow::InitResources(CDH_Weapon* weapon)
{
    if (weapon == NULL) {
        return;
    }
    m_weaponId = weapon->getId();
    m_shoot = NULL;
    m_zoom = NULL;
    m_animations.Reset();
    if (m_world.m_interface != NULL) {
        m_world.m_interface->Release();
    }
    m_world.m_interface = NULL;
    {
        XString::AnsiString name(weapon->m_scopeHudModel.m_data);
        m_world = swerve::World<CSwerveAssertingErrorHandler>(App::LoadObject3D(name));
    }
    if (m_world.m_interface == NULL) {
        return;
    }
    // Construct camera directly; the Ansi temporary is released at the end of this statement, avoiding an extra AddRef/Release from assignment.
    swerve::Camera<CSwerveAssertingErrorHandler> camera(m_world.find(SwerveHelper::Id(
        XString::AnsiString(((App*)WindowApp::m_instance)->m_helper->m_swerveGame->m_cameraName.m_data))));
    if (camera.m_interface != NULL) {
        DGHelper* helper = ((App*)WindowApp::m_instance)->m_helper;
        helper->SetCameraSize(camera, helper->m_screenWidth, helper->m_screenHeight, CAMERA_SIZE_SET_ASPECT);
    }
    m_animations.LoadFrom(swerve::Object3D<CSwerveAssertingErrorHandler>(m_world));
    m_shoot = m_animations.Anim(SwerveHelper::Id("shoot"));
    m_zoom = m_animations.Anim(SwerveHelper::Id("zoom"));
}

CGameplayHUD::CZoomedScopeWindow::~CZoomedScopeWindow()
{
    m_shoot = NULL;
    m_zoom = NULL;
    m_animations.Reset();
}

void CGameplayHUD::CZoomedScopeWindow::Update()
{
    if (m_shoot == NULL || m_zoom == NULL) {
        return;
    }
    CDH_Weapon* weapon = ((App*)WindowApp::m_instance)->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon();
    if (weapon->getId() != m_weaponId) {
        InitResources(weapon);
    }
    SwerveAnimations::Animation* shoot = m_shoot;
    if (m_shooting && shoot->m_durationSec > 0.0f) {
        float shootLength = shoot->m_durationSec;
        float phase = ((m_shootPosition / shootLength) * 2.0f - 1.0f) * 3.14158f;
        float difference = shootLength - m_zoomPosition;
        m_shootOffset = (float)((double)difference * (cos((double)phase) + 1.0) * 0.5);
    } else {
        m_shootOffset = 0.0f;
    }
    shoot->Start(0.0f, 1.0f, 0.0f, 0.0f, 1.0f);
    m_zoom->Start(0.0f, 1.0f, 0.0f, 0.0f, 1.0f);
    m_zoom->SetPositionSec(m_zoomPosition + m_shootOffset);
    m_shoot->SetPositionSec(m_shootPosition);
    m_shoot->Pause();
    m_zoom->Pause();
    if (m_shooting) {
        // The original sets the position using the old elapsed, then accumulates the current frame time.
        m_shootPosition = (float)m_shootElapsed * 0.001f;
        m_shootElapsed += WindowApp::m_instance->m_updateFrameTime;
        if (m_shootPosition > m_shoot->m_durationSec) {
            m_shootElapsed = 0;
            m_shooting = false;
            m_shootPosition = 0.0f;
        }
    }
    m_worldTime += WindowApp::m_instance->m_updateFrameTime;
    m_world.animate(m_worldTime);
}

void CGameplayHUD::CZoomedScopeWindow::Paint3D(swerve::Graphics3D<CSwerveAssertingErrorHandler>& graphics)
{
    if (m_enabled) {
        ViewWindow::Paint3D(graphics);
    }
}

CGameplayHUD::ArrowMarkerWin::ArrowMarkerWin() : WindowTransparent(false)
{
    SetLayoutType(LAYOUT_NONE);
    SetDesiredHeight(100);
    SetDesiredWidth(100);
    m_arrow = new SGImageWindowRotatable;
    m_arrow->SetArchetypeCharacter(0, 0);
    m_arrow->SetAnimation(7, 1);
    m_arrow->SetLayoutType(LAYOUT_NONE);
    AddToFront(m_arrow);
    m_marker = new SGImageWindow;
    m_marker->SetArchetypeCharacter(0, 0);
    m_marker->SetAnimation(8, 1);
    m_marker->SetLayoutType(LAYOUT_NONE);
    m_marker->SetFlags(0x800000);
    AddToFront(m_marker);
    ClearFlags(1);
}

void CGameplayHUD::ArrowMarkerWin::Update()
{
    SetCorner(m_markerX, m_markerY);
    SScreenAdaptingConstant radius(26);
    m_arrow->m_angle = m_angle;
    m_arrow->SetCorner(0, 0);
    SGImageWindow* marker = m_marker;
    int horizontalRadius = radius();
    float cosine = MathLib::Cos((float)m_angle);
    int verticalRadius = radius();
    float sine = MathLib::Sin((float)m_angle);
    marker->SetCenter((int)((float)horizontalRadius * cosine), (int)((float)verticalRadius * sine));
}
