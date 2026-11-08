// PROVISIONAL (created by b055x): owner src/bounty_hunter/ui/hud/CGameplayHUD.cpp not written yet
// CGameplayHUD.h - the in-game HUD; only what src/game/CDH_Weapon.cpp uses is declared.
#ifndef CGAMEPLAYHUD_H
#define CGAMEPLAYHUD_H

#include "WindowBase.h"
#include "CCivilianHPBar.h"
#include "CGameplayHPBar.h"
#include "CGameplayWeaponInfoWindow.h"
#include "HudWindows.h"
#include "SGImageWindowCommand.h"
#include "ServiceWindows.h"
#include "SwerveHelper.h"
#include "CSliderControl.h"
class CSwerveGame;

// b067h: 150a80 directly constructs Window, Game f9398 allocates 0x14c; not yet a full HUD owner recovery.
// Allocation address on the line above corrected: original Game f9432; +145 is written as visible by 151f8a, +bc is the scope state.
class CGameplayHUD : public Window {
public:
    // k: complete 48 original ranges, keeping the thin-declaration history above; the two nested classes are actually allocated ec/c0 in the original.
    class CZoomedScopeWindow : public ViewWindow {
    public:
        CZoomedScopeWindow();
        virtual ~CZoomedScopeWindow();
        void InitResources(CDH_Weapon* weapon);
        virtual void Update();
        virtual void Paint3D(swerve::Graphics3D<CSwerveAssertingErrorHandler>& graphics);
        SwerveAnimations m_animations;              // b0..c3
        SwerveAnimations::Animation* m_shoot;       // c4
        SwerveAnimations::Animation* m_zoom;        // c8
        float m_zoomPosition;                     // cc
        float m_shootPosition;                    // d0
        float m_shootOffset;                      // d4
        int m_weaponId;                           // d8
        int m_unresolvedDc;                       // dc, the original only memsets it; semantics not guessed.
        int m_worldTime;                          // e0
        int m_shootElapsed;                       // e4
        bool m_shooting;                          // e8
        bool m_enabled;                           // e9, ea/eb not cleared by the original.
    };
    class ArrowMarkerWin : public WindowTransparent {
    public:
        ArrowMarkerWin();
        virtual ~ArrowMarkerWin() {} // Original 152798 only calls Window.
        virtual void Update();
        SGImageWindow* m_marker;                // ac
        SGImageWindowRotatable* m_arrow;        // b0
        int m_markerX;                          // b4, not written by the original ctor.
        int m_markerY;                          // b8
        int m_angle;                            // bc
    };
    explicit CGameplayHUD(CSwerveGame* game);
    virtual ~CGameplayHUD();
    virtual void Init() {} // Original 1527bc bx lr+nop, a real weak empty callback.
    virtual void OnCommand(Event& event);
    virtual void Update();
    virtual void Paint(com::glu::platform::graphics::ICGraphics2d& graphics);
    void Show(bool visible, float hiddenOpacity);
    void switchScope();
    void OnScopeSwitched();
    void SetZoomSliderEnable(bool enabled);
    int PointerPressed(int x, int y, int pointerId); // Return code 2/-1 read after Game fa106.
    int PointerReleased(int x, int y, int pointerId);
    int PointerDragged(int x, int y, int pointerId);
    void CheckContollStateOnPointerReleased(int x, int y, int pointerId);
    void CheckContollStateOnPointerMove(int x, int y, int previousX, int previousY, int pointerId);
    void HandleBottomLayerPaint(com::glu::platform::graphics::ICGraphics2d& graphics);
    bool isWeaponModelVisible() const;
    bool GetFlagBc() const { return m_flagBc; } // Read by Game fe0b6.
    bool GetFlag145() const { return m_flag145; } // Read by Game f955c.
    int GetValue148() const { return m_value148; } // Read by original Game fd026; cursor is drawn when 0.
    /** Start the grenade throw shown on the HUD. */
    int ThrowGrenade();
    /** Power-up screen flash (0x151ad0 / 0x151b18). */  // added by b078a
    void StartFlashingEffect(int image, int intervalMs);
    void StopFlashingEffect();
    void InitCommon();
    void InitFireButton();
    void InitWeaponButton();
    void InitScopeButton();
    void UpdateHUDsVisibility();
    void UpdateUnitHPBars();
    void UpdateUnitDirectionArrow();
    void UpdateTutorialBlinking();
    void processModelAnimation();
    void switchWeapons();

    // The former m_data00[0x12c] (layout not known yet) thin view is now split into the proven Window and the +bc flag.
    unsigned char m_dataAc[0xbc - 0xac];
    bool m_flagBc; // Zeroed by original 150a9a; concrete HUD semantics await the full owner.
    // Historical thin placeholder: unsigned char m_dataBd[0x12c - 0xbd];
    bool m_unresolvedBd;              // bd, zeroed by the original constructor; no other reads/writes found.
    int m_scopePointerId;             // c0
    int m_unresolvedC4;               // c4, zeroed by the original constructor.
    int m_unresolvedC8;               // c8, same as above.
    bool m_canSwitchWeapons;          // cc
    unsigned char m_paddingCd;       // cd not written by the original; keeps ce at a single-byte offset.
    bool m_switchAfterScope;          // ce
    int m_grenadeTimeLeft;            // d0
    int m_grenadeDuration;            // d4
    SGImageWindowCommand* m_scopeButton; // d8
    SGHudImageWindow* m_fireButton;    // dc
    SGImageWindowCommand* m_pauseButton; // e0
    SGHudImageWindow* m_switchButton;  // e4
    ConsumableHudImageWindow* m_grenade; // e8
    ConsumableHudImageWindow* m_powerUp; // ec
    CGameplayHPBar* m_hpBar;          // f0
    CGameplayWeaponInfoWindow* m_weaponInfo; // f4
    FlashingEffectWindow* m_flash;    // f8
    Vector<CCivilianBar*> m_civilianBars; // fc..10b
    Window* m_controlsLayer;          // 10c
    Window* m_unitBarsLayer;          // 110
    int m_directionTime;              // 114
    CSwerveGame* m_game;              // 118
    int m_directionUnitId;            // 11c
    CSliderControl* m_slider;         // 120
    int m_unresolved124;              // 124, not written by the original ctor.
    int m_firePointerId;              // 128
    bool m_firePressed;             // 0x12c fire button held (CSwerveGame::GetDebugFireAction)
    // Historical: unsigned char m_data12d[0x144 - 0x12d];  // added by b049: intervening HUD layout unresolved
    bool m_hideWeaponModel;           // 12d
    CZoomedScopeWindow* m_zoomedScope; // 130
    ArrowMarkerWin* m_directionArrow; // 134
    ArrowMarkerWin* m_tutorialArrow;  // 138
    int m_unresolved13c;              // 13c
    int m_unresolved140;              // 140
    bool m_steadyAim;               // 0x144 steady aim input (0x168afc)  // added by b049
    bool m_flag145;                 // Zeroed by original 150b0a.
    unsigned char m_data146[0x148 - 0x146];
    int m_value148; // Original 150b0e sets 0, 151a2a sets 3; concrete state names left to the full HUD owner.
};

#endif
