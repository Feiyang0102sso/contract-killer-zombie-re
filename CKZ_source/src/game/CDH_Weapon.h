// CDH_Weapon.h - one weapon of the player: shop item, stats from weapons.xml, 3D model and its animations.
// (b055x: replaces the PROVISIONAL of b048; the old names GetId() / m_ammo stay valid.)
#ifndef CDH_WEAPON_H
#define CDH_WEAPON_H

#include "CShopItem.h"
#include "CDH_Animation.h"
#include "DataProtection.h"
#include "tws.h"
#include "MathLib.h"

class TiXmlNode;

/**
 * A weapon. State machine: m_state is the steady state (1 ready, 2 scoped), m_action the running
 * action (3 reload, 4 / 5 shot from ready / scoped, 6 scope in, 7 scope out, 8 throw) with
 * m_actionTimeLeft ms to go; AfterStateChanged() runs when the action ends.
 */
class CDH_Weapon : public CItem {
public:
    typedef swerve::Node<CSwerveAssertingErrorHandler> Node;
    typedef swerve::Group<CSwerveAssertingErrorHandler> Group;
    typedef swerve::Transform<CSwerveAssertingErrorHandler> Transform;

    /** One step of a shell by shell reload (shotguns): sound, ammo added, duration in ms. */
    struct SReloadingStep {
        // Reload steps carry all three values; copying does not default-initialize them.
        SReloadingStep(int sound, int ammo, float time)
            : m_sound(sound), m_ammo(ammo), m_time(time) {}

        int m_sound;        // 0x00 CDH_SoundManager::eSound (-1: silent)
        int m_ammo;         // 0x04 ammo added when the step starts
        float m_time;       // 0x08 ms left
    };

    /** An optional part of the model: the named nodes shown together. */
    struct ItemDesc {
        ~ItemDesc();

        XString m_name;                 // 0x00
        Vector<XString> m_nodeNames;    // 0x04
        Vector<Node> m_nodes;           // 0x14 resolved in loadWeaponModel
        const char* m_resourceId;       // 0x24
        void* m_resourceData;           // 0x28
        int m_field2c;                  // 0x2c
        int m_field30;                  // 0x30
        bool m_active;                  // 0x34
    };

    /** Mutually exclusive model parts; one of them is shown. */
    struct ItemsGroup {
        ~ItemsGroup();
        /** Show the nodes of item `id` only. */
        void setActiveID(int id);

        XString m_name;                 // 0x00
        XString m_type;                 // 0x04
        int m_activeId;                 // 0x08
        Vector<ItemDesc*> m_items;      // 0x0c
    };

    CDH_Weapon(TiXmlNode* node);
    void parseNode(TiXmlNode* node);
    /** Read the "available" flag from the save file. */
    void Load();

    // CItem
    virtual void loadResources();                                           // 0x00
    virtual void releaseResources();                                        // 0x04
    virtual bool isAvailable() const                                        // 0x0c
    {
        return m_available.Get();
    }
    virtual bool IsEquiped() const;                                         // 0x18
    virtual bool IsOwned() const;                                           // 0x1c
    virtual void setAvailable(bool available);                              // 0x20
    virtual com::glu::platform::graphics::ICRenderSurface* GetShopImage()   // 0x24
    {
        return shopImage();
    }
    virtual com::glu::platform::graphics::ICRenderSurface* GetSmallImage()  // 0x28
    {
        return recommendedImage();
    }
    virtual XString& GetName()                                              // 0x2c
    {
        return m_name;
    }
    virtual XString& GetDescription()                                       // 0x30
    {
        return m_description;
    }
    virtual int getUnlockLvl() const                                        // 0x34
    {
        return m_unlockLevel;
    }
    virtual const char* GetItemTypeName() const                             // 0x4c
    {
        return "weapon";
    }
    virtual int getId() const                                               // 0x50
    {
        return m_id;
    }
    virtual int Buy();                                                      // 0x58
    virtual bool CheckFilter(int filter);                                   // 0x64
    virtual ~CDH_Weapon();                                                  // 0x74 / 0x78
    virtual com::glu::platform::graphics::ICRenderSurface* getBigImage()    // 0x7c
    {
        return bigImage();
    }

    /** Old name used by src/dh3d (b048): the weapon id (getId, slot 0x50). */
    int GetId()
    {
        return getId();
    }

