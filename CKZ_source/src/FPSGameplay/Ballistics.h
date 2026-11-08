// Ballistics.h - projectile pools, particles, thrown models and hand grenades (original u430).
#ifndef BALLISTICS_H
#define BALLISTICS_H
#include "MathLib.h"
#include "CCollision.h"
#include "tws.h"
class Window;
class CAuxRender;
class CDH_Animation;
class HandGrenadeBallistics;

class Ballistics {
public:
    struct Projectile {
        bool m_active;         // 0x00; other scalar fields are initialized when activated
        float m_age;           // 0x04
        vec3 m_position;        // 0x08
        vec3 m_previous;        // 0x14
        vec3 m_velocity;        // 0x20
        void* m_userData;       // 0x2c
        bool m_kinematic;       // 0x30
    };
    Ballistics(int capacity, float maxAge, vec3 gravity, CCollision* collision);
    virtual ~Ballistics();
    virtual void OnPreSimulate(float elapsed) = 0;
    virtual void OnPostSimulate(float elapsed) = 0;
    virtual void OnActivate(Projectile* projectile) = 0;
    virtual int OnCollision(Projectile*, vec3* point, vec3* normal, CCollision::Triangle*) = 0;
    void Simulate(float elapsed);
    int AddProjectile(const vec3& position, const vec3& velocity, float spread, void* userData);
    void SetGravity(const vec3& gravity);
    void SetBouncing(float normal, float tangent);
    void SetDrag(float drag);
    void SetCollisionFlags(unsigned int flags);
    static Projectile* g_CurrentlyCollidingProjectile;
    static Ballistics* g_CurrentlySimulatedBallistics;

    int m_capacity;              // 0x04
    int m_next;                  // 0x08
    int m_activeCount;           // 0x0c snapshot from Simulate
    int* m_activeIndices;        // 0x10
    Projectile* m_projectiles;   // 0x14
    float m_maxAge;              // 0x18
    vec3 m_gravity;              // 0x1c
    CCollision* m_collision;     // 0x28 borrowed
    float m_normalBounce;        // 0x2c
    float m_tangentRetention;    // 0x30
    float m_drag;                // 0x34
    unsigned int m_flags;        // 0x38
};

class Fragments : public Ballistics {
public:
    enum EParticleType { PARTICLE_SEGMENT=0, PARTICLE_RAY=1, PARTICLE_TRAIL=2, PARTICLE_FLASH=3 };
    struct Particle {
        vec3 m_origin;
        int m_variant;
        float m_rotation;
        int m_removeOnKinematic;
    };
    Fragments(int capacity, float age, vec3 gravity, CCollision*, EParticleType type, vec3 size, const char* model);
    virtual ~Fragments();
    virtual void OnPreSimulate(float elapsed);
    virtual void OnPostSimulate(float elapsed);
    virtual void OnActivate(Projectile*) {} // Original 0xf13f0.
    virtual int OnCollision(Projectile*, vec3*, vec3*, CCollision::Triangle*);
    int AddProjectile(const vec3&, const vec3&, float spread, int removeOnKinematic);
    void Paint3D(swerve::Graphics3D<CSwerveAssertingErrorHandler>& graphics);
    static Fragments* g_CurrentlySimulatedFragments;
    /** original Body0x1080dc, keeps the original range check that includes the end pointer; not corrected to a half-open interval. */
    static int GetFlag(const Projectile* projectile)
    {
        if (projectile == NULL) {
            return 0;
        }
        Fragments* fragments = g_CurrentlySimulatedFragments;
        if (fragments == NULL) {
            return 0;
        }
        if (projectile < fragments->m_projectiles ||
            projectile > fragments->m_projectiles + fragments->m_capacity) {
            return 0;
        }
        if (!projectile->m_active || projectile->m_userData == NULL) {
            return 0;
        }
        return static_cast<const Particle*>(projectile->m_userData)->m_removeOnKinematic;
    }

    EParticleType m_type;        // 0x3c
    vec3 m_size;                // 0x40
    int m_particleNext;          // 0x4c
    int m_variantNext;           // 0x50
    Particle* m_particles;       // 0x54
    swerve::World<CSwerveAssertingErrorHandler> m_scene; // 0x58
    swerve::Group<CSwerveAssertingErrorHandler> m_root;  // 0x5c
    swerve::Group<CSwerveAssertingErrorHandler> m_variants[16]; // 0x60
};

