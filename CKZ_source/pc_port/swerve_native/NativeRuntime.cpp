// Native Swerve runtime core: m3gcore interface, error codes, class tables, the handle -> wrapper
// registry, reference counting, QueryInterface and the runtime root object (ISwerve).
#include "NativeRuntime.h"
#include <SwerveApi.h>
#include <SDL3/SDL.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>

namespace ckz::native {
namespace {

// Errors after this many are counted but not printed.
constexpr int ErrorLogLimit = 200;
// Set to print the native call stack of every m3gcore error (debugging).
constexpr const char* TraceVariable = "CKZ_NATIVE_TRACE";
constexpr int TraceFrames = 10;

M3GInterface m3g;
M3Genum lastError = M3G_NO_ERROR;
int loggedErrors = 0;

// Wrapper of every m3gcore object the game currently holds, so a node is always the same object.
std::unordered_map<M3GObject, Object*> wrappers;

std::map<int, ObjectClass*> classes;

/** One user parameter (id, bytes) of a loaded object. */
struct UserParameter {
    int id;
    std::vector<M3Gbyte> data;
};
std::unordered_map<M3GObject, std::vector<UserParameter>> userParameters;

void* AllocateMemory(M3Gpointer bytes)
{
    return malloc(static_cast<size_t>(bytes));
}

void FreeMemory(void* pointer)
{
    free(pointer);
}

const char* M3gErrorName(M3Genum error)
{
    switch (error) {
    case M3G_INVALID_VALUE:
        return "INVALID_VALUE";
    case M3G_INVALID_ENUM:
        return "INVALID_ENUM";
    case M3G_INVALID_OPERATION:
        return "INVALID_OPERATION";
    case M3G_INVALID_OBJECT:
        return "INVALID_OBJECT";
    case M3G_INVALID_INDEX:
        return "INVALID_INDEX";
    case M3G_OUT_OF_MEMORY:
        return "OUT_OF_MEMORY";
    case M3G_NULL_POINTER:
        return "NULL_POINTER";
    case M3G_ARITHMETIC_ERROR:
        return "ARITHMETIC_ERROR";
    case M3G_IO_ERROR:
        return "IO_ERROR";
    }
    return "?";
}

void RememberError(M3Genum errorCode, M3GInterface)
{
    lastError = errorCode;
    if (loggedErrors < ErrorLogLimit) {
        loggedErrors++;
        fprintf(stderr, "Swerve native: m3gcore error %s\n", M3gErrorName(errorCode));
        if (SDL_getenv(TraceVariable) != nullptr) {
            // Return addresses for offline symbolization (module base printed once).
            void* frames[TraceFrames] = {};
            const USHORT count = CaptureStackBackTrace(1, TraceFrames, frames, nullptr);
            fprintf(stderr, "  stack (base %08x):", static_cast<unsigned int>(reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr))));
            for (USHORT index = 0; index < count; index++) {
                fprintf(stderr, " %08x", static_cast<unsigned int>(reinterpret_cast<uintptr_t>(frames[index])));
            }
            fprintf(stderr, "\n");
        }
    }
}

Object* AllocatePlainObject()
{
    return new Object();
}

void DestroyPlainObject(Object* object)
{
    delete object;
}

// ---- unimplemented slots: every slot starts as a stub that names itself once ----

std::set<std::pair<const ObjectClass*, int>> reportedSlots;

void ReportUnimplemented(const Object* self, int slot)
{
    const auto key = std::make_pair(self->objectClass, slot);
    if (reportedSlots.insert(key).second) {
        fprintf(stderr, "Swerve native: %s slot %d (0x%x) is not implemented\n", self->objectClass->name, slot, slot * 4);
        // The caller, for offline symbolization (module base first).
        void* frames[TraceFrames] = {};
        const USHORT count = CaptureStackBackTrace(1, TraceFrames, frames, nullptr);
        fprintf(stderr, "  stack (base %08x):", static_cast<unsigned int>(reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr))));
        for (USHORT index = 0; index < count; index++) {
            fprintf(stderr, " %08x", static_cast<unsigned int>(reinterpret_cast<uintptr_t>(frames[index])));
        }
        fprintf(stderr, "\n");
    }
}

template<int Slot>
int UnimplementedSlot(Object* self)
{
    ReportUnimplemented(self, Slot);
    return ErrorGeneral;
}

template<int... Slots>
void FillUnimplemented(ObjectClass& objectClass, std::integer_sequence<int, Slots...>)
{
    ((objectClass.slots[Slots] = reinterpret_cast<void*>(&UnimplementedSlot<Slots>)), ...);
}

// ---- IQueryInterface (slots 0-2) ----

void SlotAddRef(Object* self)
{
    AddReference(self);
}

void SlotRelease(Object* self)
{
    ReleaseReference(self);
}