    void releaseWeaponModel();
    /** Refill the clip and go back to the ready state. */
    void restoreAmmo();
    /** Per frame: power up durations, minigun spin, animations, state. */
    void update(unsigned int deltaTime);
    /** Apply (or undo) the speed multipliers of the active power up. */
    void ModifyDurations();
    int isModelAnimationComplete() const;
    void ResetModelAnimation();
    void animateWeaponModel();
    /** Advance the reload steps and the running action by `deltaTime` ms. */
    void UpdateState(int deltaTime);
    /** Use a copy of `light` (plus an ambient light) for the weapon model. */
    void setDynamicLight(swerve::Light<CSwerveAssertingErrorHandler>& light);
    void updateDynamicLight(Transform& transform);
    void hideWeaponModel();
    void fetchWeaponModel();
    void unfetchWeaponModel();
    void scopeReset();
    bool isIdle() const;
    void paintWeaponModel(swerve::Graphics3D<CSwerveAssertingErrorHandler>& graphics);
    void playReloadSound();
    /** Load the model (reference counted) and set up the animations. */
    void loadWeaponModel();
    void parseTexts(TiXmlNode* node);
    void parseAnims(TiXmlNode* node);
    void parseStats(TiXmlNode* node);
    void parseGraphics(TiXmlNode* node);
    void parseSounds(TiXmlNode* node);
    /** Write the "available" flag to the save file (`saveNow`: on the next update). */
    void Save(bool saveNow);
    com::glu::platform::graphics::ICRenderSurface* gameImage() const;
    com::glu::platform::graphics::ICRenderSurface* shopImage() const;
    com::glu::platform::graphics::ICRenderSurface* recommendedImage() const;
    com::glu::platform::graphics::ICRenderSurface* bigImage() const;
    /** Start reloading when the clip is empty (or `force`); 1 when a reload started. */
    int CheckForReload(bool force);
    void AfterStateChanged();
    void ScopeIn();
    bool IsReadyForAutoShot();
    /** 1 when the weapon can shoot now. */
    int IsReadyForUse();
    /** 1 while reloading. */
    int IsReloading();
    /** Shoot once; 1 when a shot was fired. */
    int MakeShot();
    void ResetAutoShot();
    void HandleAutoShot();
    void PlayShootAnimation();
    void PlayScopeAnimation(bool out);
    void ScopeOut();
    /** Throw a grenade; `throwTime` receives the throw duration. 1 when thrown. */
    int Throw(int& throwTime);
    void PlayThrowAnimation();
    void PlayReloadAnimation(int ammo);
    void InterruptReloading();
    /** Damage of one shot at `distance` (fire power by distance, power up). */
    int GetShotDamage(float distance) const;
    void SetAnimationControllerSpeedMultiplier(int controller, float multiplier);
    /** Tilt the eye camera by the inertia angles (degrees). */
    void SetInertionAngle(float yaw, float pitch);
    /** Start action `action` lasting `time` ms. */
    void StartAction(int action, int time)
    {
        m_action = action;
        m_actionTimeLeft = time;
    }
    /** World position of barrel `barrel` seen through `camera`. */
    vec3 GetTracerEmitterPos(int barrel, swerve::Camera<CSwerveAssertingErrorHandler> camera,
                             swerve::World<CSwerveAssertingErrorHandler> world);

