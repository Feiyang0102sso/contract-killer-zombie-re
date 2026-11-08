// PROVISIONAL (created by b064): the CCollision owner is being recovered separately.
// ABI and collision records observed by Ballistics.cpp; no replacement owner implementation.
// b065 owner recovery now expands the observed layout; the preceding comments record its first use.
#ifndef CCOLLISION_H
#define CCOLLISION_H
#include "MathLib.h"
#include "tws.h"
#include "CVector.h"
class CUnitBody;
namespace swerve { template<class ErrorHandler> class Node; }
class CSwerveAssertingErrorHandler;

class CCollision {
public:
    struct Line { vec3 m_from; vec3 m_to; };
    struct Sphere { vec3 m_center; float m_radius; };
    struct AABB {
        void Split(AABB& low, AABB& high) const;
        void Add(const vec3& point);
        vec3 m_min;
        vec3 m_max;
    };
    struct Triangle { int m_vertices[3]; int m_meshIndex; };
    struct Ellipsoid {
        vec3 m_radii;       // 0x00
        vec3 m_angles;      // 0x0c, rotations about X/Y/Z, in this order
        vec3 m_position;    // 0x18
    };
    struct Penetration {
        // No user constructor: CVector<Penetration>::AddAll in the original
        // zero-fills each slot (memset 0x18) before copying, which only happens
        // when the element type has no user-declared constructor.
        int m_triangle;     // 0x00
        float m_distance;   // 0x04
        float m_radius;     // 0x08
        vec3 m_point;       // 0x0c
    };
    class OcTreeSector {
    public:
        OcTreeSector(OcTreeSector* parent, const AABB& bounds, CCollision* owner, int leafThreshold);
        virtual ~OcTreeSector();
        OcTreeSector* m_parent;       // 0x04
        AABB m_bounds;                // 0x08
        int m_triangleCount;          // 0x20
        int* m_triangleIndices;       // 0x24
        OcTreeSector* m_children[8];  // 0x28
    };
    class CKinematicBody {
    public:
        CKinematicBody() : m_body(NULL) {} // original Body103526 first clears the virtual-base backptr; 1037d6 only then writes the complete this
        // original Body D1/D0 and the virtual-base thunk make no outgoing base-class call; only a non-owning backptr, destructor inline with no side effects.
        virtual ~CKinematicBody() {}
        // b067/b069 original derived symbols confirm the third slot is named Intersect; the earlier Pick was a temporary slot name, no virtual slot added.
        virtual bool Intersect(const Line&, vec3&, vec3&) = 0;
        CUnitBody* m_body;            // 0x04 actual body back pointer
    };
    CCollision(swerve::Node<CSwerveAssertingErrorHandler>& node, int leafThreshold);
    virtual ~CCollision();
    void CollectMeshInfo(swerve::Node<CSwerveAssertingErrorHandler>& node);
    void BuildMesh(swerve::Node<CSwerveAssertingErrorHandler>& node, unsigned int& vertexOffset,
                   unsigned int& triangleOffset, unsigned int& meshOffset);
    bool IntersectNearest(const Line& line, vec3& point, vec3& normal, Triangle& triangle, bool kinematic);
    void IntersectNearestOcTree(OcTreeSector* sector, const Line& line, vec3& point, vec3& normal,
                                int& hitCount, Triangle& triangle);
    vec3 GetPenetration(Ellipsoid& ellipsoid);
    void CalculatePenetrationCache(const Mat4x4& transform, const Mat4x4& inverse);
    void CalculatePenetrationOcTree(OcTreeSector* sector, const AABB& bounds,
                                   const Mat4x4& transform, const Mat4x4& inverse);
    void CalculatePenetrationTriangle(const Mat4x4& transform, const Mat4x4& inverse, int index);
    void MoveTo(Ellipsoid& ellipsoid, const vec3& target, bool adjustHeight, int unused);
    void AddKinematicBody(CKinematicBody* body);
    void RemoveKinematicBody(CKinematicBody* body);
    static bool Intersect(const Line& line, const Sphere& sphere, vec3& point);
    static bool Intersect(const AABB& box, const vec3& first, const vec3& second, const vec3& third);
    static bool Intersect(const Line& line, const AABB& box, vec3& point);
    static bool Intersect(const Sphere& sphere, const vec3& first, const vec3& second, const vec3& third,
                          vec3& point, float& distance);
    static void GetDistance(const Sphere& sphere, const vec3& normal, const vec3& first,
                            const vec3& second, const vec3& third, vec3& point, float& distance);
    static vec3 ClosestPointOnEdge(const vec3& point, const vec3& start, const vec3& end);
    static bool IsPointWithinTriangle(const vec3& point, const vec3& normal, const vec3& first,
                                      const vec3& second, const vec3& third);

    int m_vertexCount;               // 0x04
    int m_triangleCount;             // 0x08
    int m_meshCount;                 // 0x0c
    vec3* m_vertices;                // 0x10
    Triangle* m_triangles;           // 0x14
    vec3* m_normals;                 // 0x18
    vec3* m_edgeVectors1c;           // 0x1c
    vec3* m_edgeVectors20;           // 0x20
    vec4* m_planes;                  // 0x24
    swerve::Appearance<CSwerveAssertingErrorHandler>* m_meshNodes; // 0x28 owned Swerve wrapper array (owner will complete); BuildMesh +0x144 proves Appearance
    OcTreeSector* m_root;            // 0x2c
    // The initial m_data30 opaque view is now resolved by constructor/GetPenetration offsets.
    swerve::Transform<CSwerveAssertingErrorHandler> m_transform;        // 0x30
    swerve::Transform<CSwerveAssertingErrorHandler> m_inverseTransform; // 0x34
    vec3 m_movementFilter;           // 0x38
    Ellipsoid m_currentEllipsoid;    // 0x44
    com::glu::platform::core::CVector<Penetration> m_penetrations; // 0x68
    Ellipsoid m_cachedEllipsoid;     // 0x80
    com::glu::platform::core::CVector<unsigned int> m_cachedTriangles; // 0xa4
    bool m_usingCache;               // 0xbc
    com::glu::platform::core::CVector<CKinematicBody*> m_kinematicBodies; // 0xc0
    Triangle m_kinematicTriangle;    // 0xd8, initialized to all -1
    CKinematicBody* m_lastKinematic; // 0xe8
};
#include "CCollision.inl"
#endif
