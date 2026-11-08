// PROVISIONAL (created by b049): owner src/FPSGameplay/CUnitBody.cpp not written yet
// b068-b069: complete owner declarations and observed ARM layout replace the initial LOD-only view.
#ifndef CUNITBODY_H
#define CUNITBODY_H
#include "CCollision.h"
#include "AnimationInstance.h"
#include "SwerveHelper.h"
#include "CGameAIMap.h"
#include "IExecutor.h"
#include "CEnemyWeaponType.h"
#include "CVector.h"
struct SSpawnablePerson;
class CUnit;
class CUnitTarget;
class CBH_PersonType;
class CVisualCharacterType;
class AnimatableSwerveObject;
class AnimationTransition;
class CDH_Animation;
class FrustumCulling;
#include "FrustumCulling.h"

/** Obstacles collected by a map. Static records are three vectors plus a radius (0x28). */
class ObstaclesContainer {
public:
    struct Static {
        vec3 m_first;
        vec3 m_second;
        vec3 m_direction;
        float m_radius;
    };
    void AddStatic(const vec3&, const vec3&, const vec3&, float);
    void Clear(bool dynamicObstacles, bool staticObstacles);
    Vector<Static> m_static;  // 0x00, AddStatic @0x1034d8.
    Vector<CUnit*> m_dynamic; // 0x10, units collected by CUnitsController::AvoidCollision.
};

class CUnitBody : public AnimationListener, public virtual CCollision::CKinematicBody {
public:
    typedef swerve::Group<CSwerveAssertingErrorHandler> Group;
    typedef swerve::Transform<CSwerveAssertingErrorHandler> Transform;
    typedef swerve::SkinnedMesh<CSwerveAssertingErrorHandler> SkinnedMesh;
    enum UnitState { UNIT_STATE_INITIAL=0 };
    enum OrientationControl { ORIENTATION_NONE=0, ORIENTATION_TURN=1, ORIENTATION_INSTANT=2, ORIENTATION_MOVEMENT=3 };
    enum EHitEffect { HIT_EFFECT_BODY=0, HIT_EFFECT_NONE=1, HIT_EFFECT_HEAL=2, HIT_EFFECT_RICOCHET=3, HIT_EFFECT_HEAD=4, HIT_EFFECT_SLIME=5 };
    CUnitBody(const SSpawnablePerson&, const IGameAIMap*);
    void InitBodyZones();
    void LoadWeapon();
    void ShowWeapon(bool);
    virtual ~CUnitBody();
    void Cleanup();
    void Init(CUnit*, const XString&);
    void LoadSkin();
    void InitLODs();
    void SetToPoint(const XString&);
    void Spawn();
    void BindToWorld();
    void SyncLocation();
    void Tick(int);
    void CalculateStep(int);
    void UpdateTransform(int);
    void UpdateAnimation(int);
    void SetWorldPosition(const vec3&);
    void OnNeutralization();
    void UnbindWeapon();
    void UnbindAllIcons();
    void CheckVisibility(const FrustumCulling&);
    void DefineCameraPitch(const vec3&);
    void SetOrientationToPoint(const vec3&, bool, float);
    void GetCurrentPosition(int*, int*);
    void MakeStep(int, const vec3&, float);
    void SetSteeringForce(const vec3&);
    void SetTaskForce(const vec3&);
    void SetOrientationControl(OrientationControl);
    vec3 GetSpeed() const;
    float GetRadius() const;
    void Behead();
    void DrawHitEffect(EHitEffect);
    void Blow();
    void SetWorldPositionToMassCenter();
    void StopAllAnimations(int);
    void IgnoreSteering(bool);
    WeakIntrusivePtr<AnimationInstance> StartAnimation(EPersonAnimType, bool);
    WeakIntrusivePtr<AnimationInstance> PlayAfraidAnimation();
    WeakIntrusivePtr<AnimationInstance> PlayDeathAnimation(bool);
    EPersonAnimType GetSpawnAnimation();
    WeakIntrusivePtr<AnimationInstance> PlayShotAnimation();
    void ThrowWeapon();
    void RemoveWeapon();
    bool IsRunning() const;
    void PlayWeaponSound();
    void PlayAttackSound(float);
    void PlayDeathSound(float);
    void PlayWoundSound(float);
    void ShowBulletMuzzleFlash();
    void UnbindFromWorld();
    void AddLightingController(Group);
    void BindWeaponToHand();
    void RemoveLightingController(Group);
    void BlowWeapon();
    void InitLODChannel(unsigned int, Group);
    void InitLODChannel(unsigned int, SkinnedMesh);
    void BindBoxToHand(XString);
    void ShowBox(bool);
    bool HasBox() const;
    float GetMovementSpeed();
    bool PickBounds(const CCollision::Line&);
    /** Exact body pick used after kinematic bounds collision, original 0x106e44. */
    bool Pick(const CCollision::Line& line, vec3& point, vec3& normal);
    bool Pick(float, float, float, swerve::Camera<CSwerveAssertingErrorHandler>&, EHitType&);
    virtual void eventListener(const AnimationEventCK&, WeakIntrusivePtr<AnimationInstance>);
    vec3 GetHitPosition();
    float GetDistanceToUnit(CUnit*) const;
    float GetDistanceToTarget(const CUnitTarget*) const;
    float GetDistanceToPoint(const vec3&) const;
    EPersonAnimType GetMovementAnimation();
    EPersonAnimType GetPainAnimation();
    void SetLuminance(float);
    void GetScreenPos(int&, int&, float);
    void SetSameOrientation(const CUnitBody*, bool);
    void SetState(UnitState);
    virtual bool Intersect(const CCollision::Line&, vec3&, vec3&);
    /** Only the LOD factor referenced by DGCamera::SetFOV is declared here. */
    static float m_LodDistanceModifier;
    static const float UNIT_BODY_RADIUS;

