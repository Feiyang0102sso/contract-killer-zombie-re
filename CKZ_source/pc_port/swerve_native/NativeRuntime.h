// Native Swerve runtime: the object ABI of the Swerve M3G engine, implemented on m3gcore.
// See PORT_PLAN.md, P2 (M1 backend switch, M3 native backend).
//
// The game talks to Swerve only through tws.h / SwerveApi.h: every object starts with a pointer to a
// table of functions, and the game calls slot N of that table. ckz::arm::Method calls a table entry
// directly when it is not ARM code, so a native object just needs a table of host functions at the
// same slots. Each function takes the object as its first argument, like the ARM methods.
#pragma once

#include <m3g_core.h>
#include <stdint.h>
#include <vector>

struct ISwerve;

namespace ckz::native {

/**
 * Swerve object kinds: the CreateObject types and QueryInterface ids. They are the JSR-184 classes
 * in alphabetical order with M3G 2.0's abstract Texture class inserted (Graphics3D = 8, Group = 9,
 * Loader = 14, Transform = 26, World = 31 are confirmed by the game's calls).
 */
enum ClassId : int {
    ClassAnimationController = 1,
    ClassAnimationTrack = 2,
    ClassAppearance = 3,
    ClassBackground = 4,
    ClassCamera = 5,
    ClassCompositingMode = 6,
    ClassFog = 7,
    ClassGraphics3D = 8,
    ClassGroup = 9,
    ClassImage2D = 10,
    ClassIndexBuffer = 11,
    ClassKeyframeSequence = 12,
    ClassLight = 13,
    ClassLoader = 14,
    ClassMaterial = 15,
    ClassMesh = 16,
    ClassMorphingMesh = 17,
    ClassNode = 18,
    ClassObject3D = 19,
    ClassPolygonMode = 20,
    ClassRayIntersection = 21,
    ClassSkinnedMesh = 22,
    ClassSprite3D = 23,
    ClassTexture = 24,
    ClassTexture2D = 25,
    ClassTransform = 26,
    ClassTransformable = 27,
    ClassTriangleStripArray = 28,
    ClassVertexArray = 29,
    ClassVertexBuffer = 30,
    ClassWorld = 31,
    ClassSwerve = 0x100, // the runtime root (swvNew); not a CreateObject type
};

/** Swerve error codes as CSwerve::LogError knows them (0 is success). */
enum Error : int {
    ErrorNone = 0,
    ErrorFailed = 1,
    ErrorNoMemory = 2,
    ErrorClassNotSupported = 3,
    ErrorGeneral = 0x10000,
    ErrorCorruptFile = 0x1000b,
    ErrorInvalidState = 0x1000d,
    ErrorInvalidParameter = 0x1000e,
    ErrorNullPointer = 0x1000f,
};

// Highest table slot any Swerve interface uses is 95 (SkinnedMesh::getSkeleton).
constexpr int SlotCount = 128;

struct Object;

/** One Swerve class: its virtual slot table and the interface ids QueryInterface accepts. */
struct ObjectClass {
    const char* name;
    ClassId id;
    std::vector<ClassId> interfaces;
    /** Allocates an object of this class for Wrap (derived structures may follow the Object part). */
    Object* (*allocate)();
    /** Frees an object of this class when its last reference goes (objects cannot have a vtable). */
    void (*destroy)(Object* object);
    void* slots[SlotCount];
};

/**
 * Base of every native runtime object. `functions` must stay the first member: the game reads the
 * slot table through it. Objects backed by m3gcore keep one m3gcore reference in `handle`.
 */
struct Object {
    void* const* functions;
    const ObjectClass* objectClass;
    int references;
    M3GObject handle;
};

/** Set one virtual slot. */
template<class Function>
void SetSlot(ObjectClass& objectClass, int slot, Function function)
{
    objectClass.slots[slot] = reinterpret_cast<void*>(function);
}

// ---- NativeRuntime.cpp ----

/** The m3gcore interface shared by every native object (created by CreateSwerve). */
M3GInterface Interface();

/** Swerve error code of the last m3gcore error, which is then cleared. */
int TakeError();

/** Native object of an m3gcore handle with a new reference for the caller; NULL for a NULL handle. */
Object* Wrap(M3GObject handle);

/** Store a new reference of the wrapper of `handle` in `*out` (NULL when `handle` is NULL). */
void WrapInto(M3GObject handle, void** out);

/** m3gcore handle of a native object handed back by the game (NULL for NULL). */
M3GObject HandleOf(const void* object);

void AddReference(Object* object);
void ReleaseReference(Object* object);

/**
 * Initialize a runtime-only object (no m3gcore handle) of class `id` with one reference.
 * `object` points to a derived structure whose first part is Object.
 */
void InitObject(Object* object, ClassId id);

/** Attach an m3gcore handle to an object created without one (Image2D before Initialize). */
void AttachHandle(Object* object, M3GObject handle);

/** The class table for Swerve class `id`. */
const ObjectClass& ClassOf(ClassId id);

/** Create the native runtime root object (swvNew of the native backend). */
int CreateSwerve(ISwerve** swerve);

// ---- slot fillers, one per interface layer (each also fills the layers below it) ----

void FillQueryInterfaceSlots(ObjectClass& objectClass);  // NativeRuntime.cpp
void FillObject3DSlots(ObjectClass& objectClass);         // NativeScene.cpp
void FillTransformableSlots(ObjectClass& objectClass);
void FillNodeSlots(ObjectClass& objectClass);
void FillGroupSlots(ObjectClass& objectClass);
void FillWorldSlots(ObjectClass& objectClass);
void FillMeshSlots(ObjectClass& objectClass);
void FillSkinnedMeshSlots(ObjectClass& objectClass);
void FillCameraSlots(ObjectClass& objectClass);
void FillLightSlots(ObjectClass& objectClass);
void FillSpriteSlots(ObjectClass& objectClass);
void FillAppearanceSlots(ObjectClass& objectClass);
void FillTextureSlots(ObjectClass& objectClass);
void FillBackgroundSlots(ObjectClass& objectClass);
void FillCompositingModeSlots(ObjectClass& objectClass);
void FillMaterialSlots(ObjectClass& objectClass);
void FillVertexBufferSlots(ObjectClass& objectClass);
void FillVertexArraySlots(ObjectClass& objectClass);
void FillIndexBufferSlots(ObjectClass& objectClass);
void FillAnimationControllerSlots(ObjectClass& objectClass);
void FillAnimationTrackSlots(ObjectClass& objectClass);
void FillKeyframeSequenceSlots(ObjectClass& objectClass);
void FillImage2DSlots(ObjectClass& objectClass);          // NativeImage.cpp
void FillTransformSlots(ObjectClass& objectClass);        // NativeTransform.cpp
void FillRayIntersectionSlots(ObjectClass& objectClass);
void FillLoaderSlots(ObjectClass& objectClass);           // NativeLoader.cpp
void FillGraphics3DSlots(ObjectClass& objectClass);       // NativeGraphics3D.cpp

// ---- runtime-only object types ----

/** Transform: a 4x4 matrix (not an m3gcore object). */
struct TransformObject : Object {
    M3GMatrix matrix;
};

/** Matrix of a Transform handed in by the game (NULL stays NULL). */
M3GMatrix* MatrixOf(const void* transform);

/** Create a Transform object (CreateObject type 26). */
Object* CreateTransform();
Object* CreateRayIntersection();
Object* CreateLoader();
Object* CreateGraphics3D();
/** An Image2D without pixels yet; IImage2D::Initialize creates the m3gcore image. */
Object* CreateImage2D();

/** Fill a RayIntersection object from an m3gcore pick result (15 floats) and the picked node. */
void SetPickResult(void* rayIntersection, M3GNode node, const M3Gfloat* result);

// ---- SwerveExtendedM3g.cpp ----

/** True for Swerve's extended M3G files ("\xbbSWERVE\xab" identifier). */
bool IsSwerveExtendedM3g(const std::vector<uint8_t>& data);
/** Rewrite an extended file as a standard M3G 1.0 file m3gcore can load. */
bool ConvertSwerveExtendedM3g(const std::vector<uint8_t>& data, std::vector<uint8_t>& out);

/** User parameters harvested from a loader, looked up by Object3D::getUserParameter. */
void StoreUserParameter(M3GObject object, int id, const M3Gbyte* data, int length);
/** Copy user parameter `id` of `object`; returns its length or -1 when it does not exist. */
int FindUserParameter(M3GObject object, int id, int bufferSize, char* buffer);

} // namespace ckz::native
