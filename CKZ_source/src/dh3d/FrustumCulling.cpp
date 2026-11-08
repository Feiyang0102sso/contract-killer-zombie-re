// FrustumCulling.cpp - view frustum of a camera and bounding sphere tests against it.
#include "FrustumCulling.h"
#include "CSwerve.h"
#include "App.h"
#include "DGHelper.h"
#include "CSwerveGame.h"

/** Start values of the bounding box search. */
static const float BOX_START = 10000.0f;

FrustumCulling::FrustumCulling(swerve::Camera<CSwerveAssertingErrorHandler> camera)
{
    SetCamera(camera);
}

bool FrustumCulling::CheckVisibility(const CullSphere& sphere) const
{
    swerve::Transform<CSwerveAssertingErrorHandler> transform(CSwerve::GetInstance()->m_swerve);
    CSwerveGame* game = ((App*)WindowApp::m_instance)->m_helper->m_swerveGame;
    game->m_cameraNode.getTransformTo(m_camera, transform);
    float point[4];
    point[0] = sphere.m_center.x + sphere.m_offset.x;
    point[1] = sphere.m_center.y + sphere.m_offset.y;
    point[2] = sphere.m_center.z + sphere.m_offset.z;
    point[3] = 1.0f;
    transform.transform(4, point);
    for (int index = 0; index < PLANE_COUNT; index++) {
        const Plane& plane = m_planes[index];
        float distance = plane.m_normal.m_x * point[0] + plane.m_normal.m_y * point[1] +
                         plane.m_normal.m_z * point[2] - plane.m_distance;
        if (distance < -sphere.GetRadius()) {
            return false;
        }
    }
    return true;
}

float FrustumCulling::CullSphere::GetRadius() const
{
    float scale[3];
    m_group.getScale(scale);
    float largest = scale[0];
    // Original BGT falls through for unordered scale comparisons as well.
    if (!(scale[0] > scale[1])) {
        largest = scale[2];
        if (scale[1] > scale[2]) {
            largest = scale[1];
        }
    }
    return m_radius * largest;
}

void FrustumCulling::UpdatePlanes()
{
    float params[4];
    m_camera.getProjection(params);
    float halfFov = params[0] * 0.5f;
    float sinHalf = MathLib::Sin(halfFov);
    float cosHalf = MathLib::Cos(halfFov);
    float slope = cosHalf / (params[1] * sinHalf + cosHalf);
    float sideX = MathLib::InvSqrt(slope) * slope;
    float sideZ = 1.0f - sideX * sideX;
    float sideZLength = MathLib::InvSqrt(sideZ) * sideZ;
    for (int index = 0; index < PLANE_COUNT; index++) {
        Plane& plane = m_planes[index];
        switch (index) {
        case 0:     // near
            plane.m_normal.m_x = 0;
            plane.m_normal.m_y = 0;
            plane.m_normal.m_z = -1.0f;
            plane.m_distance = params[2];
            break;
        case 2:     // left
            plane.m_normal.m_x = sideX;
            plane.m_normal.m_y = 0;
            plane.m_normal.m_z = -sideZLength;
            plane.m_distance = 0;
            break;
        case 3:     // right
            plane.m_normal.m_x = -sideX;
            plane.m_normal.m_y = 0;
            plane.m_normal.m_z = -sideZLength;
            plane.m_distance = 0;
            break;
        case 1:     // far
            plane.m_normal.m_x = 0;
            plane.m_normal.m_y = 0;
            plane.m_normal.m_z = 1.0f;
            plane.m_distance = -params[3];
            break;
        case 4:     // bottom
            plane.m_normal.m_x = 0;
            plane.m_normal.m_y = -cosHalf;
            plane.m_normal.m_z = -sinHalf;
            plane.m_distance = 0;
            break;
        case 5:     // top
            plane.m_normal.m_x = 0;
            plane.m_normal.m_y = cosHalf;
            plane.m_normal.m_z = -sinHalf;
            plane.m_distance = 0;
            break;
        }
    }
}

bool FrustumCulling::FindMeshRecursively(swerve::Group<CSwerveAssertingErrorHandler> group,
                                         swerve::Mesh<CSwerveAssertingErrorHandler>& mesh)
{
    if (group.m_interface == NULL) {
        return false;
    }
    for (int index = 0; index < group.getChildCount(); index++) {
        swerve::Mesh<CSwerveAssertingErrorHandler> child = group.getChild(index);
        bool found;
        if (child.m_interface != NULL) {
            mesh = child;
            found = true;
        } else {
            swerve::Group<CSwerveAssertingErrorHandler> subGroup = group.getChild(index);
            found = FindMeshRecursively(subGroup, mesh);
        }
        if (found == true) {
            return true;
        }
    }
    return false;
}

