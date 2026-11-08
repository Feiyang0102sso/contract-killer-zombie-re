// FrustumCulling.h - view frustum of a camera and bounding sphere tests against it.
#ifndef FRUSTUMCULLING_H
#define FRUSTUMCULLING_H

#include "tws.h"
#include "MathLib.h"

struct swvVector3D {
    float x;
    float y;
    float z;
};

class FrustumCulling {
public:
    /** Bounding sphere of a group (in the group's coordinates, scaled by the group's scale). */
    class CullSphere {
    public:
        /** Sphere around the meshes of `group`, radius multiplied by `radiusScale`. */
        CullSphere(swerve::Group<CSwerveAssertingErrorHandler> group, float radiusScale);

        /** Radius scaled by the largest scale of the group. */
        float GetRadius() const;

        swerve::Group<CSwerveAssertingErrorHandler> m_group;   // 0x00
        // Formerly temporary int m_field04; Body Cleanup@1042c8..1042f4 confirms this slot has Release ownership.
        swerve::QueryInterface<CSwerveAssertingErrorHandler> m_field04; // 0x04, concrete derived interface not confirmed
        swvVector3D m_center;                                   // 0x08
        swvVector3D m_offset;                                   // 0x14 added to the center
        float m_radius;                                         // 0x20
    };

    /** A plane: points p with normal . p - distance >= 0 are inside. */
    struct Plane {
        vec3 m_normal;          // 0x00
        float m_distance;       // 0x0c
    };

    enum { PLANE_COUNT = 6 };

    FrustumCulling(swerve::Camera<CSwerveAssertingErrorHandler> camera);

    /** False when the sphere is completely outside one of the planes. */
    bool CheckVisibility(const CullSphere& sphere) const;
    /** Planes from the perspective projection of the camera (camera coordinates). */
    void UpdatePlanes();

    void SetCamera(swerve::Camera<CSwerveAssertingErrorHandler> camera)
    {
        m_camera = camera;
        UpdatePlanes();
    }

    /** First mesh in `group` or its descendants (depth first); true when found. */
    static bool FindMeshRecursively(swerve::Group<CSwerveAssertingErrorHandler> group,
                                    swerve::Mesh<CSwerveAssertingErrorHandler>& mesh);
    /** Center and radius of the meshes / sprites of `group` (radius -1 when there is none). */
    static void GetBoundingSphere(swerve::Group<CSwerveAssertingErrorHandler> group, swvVector3D& center,
                                  float& radius);

    Plane m_planes[PLANE_COUNT];                            // 0x00
    swerve::Camera<CSwerveAssertingErrorHandler> m_camera;  // 0x60
};

#endif