/** Native objects implement every interface of their class on the same object. */
int SlotQueryInterface(Object* self, int interfaceId, void** object)
{
    bool supported = interfaceId == self->objectClass->id;
    for (ClassId id : self->objectClass->interfaces) {
        if (id == interfaceId) {
            supported = true;
        }
    }
    if (!supported) {
        *object = nullptr;
        return ErrorClassNotSupported;
    }
    AddReference(self);
    *object = self;
    return ErrorNone;
}

// ---- runtime root (ISwerve) ----

Object* rootObject;

int RootGetProperties(SwerveRuntimeProperties* properties)
{
    memset(properties, 0, sizeof(*properties));
    properties->m_pipeline = 0; // fixed function
    return ErrorNone;
}

/** Wrap a new m3gcore object; m3gCreate* hands it out without references. */
Object* WrapCreated(M3GObject handle)
{
    if (handle == nullptr) {
        return nullptr;
    }
    return Wrap(handle);
}

int RootCreateObject(int type, void** object)
{
    Object* created = nullptr;
    switch (type) {
    case ClassGraphics3D:
        created = CreateGraphics3D();
        break;
    case ClassLoader:
        created = CreateLoader();
        break;
    case ClassTransform:
        created = CreateTransform();
        break;
    case ClassRayIntersection:
        created = CreateRayIntersection();
        break;
    case ClassImage2D:
        created = CreateImage2D();
        break;
    case ClassGroup:
        created = WrapCreated(reinterpret_cast<M3GObject>(m3gCreateGroup(m3g)));
        break;
    case ClassWorld:
        created = WrapCreated(reinterpret_cast<M3GObject>(m3gCreateWorld(m3g)));
        break;
    case ClassLight:
        created = WrapCreated(reinterpret_cast<M3GObject>(m3gCreateLight(m3g)));
        break;
    case ClassCamera:
        created = WrapCreated(reinterpret_cast<M3GObject>(m3gCreateCamera(m3g)));
        break;
    case ClassAppearance:
        created = WrapCreated(reinterpret_cast<M3GObject>(m3gCreateAppearance(m3g)));
        break;
    case ClassBackground:
        created = WrapCreated(reinterpret_cast<M3GObject>(m3gCreateBackground(m3g)));
        break;
    case ClassCompositingMode:
        created = WrapCreated(reinterpret_cast<M3GObject>(m3gCreateCompositingMode(m3g)));
        break;
    case ClassFog:
        created = WrapCreated(reinterpret_cast<M3GObject>(m3gCreateFog(m3g)));
        break;
    case ClassMaterial:
        created = WrapCreated(reinterpret_cast<M3GObject>(m3gCreateMaterial(m3g)));
        break;
    case ClassPolygonMode:
        created = WrapCreated(reinterpret_cast<M3GObject>(m3gCreatePolygonMode(m3g)));
        break;
    case ClassAnimationController:
        created = WrapCreated(reinterpret_cast<M3GObject>(m3gCreateAnimationController(m3g)));
        break;
    default:
        fprintf(stderr, "Swerve native: CreateObject of type %d is not supported\n", type);
        *object = nullptr;
        return ErrorClassNotSupported;
    }
    *object = created;
    if (created == nullptr) {
        return ErrorNoMemory;
    }
    return TakeError();
}

int RootInitialize(Object*)
{
    return ErrorNone;
}

/** Identity for Object3D::operator== : one object per runtime object, whatever interface it is seen as. */
int RootGetObjectIdentity(void* object)
{
    return static_cast<int>(reinterpret_cast<uintptr_t>(object));
}

void FillRootSlots(ObjectClass& objectClass)
{
    FillQueryInterfaceSlots(objectClass);
    SetSlot(objectClass, 2, &RootGetProperties);      // 0x08 (no self)
    SetSlot(objectClass, 3, &RootCreateObject);       // 0x0c (no self)
    SetSlot(objectClass, 5, &RootInitialize);         // 0x14
    SetSlot(objectClass, 6, &RootGetObjectIdentity);  // 0x18 (no self)
}

// ---- class tables ----

struct ClassSpec {
    ClassId id;
    const char* name;
    std::vector<ClassId> interfaces;
    void (*fill)(ObjectClass& objectClass);
};