void FrustumCulling::GetBoundingSphere(swerve::Group<CSwerveAssertingErrorHandler> group, swvVector3D& center,
                                       float& radius)
{
    radius = -1.0f;
    center.x = 0;
    center.y = 0;
    center.z = 0;
    swerve::Mesh<CSwerveAssertingErrorHandler> firstMesh;
    if (FindMeshRecursively(group, firstMesh) == true) {
        swerve::Node<CSwerveAssertingErrorHandler> parent = firstMesh.getParent();
        group = swerve::Group<CSwerveAssertingErrorHandler>(parent);
    }
    if (group.m_interface == NULL) {
        return;
    }
    float maxX = -BOX_START;
    float minX = BOX_START;
    float maxY = -BOX_START;
    float minY = BOX_START;
    float maxZ = -BOX_START;
    float minZ = BOX_START;
    for (int index = 0; index < group.getChildCount(); index++) {
        swerve::Mesh<CSwerveAssertingErrorHandler> mesh = group.getChild(index);
        if (mesh.m_interface != NULL) {
            swerve::VertexBuffer<CSwerveAssertingErrorHandler> vertexBuffer = mesh.getVertexBuffer();
            int vertexCount = vertexBuffer.getVertexCount();
            float scaleBias[4];
            swerve::VertexArray<CSwerveAssertingErrorHandler> positions = vertexBuffer.getPositions(scaleBias);
            swerve::Transform<CSwerveAssertingErrorHandler> transform(CSwerve::GetInstance()->m_swerve);
            float* vertices = new float[vertexCount * 4];
            transform.transform((swerve::IVertexArrayInterface*)positions.m_interface, vertexCount * 4, vertices, true);
            mesh.getTransformTo(group, transform);
            float* vertex = vertices;
            for (int vertexIndex = 0; vertexIndex < vertexCount; vertexIndex++) {
                vertex[0] = vertex[0] * scaleBias[0] + scaleBias[1];
                vertex[1] = vertex[1] * scaleBias[0] + scaleBias[2];
                vertex[2] = vertex[2] * scaleBias[0] + scaleBias[3];
                vertex[3] = 1.0f;
                transform.transform(4, vertex);
                if (vertex[2] > maxZ) {
                    maxZ = vertex[2];
                }
                if (vertex[2] < minZ) {
                    minZ = vertex[2];
                }
                if (vertex[1] > maxY) {
                    maxY = vertex[1];
                }
                if (vertex[1] < minY) {
                    minY = vertex[1];
                }
                if (vertex[0] > maxX) {
                    maxX = vertex[0];
                }
                if (vertex[0] < minX) {
                    minX = vertex[0];
                }
                vertex += 4;
            }
            center.x = (maxX + minX) * 0.5f;
            center.y = (maxY + minY) * 0.5f;
            center.z = (maxZ + minZ) * 0.5f;
            vertex = vertices;
            for (int vertexIndex = 0; vertexIndex < vertexCount; vertexIndex++) {
                float deltaX = vertex[0] - center.x;
                float deltaY = vertex[1] - center.y;
                float deltaZ = vertex[2] - center.z;
                float distance = DGHelper::sqrt(deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ);
                if (radius < distance) {
                    radius = distance;
                }
                vertex += 4;
            }
            if (vertices != NULL) {
                delete[] vertices;
            }
        } else {
            swerve::Sprite3D<CSwerveAssertingErrorHandler> sprite = group.getChild(index);
            if (sprite.m_interface != NULL) {
                float scale[3];
                sprite.getScale(scale);
                radius = DGHelper::sqrt(scale[0] * scale[0] + scale[1] * scale[1]) * 0.5f;
            }
        }
    }
}

FrustumCulling::CullSphere::CullSphere(swerve::Group<CSwerveAssertingErrorHandler> group, float radiusScale)
    : m_group(group)
{
    m_field04.m_interface = NULL; // The original ctor zeroes +4; the real wrapper must also release it in the destructor.
    m_center.x = 0;
    m_center.y = 0;
    m_center.z = 0;
    m_offset.x = 0;
    m_offset.y = 0;
    m_offset.z = 0;
    m_radius = 0;
    GetBoundingSphere(group, m_center, m_radius);
    m_radius = m_radius * radiusScale;
}
