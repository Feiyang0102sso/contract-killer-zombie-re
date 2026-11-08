// DGWorld.h - the 3D world of a level: visibility of its nodes per camera angle and the hit effects.
#ifndef DGWORLD_H
#define DGWORLD_H

#include "tws.h"
#include "Vector_ss.h"
#include "FrustumCulling.h"

class vec3;

class DGWorld {
public:
    /** Hit effect kinds (index of m_effectNodes / m_effectDurations). */
    enum {
        EFFECT_BLAST = 3,
        EFFECT_BODY_BLOW = 4,
        EFFECT_HEAD_BLOW = 5,
        EFFECT_BODY_HIT = 6,
        EFFECT_HEAL_BLAST = 7,
        EFFECT_SLIME_BLAST = 8,
        EFFECT_STONE_BLAST = 9,
        EFFECT_COUNT = 10
    };

    /** A running hit effect. */
    struct EffectInstance {
        // Original placement copies zero the aggregate before member copy construction.
        int m_type;                                                 // 0x00
        swerve::Transform<CSwerveAssertingErrorHandler> m_transform;  // 0x04
        int m_time;                                                 // 0x08 milliseconds since the start
    };

    DGWorld(swerve::World<CSwerveAssertingErrorHandler> world, const char* name);
    ~DGWorld();

    /** Load the effect scenes and their durations. */
    void loadHitEffects();
    /** Animate the world and show the nodes visible from the camera angle. */
    void update();
    /** Start effect `type` at `position` facing `direction`. */
    void BlastImpl(int type, const vec3& position, const vec3& direction);
    void blast(const vec3& position, const vec3& direction);
    void slime_blast(const vec3& position, const vec3& direction);
    void stoneBlast(const vec3& position, const vec3& direction);
    void bodyBlow(const vec3& position, const vec3& direction);
    void headBlow(const vec3& position, const vec3& direction);
    void bodyHit(const vec3& position, const vec3& direction);
    void healBlast(const vec3& position, const vec3& direction);
    /** Animate and render the running effects (finished ones are removed). */
    void paintHitEffects(swerve::Graphics3D<CSwerveAssertingErrorHandler>& graphics);

    swerve::World<CSwerveAssertingErrorHandler> m_world;                    // 0x00
    swerve::Group<CSwerveAssertingErrorHandler> m_fireGeometry;             // 0x04
    swerve::Transform<CSwerveAssertingErrorHandler> m_fireTransform;        // 0x08 fire geometry to world
    bool m_field0c;                                                         // 0x0c
    Vector<FrustumCulling::CullSphere> m_cullSpheres;                       // 0x10
    unsigned char* m_visibility;                                            // 0x20 per angle row and node
    swerve::Node<CSwerveAssertingErrorHandler>* m_nodes;                    // 0x24 new[]
    int m_nodeCount;                                                        // 0x28
    swerve::RayIntersection<CSwerveAssertingErrorHandler> m_rayIntersection;  // 0x2c
    int m_field30[6];                                                       // 0x30 not touched here
    swerve::Node<CSwerveAssertingErrorHandler> m_effectNodes[EFFECT_COUNT];   // 0x48
    int m_effectDurations[EFFECT_COUNT];                                    // 0x70 milliseconds
    Vector<EffectInstance> m_effects;                                       // 0x98
};

#endif