const std::vector<ClassSpec>& ClassSpecs()
{
    static const std::vector<ClassSpec> specs = {
        { ClassSwerve, "Swerve", {}, FillRootSlots },
        { ClassGraphics3D, "Graphics3D", {}, FillGraphics3DSlots },
        { ClassLoader, "Loader", {}, FillLoaderSlots },
        { ClassTransform, "Transform", {}, FillTransformSlots },
        { ClassRayIntersection, "RayIntersection", {}, FillRayIntersectionSlots },
        { ClassAnimationController, "AnimationController", { ClassObject3D }, FillAnimationControllerSlots },
        { ClassAnimationTrack, "AnimationTrack", { ClassObject3D }, FillAnimationTrackSlots },
        { ClassAppearance, "Appearance", { ClassObject3D }, FillAppearanceSlots },
        { ClassBackground, "Background", { ClassObject3D }, FillBackgroundSlots },
        { ClassCamera, "Camera", { ClassNode, ClassTransformable, ClassObject3D }, FillCameraSlots },
        { ClassCompositingMode, "CompositingMode", { ClassObject3D }, FillCompositingModeSlots },
        { ClassFog, "Fog", { ClassObject3D }, FillObject3DSlots },
        { ClassGroup, "Group", { ClassNode, ClassTransformable, ClassObject3D }, FillGroupSlots },
        { ClassImage2D, "Image2D", { ClassObject3D }, FillImage2DSlots },
        { ClassTriangleStripArray, "TriangleStripArray", { ClassIndexBuffer, ClassObject3D }, FillIndexBufferSlots },
        { ClassKeyframeSequence, "KeyframeSequence", { ClassObject3D }, FillKeyframeSequenceSlots },
        { ClassLight, "Light", { ClassNode, ClassTransformable, ClassObject3D }, FillLightSlots },
        { ClassMaterial, "Material", { ClassObject3D }, FillMaterialSlots },
        { ClassMesh, "Mesh", { ClassNode, ClassTransformable, ClassObject3D }, FillMeshSlots },
        { ClassMorphingMesh, "MorphingMesh", { ClassMesh, ClassNode, ClassTransformable, ClassObject3D }, FillMeshSlots },
        { ClassPolygonMode, "PolygonMode", { ClassObject3D }, FillObject3DSlots },
        { ClassSkinnedMesh, "SkinnedMesh", { ClassMesh, ClassNode, ClassTransformable, ClassObject3D },
          FillSkinnedMeshSlots },
        { ClassSprite3D, "Sprite3D", { ClassNode, ClassTransformable, ClassObject3D }, FillSpriteSlots },
        { ClassTexture2D, "Texture2D", { ClassTexture, ClassTransformable, ClassObject3D }, FillTextureSlots },
        { ClassVertexArray, "VertexArray", { ClassObject3D }, FillVertexArraySlots },
        { ClassVertexBuffer, "VertexBuffer", { ClassObject3D }, FillVertexBufferSlots },
        { ClassWorld, "World", { ClassGroup, ClassNode, ClassTransformable, ClassObject3D }, FillWorldSlots },
    };
    return specs;
}

void BuildClasses()
{
    for (const ClassSpec& spec : ClassSpecs()) {
        ObjectClass* objectClass = new ObjectClass();
        objectClass->name = spec.name;
        objectClass->id = spec.id;
        objectClass->interfaces = spec.interfaces;
        objectClass->allocate = AllocatePlainObject;
        objectClass->destroy = DestroyPlainObject;
        FillUnimplemented(*objectClass, std::make_integer_sequence<int, SlotCount>());
        spec.fill(*objectClass);
        classes[spec.id] = objectClass;
    }
}

/** Swerve class of an m3gcore object. */
ClassId ClassOfHandle(M3GObject handle)
{
    switch (m3gGetClass(handle)) {
    case M3G_CLASS_ANIMATION_CONTROLLER:
        return ClassAnimationController;
    case M3G_CLASS_ANIMATION_TRACK:
        return ClassAnimationTrack;
    case M3G_CLASS_APPEARANCE:
        return ClassAppearance;
    case M3G_CLASS_BACKGROUND:
        return ClassBackground;
    case M3G_CLASS_CAMERA:
        return ClassCamera;
    case M3G_CLASS_COMPOSITING_MODE:
        return ClassCompositingMode;
    case M3G_CLASS_FOG:
        return ClassFog;
    case M3G_CLASS_GROUP:
        return ClassGroup;
    case M3G_CLASS_IMAGE:
        return ClassImage2D;
    case M3G_CLASS_INDEX_BUFFER:
        return ClassTriangleStripArray;
    case M3G_CLASS_KEYFRAME_SEQUENCE:
        return ClassKeyframeSequence;
    case M3G_CLASS_LIGHT:
        return ClassLight;
    case M3G_CLASS_MATERIAL:
        return ClassMaterial;
    case M3G_CLASS_MESH:
        return ClassMesh;
    case M3G_CLASS_MORPHING_MESH:
        return ClassMorphingMesh;
    case M3G_CLASS_POLYGON_MODE:
        return ClassPolygonMode;
    case M3G_CLASS_SKINNED_MESH:
        return ClassSkinnedMesh;
    case M3G_CLASS_SPRITE:
        return ClassSprite3D;
    case M3G_CLASS_TEXTURE:
        return ClassTexture2D;
    case M3G_CLASS_VERTEX_ARRAY:
        return ClassVertexArray;
    case M3G_CLASS_VERTEX_BUFFER:
        return ClassVertexBuffer;
    case M3G_CLASS_WORLD:
        return ClassWorld;
    default:
        break;
    }
    fprintf(stderr, "Swerve native: m3gcore class %d has no Swerve class\n", m3gGetClass(handle));
    abort();
}

} // namespace

