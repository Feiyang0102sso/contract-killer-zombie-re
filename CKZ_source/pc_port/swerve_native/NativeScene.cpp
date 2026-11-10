// Native Swerve scene graph slots: Object3D, Transformable, Node, Group, World, Mesh, SkinnedMesh,
// Camera, Light, Sprite3D, Appearance, Texture2D, Background, CompositingMode, Material,
// VertexBuffer, VertexArray, IndexBuffer and the animation classes.
// Slot numbers and signatures follow swerve_compat/tws.h (slot = byte offset / 4).
#include "NativeRuntime.h"
#include "M3gCkzExtensions.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <vector>

namespace ckz::native {
namespace {

/** The m3gcore handle of `object` as the m3gcore type the call expects. */
template<class Handle>
Handle H(const void* object)
{
    return reinterpret_cast<Handle>(HandleOf(object));
}

bool IsNodeClass(ClassId id)
{
    switch (id) {
    case ClassCamera:
    case ClassGroup:
    case ClassWorld:
    case ClassLight:
    case ClassMesh:
    case ClassMorphingMesh:
    case ClassSkinnedMesh:
    case ClassSprite3D:
        return true;
    default:
        return false;
    }
}

// ================= Object3D (slots 3-24) =================

int GetUserID(Object* self, int* userID)
{
    *userID = m3gGetUserID(self->handle);
    return TakeError();
}

int SetUserID(Object* self, int userID)
{
    m3gSetUserID(self->handle, userID);
    return TakeError();
}

int GetAnimationTrackCount(Object* self, int* count)
{
    *count = m3gGetAnimationTrackCount(self->handle);
    return TakeError();
}

int GetAnimationTrack(Object* self, int index, void** track)
{
    WrapInto(reinterpret_cast<M3GObject>(m3gGetAnimationTrack(self->handle, index)), track);
    return TakeError();
}

int AddAnimationTrack(Object* self, Object* track)
{
    m3gAddAnimationTrack(self->handle, H<M3GAnimationTrack>(track));
    return TakeError();
}

int RemoveAnimationTrack(Object* self, Object* track)
{
    m3gRemoveAnimationTrack(self->handle, H<M3GAnimationTrack>(track));
    return TakeError();
}

int Animate(Object* self, int worldTime, int* validity)
{
    const M3Gint result = m3gAnimate(self->handle, worldTime);
    if (validity != nullptr) {
        *validity = result;
    }
    return TakeError();
}

int Find(Object* self, int userID, void** found)
{
    WrapInto(m3gFind(self->handle, userID), found);
    return TakeError();
}

/** The objects this one references; the first `capacity` go to `references` (new references). */
int GetReferences(Object* self, unsigned int capacity, void** references, int* count)
{
    const M3Gint total = m3gGetReferences(self->handle, nullptr, 0);
    if (count != nullptr) {
        *count = total;
    }
    if (references == nullptr || capacity == 0) {
        return TakeError();
    }
    std::vector<M3Gulong> handles(static_cast<size_t>(total) + 1);
    m3gGetReferences(self->handle, handles.data(), total);
    for (unsigned int index = 0; index < capacity; index++) {
        if (static_cast<int>(index) < total) {
            WrapInto(reinterpret_cast<M3GObject>(static_cast<uintptr_t>(handles[index])), &references[index]);
        } else {
            references[index] = nullptr;
        }
    }
    return TakeError();
}

/** User parameter `id` from the M3G file (kept by the loader, see NativeLoader.cpp). */
int GetUserParameter(Object* self, int id, int bufferSize, char* buffer, int* length)
{
    const int found = FindUserParameter(self->handle, id, bufferSize, buffer);
    if (found < 0) {
        if (length != nullptr) {
            *length = 0;
        }
        return ErrorInvalidParameter;
    }
    if (length != nullptr) {
        *length = found;
    }
    return ErrorNone;
}

int Duplicate(Object* self, void** copy)
{
    // m3gcore records (original, clone) pairs of every duplicated node to fix up alignment references.
    int referenceSlots = 2;
    if (IsNodeClass(self->objectClass->id)) {
        referenceSlots = 2 * m3gGetSubtreeSize(H<M3GNode>(self)) + 2;
    }
    std::vector<M3Gulong> references(static_cast<size_t>(referenceSlots));
    const M3GObject clone = m3gDuplicate(self->handle, references.data());
    WrapInto(clone, copy);
    return TakeError();
}

// ================= Transformable (slots 25-43) =================

int GetTranslation(Object* self, float* translation)
{
    m3gGetTranslation(H<M3GTransformable>(self), translation);
    return TakeError();
}

int SetTranslation(Object* self, const float* translation)
{
    m3gSetTranslation(H<M3GTransformable>(self), translation[0], translation[1], translation[2]);
    return TakeError();
}

int GetScale(Object* self, float* scale)
{
    m3gGetScale(H<M3GTransformable>(self), scale);
    return TakeError();
}

int SetScale(Object* self, const float* scale)
{
    m3gSetScale(H<M3GTransformable>(self), scale[0], scale[1], scale[2]);
    return TakeError();
}

/** Orientation as angle (degrees) and axis x, y, z. */
int GetOrientation(Object* self, float* angleAxis)
{
    m3gGetOrientation(H<M3GTransformable>(self), angleAxis);
    return TakeError();
}

int SetOrientation(Object* self, const float* angleAxis)
{
    m3gSetOrientation(H<M3GTransformable>(self), angleAxis[0], angleAxis[1], angleAxis[2], angleAxis[3]);
    return TakeError();
}

/** Orientation as quaternion x, y, z, w (CUnitBody::UpdateTransform reads it, then ignores it). */
int GetOrientationQuat(Object* self, float* quaternion)
{
    constexpr float DegreesToRadians = 3.14159265358979f / 180.0f;
    float angleAxis[4];
    m3gGetOrientation(H<M3GTransformable>(self), angleAxis);
    const float halfAngle = angleAxis[0] * DegreesToRadians * 0.5f;
    const float sine = sinf(halfAngle);
    quaternion[0] = angleAxis[1] * sine;
    quaternion[1] = angleAxis[2] * sine;
    quaternion[2] = angleAxis[3] * sine;
    quaternion[3] = cosf(halfAngle);
    return TakeError();
}

int Translate(Object* self, const float* translation)
{
    m3gTranslate(H<M3GTransformable>(self), translation[0], translation[1], translation[2]);
    return TakeError();
}

int PostRotate(Object* self, const float* angleAxis)
{
    m3gPostRotate(H<M3GTransformable>(self), angleAxis[0], angleAxis[1], angleAxis[2], angleAxis[3]);
    return TakeError();
}

/**
 * Slot 0xa4 (named getCompositeTransform in tws.h) returns only the generic matrix, M3G's
 * getTransform: on the ARM backend the weapon's eyecamTransform node has translation
 * (0.4, 0.03, 0.15) and a 120 degree orientation, yet this slot returns an identity matrix.
 * The game reads it, rotates it and writes it back with setTransform every frame
 * (CDH_Weapon::SetInertionAngle); the full composite would apply the node's translation and
 * orientation twice and turn the weapon camera away from the weapon.
 */
int GetGenericTransform(Object* self, Object* transform)
{
    m3gGetTransform(H<M3GTransformable>(self), MatrixOf(transform));
    return TakeError();
}

/**
 * M3G setTransform: the generic matrix after translation, orientation and scale.
 * m3gcore demands a node transform whose bottom row is exactly (0, 0, 0, 1). The game passes
 * products of rotations whose bottom row is off by ~1e-9, which Swerve accepts, so a bottom row
 * within BottomRowTolerance is made exact first.
 */
int SetTransform(Object* self, Object* transform)
{
    constexpr float BottomRowTolerance = 1e-4f;
    M3GTransformable transformable = H<M3GTransformable>(self);
    if (transform == nullptr) {
        m3gSetTransform(transformable, nullptr);
        return TakeError();
    }
    float rows[16];
    m3gGetMatrixRows(MatrixOf(transform), rows);
    bool nearUnity = fabsf(rows[12]) < BottomRowTolerance && fabsf(rows[13]) < BottomRowTolerance &&
        fabsf(rows[14]) < BottomRowTolerance && fabsf(rows[15] - 1.0f) < BottomRowTolerance;
    M3GMatrix matrix;
    if (nearUnity) {
        rows[12] = 0.0f;
        rows[13] = 0.0f;
        rows[14] = 0.0f;
        rows[15] = 1.0f;
    }
    m3gSetMatrixRows(&matrix, rows);
    m3gSetTransform(H<M3GTransformable>(self), &matrix);
    const int error = TakeError();
    static int logged = 0;
    if (error != ErrorNone && logged < 3) {
        logged++;
        fprintf(stderr, "Swerve native: setTransform rejected, bottom row %g %g %g %g\n", rows[12], rows[13], rows[14], rows[15]);
    }
    return error;
}

// ================= Node (slots 44-72) =================

int GetParent(Object* self, void** parent)
{
    WrapInto(reinterpret_cast<M3GObject>(m3gGetParent(H<M3GNode>(self))), parent);
    return TakeError();
}

int SetAlphaFactor(Object* self, float alpha)
{
    m3gSetAlphaFactor(H<M3GNode>(self), alpha);
    return TakeError();
}

int GetRenderingEnable(Object* self, unsigned char* enabled)
{
    *enabled = static_cast<unsigned char>(m3gIsEnabled(H<M3GNode>(self), M3G_SETGET_RENDERING) != 0);
    return TakeError();
}

int SetRenderingEnable(Object* self, bool enable)
{
    m3gEnable(H<M3GNode>(self), M3G_SETGET_RENDERING, enable);
    return TakeError();
}

int GetPickingEnable(Object* self, unsigned char* enabled)
{
    *enabled = static_cast<unsigned char>(m3gIsEnabled(H<M3GNode>(self), M3G_SETGET_PICKING) != 0);
    return TakeError();
}

int SetPickingEnable(Object* self, bool enable)
{
    m3gEnable(H<M3GNode>(self), M3G_SETGET_PICKING, enable);
    return TakeError();
}

int SetScope(Object* self, int scope)
{
    m3gSetScope(H<M3GNode>(self), scope);
    return TakeError();
}

/** Transform from this node to `target` into `transform`; *result says whether a path exists. */
int GetTransformTo(Object* self, Object* target, Object* transform, bool* result)
{
    const M3Gbool found = m3gGetTransformTo(H<M3GNode>(self), H<M3GNode>(target), MatrixOf(transform));
    if (result != nullptr) {
        *result = found != 0;
    }
    return TakeError();
}

int SetAlignment(Object* self, Object* zReference, int zTarget, Object* yReference, int yTarget)
{
    m3gSetAlignment(H<M3GNode>(self), H<M3GNode>(zReference), zTarget, H<M3GNode>(yReference), yTarget);
    return TakeError();
}

int Align(Object* self, Object* reference)
{
    m3gAlignNode(H<M3GNode>(self), H<M3GNode>(reference));
    return TakeError();
}

// ================= Group (slots 73-84), World (85-89) =================

int GetChildCount(Object* self, int* count)
{
    *count = m3gGetChildCount(H<M3GGroup>(self));
    return TakeError();
}

int GetChild(Object* self, int index, void** child)
{
    WrapInto(reinterpret_cast<M3GObject>(m3gGetChild(H<M3GGroup>(self), index)), child);
    return TakeError();
}

int AddChild(Object* self, Object* child)
{
    m3gAddChild(H<M3GGroup>(self), H<M3GNode>(child));
    return TakeError();
}

/** Swerve also removes nodes m3gcore flags as skeleton bones (duplicated heads, see M3gCkzCore.c). */
int RemoveChild(Object* self, Object* child)
{
    ckzM3gRemoveChild(H<M3GGroup>(self), H<M3GNode>(child));
    return TakeError();
}

// ---- picking ----
//
// m3gcore renders the skeleton of a SkinnedMesh but never picks it (m3gSkinnedMeshRayIntersect only
// tests the skinned triangles). Swerve picks it: the zombie heads and the head hit spheres
// (CVisualCharacterType::LoadModel, user id GetHitzoneMeshUserId) hang under the head bone, so with
// m3gcore alone a head shot hits nothing. The pick slots therefore run m3gcore's pick and then pick
// every reachable skeleton themselves, keeping the nearest hit.

constexpr int PickResultFloats = 15;
constexpr int PickRayFloats = 6;

/** Pick ray as two points (start x, y, z, end x, y, z) in the coordinates of the picked group. */
struct PickSegment {
    M3Gfloat points[PickRayFloats];
};

/** Collect the skeleton groups of the SkinnedMeshes under `node` reachable through pickable nodes. */
void CollectPickableSkeletons(M3GNode node, std::vector<M3GGroup>& skeletons)
{
    if (!m3gIsEnabled(node, M3G_SETGET_PICKING)) {
        return;
    }
    const M3GClass nodeClass = m3gGetClass(reinterpret_cast<M3GObject>(node));
    if (nodeClass == M3G_CLASS_SKINNED_MESH) {
        const M3GGroup skeleton = m3gGetSkeleton(reinterpret_cast<M3GSkinnedMesh>(node));
        if (skeleton != nullptr && m3gIsEnabled(reinterpret_cast<M3GNode>(skeleton), M3G_SETGET_PICKING)) {
            skeletons.push_back(skeleton);
            // A skeleton may hold SkinnedMeshes of its own.
            CollectPickableSkeletons(reinterpret_cast<M3GNode>(skeleton), skeletons);
        }
        return;
    }
    if (nodeClass != M3G_CLASS_GROUP && nodeClass != M3G_CLASS_WORLD) {
        return;
    }
    const M3GGroup group = reinterpret_cast<M3GGroup>(node);
    const M3Gint childCount = m3gGetChildCount(group);
    for (M3Gint index = 0; index < childCount; index++) {
        CollectPickableSkeletons(m3gGetChild(group, index), skeletons);
    }
}

/** Transform a point (x, y, z) by `matrix` with the perspective divide. */
void TransformPoint(const M3GMatrix* matrix, const M3Gfloat* point, M3Gfloat* out)
{
    M3GVec4 vector;
    vector.x = point[0];
    vector.y = point[1];
    vector.z = point[2];
    vector.w = 1.0f;
    m3gTransformVec4(matrix, &vector);
    out[0] = vector.x / vector.w;
    out[1] = vector.y / vector.w;
    out[2] = vector.z / vector.w;
}

/**
 * Pick the skeletons under `root` with `segment` (root coordinates) and replace `node` / `result`
 * when a skeleton hit is nearer. Distances compare directly: m3gcore's distance is the fraction of
 * the segment, which an affine change of coordinates keeps.
 */
void PickSkeletons(M3GGroup root, int scope, const PickSegment& segment, M3GNode& node, M3Gfloat* result,
                   M3GCamera camera = nullptr, float x = 0.0f, float y = 0.0f)
{
    std::vector<M3GGroup> skeletons;
    // The root itself must be pickable, as in m3gcore's own pick path test.
    if (!m3gIsEnabled(reinterpret_cast<M3GNode>(root), M3G_SETGET_PICKING)) {
        return;
    }
    const M3Gint childCount = m3gGetChildCount(root);
    for (M3Gint index = 0; index < childCount; index++) {
        CollectPickableSkeletons(m3gGetChild(root, index), skeletons);
    }
    for (M3GGroup skeleton : skeletons) {
        M3GMatrix rootFromSkeleton;
        if (!m3gGetTransformTo(reinterpret_cast<M3GNode>(skeleton), reinterpret_cast<M3GNode>(root), &rootFromSkeleton)) {
            continue;
        }
        M3GMatrix skeletonFromRoot = rootFromSkeleton;
        if (!m3gInvertMatrix(&skeletonFromRoot)) {
            continue;
        }
        M3Gfloat start[3];
        M3Gfloat end[3];
        TransformPoint(&skeletonFromRoot, &segment.points[0], start);
        TransformPoint(&skeletonFromRoot, &segment.points[3], end);
        // m3gPick3D takes origin and direction (it adds them itself).
        M3Gfloat localRay[PickRayFloats] = {
            start[0], start[1], start[2],
            end[0] - start[0], end[1] - start[1], end[2] - start[2],
        };
        M3Gfloat localResult[PickResultFloats] = {};
        M3GNode skeletonHit = nullptr;
        if (camera != nullptr) {
            // A camera pick picks the skeleton through the camera too: m3gcore only picks
            // Sprite3Ds in 2D picks, and Swerve hits sprites hanging on bones (e.g. glows on a
            // zombie, m3g_122462da). Its distance is the same near-to-far fraction as the root's.
            skeletonHit = m3gPick2D(skeleton, scope, x, y, camera, localResult);
        } else {
            skeletonHit = m3gPick3D(skeleton, scope, localRay, localResult);
        }
        if (skeletonHit == nullptr) {
            continue;
        }
        if (node != nullptr && localResult[0] >= result[0]) {
            continue;
        }
        node = skeletonHit;
        // Distance, submesh, texture coordinates and normal belong to the hit; the ray stays the root's.
        for (int index = 0; index < 9; index++) {
            result[index] = localResult[index];
        }
        result[9] = segment.points[0];
        result[10] = segment.points[1];
        result[11] = segment.points[2];
        result[12] = segment.points[3] - segment.points[0];
        result[13] = segment.points[4] - segment.points[1];
        result[14] = segment.points[5] - segment.points[2];
    }
}

/**
 * The pick segment of viewport point (x, y) of `camera` in `group` coordinates, from the near to the
 * far clipping plane, computed as m3gPick2D does. False when the camera is not connected to `group`.
 */
bool CameraPickSegment(M3GGroup group, float x, float y, M3GCamera camera, PickSegment& segment)
{
    M3GMatrix inverseProjection;
    m3gGetProjectionAsMatrix(camera, &inverseProjection);
    if (!m3gInvertMatrix(&inverseProjection)) {
        return false;
    }
    M3GMatrix groupFromCamera;
    if (!m3gGetTransformTo(reinterpret_cast<M3GNode>(camera), reinterpret_cast<M3GNode>(group), &groupFromCamera)) {
        return false;
    }
    const M3Gfloat nearClip[3] = { 2.0f * x - 1.0f, 1.0f - 2.0f * y, -1.0f };
    const M3Gfloat farClip[3] = { 2.0f * x - 1.0f, 1.0f - 2.0f * y, 1.0f };
    M3Gfloat nearCamera[3];
    M3Gfloat farCamera[3];
    TransformPoint(&inverseProjection, nearClip, nearCamera);
    TransformPoint(&inverseProjection, farClip, farCamera);
    TransformPoint(&groupFromCamera, nearCamera, &segment.points[0]);
    TransformPoint(&groupFromCamera, farCamera, &segment.points[3]);
    return true;
}

/** Store a pick in the game's RayIntersection and hit flag. */
void StorePick(Object* ray, unsigned char* hit, M3GNode node, const M3Gfloat* result)
{
    if (ray != nullptr) {
        SetPickResult(ray, node, result);
    }
    if (hit != nullptr) {
        *hit = static_cast<unsigned char>(node != nullptr);
    }
}

/** Pick along a ray from `origin` in `direction`; the hit goes into the RayIntersection `ray`. */
int PickRay(Object* self, int scope, const float* origin, const float* direction, Object* ray, unsigned char* hit)
{
    // The skeleton picks take the ray as two points: origin and origin + direction.
    PickSegment segment;
    for (int axis = 0; axis < 3; axis++) {
        segment.points[axis] = origin[axis];
        segment.points[3 + axis] = origin[axis] + direction[axis];
    }
    // m3gPick3D takes origin and direction and adds them itself (m3g_group.c). Handing it the end
    // point instead doubled the origin into the direction: CUnitBody::Pick(line), the exact body
    // test of projectiles that reach a zombie's bounding box, picked along a wrong ray far from the
    // origin and mostly missed (launcher shells flew through zombies). Verified against the ARM
    // backend by tests/M3gBackendTest.cpp (ray picks).
    M3Gfloat originAndDirection[PickRayFloats];
    for (int axis = 0; axis < 3; axis++) {
        originAndDirection[axis] = origin[axis];
        originAndDirection[3 + axis] = direction[axis];
    }
    M3Gfloat result[PickResultFloats] = {};
    M3GNode node = m3gPick3D(H<M3GGroup>(self), scope, originAndDirection, result);
    int error = TakeError();
    if (error == ErrorNone) {
        PickSkeletons(H<M3GGroup>(self), scope, segment, node, result);
        error = TakeError();
    }
    StorePick(ray, hit, node, result);
    return error;
}

/** Pick through viewport point (x, y) of `camera`. */
int PickCamera(Object* self, int scope, float x, float y, Object* camera, Object* ray, unsigned char* hit)
{
    M3Gfloat result[PickResultFloats] = {};
    M3GNode node = m3gPick2D(H<M3GGroup>(self), scope, x, y, H<M3GCamera>(camera), result);
    int error = TakeError();
    PickSegment segment;
    if (error == ErrorNone && CameraPickSegment(H<M3GGroup>(self), x, y, H<M3GCamera>(camera), segment)) {
        PickSkeletons(H<M3GGroup>(self), scope, segment, node, result, H<M3GCamera>(camera), x, y);
        error = TakeError();
    }
    StorePick(ray, hit, node, result);
    return error;
}

int GetActiveCamera(Object* self, void** camera)
{
    WrapInto(reinterpret_cast<M3GObject>(m3gGetActiveCamera(H<M3GWorld>(self))), camera);
    return TakeError();
}

int SetActiveCamera(Object* self, Object* camera)
{
    m3gSetActiveCamera(H<M3GWorld>(self), H<M3GCamera>(camera));
    return TakeError();
}

int GetBackground(Object* self, void** background)
{
    WrapInto(reinterpret_cast<M3GObject>(m3gGetBackground(H<M3GWorld>(self))), background);
    return TakeError();
}

// ================= Mesh (slots 73-83), SkinnedMesh (84-95) =================

int GetSubmeshCount(Object* self, int* count)
{
    *count = m3gGetSubmeshCount(H<M3GMesh>(self));
    return TakeError();
}

int GetVertexBuffer(Object* self, void** vertexBuffer)
{
    WrapInto(reinterpret_cast<M3GObject>(m3gGetVertexBuffer(H<M3GMesh>(self))), vertexBuffer);
    return TakeError();
}

int GetIndexBuffer(Object* self, int index, void** indexBuffer)
{
    WrapInto(reinterpret_cast<M3GObject>(m3gGetIndexBuffer(H<M3GMesh>(self), index)), indexBuffer);
    return TakeError();
}

int GetAppearance(Object* self, int index, void** appearance)
{
    WrapInto(reinterpret_cast<M3GObject>(m3gGetAppearance(H<M3GMesh>(self), index)), appearance);
    return TakeError();
}

int SetAppearance(Object* self, int index, Object* appearance)
{
    m3gSetAppearance(H<M3GMesh>(self), index, H<M3GAppearance>(appearance));
    return TakeError();
}

int GetSkeleton(Object* self, void** skeleton)
{
    WrapInto(reinterpret_cast<M3GObject>(m3gGetSkeleton(H<M3GSkinnedMesh>(self))), skeleton);
    return TakeError();
}

// ================= Camera (slots 73-77) =================

/** Projection parameters (fovy or height, aspect ratio, near, far); *projectionType gets the M3G type. */
int GetProjectionParams(Object* self, int count, float* params, int* projectionType)
{
    M3Gfloat values[4] = {};
    const M3Gint type = m3gGetProjectionAsParams(H<M3GCamera>(self), values);
    if (params != nullptr) {
        int copied = count;
        if (copied > 4) {
            copied = 4;
        }
        for (int index = 0; index < copied; index++) {
            params[index] = values[index];
        }
    }
    if (projectionType != nullptr) {
        *projectionType = type;
    }
    return TakeError();
}

int GetProjectionMatrix(Object* self, Object* transform, int* projectionType)
{
    const M3Gint type = m3gGetProjectionAsMatrix(H<M3GCamera>(self), MatrixOf(transform));
    if (projectionType != nullptr) {
        *projectionType = type;
    }
    return TakeError();
}

int SetParallel(Object* self, float height, float aspectRatio, float nearClip, float farClip)
{
    m3gSetParallel(H<M3GCamera>(self), height, aspectRatio, nearClip, farClip);
    return TakeError();
}

int SetPerspective(Object* self, float fovy, float aspectRatio, float nearClip, float farClip)
{
    m3gSetPerspective(H<M3GCamera>(self), fovy, aspectRatio, nearClip, farClip);
    return TakeError();
}

// ================= Light (slots 73-78) =================

int SetLightMode(Object* self, int mode)
{
    m3gSetLightMode(H<M3GLight>(self), mode);
    return TakeError();
}

int SetLightColor(Object* self, int color)
{
    m3gSetLightColor(H<M3GLight>(self), static_cast<M3Guint>(color));
    return TakeError();
}

int SetLightIntensity(Object* self, float intensity)
{
    m3gSetIntensity(H<M3GLight>(self), intensity);
    return TakeError();
}

// ================= Appearance (slots 25-42) =================

int GetCompositingMode(Object* self, void** mode)
{
    WrapInto(reinterpret_cast<M3GObject>(m3gGetCompositingMode(H<M3GAppearance>(self))), mode);
    return TakeError();
}

int SetCompositingMode(Object* self, Object* mode)
{
    m3gSetCompositingMode(H<M3GAppearance>(self), H<M3GCompositingMode>(mode));
    return TakeError();
}

int GetMaterial(Object* self, void** material)
{
    WrapInto(reinterpret_cast<M3GObject>(m3gGetMaterial(H<M3GAppearance>(self))), material);
    return TakeError();
}

int GetFog(Object* self, void** fog)
{
    WrapInto(reinterpret_cast<M3GObject>(m3gGetFog(H<M3GAppearance>(self))), fog);
    return TakeError();
}

int SetFog(Object* self, Object* fog)
{
    m3gSetFog(H<M3GAppearance>(self), H<M3GFog>(fog));
    return TakeError();
}

int GetTexture(Object* self, int index, void** texture)
{
    WrapInto(reinterpret_cast<M3GObject>(m3gGetTexture(H<M3GAppearance>(self), index)), texture);
    return TakeError();
}

int SetTexture(Object* self, int index, Object* texture)
{
    m3gSetTexture(H<M3GAppearance>(self), index, H<M3GTexture>(texture));
    return TakeError();
}

// ================= Texture2D (slot 54) =================

int SetTextureImage(Object* self, Object* image)
{
    m3gSetTextureImage(H<M3GTexture>(self), H<M3GImage>(image));
    return TakeError();
}

// ================= Background (slots 33-42) =================

int GetCropX(Object* self, int* value)
{
    *value = m3gGetBgCrop(H<M3GBackground>(self), M3G_GET_CROPX);
    return TakeError();
}

int GetCropY(Object* self, int* value)
{
    *value = m3gGetBgCrop(H<M3GBackground>(self), M3G_GET_CROPY);
    return TakeError();
}

int GetCropWidth(Object* self, int* value)
{
    *value = m3gGetBgCrop(H<M3GBackground>(self), M3G_GET_CROPWIDTH);
    return TakeError();
}

int GetCropHeight(Object* self, int* value)
{
    *value = m3gGetBgCrop(H<M3GBackground>(self), M3G_GET_CROPHEIGHT);
    return TakeError();
}

int SetColorClearEnable(Object* self, bool enable)
{
    m3gSetBgEnable(H<M3GBackground>(self), M3G_SETGET_COLORCLEAR, enable);
    return TakeError();
}

int SetDepthClearEnable(Object* self, bool enable)
{
    m3gSetBgEnable(H<M3GBackground>(self), M3G_SETGET_DEPTHCLEAR, enable);
    return TakeError();
}

// ================= CompositingMode (slots 42, 44), Material (31) =================

int SetDepthTestEnable(Object* self, bool enable)
{
    m3gEnableDepthTest(H<M3GCompositingMode>(self), enable);
    return TakeError();
}

int SetDepthWriteEnable(Object* self, bool enable)
{
    m3gEnableDepthWrite(H<M3GCompositingMode>(self), enable);
    return TakeError();
}

/** Color of `target` (Material.AMBIENT 0x400, DIFFUSE 0x800, ...) as 0xAARRGGBB. */
int GetMaterialColor(Object* self, int target, int* color)
{
    *color = static_cast<int>(m3gGetColor(H<M3GMaterial>(self), target));
    return TakeError();
}

// ================= VertexBuffer (37, 41), VertexArray (28, 31), IndexBuffer (32, 33) =================

int GetBufferVertexCount(Object* self, int* count)
{
    *count = m3gGetVertexCount(H<M3GVertexBuffer>(self));
    return TakeError();
}

/** Position array; scaleBias gets the scale and the three bias values. */
int GetPositions(Object* self, int scaleBiasCount, float* scaleBias, void** positions)
{
    const M3GVertexArray array = m3gGetVertexArray(H<M3GVertexBuffer>(self), M3G_GET_POSITIONS, scaleBias, scaleBiasCount);
    WrapInto(reinterpret_cast<M3GObject>(array), positions);
    return TakeError();
}

int GetArrayVertexCount(Object* self, int* count)
{
    M3Gsizei vertexCount = 0;
    M3Gint size = 0;
    M3Gdatatype type = M3G_SHORT;
    M3Gsizei stride = 0;
    m3gGetVertexArrayParams(H<M3GVertexArray>(self), &vertexCount, &size, &type, &stride);
    *count = vertexCount;
    return TakeError();
}

/** Components of `vertexCount` vertices from `firstVertex` as 16 bit values (valueCount of them). */
int GetVertexValues(Object* self, int firstVertex, int vertexCount, int valueCount, short* values)
{
    m3gGetVertexArrayElements(H<M3GVertexArray>(self), firstVertex, vertexCount, valueCount, M3G_SHORT, values);
    return TakeError();
}

/** Triangle list index count of the strips (3 per triangle). */
int GetIndexCount(Object* self, int* count)
{
    *count = m3gGetBatchSize(H<M3GIndexBuffer>(self), 0);
    return TakeError();
}

int GetIndices(Object* self, int count, unsigned int* indices)
{
    const M3Gint size = m3gGetBatchSize(H<M3GIndexBuffer>(self), 0);
    if (count < size) {
        return ErrorInvalidParameter;
    }
    // Swerve's corner order of the odd strip triangles, see M3gCkzCore.c.
    ckzM3gGetSwerveTriangleIndices(H<M3GIndexBuffer>(self), reinterpret_cast<M3Gint*>(indices));
    return TakeError();
}

// ================= AnimationController, AnimationTrack, KeyframeSequence =================

int GetWeight(Object* self, float* weight)
{
    *weight = m3gGetWeight(H<M3GAnimationController>(self));
    return TakeError();
}

int SetWeight(Object* self, float weight)
{
    m3gSetWeight(H<M3GAnimationController>(self), weight);
    return TakeError();
}

int GetSpeed(Object* self, float* speed)
{
    *speed = m3gGetSpeed(H<M3GAnimationController>(self));
    return TakeError();
}

int GetPosition(Object* self, int worldTime, float* sequenceTime)
{
    *sequenceTime = m3gGetPosition(H<M3GAnimationController>(self), worldTime);
    return TakeError();
}

int SetPosition(Object* self, float sequenceTime, int worldTime)
{
    m3gSetPosition(H<M3GAnimationController>(self), sequenceTime, worldTime);
    return TakeError();
}

int SetSpeed(Object* self, float speed, int worldTime)
{
    m3gSetSpeed(H<M3GAnimationController>(self), speed, worldTime);
    return TakeError();
}

int GetController(Object* self, void** controller)
{
    WrapInto(reinterpret_cast<M3GObject>(m3gGetController(H<M3GAnimationTrack>(self))), controller);
    return TakeError();
}

int SetController(Object* self, Object* controller)
{
    m3gSetController(H<M3GAnimationTrack>(self), H<M3GAnimationController>(controller));
    return TakeError();
}

int GetKeyframeSequence(Object* self, void** sequence)
{
    WrapInto(reinterpret_cast<M3GObject>(m3gGetSequence(H<M3GAnimationTrack>(self))), sequence);
    return TakeError();
}

int GetDuration(Object* self, int* duration)
{
    *duration = m3gGetDuration(H<M3GKeyframeSequence>(self));
    return TakeError();
}

int GetRepeatMode(Object* self, int* repeatMode)
{
    *repeatMode = m3gGetRepeatMode(H<M3GKeyframeSequence>(self));
    return TakeError();
}

} // namespace

void FillObject3DSlots(ObjectClass& objectClass)
{
    FillQueryInterfaceSlots(objectClass);
    SetSlot(objectClass, 3, &GetUserID);                 // 0x0c
    SetSlot(objectClass, 4, &SetUserID);                 // 0x10
    SetSlot(objectClass, 5, &GetAnimationTrackCount);    // 0x14
    SetSlot(objectClass, 8, &GetAnimationTrack);         // 0x20
    SetSlot(objectClass, 10, &AddAnimationTrack);        // 0x28
    SetSlot(objectClass, 12, &RemoveAnimationTrack);     // 0x30
    SetSlot(objectClass, 13, &Animate);                  // 0x34
    SetSlot(objectClass, 15, &Find);                     // 0x3c
    SetSlot(objectClass, 18, &GetReferences);            // 0x48
    SetSlot(objectClass, 19, &GetUserParameter);         // 0x4c
    SetSlot(objectClass, 24, &Duplicate);                // 0x60
}

void FillTransformableSlots(ObjectClass& objectClass)
{
    FillObject3DSlots(objectClass);
    SetSlot(objectClass, 27, &GetTranslation);           // 0x6c
    SetSlot(objectClass, 28, &SetTranslation);           // 0x70
    SetSlot(objectClass, 29, &GetScale);                 // 0x74
    SetSlot(objectClass, 30, &SetScale);                 // 0x78
    SetSlot(objectClass, 31, &GetOrientation);           // 0x7c
    SetSlot(objectClass, 32, &SetOrientation);           // 0x80
    SetSlot(objectClass, 33, &GetOrientationQuat);       // 0x84
    SetSlot(objectClass, 35, &Translate);                // 0x8c
    SetSlot(objectClass, 38, &PostRotate);               // 0x98
    SetSlot(objectClass, 41, &GetGenericTransform);      // 0xa4
    SetSlot(objectClass, 43, &SetTransform);             // 0xac
}

void FillNodeSlots(ObjectClass& objectClass)
{
    FillTransformableSlots(objectClass);
    SetSlot(objectClass, 45, &GetParent);                // 0xb4
    SetSlot(objectClass, 47, &SetAlphaFactor);           // 0xbc
    SetSlot(objectClass, 48, &GetRenderingEnable);       // 0xc0
    SetSlot(objectClass, 49, &SetRenderingEnable);       // 0xc4
    SetSlot(objectClass, 50, &GetPickingEnable);         // 0xc8
    SetSlot(objectClass, 51, &SetPickingEnable);         // 0xcc
    SetSlot(objectClass, 53, &SetScope);                 // 0xd4
    SetSlot(objectClass, 58, &GetTransformTo);           // 0xe8
    SetSlot(objectClass, 59, &SetAlignment);             // 0xec
    SetSlot(objectClass, 62, &Align);                    // 0xf8
}

void FillGroupSlots(ObjectClass& objectClass)
{
    FillNodeSlots(objectClass);
    SetSlot(objectClass, 73, &GetChildCount);            // 0x124
    SetSlot(objectClass, 80, &GetChild);                 // 0x140
    SetSlot(objectClass, 81, &AddChild);                 // 0x144
    SetSlot(objectClass, 82, &RemoveChild);              // 0x148
    SetSlot(objectClass, 83, &PickRay);                  // 0x14c
    SetSlot(objectClass, 84, &PickCamera);               // 0x150
}

void FillWorldSlots(ObjectClass& objectClass)
{
    FillGroupSlots(objectClass);
    SetSlot(objectClass, 87, &GetActiveCamera);          // 0x15c
    SetSlot(objectClass, 88, &SetActiveCamera);          // 0x160
    SetSlot(objectClass, 89, &GetBackground);            // 0x164
}

void FillMeshSlots(ObjectClass& objectClass)
{
    FillNodeSlots(objectClass);
    SetSlot(objectClass, 76, &GetSubmeshCount);          // 0x130
    SetSlot(objectClass, 77, &GetVertexBuffer);          // 0x134
    SetSlot(objectClass, 80, &GetIndexBuffer);           // 0x140 (CCollision: triangle indices)
    SetSlot(objectClass, 81, &GetAppearance);            // 0x144
    SetSlot(objectClass, 82, &GetIndexBuffer);           // 0x148
    SetSlot(objectClass, 83, &SetAppearance);            // 0x14c
}

void FillSkinnedMeshSlots(ObjectClass& objectClass)
{
    FillMeshSlots(objectClass);
    SetSlot(objectClass, 95, &GetSkeleton);              // 0x17c
}

void FillCameraSlots(ObjectClass& objectClass)
{
    FillNodeSlots(objectClass);
    SetSlot(objectClass, 73, &GetProjectionParams);      // 0x124
    SetSlot(objectClass, 74, &GetProjectionMatrix);      // 0x128
    SetSlot(objectClass, 76, &SetParallel);              // 0x130
    SetSlot(objectClass, 77, &SetPerspective);           // 0x134
}

void FillLightSlots(ObjectClass& objectClass)
{
    FillNodeSlots(objectClass);
    SetSlot(objectClass, 74, &SetLightMode);             // 0x128
    SetSlot(objectClass, 76, &SetLightColor);            // 0x130
    SetSlot(objectClass, 78, &SetLightIntensity);        // 0x138
}

void FillSpriteSlots(ObjectClass& objectClass)
{
    FillNodeSlots(objectClass);
}

void FillAppearanceSlots(ObjectClass& objectClass)
{
    FillObject3DSlots(objectClass);
    SetSlot(objectClass, 29, &GetCompositingMode);       // 0x74
    SetSlot(objectClass, 30, &SetCompositingMode);       // 0x78
    SetSlot(objectClass, 35, &GetMaterial);              // 0x8c
    SetSlot(objectClass, 37, &GetFog);                   // 0x94
    SetSlot(objectClass, 38, &SetFog);                   // 0x98
    SetSlot(objectClass, 41, &GetTexture);               // 0xa4
    SetSlot(objectClass, 42, &SetTexture);               // 0xa8
}

void FillTextureSlots(ObjectClass& objectClass)
{
    FillTransformableSlots(objectClass);
    SetSlot(objectClass, 54, &SetTextureImage);          // 0xd8
}

void FillBackgroundSlots(ObjectClass& objectClass)
{
    FillObject3DSlots(objectClass);
    SetSlot(objectClass, 33, &GetCropX);                 // 0x84
    SetSlot(objectClass, 34, &GetCropY);                 // 0x88
    SetSlot(objectClass, 35, &GetCropWidth);             // 0x8c
    SetSlot(objectClass, 36, &GetCropHeight);            // 0x90
    SetSlot(objectClass, 38, &SetColorClearEnable);      // 0x98
    SetSlot(objectClass, 42, &SetDepthClearEnable);      // 0xa8
}

void FillCompositingModeSlots(ObjectClass& objectClass)
{
    FillObject3DSlots(objectClass);
    SetSlot(objectClass, 42, &SetDepthTestEnable);       // 0xa8
    SetSlot(objectClass, 44, &SetDepthWriteEnable);      // 0xb0
}

void FillMaterialSlots(ObjectClass& objectClass)
{
    FillObject3DSlots(objectClass);
    SetSlot(objectClass, 31, &GetMaterialColor);         // 0x7c
}

void FillVertexBufferSlots(ObjectClass& objectClass)
{
    FillObject3DSlots(objectClass);
    SetSlot(objectClass, 37, &GetBufferVertexCount);     // 0x94
    SetSlot(objectClass, 41, &GetPositions);             // 0xa4
}

void FillVertexArraySlots(ObjectClass& objectClass)
{
    FillObject3DSlots(objectClass);
    SetSlot(objectClass, 28, &GetArrayVertexCount);      // 0x70
    SetSlot(objectClass, 31, &GetVertexValues);          // 0x7c
}

void FillIndexBufferSlots(ObjectClass& objectClass)
{
    FillObject3DSlots(objectClass);
    SetSlot(objectClass, 32, &GetIndexCount);            // 0x80
    SetSlot(objectClass, 33, &GetIndices);               // 0x84
}

void FillAnimationControllerSlots(ObjectClass& objectClass)
{
    FillObject3DSlots(objectClass);
    SetSlot(objectClass, 27, &GetWeight);                // 0x6c
    SetSlot(objectClass, 28, &SetWeight);                // 0x70
    SetSlot(objectClass, 31, &GetSpeed);                 // 0x7c
    SetSlot(objectClass, 35, &GetPosition);              // 0x8c
    SetSlot(objectClass, 36, &SetPosition);              // 0x90
    SetSlot(objectClass, 38, &SetSpeed);                 // 0x98
}

void FillAnimationTrackSlots(ObjectClass& objectClass)
{
    FillObject3DSlots(objectClass);
    SetSlot(objectClass, 29, &GetController);            // 0x74
    SetSlot(objectClass, 30, &SetController);            // 0x78
    SetSlot(objectClass, 31, &GetKeyframeSequence);      // 0x7c
}

void FillKeyframeSequenceSlots(ObjectClass& objectClass)
{
    FillObject3DSlots(objectClass);
    SetSlot(objectClass, 29, &GetDuration);              // 0x74
    SetSlot(objectClass, 31, &GetRepeatMode);            // 0x7c
}

} // namespace ckz::native
