// Native Swerve Transform (a 4x4 matrix) and RayIntersection (a pick result). Neither is an
// m3gcore object: m3gcore takes matrices as M3GMatrix values and returns pick results as floats.
#include "NativeRuntime.h"
#include <stdio.h>
#include <string.h>

namespace ckz::native {
namespace {

constexpr int MatrixFloats = 16;
constexpr int PickResultFloats = 15;

TransformObject* AsTransform(const void* object)
{
    return static_cast<TransformObject*>(const_cast<void*>(object));
}

void DestroyTransform(Object* object)
{
    delete static_cast<TransformObject*>(object);
}

// ================= Transform (slots 3-15) =================

/** The 16 floats, row major. */
int TransformGet(Object* self, int count, float* matrix)
{
    if (count < MatrixFloats) {
        return ErrorInvalidParameter;
    }
    m3gGetMatrixRows(&AsTransform(self)->matrix, matrix);
    return ErrorNone;
}

int TransformSet(Object* self, int count, const float* matrix)
{
    if (count < MatrixFloats) {
        return ErrorInvalidParameter;
    }
    m3gSetMatrixRows(&AsTransform(self)->matrix, matrix);
    return ErrorNone;
}

/**
 * Slot 6 is M3G's Transform.set(Transform): this = other (a copy, not a product). Probed on the
 * ARM backend with two non-identity matrices: the result is exactly `other`. tws.h names it
 * `multiply`; Fragments::Paint3D, SmokeTrail::Paint3D and the grenade flares use it to copy
 * scratch (= R * basis) back into basis. A product squared the particle scale (~300x) and drew
 * screen-filling white triangles at every bullet impact (bug A).
 */
int TransformCopy(Object* self, Object* other)
{
    AsTransform(self)->matrix = *MatrixOf(other);
    return ErrorNone;
}

int TransformSetIdentity(Object* self)
{
    m3gIdentityMatrix(&AsTransform(self)->matrix);
    return ErrorNone;
}

int TransformInvert(Object* self)
{
    if (!m3gInvertMatrix(&AsTransform(self)->matrix)) {
        return ErrorGeneral; // singular matrix (M3G ArithmeticException)
    }
    return ErrorNone;
}

/** this = this * other. */
int TransformPostMultiply(Object* self, Object* other)
{
    m3gPostMultiplyMatrix(&AsTransform(self)->matrix, MatrixOf(other));
    return ErrorNone;
}

/** Rotation given as angle (degrees) and axis x, y, z. */
int TransformPostRotate(Object* self, const float* angleAxis)
{
    m3gPostRotateMatrix(&AsTransform(self)->matrix, angleAxis[0], angleAxis[1], angleAxis[2], angleAxis[3]);
    return ErrorNone;
}

/** Rotation given as quaternion x, y, z, w. */
int TransformPostRotateQuat(Object* self, const float* quaternion)
{
    M3GQuat quat;
    m3gSetQuat(&quat, quaternion);
    m3gPostRotateMatrixQuat(&AsTransform(self)->matrix, &quat);
    return ErrorNone;
}

int TransformPostScale(Object* self, const float* scale)
{
    m3gPostScaleMatrix(&AsTransform(self)->matrix, scale[0], scale[1], scale[2]);
    return ErrorNone;
}

int TransformPostTranslate(Object* self, const float* translation)
{
    m3gPostTranslateMatrix(&AsTransform(self)->matrix, translation[0], translation[1], translation[2]);
    return ErrorNone;
}

/** Transform the positions of `vertices` into `out` (4 floats per vertex). */
int TransformVertexArray(Object* self, Object* vertices, int outCount, float* out, bool w)
{
    m3gTransformArray(reinterpret_cast<M3GVertexArray>(vertices->handle), &AsTransform(self)->matrix, out, outCount, w);
    return TakeError();
}

/** Transform `count` floats in place, 4 per vector (x, y, z, w). */
int TransformVectors(Object* self, int count, float* vectors)
{
    for (int offset = 0; offset + 4 <= count; offset += 4) {
        M3GVec4 vector;
        vector.x = vectors[offset];
        vector.y = vectors[offset + 1];
        vector.z = vectors[offset + 2];
        vector.w = vectors[offset + 3];
        m3gTransformVec4(&AsTransform(self)->matrix, &vector);
        vectors[offset] = vector.x;
        vectors[offset + 1] = vector.y;
        vectors[offset + 2] = vector.z;
        vectors[offset + 3] = vector.w;
    }
    return ErrorNone;
}

// ================= RayIntersection (slots 3-11) =================

/** Result of the last pick: the node (one m3gcore reference) and m3gcore's 15 result floats. */
struct RayObject : Object {
    M3GNode node;
    M3Gfloat result[PickResultFloats];
};

RayObject* AsRay(const void* object)
{
    return static_cast<RayObject*>(const_cast<void*>(object));
}

void DestroyRay(Object* object)
{
    RayObject* ray = AsRay(object);
    if (ray->node != nullptr) {
        m3gDeleteRef(reinterpret_cast<M3GObject>(ray->node));
    }
    delete ray;
}

int RayGetDistance(Object* self, float* distance)
{
    *distance = AsRay(self)->result[0];
    return ErrorNone;
}

int RayGetIntersected(Object* self, void** node)
{
    WrapInto(reinterpret_cast<M3GObject>(AsRay(self)->node), node);
    return ErrorNone;
}

int RayGetNormalX(Object* self, float* normal)
{
    *normal = AsRay(self)->result[6];
    return ErrorNone;
}

int RayGetNormalY(Object* self, float* normal)
{
    *normal = AsRay(self)->result[7];
    return ErrorNone;
}

int RayGetNormalZ(Object* self, float* normal)
{
    *normal = AsRay(self)->result[8];
    return ErrorNone;
}

/** Ray origin x, y, z and direction x, y, z. */
int RayGetRay(Object* self, int count, float* originAndDirection)
{
    int copied = count;
    if (copied > 6) {
        copied = 6;
    }
    for (int index = 0; index < copied; index++) {
        originAndDirection[index] = AsRay(self)->result[9 + index];
    }
    return ErrorNone;
}

} // namespace

M3GMatrix* MatrixOf(const void* transform)
{
    if (transform == nullptr) {
        return nullptr;
    }
    return &AsTransform(transform)->matrix;
}

Object* CreateTransform()
{
    TransformObject* transform = new TransformObject();
    InitObject(transform, ClassTransform);
    m3gIdentityMatrix(&transform->matrix);
    return transform;
}

Object* CreateRayIntersection()
{
    RayObject* ray = new RayObject();
    InitObject(ray, ClassRayIntersection);
    ray->node = nullptr;
    memset(ray->result, 0, sizeof(ray->result));
    return ray;
}

void SetPickResult(void* rayIntersection, M3GNode node, const M3Gfloat* result)
{
    RayObject* ray = AsRay(rayIntersection);
    if (node != nullptr) {
        m3gAddRef(reinterpret_cast<M3GObject>(node));
    }
    if (ray->node != nullptr) {
        m3gDeleteRef(reinterpret_cast<M3GObject>(ray->node));
    }
    ray->node = node;
    memcpy(ray->result, result, sizeof(ray->result));
}

void FillTransformSlots(ObjectClass& objectClass)
{
    FillQueryInterfaceSlots(objectClass);
    objectClass.destroy = DestroyTransform;
    SetSlot(objectClass, 4, &TransformGet);               // 0x10
    SetSlot(objectClass, 5, &TransformSet);               // 0x14
    SetSlot(objectClass, 6, &TransformCopy);              // 0x18
    SetSlot(objectClass, 7, &TransformSetIdentity);       // 0x1c
    SetSlot(objectClass, 8, &TransformInvert);            // 0x20
    SetSlot(objectClass, 9, &TransformPostMultiply);      // 0x24
    SetSlot(objectClass, 10, &TransformPostRotate);       // 0x28
    SetSlot(objectClass, 11, &TransformPostRotateQuat);   // 0x2c
    SetSlot(objectClass, 12, &TransformPostScale);        // 0x30
    SetSlot(objectClass, 13, &TransformPostTranslate);    // 0x34
    SetSlot(objectClass, 14, &TransformVertexArray);      // 0x38
    SetSlot(objectClass, 15, &TransformVectors);          // 0x3c
}

void FillRayIntersectionSlots(ObjectClass& objectClass)
{
    FillQueryInterfaceSlots(objectClass);
    objectClass.destroy = DestroyRay;
    SetSlot(objectClass, 3, &RayGetDistance);             // 0x0c
    SetSlot(objectClass, 4, &RayGetIntersected);          // 0x10
    SetSlot(objectClass, 6, &RayGetNormalX);              // 0x18
    SetSlot(objectClass, 7, &RayGetNormalY);              // 0x1c
    SetSlot(objectClass, 8, &RayGetNormalZ);              // 0x20
    SetSlot(objectClass, 11, &RayGetRay);                 // 0x2c
}

} // namespace ckz::native