    UnitState m_state;           // 0x004.
    ObstaclesContainer m_obstacles; // 0x008.
    OrientationControl m_orientationControl; // 0x028.
    bool m_ignoreSteering;       // 0x02c.
    float m_speed;               // 0x030.
    FrustumCulling::CullSphere* m_cullSphere; // 0x034.
    CUnit* m_owner;              // 0x038.
    Group m_model;              // 0x03c.
    Group m_root;               // 0x040.
    Group m_skin;               // 0x044.
    Group m_headKillZone;        // 0x048.
    Group m_headArmorZone;       // 0x04c.
    Group m_bodyVitalZone;       // 0x050.
    Group m_bodyArmorZone;       // 0x054.
    Group m_weapons[3];          // 0x058.
    Group m_group64;             // 0x064.
    Group m_box;                 // 0x068.
    Group m_icon;                // 0x06c.
    swerve::RayIntersection<CSwerveAssertingErrorHandler> m_intersection; // 0x070.
    swerve::RayIntersection<CSwerveAssertingErrorHandler> m_intersection2; // 0x074.
    AnimatableSwerveObject* m_animatable; // 0x078.
    AnimationTransition* m_transition; // 0x07c.
    CBH_PersonType* m_personType; // 0x080.
    CVisualCharacterType* m_visualType; // 0x084.
    const IGameAIMap* m_map;     // 0x088.
    XString m_spawnPoint;        // 0x08c.
    SwerveAnimations::Animation* m_animations[37]; // 0x090, initialized NULL.
    SwerveAnimations m_swerveAnimations; // 0x124.
    int m_field138;             // 0x138, initialized zero.
    int m_field13c;             // 0x13c, initialized zero.
    IGameAIMap::Location m_location; // 0x140.
    CDH_Animation* m_muzzleFlash; // 0x164.
    CDH_Animation* m_lighting[5]; // 0x168.
    EPersonAnimType m_animation; // 0x17c.
    float m_luminance;           // 0x180.
    float m_height;              // 0x184.
    int m_spawnPointId;          // 0x188.
    int m_previousPointId;       // 0x18c.
    bool m_turning;              // 0x190.
    bool m_transformDirty;       // 0x191.
    vec3 m_position;             // 0x194.
    vec3 m_orientation;          // 0x1a0.
    float m_yaw;                 // 0x1ac.
    float m_field1b0;            // 0x1b0, initialized zero.
    vec3 m_attackSource;         // 0x1b4.
    vec3 m_steeringForce;        // 0x1c0.
    vec3 m_taskForce;            // 0x1cc.
    CCollision::Ellipsoid m_ellipsoid; // 0x1d8.
    CCollision::AABB m_bounds;   // 0x1fc. The original Intersect first copies the whole object of the same type, then translates.
    bool m_refreshBounds;        // 0x214.
    int m_boundsTimer;           // 0x218.
    vec3 m_forward;              // 0x21c.
    float m_stepSpeed;           // 0x228.
    int m_cameraPitch;           // 0x22c.
    int m_field230;              // 0x230.
    bool m_flag234;              // 0x234.
    bool m_flag235;              // 0x235.
    bool m_visible;              // 0x236.
    bool m_beheaded;             // 0x237.
    bool m_spawning;             // 0x238.
    bool m_flag239;              // 0x239.
    Transform m_transform;       // 0x23c.
    Transform m_scratch;         // 0x240.
    vec3 m_hitRayOrigin;         // 0x244.
    vec3 m_hitRayDirection;      // 0x250.
    float m_hitRayFraction;      // 0x25c.
    vec3 m_hitNormal;            // 0x260.
    float m_scale;               // 0x26c.
    float m_pickDistance;        // 0x270.
    int m_updateFrameCounter;    // 0x274.
    unsigned int m_seed;         // 0x278.
    float m_turnRate;            // 0x27c.
    Group m_heads[3];            // 0x280.
    unsigned int m_lodCount;     // 0x28c.
    unsigned int m_currentLod;   // 0x290.
    float m_cameraDistance;      // 0x294.
    float m_lodDistances[3];     // 0x298.
    Group m_lods[3];             // 0x2a4.
    com::glu::platform::core::CVector<int> m_lodChannels[3]; // 0x2b0.
    int m_animationElapsed;      // 0x2f8.
    int m_animationQuantum;      // 0x2fc.
    bool m_autoAnimationLod;     // 0x300.
    int m_forcedLod;             // 0x304.
    bool m_weaponShown;          // 0x308.
    bool m_weaponLoaded;         // 0x309.
    int m_soundCooldown;         // 0x30c.
    void* m_field310;            // 0x310.
    void* m_field314;            // 0x314.
    void* m_field318;            // 0x318. CKinematicBody follows virtually at 0x31c.
};
bool CalculateRotation(float&, float, float, const vec3&);
#endif