    int m_modelRefCount;                    // 0x80 loadWeaponModel / releaseWeaponModel
    int m_state;                            // 0x84
    int m_action;                           // 0x88
    int m_actionTimeLeft;                   // 0x8c ms
    bool m_reloadPending;                   // 0x90 reload after scoping out
    bool m_throwPending;                    // 0x91 throw after scoping out
    int m_field94;                          // 0x94 not used here
    int m_field98;                          // 0x98 not used here
    XString m_field9c;                      // 0x9c not used here
    int m_id;                               // 0xa0 "wid"
    XString m_name;                         // 0xa4
    XString m_description;                  // 0xa8
    Encrypted<bool> m_available;            // 0xac owned
    bool m_fetched;                         // 0xb4 model shown
    bool m_idle;                            // 0xb5 idle animation running
    bool m_interruptingReload;              // 0xb6
    bool m_spinning;                        // 0xb7 minigun barrel turning
    int m_spinSoundCount;                   // 0xb8 minigun spin sounds played
    Group m_eyecamTransform;                // 0xbc
    Transform m_baseTransform;              // 0xc0
    Transform m_inertionTransform;          // 0xc4
    int m_barrelCount;                      // 0xc8
    Group m_barrels[2];                     // 0xcc dummyBarrelNNTransform
    Transform m_barrelTransform;            // 0xd4 scratch (GetTracerEmitterPos)
    Transform m_cameraTransform;            // 0xd8 scratch (GetTracerEmitterPos)
    bool m_handsSeparated;                  // 0xdc
    float m_shotDeferringTime;              // 0xe0
    int m_shootType;                        // 0xe4 2: automatic
    int m_projectileType;                   // 0xe8 2: no reload
    int m_clipSize;                         // 0xec "num_of_shots"
    int m_rateOfFire;                       // 0xf0 ms per shot
    int m_baseRateOfFire;                   // 0xf4
    int m_reloadTime;                       // 0xf8 ms
    int m_baseReloadTime;                   // 0xfc
    int m_scopeTime;                        // 0x100 ms
    int m_throwTime;                        // 0x104 ms
    int m_baseThrowTime;                    // 0x108
    int m_throwDelayLeft;                   // 0x10c ms until the grenade leaves the hand
    bool m_grenadePending;                  // 0x110
    int m_autoShotsLeft;                    // 0x114
    bool m_autoShot;                        // 0x118
    int m_weaponType;                       // 0x11c 1 shotgun, 8 / 9 heavy (9 minigun)
    int m_unlockLevel;                      // 0x120
    int m_bonusXp;                          // 0x124
    int m_damageMin;                        // 0x128
    int m_damageMax;                        // 0x12c
    int m_firePower[6];                     // 0x130 percent by distance (0..10 m, 10..20 m, ...)
    float m_zoomFovMin;                     // 0x148
    float m_zoomFovMax;                     // 0x14c
    float m_aimZoomFov;                     // 0x150
    bool m_scopeAttached;                   // 0x154
    int m_countInQueue;                     // 0x158
    XString m_accuracyDesc;                 // 0x15c
    XString m_damageDesc;                   // 0x160
    float m_headshotMultiplier;             // 0x164
    int m_throwDelay;                       // 0x168 ms
    int m_baseThrowDelay;                   // 0x16c
    float m_minAimDispersion;               // 0x170
    float m_maxAimDispersion;               // 0x174
    float m_aimDispersionPerShoot;          // 0x178
    float m_aimDispersionPerTurnAngle;      // 0x17c
    float m_minAimRecoverySpeed;            // 0x180
    float m_maxAimRecoverySpeed;            // 0x184
    Vector<SReloadingStep> m_reloadingSteps;    // 0x188
    int m_reloadingStepStarted;             // 0x198
    int m_ammo;                             // 0x19c ammo in the clip
    const char* m_modelId;                  // 0x1a0
    const char* m_textureId;                // 0x1a4
    int m_textureNodeId;                    // 0x1a8
    const char* m_gameImageId;              // 0x1ac
    const char* m_shopImageId;              // 0x1b0
    int m_field1b4;                         // 0x1b4
    int m_field1b8;                         // 0x1b8
    const char* m_recommendedImageId;       // 0x1bc
    const char* m_bigImageId;               // 0x1c0
    int m_spriteScopeArch;                  // 0x1c4
    int m_field1c8;                         // 0x1c8
    int m_field1cc;                         // 0x1cc
    int m_field1d0;                         // 0x1d0
    int m_field1d4;                         // 0x1d4
    int m_field1d8[5];                      // 0x1d8 not used here
    int m_shootSound;                       // 0x1ec
    int m_reloadSound;                      // 0x1f0
    int m_noAmmoSound;                      // 0x1f4
    int m_switchSound;                      // 0x1f8
    swerve::World<CSwerveAssertingErrorHandler> m_world;                // 0x1fc the model
    swerve::Transformable<CSwerveAssertingErrorHandler> m_controller;   // 0x200 weaponControllerTransform
    CDH_Animation* m_animation;             // 0x204 shoot / fetch / hide / ... controllers
    CDH_Animation* m_scopeAnimation;        // 0x208
    swerve::Camera<CSwerveAssertingErrorHandler> m_eyeCamera;           // 0x20c
    swerve::Camera<CSwerveAssertingErrorHandler> m_menuCamera;          // 0x210
    XString m_scopeHudModel;                // 0x214
    XString m_shootAnimation;               // 0x218
    XString m_reloadAnimation;              // 0x21c
    XString m_fetchAnimation;               // 0x220
    XString m_hideAnimation;                // 0x224
    XString m_aimAnimation;                 // 0x228
    XString m_unusedAnimation;              // 0x22c
    XString m_throwAnimation;               // 0x230
    bool m_lightAdded;                      // 0x234
    swerve::Light<CSwerveAssertingErrorHandler> m_light;                // 0x238
    Group m_lightGroup;                     // 0x23c
    float m_spinSpeed;                      // 0x240 minigun barrel speed (0..1)
    float m_spinUpTime;                     // 0x244
    bool m_powerUpApplied;                  // 0x248 ModifyDurations ran with a power up
    swerve::Mesh<CSwerveAssertingErrorHandler> m_fugas;                 // 0x24c mine mesh (weapon 0x23)
    Vector<ItemsGroup*> m_itemsGroups;      // 0x250
};

#endif