class SmokeTrail : public Ballistics {
public:
    struct Rotation { float m_angle; float m_speed; };
    SmokeTrail(int capacity, float age, CCollision* collision);
    virtual ~SmokeTrail();
    virtual void OnPreSimulate(float) {} // Original 0xf13f4.
    virtual void OnPostSimulate(float) {} // Original 0xf13f8.
    virtual void OnActivate(Projectile*) {} // Original 0xf13fc.
    virtual int OnCollision(Projectile*, vec3*, vec3*, CCollision::Triangle*) { return 0; } // 0xf1400.
    int AddProjectile(const vec3&, const vec3&, float spread);
    void Simulate(float elapsed);
    void Paint3D(swerve::Graphics3D<CSwerveAssertingErrorHandler>& graphics);
    swerve::World<CSwerveAssertingErrorHandler> m_scene; // 0x3c
    swerve::Group<CSwerveAssertingErrorHandler> m_root;  // 0x40
    swerve::Group<CSwerveAssertingErrorHandler> m_variants[16]; // 0x44
    int m_rotationNext;           // 0x84
    Rotation* m_rotations;        // 0x88
    swerve::Transform<CSwerveAssertingErrorHandler> m_basis; // 0x8c
    swerve::Transform<CSwerveAssertingErrorHandler> m_scratch; // 0x90
};

class GrenadeLauncher : public Ballistics {
public:
    struct Flight {
        float m_distance;
        float m_trailDistance;
        int m_splitCount;
        vec3 m_aimDirection;
        bool m_guided;
        int m_flash1;
        int m_flash2;
        bool m_alternate;
        bool m_targetValid;
        vec3 m_target;
    };
    GrenadeLauncher(CCollision*, const char* model, const char* alternateModel);
    virtual ~GrenadeLauncher();
    virtual void OnPreSimulate(float) {} // Original 0xf1404.
    virtual void OnPostSimulate(float) {} // Original 0xf1408.
    virtual void OnActivate(Projectile*) {} // Original 0xf140c.
    virtual int OnCollision(Projectile*, vec3*, vec3*, CCollision::Triangle*);
    void SetTrail(float spacing, float velocity, float age);
    int AddProjectile(const vec3&, const vec3&, float spread, vec3 aimDirection, bool alternate);
    void Simulate(float elapsed);
    void Paint3D(swerve::Graphics3D<CSwerveAssertingErrorHandler>& graphics);
    void IntersectProjectiles(HandGrenadeBallistics* grenades);
    void SetExplosionRadius(float radius);
    swerve::World<CSwerveAssertingErrorHandler> m_model; // 0x3c
    swerve::World<CSwerveAssertingErrorHandler> m_alternateModel; // 0x40
    SmokeTrail* m_smoke;           // 0x44
    Fragments* m_flash;            // 0x48
    int m_flightNext;              // 0x4c
    Flight* m_flights;             // 0x50
    float m_trailSpacing;          // 0x54
    float m_trailVelocity;         // 0x58
    float m_trailAge;              // 0x5c
    float m_explosionRadius;       // 0x60
};

class UnitBodyBallistics : public Ballistics {
public:
    UnitBodyBallistics(CCollision*, CAuxRender*);
    virtual ~UnitBodyBallistics();
    virtual void OnPreSimulate(float) {} // 0xf1410.
    virtual void OnPostSimulate(float) {} // 0xf1414.
    virtual void OnActivate(Projectile*) {} // 0xf1418.
    virtual int OnCollision(Projectile*, vec3*, vec3*, CCollision::Triangle*);
    void Paint3D(swerve::Graphics3D<CSwerveAssertingErrorHandler>& graphics);
    CAuxRender* m_auxRender;       // 0x3c borrowed
};

class SwerveBallistics : public Ballistics {
public:
    SwerveBallistics(int capacity, float age, CCollision*);
    virtual ~SwerveBallistics();
    virtual void OnPreSimulate(float) {} // 0xf141c.
    virtual void OnPostSimulate(float) {} // 0xf1420.
    virtual void OnActivate(Projectile*);
    virtual int OnCollision(Projectile*, vec3*, vec3*, CCollision::Triangle*);
    void AddProjectile(swerve::Group<CSwerveAssertingErrorHandler>, const vec3&, const vec3&, float spread);
    void PostSimulate();
};

class HitPoints : public Ballistics {
public:
    HitPoints(int capacity, float age, CCollision*);
    virtual ~HitPoints();
    virtual void OnPreSimulate(float) {} // 0xf1424.
    virtual void OnPostSimulate(float) {} // 0xf1428.
    virtual void OnActivate(Projectile*);
    virtual int OnCollision(Projectile*, vec3*, vec3*, CCollision::Triangle*);
    void AddWindow(Window* window);
    void AddWindow(Window*, const vec3&, const vec3&);
    void PostSimulate();
    vec3 m_position;               // 0x3c
    vec3 m_velocity;               // 0x48
    int m_selected;               // 0x54 active-list position
};