M3GInterface Interface()
{
    return m3g;
}

int TakeError()
{
    const M3Genum error = lastError;
    lastError = M3G_NO_ERROR;
    switch (error) {
    case M3G_NO_ERROR:
        return ErrorNone;
    case M3G_OUT_OF_MEMORY:
        return ErrorNoMemory;
    case M3G_NULL_POINTER:
        return ErrorNullPointer;
    case M3G_INVALID_OPERATION:
        return ErrorInvalidState;
    case M3G_IO_ERROR:
        return ErrorCorruptFile;
    case M3G_INVALID_VALUE:
    case M3G_INVALID_ENUM:
    case M3G_INVALID_INDEX:
    case M3G_INVALID_OBJECT:
        return ErrorInvalidParameter;
    }
    return ErrorGeneral;
}

const ObjectClass& ClassOf(ClassId id)
{
    return *classes.at(id);
}

Object* Wrap(M3GObject handle)
{
    if (handle == nullptr) {
        return nullptr;
    }
    const auto found = wrappers.find(handle);
    if (found != wrappers.end()) {
        found->second->references++;
        return found->second;
    }
    const ObjectClass& objectClass = ClassOf(ClassOfHandle(handle));
    Object* object = objectClass.allocate();
    object->functions = objectClass.slots;
    object->objectClass = &objectClass;
    object->references = 1;
    object->handle = handle;
    m3gAddRef(handle);
    wrappers[handle] = object;
    return object;
}

void WrapInto(M3GObject handle, void** out)
{
    *out = Wrap(handle);
}

M3GObject HandleOf(const void* object)
{
    if (object == nullptr) {
        return nullptr;
    }
    return static_cast<const Object*>(object)->handle;
}

void AddReference(Object* object)
{
    object->references++;
}

void ReleaseReference(Object* object)
{
    object->references--;
    if (object->references > 0) {
        return;
    }
    if (object->handle != nullptr) {
        wrappers.erase(object->handle);
        m3gDeleteRef(object->handle);
        object->handle = nullptr;
    }
    object->objectClass->destroy(object);
}

void InitObject(Object* object, ClassId id)
{
    const ObjectClass& objectClass = ClassOf(id);
    object->functions = objectClass.slots;
    object->objectClass = &objectClass;
    object->references = 1;
    object->handle = nullptr;
}

void AttachHandle(Object* object, M3GObject handle)
{
    m3gAddRef(handle);
    object->handle = handle;
    wrappers[handle] = object;
}

void FillQueryInterfaceSlots(ObjectClass& objectClass)
{
    SetSlot(objectClass, 0, &SlotAddRef);
    SetSlot(objectClass, 1, &SlotRelease);
    SetSlot(objectClass, 2, &SlotQueryInterface);
}

void StoreUserParameter(M3GObject object, int id, const M3Gbyte* data, int length)
{
    UserParameter parameter;
    parameter.id = id;
    parameter.data.assign(data, data + length);
    userParameters[object].push_back(parameter);
}

int FindUserParameter(M3GObject object, int id, int bufferSize, char* buffer)
{
    const auto found = userParameters.find(object);
    if (found == userParameters.end()) {
        return -1;
    }
    for (const UserParameter& parameter : found->second) {
        if (parameter.id != id) {
            continue;
        }
        const int length = static_cast<int>(parameter.data.size());
        if (buffer != nullptr && bufferSize > 0) {
            int copied = length;
            if (copied > bufferSize) {
                copied = bufferSize;
            }
            memcpy(buffer, parameter.data.data(), static_cast<size_t>(copied));
        }
        return length;
    }
    return -1;
}

int CreateSwerve(ISwerve** swerve)
{
    if (m3g == nullptr) {
        M3Gparams params = {};
        params.mallocFunc = AllocateMemory;
        params.freeFunc = FreeMemory;
        params.errorFunc = RememberError;
        m3g = m3gCreateInterface(&params);
        if (m3g == nullptr) {
            fprintf(stderr, "Swerve native: m3gCreateInterface failed\n");
            return ErrorFailed;
        }
        BuildClasses();
        rootObject = new Object();
        InitObject(rootObject, ClassSwerve);
        fprintf(stderr, "Swerve native backend ready (m3gcore)\n");
    } else {
        AddReference(rootObject);
    }
    *swerve = reinterpret_cast<ISwerve*>(rootObject);
    return ErrorNone;
}

} // namespace ckz::native
