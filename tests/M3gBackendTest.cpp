// Differential test of the two Swerve backends (PORT_PLAN.md P2 / M2): every M3G file of
// zombsniper.big is loaded with the native m3gcore runtime and with the original ARM Swerve core,
// through the game's own loader (CSwerveLoader), and both scene graphs are compared through the
// same object slots the game calls: node classes, user ids, transforms, world matrices, rendering
// and picking switches, mesh data (the game's collision mesh, CCollision::BuildMesh, is built from
// it), camera projections, the scene after animate() at several times, and screen ray picks.
//
// The original Swerve is the reference: a difference is a bug of the native backend unless it is
// listed in KnownDifferences below with its reason.
#include "BigArchive.h"
#include "GameHarness.h"
#include "../CKZ_source/pc_port/swerve_compat/ArmBackend.h"
#include "../CKZ_source/pc_port/platform_sdl/Config.h"
#include "../CKZ_source/pc_port/platform_sdl/Host.h"
#include <gtest/gtest.h>
// The PC copy of the Swerve API (ARM-aware calls) must come before any game header that
// includes "SwerveApi.h" next to itself (packages/swerve/code/inc, the iOS original).
#include <SwerveApi.h>
#include <math.h>
#include <algorithm>
#include <stdio.h>
#include <string.h>
#include <map>
#include <string>
#include <vector>
#include "CSwerve.h"
#include "CSwerveLoader.h"
#include "tws.h"