class HandGrenadeBallistics : public Ballistics {
public:
    class HandGrenade {
    public:
        // Original FragGrenade/StoneGrenade ctors store 0x1c before the derived vptr: base ctor init.
        HandGrenade() : m_rotationSpeed(0.0f) {}
        virtual ~HandGrenade() {}
        virtual swerve::Group<CSwerveAssertingErrorHandler> GetVisualModel() = 0;
        virtual void OnBulletHit(float distance) = 0;
        virtual int OnCollision(const vec3&, const vec3&, const CCollision::Triangle&) = 0;
        virtual void OnUpdate(float elapsed, const Projectile*) = 0;
        virtual bool IsExploded() = 0;
        virtual bool HasIgniteFlare() { return true; } // Original 0xf1430.
        virtual void SetLuminance(float) {} // 0xf1434.
        virtual bool IsDynamicallyScaled() { return true; } // 0xf1438.
        virtual float GetHitzoneRadius() { return 0.2f; } // 0xf143c.
        virtual bool CanBeBeacon() { return true; } // 0xf1448.
        virtual bool HasPlayerRelativeDamping() { return true; } // 0xf144c.
        vec3 m_normal;            // 0x04
        vec3 m_rotationAxis;      // 0x10
        float m_rotationSpeed;    // 0x1c
    };
    class FragGrenade : public HandGrenade {
    public:
        FragGrenade();
        virtual ~FragGrenade();
        virtual swerve::Group<CSwerveAssertingErrorHandler> GetVisualModel();
        virtual void OnBulletHit(float distance);
        virtual int OnCollision(const vec3&, const vec3&, const CCollision::Triangle&);
        virtual void OnUpdate(float elapsed, const Projectile*);
        virtual bool IsExploded();
        swerve::Group<CSwerveAssertingErrorHandler> m_model; // 0x20
        float m_delay;           // 0x24
        bool m_contacted;        // 0x28
        bool m_exploded;         // 0x29
        int m_collisionCount;    // 0x2c
    };
    class StoneGrenade : public HandGrenade {
    public:
        StoneGrenade(swerve::Group<CSwerveAssertingErrorHandler>, int hp);
        virtual ~StoneGrenade();
        virtual swerve::Group<CSwerveAssertingErrorHandler> GetVisualModel();
        virtual void OnBulletHit(float distance);
        virtual int OnCollision(const vec3&, const vec3&, const CCollision::Triangle&);
        virtual void OnUpdate(float elapsed, const Projectile*);
        virtual bool IsExploded();
        virtual bool HasIgniteFlare() { return false; } // 0xf1450.
        virtual void SetLuminance(float brightness);
        virtual bool IsDynamicallyScaled() { return false; } // 0xf1454.
        virtual float GetHitzoneRadius() { return 0.5f; } // 0xf1458.
        virtual bool CanBeBeacon() { return false; } // 0xf1460.
        virtual bool HasPlayerRelativeDamping() { return false; } // 0xf1464.
        swerve::Group<CSwerveAssertingErrorHandler> m_model; // 0x20
        bool m_exploded;         // 0x24
        bool m_hitWorld;         // 0x25
        int m_hp;                // 0x28
        CDH_Animation* m_lighting; // 0x2c
    };
    HandGrenadeBallistics(CCollision*);
    virtual ~HandGrenadeBallistics();
    virtual void OnPreSimulate(float elapsed);
    virtual void OnPostSimulate(float) {} // Original 0xf142c.
    virtual void OnActivate(Projectile*);
    virtual int OnCollision(Projectile*, vec3*, vec3*, CCollision::Triangle*);
    void AddProjectile(const vec3&, const vec3&, float spread, HandGrenade*);
    void PostSimulate();
    void Paint3D(swerve::Graphics3D<CSwerveAssertingErrorHandler>& graphics);
    bool Pick(const CCollision::Line& line, int& activeIndex);
    void Pick(float x, float y, swerve::Camera<CSwerveAssertingErrorHandler> camera);
    static int GetHitzoneUserId();
    swerve::World<CSwerveAssertingErrorHandler> m_flareScene; // 0x3c
    swerve::Group<CSwerveAssertingErrorHandler> m_flareRoot;  // 0x40
    swerve::Group<CSwerveAssertingErrorHandler> m_flares[8];  // 0x44
    swerve::World<CSwerveAssertingErrorHandler> m_auxScene;  // 0x64
    swerve::Mesh<CSwerveAssertingErrorHandler> m_sphere;     // 0x68
};
#endif