namespace ckz::test {
namespace {

using swerve::IAnimationTrackInterface;
using swerve::ICameraInterface;
using swerve::IGroupInterface;
using swerve::IIndexBufferInterface;
using swerve::IMeshInterface;
using swerve::INodeInterface;
using swerve::IObject3DInterface;
using swerve::IRayIntersectionInterface;
using swerve::ISkinnedMeshInterface;
using swerve::ITransformableInterface;
using swerve::ITransformInterface;
using swerve::IVertexArrayInterface;
using swerve::IVertexBufferInterface;
using swerve::IWorldInterface;

// Swerve class ids (CreateObject types and QueryInterface ids, NativeRuntime.h).
constexpr int ClassCamera = 5;
constexpr int ClassGroup = 9;
constexpr int ClassLight = 13;
constexpr int ClassMesh = 16;
constexpr int ClassNode = 18;
constexpr int ClassRayIntersection = 21;
constexpr int ClassSkinnedMesh = 22;
constexpr int ClassSprite3D = 23;
constexpr int ClassTransform = 26;
constexpr int ClassTransformable = 27;
constexpr int ClassWorld = 31;
// The classes recorded for every object, in this order (one bit each).
const int RecordedClasses[] = { ClassNode, ClassGroup, ClassWorld, ClassMesh, ClassSkinnedMesh, ClassCamera,
                                ClassLight, ClassSprite3D, ClassTransformable };

// Solo phase (how the game plays animations, CDH_Animation): each of the first SoloControllers
// controllers alone at weight 1, compared SoloTimes ms after it was started.
constexpr size_t SoloControllers = 4;
const int SoloTimes[] = { 95, 455, 1305 };
// World time at which the solo phase starts (after the AnimationTimes).
constexpr int SoloStartTime = 10000;
// Sequence time the solo controller starts at: the fraction tells whether the sequence time is
// sampled as a float, rounded or truncated to whole milliseconds.
constexpr float SoloSequenceStart = 0.6f;
// World times (ms) at which the animated scene is compared.
const int AnimationTimes[] = { 0, 150, 700, 2300, 6100 };
// Picks: a grid of PickGrid x PickGrid normalized screen points through the active camera.
constexpr int PickGrid = 9;
// User ids are recorded as id / IdSplit and id % IdSplit, each exact in a float.
constexpr int IdSplit = 100000;
// Two floats agree when |a - b| <= Tolerance * max(1, |a|, |b|).
constexpr float Tolerance = 2e-3f;
// Mismatches reported per file; the rest is only counted.
constexpr int ReportedMismatches = 12;
// Set CKZ_DIFF_ALL=1 to report every mismatching record (for investigating one file).
constexpr const char* ReportAllVariable = "CKZ_DIFF_ALL";
// Triangle indices shown in full per submesh (all of them go into a checksum).
constexpr size_t IndicesShown = 18;
// Largest number of vertices whose positions are compared one by one (the rest by checksum).
constexpr int MaxVertexComponents = 1 << 20;

/** One recorded value of a scene: where it was read, what, the call's error code and the numbers. */
struct Record {
    std::string path;   // e.g. "/World#1234/Group#5/Mesh#77"
    std::string field;  // e.g. "translation"
    int error;
    std::vector<float> values;
    bool quaternion;    // q and -q are the same orientation
};

/** All records of one backend's scene, in walk order. */
typedef std::vector<Record> Snapshot;

/** A Swerve runtime, the per-backend objects the walk needs, and its name for messages. */
struct Backend {
    const char* name;
    ISwerve* swerve;
    ITransformInterface* transform;
    IRayIntersectionInterface* intersection;
    int worldTime; // time of the last animate(), for the controllers' sequence positions
};

void* CreateObject(ISwerve* swerve, int type)
{
    void* object = nullptr;
    swerve->m_functions->CreateObject(type, &object);
    return object;
}

/** QueryInterface for `classId`; the returned reference is released at once (only "is it?" is asked). */
bool Supports(IObject3DInterface* object, int classId)
{
    void* found = nullptr;
    const int error = object->QueryInterface(classId, &found);
    if (error != 0 || found == nullptr) {
        return false;
    }
    static_cast<IQueryInterface*>(found)->Release();
    return true;
}

void Add(Snapshot& snapshot, const std::string& path, const char* field, int error, std::vector<float> values,
         bool quaternion = false)
{
    Record record;
    record.path = path;
    record.field = field;
    record.error = error;
    record.values = std::move(values);
    record.quaternion = quaternion;
    snapshot.push_back(std::move(record));
}

/** FNV-1a over raw bytes: exact comparison of large integer arrays. */
float Checksum(const void* data, size_t size)
{
    const auto* bytes = static_cast<const uint8_t*>(data);
    uint32_t hash = 2166136261u;
    for (size_t index = 0; index < size; index++) {
        hash ^= bytes[index];
        hash *= 16777619u;
    }
    // Stored as two exact halves (a float holds 24 bits exactly).
    return static_cast<float>(hash >> 16) * 65536.0f + static_cast<float>(hash & 0xffff);
}

std::vector<float> ReadMatrix(ITransformInterface* transform)
{
    std::vector<float> matrix(16, 0.0f);
    transform->get(16, matrix.data());
    return matrix;
}

void WalkObject(Backend& backend, IObject3DInterface* object, INodeInterface* root, const std::string& parentPath,
                Snapshot& snapshot, int depth);

/** Mesh data: what CCollision::CollectMeshInfo / BuildMesh read, plus the appearances' presence. */
void RecordMesh(IMeshInterface* mesh, const std::string& path, Snapshot& snapshot)
{
    int submeshCount = 0;
    const int submeshError = mesh->getSubmeshCount(&submeshCount);
    Add(snapshot, path, "submeshCount", submeshError, { static_cast<float>(submeshCount) });
    IVertexBufferInterface* vertexBuffer = nullptr;
    const int bufferError = mesh->getVertexBuffer(&vertexBuffer);
    Add(snapshot, path, "vertexBuffer", bufferError, { static_cast<float>(vertexBuffer != nullptr) });
    if (vertexBuffer != nullptr) {
        int vertexCount = 0;
        const int countError = vertexBuffer->getVertexCount(&vertexCount);
        Add(snapshot, path, "vertexCount", countError, { static_cast<float>(vertexCount) });
        float scaleBias[4] = {};
        IVertexArrayInterface* positions = nullptr;
        const int positionsError = vertexBuffer->getPositions(4, scaleBias, &positions);
        Add(snapshot, path, "positionScaleBias", positionsError,
            { scaleBias[0], scaleBias[1], scaleBias[2], scaleBias[3] });
        if (positions != nullptr) {
            int arrayCount = 0;
            const int arrayCountError = positions->getVertexCount(&arrayCount);
            Add(snapshot, path, "positionCount", arrayCountError, { static_cast<float>(arrayCount) });
            if (arrayCount > 0 && arrayCount * 3 <= MaxVertexComponents) {
                std::vector<short> components(static_cast<size_t>(arrayCount) * 3);
                const int getError = positions->get(0, arrayCount, arrayCount * 3, components.data());
                Add(snapshot, path, "positionValues", getError,
                    { Checksum(components.data(), components.size() * sizeof(short)) });
            }
            positions->Release();
        }
        vertexBuffer->Release();
    }
    for (int submesh = 0; submesh < submeshCount; submesh++) {
        const std::string submeshPath = path + "/submesh" + std::to_string(submesh);
        IIndexBufferInterface* indexBuffer = nullptr;
        const int indexError = mesh->getTriangleIndexBuffer(submesh, &indexBuffer);
        Add(snapshot, submeshPath, "indexBuffer", indexError, { static_cast<float>(indexBuffer != nullptr) });
        if (indexBuffer != nullptr) {
            int indexCount = 0;
            const int indexCountError = indexBuffer->getIndexCount(&indexCount);
            Add(snapshot, submeshPath, "indexCount", indexCountError, { static_cast<float>(indexCount) });
            if (indexCount > 0) {
                std::vector<unsigned int> indices(static_cast<size_t>(indexCount));
                const int indicesError = indexBuffer->getIndices(indexCount, indices.data());
                Add(snapshot, submeshPath, "indices", indicesError,
                    { Checksum(indices.data(), indices.size() * sizeof(unsigned int)) });
                // The first triangles in full, to show how a difference looks.
                std::vector<float> head;
                for (size_t index = 0; index < indices.size() && index < IndicesShown; index++) {
                    head.push_back(static_cast<float>(indices[index]));
                }
                Add(snapshot, submeshPath, "firstIndices", indicesError, head);
            }
            indexBuffer->Release();
        }
        swerve::IAppearanceInterface* appearance = nullptr;
        const int appearanceError = mesh->getAppearance(submesh, &appearance);
        Add(snapshot, submeshPath, "appearance", appearanceError, { static_cast<float>(appearance != nullptr) });
        if (appearance != nullptr) {
            appearance->Release();
        }
    }
}

/** Transformable state: TRS, orientation quaternion, composite transform. */
void RecordTransformable(Backend& backend, ITransformableInterface* transformable, const std::string& path,
                         Snapshot& snapshot)
{
    float translation[3] = {};
    const int translationError = transformable->getTranslation(translation);
    Add(snapshot, path, "translation", translationError, { translation[0], translation[1], translation[2] });
    float scale[3] = {};
    const int scaleError = transformable->getScale(scale);
    Add(snapshot, path, "scale", scaleError, { scale[0], scale[1], scale[2] });
    float quaternion[4] = {};
    const int orientationError = transformable->getOrientation84(quaternion);
    Add(snapshot, path, "orientationQuat", orientationError,
        { quaternion[0], quaternion[1], quaternion[2], quaternion[3] }, true);
    backend.transform->setIdentity();
    const int compositeError = transformable->getCompositeTransform(backend.transform);
    Add(snapshot, path, "compositeTransform", compositeError, ReadMatrix(backend.transform));
}

/** Node state: switches and the transform to the scene root. */
void RecordNode(Backend& backend, INodeInterface* node, INodeInterface* root, const std::string& path, Snapshot& snapshot)
{
    unsigned char rendering = 0;
    const int renderingError = node->getRenderingEnable(&rendering);
    Add(snapshot, path, "renderingEnable", renderingError, { static_cast<float>(rendering) });
    unsigned char picking = 0;
    const int pickingError = node->getPickingEnable(&picking);
    Add(snapshot, path, "pickingEnable", pickingError, { static_cast<float>(picking) });
    backend.transform->setIdentity();
    bool reachable = false;
    const int toRootError = node->getTransformTo(root, backend.transform, &reachable);
    std::vector<float> matrix = ReadMatrix(backend.transform);
    matrix.push_back(static_cast<float>(reachable));
    Add(snapshot, path, "transformToRoot", toRootError, matrix);
}

void RecordCamera(ICameraInterface* camera, const std::string& path, Snapshot& snapshot)
{
    float parameters[4] = {};
    int projectionType = 0;
    const int error = camera->getProjection(4, parameters, &projectionType);
    Add(snapshot, path, "projection", error,
        { parameters[0], parameters[1], parameters[2], parameters[3], static_cast<float>(projectionType) });
}

void WalkChildren(Backend& backend, IGroupInterface* group, INodeInterface* root, const std::string& path,
                  Snapshot& snapshot, int depth)
{
    int childCount = 0;
    const int countError = group->getChildCount(&childCount);
    Add(snapshot, path, "childCount", countError, { static_cast<float>(childCount) });
    for (int index = 0; index < childCount; index++) {
        INodeInterface* child = nullptr;
        group->getChild(index, &child);
        if (child == nullptr) {
            Add(snapshot, path, "missingChild", 0, { static_cast<float>(index) });
            continue;
        }
        WalkObject(backend, child, root, path, snapshot, depth + 1);
        child->Release();
    }
}

/**
 * One animation track as numbers: track user id, controller user id, weight, speed, sequence
 * position now, sequence user id, duration, repeat mode (-1 where a part is missing).
 */
std::vector<float> DescribeTrack(Backend& backend, IObject3DInterface* object, int index, int& error)
{
    std::vector<float> values(8, -1.0f);
    IAnimationTrackInterface* track = nullptr;
    error = object->getAnimationTrack(index, &track);
    if (track == nullptr) {
        return values;
    }
    int trackId = 0;
    track->getUserID(&trackId);
    values[0] = static_cast<float>(trackId);
    swerve::IAnimationControllerInterface* controller = nullptr;
    error += track->getController(&controller);
    if (controller != nullptr) {
        int controllerId = 0;
        float weight = 0.0f;
        float speed = 0.0f;
        float position = 0.0f;
        controller->getUserID(&controllerId);
        error += controller->getWeight(&weight);
        error += controller->getSpeed(&speed);
        error += controller->getPosition(backend.worldTime, &position);
        values[1] = static_cast<float>(controllerId);
        values[2] = weight;
        values[3] = speed;
        values[4] = position;
        controller->Release();
    }
    swerve::IKeyframeSequenceInterface* sequence = nullptr;
    error += track->getKeyframeSequence(&sequence);
    if (sequence != nullptr) {
        int sequenceId = 0;
        int duration = 0;
        int repeatMode = 0;
        sequence->getUserID(&sequenceId);
        error += sequence->getDuration(&duration);
        error += sequence->getRepeatMode(&repeatMode);
        values[5] = static_cast<float>(sequenceId);
        values[6] = static_cast<float>(duration);
        values[7] = static_cast<float>(repeatMode);
        sequence->Release();
    }
    track->Release();
    return values;
}

/** Sort order of track descriptions: plain lexicographic order of the numbers. */
bool TrackComesFirst(const std::vector<float>& left, const std::vector<float>& right)
{
    return left < right;
}

/**
 * The animation tracks of an object as a set. Their order is not compared: m3gcore keeps them
 * sorted by target property (for blending), Swerve in another order, and every place the game
 * walks tracks by index only looks for a controller or the longest duration (CDH_Animation,
 * SwerveHelper), so the order cannot change the game.
 */
void RecordAnimationTracks(Backend& backend, IObject3DInterface* object, int trackCount, const std::string& path,
                           Snapshot& snapshot)
{
    std::vector<std::vector<float>> tracks;
    int errors = 0;
    for (int index = 0; index < trackCount; index++) {
        int error = 0;
        tracks.push_back(DescribeTrack(backend, object, index, error));
        if (error != 0) {
            errors++;
        }
    }
    std::sort(tracks.begin(), tracks.end(), TrackComesFirst);
    Add(snapshot, path, "trackErrors", 0, { static_cast<float>(errors) });
    for (size_t index = 0; index < tracks.size(); index++) {
        Add(snapshot, path + "/track" + std::to_string(index), "track", 0, tracks[index]);
    }
}

void WalkObject(Backend& backend, IObject3DInterface* object, INodeInterface* root, const std::string& parentPath,
                Snapshot& snapshot, int depth)
{
    int userId = 0;
    const int userIdError = object->getUserID(&userId);
    unsigned int classMask = 0;
    for (size_t index = 0; index < sizeof(RecordedClasses) / sizeof(RecordedClasses[0]); index++) {
        if (Supports(object, RecordedClasses[index])) {
            classMask |= 1u << index;
        }
    }
    const std::string path = parentPath + "/" + std::to_string(classMask) + "#" + std::to_string(userId);
    Add(snapshot, path, "userID", userIdError, { static_cast<float>(userId) });
    Add(snapshot, path, "classMask", 0, { static_cast<float>(classMask) });
    int trackCount = 0;
    const int trackError = object->getAnimationTrackCount(&trackCount);
    Add(snapshot, path, "animationTracks", trackError, { static_cast<float>(trackCount) });
    RecordAnimationTracks(backend, object, trackCount, path, snapshot);
    if (depth > 64) {
        Add(snapshot, path, "tooDeep", 0, {});
        return;
    }
    if (Supports(object, ClassTransformable)) {
        RecordTransformable(backend, static_cast<ITransformableInterface*>(object), path, snapshot);
    }
    if (Supports(object, ClassNode)) {
        RecordNode(backend, static_cast<INodeInterface*>(object), root, path, snapshot);
    }
    if (Supports(object, ClassCamera)) {
        RecordCamera(static_cast<ICameraInterface*>(object), path, snapshot);
    }
    if (Supports(object, ClassMesh)) {
        RecordMesh(static_cast<IMeshInterface*>(object), path, snapshot);
    }
    if (Supports(object, ClassSkinnedMesh)) {
        IGroupInterface* skeleton = nullptr;
        const int skeletonError = static_cast<ISkinnedMeshInterface*>(object)->getSkeleton(&skeleton);
        Add(snapshot, path, "skeleton", skeletonError, { static_cast<float>(skeleton != nullptr) });
        if (skeleton != nullptr) {
            WalkObject(backend, skeleton, root, path + "/skeleton", snapshot, depth + 1);
            skeleton->Release();
        }
    }
    if (Supports(object, ClassGroup)) {
        WalkChildren(backend, static_cast<IGroupInterface*>(object), root, path, snapshot, depth);
    }
}

/** Everything a pick result tells the game: hit, distance, picked node id, normal and ray. */
void RecordIntersection(Backend& backend, int error, unsigned char hit, const std::string& path, Snapshot& snapshot)
{
    std::vector<float> values;
    values.push_back(static_cast<float>(hit));
    if (hit) {
        float distance = 0.0f;
        backend.intersection->getDistance(&distance);
        int pickedId = 0;
        unsigned int pickedClasses = 0;
        INodeInterface* picked = nullptr;
        backend.intersection->getIntersected(&picked);
        if (picked != nullptr) {
            picked->getUserID(&pickedId);
            for (size_t index = 0; index < sizeof(RecordedClasses) / sizeof(RecordedClasses[0]); index++) {
                if (Supports(picked, RecordedClasses[index])) {
                    pickedClasses |= 1u << index;
                }
            }
            picked->Release();
        }
        float normal[3] = {};
        backend.intersection->getNormalX(&normal[0]);
        backend.intersection->getNormalY(&normal[1]);
        backend.intersection->getNormalZ(&normal[2]);
        float ray[6] = {};
        backend.intersection->getRay(6, ray);
        values.push_back(distance);
        // The user id in two exact parts (a float holds 24 bits), then the picked node's classes.
        values.push_back(static_cast<float>(pickedId / IdSplit));
        values.push_back(static_cast<float>(pickedId % IdSplit));
        values.push_back(static_cast<float>(pickedClasses));
        for (float component : normal) {
            values.push_back(component);
        }
        for (float component : ray) {
            values.push_back(component);
        }
    }
    Add(snapshot, path, "pick", error, values);
}

/**
 * Picks of a world: through the active camera on a screen grid (Group slot 84, the game's shots
 * and head picks) and as rays from the camera position (slot 83, CUnitBody::Pick of projectiles).
 */
void RecordPicks(Backend& backend, IObject3DInterface* rootObject, const std::string& path, Snapshot& snapshot)
{
    if (!Supports(rootObject, ClassWorld)) {
        return;
    }
    auto* world = static_cast<IWorldInterface*>(rootObject);
    ICameraInterface* camera = nullptr;
    world->getActiveCamera(&camera);
    if (camera == nullptr) {
        return;
    }
    // Camera axes in world space for the rays.
    backend.transform->setIdentity();
    bool reachable = false;
    camera->getTransformTo(world, backend.transform, &reachable);
    const std::vector<float> cameraMatrix = ReadMatrix(backend.transform);
    const float origin[3] = { cameraMatrix[3], cameraMatrix[7], cameraMatrix[11] };
    for (int row = 0; row < PickGrid; row++) {
        for (int column = 0; column < PickGrid; column++) {
            const float x = (column + 0.5f) / PickGrid;
            const float y = (row + 0.5f) / PickGrid;
            const std::string pickPath = path + "/pick(" + std::to_string(column) + "," + std::to_string(row) + ")";
            unsigned char hit = 0;
            const int error = world->pick(-1, x, y, camera, backend.intersection, &hit);
            RecordIntersection(backend, error, hit, pickPath + "camera", snapshot);
            // Ray: forward (-Z of the camera) tilted by the grid position.
            const float side = x - 0.5f;
            const float up = 0.5f - y;
            float direction[3];
            for (int axis = 0; axis < 3; axis++) {
                const int row4 = axis * 4;
                direction[axis] = -cameraMatrix[row4 + 2] + side * cameraMatrix[row4] + up * cameraMatrix[row4 + 1];
            }
            unsigned char rayHit = 0;
            const int rayError = world->pick(-1, origin, direction, backend.intersection, &rayHit);
            RecordIntersection(backend, rayError, rayHit, pickPath + "ray", snapshot);
        }
    }
    camera->Release();
}

/** Load `data` with the backend through CSwerveLoader (the game's loader picks CSwerve's runtime). */
int LoadWith(ISwerve* runtime, const std::vector<uint8_t>& data, std::vector<IObject3D*>& roots)
{
    CSwerve* game = CSwerve::GetInstance();
    ISwerve* saved = game->m_swerve;
    game->m_swerve = runtime;
    com::glu::platform::core::CVector<IObject3D*> objects;
    const int error = CSwerveLoader::load(static_cast<unsigned int>(data.size()), data.data(), 0, objects);
    game->m_swerve = saved;
    for (int index = 0; index < objects.m_size; index++) {
        roots.push_back(objects.m_data[index]);
    }
    return error;
}

void ReleaseRoots(std::vector<IObject3D*>& roots)
{
    for (IObject3D* root : roots) {
        root->m_functions->Release(root);
    }
    roots.clear();
}

/** Everything compared for one root object at one moment (the picks only when `withPicks`). */
void SnapshotRoot(Backend& backend, IObject3D* root, const std::string& label, Snapshot& snapshot, bool withPicks = true)
{
    auto* object = reinterpret_cast<IObject3DInterface*>(root);
    INodeInterface* rootNode = nullptr;
    if (Supports(object, ClassNode)) {
        rootNode = static_cast<INodeInterface*>(object);
    }
    WalkObject(backend, object, rootNode, label, snapshot, 0);
    if (withPicks) {
        RecordPicks(backend, object, label, snapshot);
    }
}

/** The animation controllers of a scene by user id (nodes and skeletons; id 0 is skipped). */
typedef std::map<int, swerve::IAnimationControllerInterface*> Controllers;

void CollectControllers(IObject3DInterface* object, Controllers& controllers, int depth)
{
    if (depth > 64) {
        return;
    }
    int trackCount = 0;
    object->getAnimationTrackCount(&trackCount);
    for (int index = 0; index < trackCount; index++) {
        IAnimationTrackInterface* track = nullptr;
        object->getAnimationTrack(index, &track);
        if (track == nullptr) {
            continue;
        }
        swerve::IAnimationControllerInterface* controller = nullptr;
        track->getController(&controller);
        track->Release();
        if (controller == nullptr) {
            continue;
        }
        int id = 0;
        controller->getUserID(&id);
        if (id == 0 || controllers.count(id) != 0) {
            controller->Release();
            continue;
        }
        // Keeps the reference until ReleaseControllers.
        controllers[id] = controller;
    }
    if (Supports(object, ClassSkinnedMesh)) {
        IGroupInterface* skeleton = nullptr;
        static_cast<ISkinnedMeshInterface*>(object)->getSkeleton(&skeleton);
        if (skeleton != nullptr) {
            CollectControllers(skeleton, controllers, depth + 1);
            skeleton->Release();
        }
    }
    if (Supports(object, ClassGroup)) {
        auto* group = static_cast<IGroupInterface*>(object);
        int childCount = 0;
        group->getChildCount(&childCount);
        for (int index = 0; index < childCount; index++) {
            INodeInterface* child = nullptr;
            group->getChild(index, &child);
            if (child != nullptr) {
                CollectControllers(child, controllers, depth + 1);
                child->Release();
            }
        }
    }
}

void ReleaseControllers(Controllers& controllers)
{
    for (auto& entry : controllers) {
        entry.second->Release();
    }
    controllers.clear();
}

/** Weight 1 for `solo`, 0 for every other controller; `solo` starts its sequence at `worldTime`. */
void SoloController(Controllers& controllers, int solo, int worldTime)
{
    for (auto& entry : controllers) {
        if (entry.first == solo) {
            entry.second->setWeight(1.0f);
            entry.second->setPosition(SoloSequenceStart, worldTime);
        } else {
            entry.second->setWeight(0.0f);
        }
    }
}

bool FloatsAgree(float left, float right)
{
    if (isnan(left) && isnan(right)) {
        return true;
    }
    float scale = 1.0f;
    if (fabsf(left) > scale) {
        scale = fabsf(left);
    }
    if (fabsf(right) > scale) {
        scale = fabsf(right);
    }
    return fabsf(left - right) <= Tolerance * scale;
}

// Pick records: hit, distance, id / IdSplit, id % IdSplit, classes, normal (3), ray (6).
constexpr size_t PickDistance = 1;
constexpr size_t PickIdHigh = 2;
constexpr size_t PickIdLow = 3;
constexpr size_t PickClasses = 4;
constexpr size_t PickNormal = 5;
constexpr size_t PickRay = 8;
// Skinning precision: the original skins with slightly other numbers, so the face normal of a
// zombie hit may turn by about a degree. Normals agree within this per component (about 2 deg).
constexpr float NormalTolerance = 0.035f;
// Coplanar faces at the same distance (decals on walls) are hit in an order that depends on the
// last bit of each backend's arithmetic; such ties are counted, not failed.
constexpr float TieTolerance = 2e-4f;
// Ties seen in the current test (reset per file).
int pickTies = 0;

/** Pick records: the hit and its ray strictly, the normal within NormalTolerance, ties counted. */
bool PicksAgree(const Record& expected, const Record& actual)
{
    if (expected.values.size() != actual.values.size()) {
        return false;
    }
    if (expected.values.size() <= 1) {
        return expected.values == actual.values;
    }
    for (size_t index = PickRay; index < expected.values.size(); index++) {
        if (!FloatsAgree(expected.values[index], actual.values[index])) {
            return false;
        }
    }
    const float expectedDistance = expected.values[PickDistance];
    const float actualDistance = actual.values[PickDistance];
    const bool sameNode = expected.values[PickIdHigh] == actual.values[PickIdHigh] &&
                          expected.values[PickIdLow] == actual.values[PickIdLow] &&
                          expected.values[PickClasses] == actual.values[PickClasses];
    if (!sameNode) {
        const float scale = fabsf(expectedDistance) + 1e-6f;
        if (fabsf(expectedDistance - actualDistance) <= TieTolerance * scale) {
            pickTies++;
            return true;
        }
        return false;
    }
    if (!FloatsAgree(expectedDistance, actualDistance)) {
        return false;
    }
    for (size_t index = PickNormal; index < PickRay; index++) {
        if (fabsf(expected.values[index] - actual.values[index]) > NormalTolerance) {
            return false;
        }
    }
    return true;
}

bool ValuesAgree(const Record& expected, const Record& actual)
{
    if (expected.field == "pick") {
        return PicksAgree(expected, actual);
    }
    if (expected.values.size() != actual.values.size()) {
        return false;
    }
    bool same = true;
    bool negated = expected.quaternion;
    for (size_t index = 0; index < expected.values.size(); index++) {
        if (!FloatsAgree(expected.values[index], actual.values[index])) {
            same = false;
        }
        if (!FloatsAgree(expected.values[index], -actual.values[index])) {
            negated = false;
        }
    }
    return same || negated;
}

std::string FormatValues(const std::vector<float>& values)
{
    std::string text;
    for (size_t index = 0; index < values.size(); index++) {
        char number[32];
        snprintf(number, sizeof(number), "%.6g", values[index]);
        if (index > 0) {
            text += " ";
        }
        text += number;
    }
    return text;
}

/**
 * Compare the native snapshot with the original one. Records are matched in walk order; a
 * structural difference (another path) stops the comparison because the walks diverged.
 * Returns the number of mismatching records.
 */
int CompareSnapshots(const std::string& file, const Snapshot& original, const Snapshot& native)
{
    int mismatches = 0;
    const size_t count = std::min(original.size(), native.size());
    for (size_t index = 0; index < count; index++) {
        const Record& expected = original[index];
        const Record& actual = native[index];
        if (expected.path != actual.path || expected.field != actual.field) {
            ADD_FAILURE() << file << ": scene graphs diverge at record " << index << "\n  original " << expected.path
                          << " " << expected.field << "\n  native   " << actual.path << " " << actual.field;
            return mismatches + 1;
        }
        const bool errorsAgree = (expected.error == 0) == (actual.error == 0);
        if (errorsAgree && ValuesAgree(expected, actual)) {
            continue;
        }
        mismatches++;
        if (mismatches <= ReportedMismatches || SDL_getenv(ReportAllVariable) != nullptr) {
            ADD_FAILURE() << file << ": " << expected.path << " " << expected.field << "\n  original (error " << expected.error
                          << "): " << FormatValues(expected.values) << "\n  native   (error " << actual.error
                          << "): " << FormatValues(actual.values);
        }
    }
    if (original.size() != native.size()) {
        ADD_FAILURE() << file << ": " << original.size() << " original records, " << native.size() << " native records";
        mismatches++;
    }
    if (mismatches > ReportedMismatches) {
        ADD_FAILURE() << file << ": " << mismatches << " mismatching records in total";
    }
    return mismatches;
}

/** Both backends for the whole suite: the game's native runtime and one original ARM runtime. */
struct Backends {
    Backend native;
    Backend original;
    bool ready;
};

Backends backends = {};

Backend MakeBackend(const char* name, ISwerve* swerve)
{
    Backend backend = {};
    backend.name = name;
    backend.swerve = swerve;
    backend.transform = static_cast<ITransformInterface*>(CreateObject(swerve, ClassTransform));
    backend.intersection = static_cast<IRayIntersectionInterface*>(CreateObject(swerve, ClassRayIntersection));
    return backend;
}

/** The M3G resources of zombsniper.big, read once. */
std::vector<BigEntry> M3gEntries()
{
    static std::vector<BigEntry> entries;
    static bool scanned = false;
    if (scanned) {
        return entries;
    }
    scanned = true;
    BigArchive archive;
    if (!archive.Open(std::filesystem::path(ckz::assetPath) / L"zombsniper.big")) {
        return entries;
    }
    for (const BigEntry& entry : archive.Entries()) {
        // Only the first bytes tell: read small entries whole, the identifier is all that is needed.
        if (IsM3gData(archive.Read(entry))) {
            entries.push_back(entry);
        }
    }
    fprintf(stderr, "M3G backend test: %zu M3G files in zombsniper.big\n", entries.size());
    return entries;
}

class M3gBackends : public testing::TestWithParam<BigEntry> {
protected:
    static void SetUpTestSuite()
    {
        if (backends.ready) {
            return;
        }
        GameHarness* harness = GameHarness::Get();
        if (harness == nullptr) {
            return;
        }
        // CSwerve exists once the applet is initialized; a few frames settle the start-up.
        harness->RunFrames(3);
        if (ckz::config.engine != ckz::EngineM3g || CSwerve::GetInstance() == nullptr) {
            fprintf(stderr, "M3G backend test: needs Engine=m3g in zombsniper.cfg\n");
            return;
        }
        ISwerve* original = nullptr;
        const int createError = ckz::arm::CreateSwerve(&original);
        fprintf(stderr, "M3G backend test: ARM swvNew returned %d\n", createError);
        if (createError != 0 || original == nullptr) {
            return;
        }
        // The porting layer asks CSwerve's runtime for its properties (malij297_GetEnvironment):
        // while the original runs, that must be the original.
        ISwerve* gameRuntime = CSwerve::GetInstance()->m_swerve;
        CSwerve::GetInstance()->m_swerve = original;
        const int initializeError = original->m_functions->Initialize(original);
        CSwerve::GetInstance()->m_swerve = gameRuntime;
        fprintf(stderr, "M3G backend test: ARM Initialize returned %d\n", initializeError);
        if (initializeError != 0) {
            return;
        }
        backends.native = MakeBackend("native", CSwerve::GetInstance()->m_swerve);
        fprintf(stderr, "M3G backend test: native helpers created\n");
        backends.original = MakeBackend("original", original);
        fprintf(stderr, "M3G backend test: original helpers created\n");
        backends.ready = true;
    }
};

/** Solo phase: every controller alone, as the game plays an animation. Returns the mismatches. */
int CompareSoloAnimations(const std::string& file, const std::string& label, IObject3D* originalRoot, IObject3D* nativeRoot)
{
    auto* originalObject = reinterpret_cast<IObject3DInterface*>(originalRoot);
    auto* nativeObject = reinterpret_cast<IObject3DInterface*>(nativeRoot);
    Controllers originalControllers;
    Controllers nativeControllers;
    CollectControllers(originalObject, originalControllers, 0);
    CollectControllers(nativeObject, nativeControllers, 0);
    int mismatches = 0;
    if (originalControllers.size() != nativeControllers.size()) {
        ADD_FAILURE() << file << ": " << originalControllers.size() << " original controllers, " << nativeControllers.size()
                      << " native controllers";
        mismatches++;
    }
    size_t soloCount = 0;
    for (auto& entry : originalControllers) {
        if (soloCount == SoloControllers) {
            break;
        }
        if (nativeControllers.count(entry.first) == 0) {
            continue;
        }
        soloCount++;
        const int start = SoloStartTime + static_cast<int>(soloCount) * 10000;
        SoloController(originalControllers, entry.first, start);
        SoloController(nativeControllers, entry.first, start);
        for (int offset : SoloTimes) {
            const int time = start + offset;
            int validity = 0;
            originalObject->animate(time, &validity);
            nativeObject->animate(time, &validity);
            backends.original.worldTime = time;
            backends.native.worldTime = time;
            Snapshot originalSnapshot;
            Snapshot nativeSnapshot;
            const std::string soloLabel = label + "@solo" + std::to_string(entry.first) + "+" + std::to_string(offset);
            SnapshotRoot(backends.original, originalRoot, soloLabel, originalSnapshot, false);
            SnapshotRoot(backends.native, nativeRoot, soloLabel, nativeSnapshot, false);
            mismatches += CompareSnapshots(file + " (solo " + std::to_string(entry.first) + " +" + std::to_string(offset) + ")",
                                           originalSnapshot, nativeSnapshot);
        }
    }
    ReleaseControllers(originalControllers);
    ReleaseControllers(nativeControllers);
    return mismatches;
}

TEST_P(M3gBackends, SameSceneGraph)
{
    ASSERT_TRUE(backends.ready) << "both Swerve backends are needed (Engine=m3g, swerve-arm.bin present)";
    const BigEntry& entry = GetParam();
    const std::string file = EntryName(entry);
    BigArchive archive;
    ASSERT_TRUE(archive.Open(std::filesystem::path(ckz::assetPath) / L"zombsniper.big"));
    const std::vector<uint8_t> data = archive.Read(entry);
    ASSERT_TRUE(IsM3gData(data));

    pickTies = 0;
    std::vector<IObject3D*> originalRoots;
    std::vector<IObject3D*> nativeRoots;
    const int originalError = LoadWith(backends.original.swerve, data, originalRoots);
    const int nativeError = LoadWith(backends.native.swerve, data, nativeRoots);
    ASSERT_EQ(originalError, 0) << file << ": the original Swerve does not load it";
    ASSERT_EQ(nativeError, 0) << file << ": the native runtime does not load it";
    ASSERT_EQ(originalRoots.size(), nativeRoots.size()) << file << ": root object count";

    int mismatches = 0;
    for (size_t rootIndex = 0; rootIndex < originalRoots.size(); rootIndex++) {
        const std::string label = "root" + std::to_string(rootIndex);
        // As loaded.
        backends.original.worldTime = 0;
        backends.native.worldTime = 0;
        Snapshot originalSnapshot;
        Snapshot nativeSnapshot;
        SnapshotRoot(backends.original, originalRoots[rootIndex], label, originalSnapshot);
        SnapshotRoot(backends.native, nativeRoots[rootIndex], label, nativeSnapshot);
        mismatches += CompareSnapshots(file + " (loaded)", originalSnapshot, nativeSnapshot);
        // After animate() at increasing world times, like the game's per-frame animation.
        for (int time : AnimationTimes) {
            auto* originalRoot = reinterpret_cast<IObject3DInterface*>(originalRoots[rootIndex]);
            auto* nativeRoot = reinterpret_cast<IObject3DInterface*>(nativeRoots[rootIndex]);
            int originalValidity = 0;
            int nativeValidity = 0;
            backends.original.worldTime = time;
            backends.native.worldTime = time;
            const int originalAnimateError = originalRoot->animate(time, &originalValidity);
            const int nativeAnimateError = nativeRoot->animate(time, &nativeValidity);
            EXPECT_EQ(originalAnimateError == 0, nativeAnimateError == 0) << file << ": animate(" << time << ") error";
            Snapshot originalAnimated;
            Snapshot nativeAnimated;
            const std::string animatedLabel = label + "@" + std::to_string(time);
            SnapshotRoot(backends.original, originalRoots[rootIndex], animatedLabel, originalAnimated);
            SnapshotRoot(backends.native, nativeRoots[rootIndex], animatedLabel, nativeAnimated);
            mismatches += CompareSnapshots(file + " (animate " + std::to_string(time) + ")", originalAnimated, nativeAnimated);
        }
    }
    ReleaseRoots(originalRoots);
    ReleaseRoots(nativeRoots);

    // Solo phase on freshly loaded scenes: a property whose controllers all have weight 0 keeps its
    // last value, so the scenes animated above would carry their differences into it.
    ASSERT_EQ(LoadWith(backends.original.swerve, data, originalRoots), 0);
    ASSERT_EQ(LoadWith(backends.native.swerve, data, nativeRoots), 0);
    for (size_t rootIndex = 0; rootIndex < originalRoots.size() && rootIndex < nativeRoots.size(); rootIndex++) {
        const std::string label = "root" + std::to_string(rootIndex);
        mismatches += CompareSoloAnimations(file, label, originalRoots[rootIndex], nativeRoots[rootIndex]);
    }
    ReleaseRoots(originalRoots);
    ReleaseRoots(nativeRoots);
    fprintf(stderr, "M3G backend test: %s %zu bytes, %d mismatching records, %d coplanar pick ties\n", file.c_str(),
            data.size(), mismatches, pickTies);
}

std::string ParamName(const testing::TestParamInfo<BigEntry>& info)
{
    return EntryName(info.param);
}

INSTANTIATE_TEST_SUITE_P(Zombsniper, M3gBackends, testing::ValuesIn(M3gEntries()), ParamName);

} // namespace
} // namespace ckz::test
