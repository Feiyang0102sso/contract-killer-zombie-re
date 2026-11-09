// SwerveHelper.cpp - helpers around the Swerve (M3G) scene graph and the animation player
// SwerveAnimations (b051x).
#include "SwerveHelper.h"
#include <float.h>
#include "tws_loader.h"
#include "CSwerve.h"
#include "CStrWChar.h"
#include "CFileUtil.h"
#include "ICFileMgr.h"
#include "ICFile.h"
#include "CVector.h"
#include "WindowApp.h"

using namespace com::glu::platform::components;
using namespace com::glu::platform::core;

/** Object types of ISwerve::CreateObject (Swerve class ids). */
enum {
    SwerveObject_Background = 4,
    SwerveObject_CompositingMode = 6,
    SwerveObject_Group = 9,
    SwerveObject_Transform = 0x1a
};

/** Weights below this count as zero. */
static const float MIN_WEIGHT = 1e-05f;

typedef swerve::Object3D<CSwerveAssertingErrorHandler> Object3D;
typedef swerve::Node<CSwerveAssertingErrorHandler> Node;
typedef swerve::Group<CSwerveAssertingErrorHandler> Group;
typedef swerve::Mesh<CSwerveAssertingErrorHandler> Mesh;
typedef swerve::SkinnedMesh<CSwerveAssertingErrorHandler> SkinnedMesh;
typedef swerve::Appearance<CSwerveAssertingErrorHandler> Appearance;
typedef swerve::Texture2D<CSwerveAssertingErrorHandler> Texture2D;
typedef swerve::VertexBuffer<CSwerveAssertingErrorHandler> VertexBuffer;
typedef swerve::VertexArray<CSwerveAssertingErrorHandler> VertexArray;
typedef swerve::Transform<CSwerveAssertingErrorHandler> Transform;
typedef swerve::Transformable<CSwerveAssertingErrorHandler> Transformable;
typedef swerve::AnimationTrack<CSwerveAssertingErrorHandler> AnimationTrack;
typedef swerve::AnimationController<CSwerveAssertingErrorHandler> AnimationController;
typedef swerve::KeyframeSequence<CSwerveAssertingErrorHandler> KeyframeSequence;

void SwerveHelper::CopyAnimationTracks_r(Object3D& source, const Object3D& target, Vector<int>& controllerIds,
                                         Vector<AnimationController>& controllers, int userIdOffset)
{
    if (source.m_interface == NULL || target.m_interface == NULL) {
        return;
    }

    // tracks of this object: duplicated, controllers duplicated once per user id
    int trackCount = source.getAnimationTrackCount();
    for (int trackIndex = 0; trackIndex < trackCount; trackIndex++) {
        AnimationTrack track = source.getAnimationTrack(trackIndex);
        if (track.m_interface != NULL) {
            AnimationController controller = track.getController();
            if (controller.m_interface != NULL) {
                int controllerId = controller.getUserID();
                if (controllerId != 0) {
                    AnimationTrack trackCopy(track.duplicate());
                    // the target is const: the track is added through a temporary handle
                    Object3D(target).addAnimationTrack(trackCopy);
                    int index = controllerIds.indexOf(controllerId);
                    if (index == -1) {
                        controllerIds.addElement(controllerId);
                        AnimationController controllerCopy(controller.duplicate());
                        controllerCopy.setUserID(userIdOffset + controllerId);
                        trackCopy.setController(controllerCopy);
                        controllers.addElement(controllerCopy);
                    } else {
                        trackCopy.setController(controllers.elementAt(index));
                    }
                }
            }
        }
    }

    // children (matched by user id, else by position), mesh textures and skeletons
    Group sourceGroup(source);
    Group targetGroup(target);
    if (sourceGroup.m_interface == NULL || targetGroup.m_interface == NULL) {
        return;
    }
    int childCount = sourceGroup.getChildCount();
    for (int childIndex = 0; childIndex < childCount; childIndex++) {
        Node sourceChild = sourceGroup.getChild(childIndex);
        int childId = sourceChild.getUserID();
        Node targetChild;
        if (childId != 0) {
            targetChild = Node(FindChild(targetGroup, childId));
        }
        if (targetChild.m_interface == NULL) {
            targetChild = targetGroup.getChild(childIndex);
        }
        if (sourceChild.m_interface == NULL || targetChild.m_interface == NULL) {
            continue;
        }

        Group sourceChildGroup(sourceChild);
        Group targetChildGroup(targetChild);
        if (sourceChildGroup.m_interface != NULL && targetChildGroup.m_interface != NULL) {
            CopyAnimationTracks_r(sourceChildGroup, targetChildGroup, controllerIds, controllers, userIdOffset);
        }

        Mesh sourceMesh(sourceChild);
        Mesh targetMesh(targetChild);
        if (sourceMesh.m_interface == NULL || targetMesh.m_interface == NULL) {
            continue;
        }
        int submeshCount = sourceMesh.getSubmeshCount();
        for (int submesh = 0; submesh < submeshCount; submesh++) {
            Appearance sourceAppearance = sourceMesh.getAppearance(submesh);
            Appearance targetAppearance = targetMesh.getAppearance(submesh);
            if (sourceAppearance.m_interface != NULL && targetAppearance.m_interface != NULL) {
                Texture2D sourceTexture = sourceAppearance.getTexture(0);
                Texture2D targetTexture = targetAppearance.getTexture(0);
                if (sourceTexture.m_interface != NULL && targetTexture.m_interface != NULL) {
                    CopyAnimationTracks_r(sourceTexture, targetTexture, controllerIds, controllers, userIdOffset);
                }
            }
        }

        SkinnedMesh sourceSkinnedMesh(sourceMesh);
        SkinnedMesh targetSkinnedMesh(targetMesh);
        if (sourceSkinnedMesh.m_interface != NULL && targetSkinnedMesh.m_interface != NULL) {
            Group sourceSkeleton = sourceSkinnedMesh.getSkeleton();
            Group targetSkeleton = targetSkinnedMesh.getSkeleton();
            if (sourceSkeleton.m_interface != NULL && targetSkeleton.m_interface != NULL) {
                CopyAnimationTracks_r(sourceSkeleton, targetSkeleton, controllerIds, controllers, userIdOffset);
            }
        }
    }
}

Object3D SwerveHelper::FindChild(Group group, int userID)
{
    int childCount = group.getChildCount();
    for (int index = 0; index < childCount; index++) {
        Object3D child = group.getChild(index);
        if (child.getUserID() == userID) {
            return child;
        }
    }
    return Object3D();
}

void SwerveHelper::SeparateAnimationAppearances_r(Object3D object)
{
    if (object.m_interface == NULL) {
        return;
    }

    // every submesh gets its own copy of its appearance and of its first two textures
    Mesh mesh(object);
    if (mesh.m_interface != NULL) {
        int submeshCount = mesh.getSubmeshCount();
        for (int submesh = 0; submesh < submeshCount; submesh++) {
            Appearance appearance = mesh.getAppearance(submesh);
            if (appearance.m_interface == NULL) {
                continue;
            }
            appearance = Appearance(appearance.duplicate());
            int textureIndex = 0;
            Texture2D texture = appearance.getTexture(0);
            while (texture.m_interface != NULL) {
                appearance.setTexture(textureIndex, Texture2D(texture.duplicate()));
                if (textureIndex == 1) {
                    break;
                }
                textureIndex++;
                texture = appearance.getTexture(textureIndex);
            }
            mesh.setAppearance(submesh, appearance);
        }
    }

    unsigned int referenceCount = object.getReferences(0, NULL);
    if (referenceCount != 0) {
        Object3D* references = new Object3D[referenceCount];
        object.getReferences(referenceCount, references);
        for (int index = 0; index < (int)referenceCount; index++) {
            SeparateAnimationAppearances_r(references[index]);
        }
        delete[] references;
    }
}

void SwerveHelper::RemoveAnimationTracks_r(Object3D object)
{
    if (object.m_interface == NULL) {
        return;
    }

    int trackCount = object.getAnimationTrackCount();
    if (trackCount > 0) {
        Vector<AnimationTrack> tracks;
        for (int index = 0; index < trackCount; index++) {
            AnimationTrack track = object.getAnimationTrack(index);
            if (track.m_interface != NULL) {
                tracks.addElement(track);
            }
        }
        for (int index = 0; index < tracks.size(); index++) {
            object.removeAnimationTrack(tracks[index]);
        }
    }

    unsigned int referenceCount = object.getReferences(0, NULL);
    if (referenceCount != 0) {
        Object3D* references = new Object3D[referenceCount];
        object.getReferences(referenceCount, references);
        for (int index = 0; index < (int)referenceCount; index++) {
            RemoveAnimationTracks_r(references[index]);
        }
        delete[] references;
    }
}

void SwerveHelper::CopyAnimationsTracks(Group source, const Group& target, int userIdOffset)
{
    SeparateAnimationAppearances_r(target);
    RemoveAnimationTracks_r(target);
    Vector<int> controllerIds;
    Vector<AnimationController> controllers;
    CopyAnimationTracks_r(source, target, controllerIds, controllers, userIdOffset);
}

Group SwerveHelper::FullObjectDuplicate(Group group, int userIdOffset)
{
    Group copy(group.duplicate());
    CopyAnimationsTracks(group, copy, userIdOffset);
    return copy;
}

int SwerveHelper::Id(const char* name)
{
    return CSwerve::GenerateUserID(name);
}

Object3D SwerveHelper::Find(Object3D root, const char* name)
{
    return root.find(Id(name));
}

void SwerveHelper::ResetCompositeTransform(Transformable& transformable)
{
    float translation[3];
    translation[0] = 0.0f;
    translation[1] = 0.0f;
    translation[2] = 0.0f;
    transformable.setTranslation(translation);
    float scale[3];
    scale[0] = 1.0f;
    scale[1] = 1.0f;
    scale[2] = 1.0f;
    transformable.setScale(scale);
    float orientation[4];
    orientation[0] = 0.0f;
    orientation[1] = 0.0f;
    orientation[2] = 0.0f;
    orientation[3] = 1.0f;
    transformable.setOrientation(orientation);
    transformable.setTransform(Transform());
}

Object3D SwerveHelper::LoadObject3D(const char* name)
{
    // IObject3D (SwerveApi.h) is the C view of the same runtime objects as IQueryInterface
    return Object3D((IQueryInterface*)CSwerve::GetInstance()->LoadM3GResource(name));
}

Object3D SwerveHelper::LoadObject3D(const wchar_t* fileName, int index)
{
    Object3D object;
    CStrWChar path;
    CFileUtil::GetApplicationPathForFile(path, fileName);
    ICFile* file = ICFileMgr::GetInstance()->Open(path.m_string, ICFileMgr::AccessRead);
    if (file != NULL) {
        file->Seek(0, ICFile::SeekEnd);
        unsigned int size = file->Tell();
        file->Seek(0, ICFile::SeekSet);
        unsigned char* data = (unsigned char*)malloc(size);
        if (data != NULL) {
            if (file->Read(data, size) == size) {
                CVector<Object3D> objects;
                swerve::Loader<CSwerveAssertingErrorHandler>::load(size, data, 0, objects);
                if (index >= 0 && index < objects.Size()) {
                    object = objects[index];
                }
            }
            free(data);
        }
        ICFileMgr::GetInstance()->Close(file);
    }
    return object;
}

Transform SwerveHelper::NewTransform()
{
    ISwerve* runtime = CSwerve::GetInstance()->m_swerve;
    Transform transform;
    runtime->m_functions->CreateObject(SwerveObject_Transform, (void**)&transform.m_interface);
    return transform;
}

/** New runtime object of class `type` (one reference, NULL on failure). */
static IQueryInterface* CreateSwerveObject(ISwerve* runtime, int type)
{
    void* object;
    runtime->m_functions->CreateObject(type, &object);
    return (IQueryInterface*)object;
}

swerve::Background<CSwerveAssertingErrorHandler> SwerveHelper::NewBackground()
{
    IQueryInterface* background = CreateSwerveObject(CSwerve::GetInstance()->m_swerve, SwerveObject_Background);
    return swerve::Background<CSwerveAssertingErrorHandler>(background);
}

Group SwerveHelper::NewGroup()
{
    return Group(CreateSwerveObject(CSwerve::GetInstance()->m_swerve, SwerveObject_Group));
}

swerve::CompositingMode<CSwerveAssertingErrorHandler> SwerveHelper::NewCompositingMode()
{
    IQueryInterface* compositingMode = CreateSwerveObject(CSwerve::GetInstance()->m_swerve,
                                                          SwerveObject_CompositingMode);
    return swerve::CompositingMode<CSwerveAssertingErrorHandler>(compositingMode);
}

Mat4x4& SwerveHelper::SetMatrix(Mat4x4& matrix, const Transform& transform)
{
    transform.get(matrix.ToFloatPtr());
    return matrix;
}

vec3 SwerveHelper::GetTranslation(const Transform& transform)
{
    Mat4x4 matrix(1.0f);
    transform.get(matrix.ToFloatPtr());
    return matrix.Translation();
}

SwerveAnimations::Animation::~Animation()
{
}

bool SwerveAnimations::Animation::IsPlaying() const
{
    if (m_stopTime != 0) {
        return false;
    }
    return m_controller.getWeight() != 0.0f;
}

void SwerveAnimations::Animation::Start(float delaySec, float speed, float fadeSec, float startWeight,
                                        float endWeight)
{
    int delayMS = (int)(delaySec * 1000.0f);
    int fadeMS = (int)(fadeSec * 1000.0f);
    int startTime = m_owner->TimeMS() + delayMS;
    StartAtTimeMS(startTime, speed, startTime + fadeMS, startWeight, endWeight);
}

void SwerveAnimations::Animation::StartAtTimeMS(int startTime, float speed, int endTime, float startWeight,
                                                float endWeight)
{
    m_paused = false;
    m_stopTime = 0;
    m_fadeStartTime = startTime;
    m_fadeEndTime = endTime;
    float weight = endWeight;
    if (endTime > startTime) {
        // ?: gives the original vmax
        weight = MIN_WEIGHT > startWeight ? MIN_WEIGHT : startWeight;
    }
    m_fadeStartWeight = weight;
    m_fadeEndWeight = endWeight;
    m_weight = weight;
    m_controller.setWeight(weight);

    // backwards from the end, forwards from the start
    float position;
    if (speed >= 0.0f) {
        position = 0.0f;
    } else {
        position = (float)m_durationMS;
    }
    m_controller.setPosition(position, startTime);
    m_controller.setSpeed(speed, startTime);

    if (m_channelId < 0) {
        return;
    }
    // the other channels of the group stop
    for (Animation* other = m_owner->m_animations; other != NULL; other = other->m_next) {
        if (other != this && other->m_groupId == m_groupId && other->m_channelId >= 0 &&
            other->m_channelId != m_channelId && other->IsPlaying()) {
            other->Stop();
        }
    }
    if (m_blendable) {
        m_owner->RegisterBlending(this);
    }
}

void SwerveAnimations::RegisterBlending(Animation* animation)
{
    Blend* blend = m_blends;
    while (blend != NULL) {
        if (blend->m_groupId == animation->m_groupId && blend->m_channelId == animation->m_channelId) {
            break;
        }
        blend = blend->m_next;
    }
    if (blend == NULL) {
        blend = new Blend();
        blend->m_groupId = animation->m_groupId;
        blend->m_channelId = animation->m_channelId;
        blend->m_next = m_blends;
        m_blends = blend;
    }
    int index = blend->m_animations.indexOf(animation);
    if (index >= 0) {
        blend->m_animations.removeElementAt(index);
    }
    blend->m_animations.addElement(animation);
    blend->m_started = false;
}

void SwerveAnimations::Animation::SetWeight(float weight)
{
    m_weight = weight;
    m_controller.setWeight(weight);
    if (weight < MIN_WEIGHT) {
        Stop();
    }
}

void SwerveAnimations::Animation::SetPositionSec(float seconds)
{
    m_controller.setPosition(seconds * 1000.0f, m_owner->TimeMS());
}

void SwerveAnimations::Animation::Pause(int time)
{
    if (m_paused) {
        return;
    }
    if (!IsPlaying()) {
        return;
    }
    m_paused = true;
    m_pausedSpeed = m_controller.getSpeed();
    m_controller.setSpeed(0.0f, time);
}

void SwerveAnimations::Animation::Pause()
{
    Pause(m_owner->TimeMS());
}

void SwerveAnimations::Animation::Update(int time)
{
    if (m_stopTime != 0) {
        Stop();
        return;
    }
    if (!IsPlaying()) {
        return;
    }

    // a non looping sequence that ran out stops at the next update
    if (!m_looping) {
        float position = m_controller.getPosition(time);
        float speed = m_controller.getSpeed();
        float positionSec = position * 0.001f;
        if (speed < 0.0f) {
            if (positionSec < 0.0f) {
                m_stopTime = time;
                return;
            }
        } else if (speed > 0.0f) {
            if (positionSec >= m_durationSec) {
                m_stopTime = time;
                return;
            }
        }
    }

    // weight fade
    if (m_fadeStartTime < m_fadeEndTime) {
        float weight;
        if (m_fadeStartTime >= time) {
            weight = m_fadeStartWeight;
        } else if (m_fadeEndTime < time) {
            weight = m_fadeEndWeight;
        } else {
            float progress = (float)(time - m_fadeStartTime) / (float)(m_fadeEndTime - m_fadeStartTime);
            weight = (1.0f - progress) * m_fadeStartWeight + m_fadeEndWeight * progress;
        }
        SetWeight(weight);
    }
}

SwerveAnimations::SwerveAnimations()
{
    m_animations = NULL;
    m_animationCount = 0;
    m_blends = NULL;
}

SwerveAnimations::~SwerveAnimations()
{
    Reset();
}

void SwerveAnimations::Reset()
{
    while (m_animations != NULL) {
        Animation* animation = m_animations;
        m_animations = animation->m_next;
        animation->m_next = NULL;
        delete animation;
        m_animationCount--;
    }
    while (m_blends != NULL) {
        Blend* blend = m_blends;
        m_blends = blend->m_next;
        blend->m_next = NULL;
        delete blend;
    }
    m_root.Clear();
}

SwerveAnimations::Animation* SwerveAnimations::RegisterAnimationTrack(const AnimationTrack& track)
{
    AnimationController controller = track.getController();
    int controllerId = controller.getUserID();
    if (controllerId == 0) {
        controller.setWeight(0.0f);
        return NULL;
    }

    Animation* animation = Anim(controllerId);
    if (animation == NULL) {
        animation = new Animation();
        animation->m_owner = this;
        animation->m_controller = controller;
        animation->m_next = m_animations;
        m_animations = animation;
        m_animationCount++;
        animation->Stop();
    }

    KeyframeSequence sequence = track.getKeyframeSequence();
    animation->m_looping = animation->m_looping || sequence.getRepeatMode() == KeyframeSequence::LOOP;
    int durationMS = animation->m_durationMS;
    int sequenceDuration = sequence.getDuration();
    if (durationMS > sequenceDuration) {
        sequenceDuration = durationMS;
    }
    animation->m_durationMS = sequenceDuration;
    animation->m_durationSec = (float)animation->m_durationMS * 0.001f;
    return animation;
}

SwerveAnimations::Animation* SwerveAnimations::Anim(int userID) const
{
    for (Animation* animation = m_animations; animation != NULL; animation = animation->m_next) {
        if (animation->m_controller.getUserID() == userID) {
            return animation;
        }
    }
    return NULL;
}

int SwerveAnimations::LoadFrom(Object3D root)
{
    Reset();
    m_root = root;
    LoadFrom_r(root);
    return m_animationCount;
}

void SwerveAnimations::LoadFrom_r(Object3D& object)
{
    AnimationTrack track(object);
    if (track.m_interface != NULL) {
        RegisterAnimationTrack(track);
    } else if (object.m_interface != NULL) {
        unsigned int referenceCount = object.getReferences(0, NULL);
        if (referenceCount != 0) {
            Object3D* references = new Object3D[referenceCount];
            object.getReferences(referenceCount, references);
            for (int index = 0; index < (int)referenceCount; index++) {
                LoadFrom_r(references[index]);
            }
            delete[] references;
        }
    }
}

int SwerveAnimations::TimeMS()
{
    return WindowApp::m_instance->m_updateTime;
}

void SwerveAnimations::Update(int time)
{
    // a newly started blended animation fades the others of its channel out at its fade rate
    for (Blend* blend = m_blends; blend != NULL; blend = blend->m_next) {
        if (blend->m_started) {
            continue;
        }
        Animation* newest = blend->m_animations.elementAt(blend->m_animations.size() - 1);
        if (newest->m_fadeStartTime > time) {
            continue;
        }
        blend->m_started = true;
        int fadeMS = newest->m_fadeEndTime - newest->m_fadeStartTime;
        float fadeRate = 0.0f;
        if (fadeMS > 0) {
            fadeRate = (newest->m_fadeEndWeight - newest->m_fadeStartWeight) / ((float)fadeMS * 0.001f);
        }
        for (int index = 0; index < blend->m_animations.size(); index++) {
            Animation* other = blend->m_animations.elementAt(index);
            if (other == newest) {
                continue;
            }
            if (other->IsPlaying() && other->m_blendable) {
                int startTime = newest->m_fadeStartTime;
                other->m_fadeStartTime = startTime;
                int fadeOutMS = 0;
                if (fadeMS > 0) {
                    fadeOutMS = (int)(other->m_weight * 1000.0f / fadeRate);
                }
                other->m_fadeEndTime = startTime + fadeOutMS;
                other->m_fadeStartWeight = other->m_weight;
                other->m_fadeEndWeight = 0.0f;
            }
        }
    }

    for (Animation* animation = m_animations; animation != NULL; animation = animation->m_next) {
        animation->Update(time);
    }

    // drop finished blends; the older animations share what the current one leaves of the weight
    Blend* previous = NULL;
    Blend* blend = m_blends;
    while (blend != NULL) {
        Blend* next = blend->m_next;
        for (int index = blend->m_animations.size() - 1; index >= 0; index--) {
            Animation* animation = blend->m_animations.elementAt(index);
            if (!animation->IsPlaying() || !animation->m_blendable) {
                blend->m_animations.removeElementAt(index);
            }
        }
        if (blend->m_animations.size() <= 0) {
            if (previous == NULL) {
                m_blends = next;
            } else {
                previous->m_next = next;
            }
            blend->m_next = NULL;
            delete blend;
            blend = next;
            continue;
        }

        int current = blend->m_animations.size() - 1;
        if (!blend->m_started) {
            current--;
        }
        for (; current >= 0; current--) {
            Animation* currentAnimation = blend->m_animations.elementAt(current);
            if (currentAnimation->m_fadeStartTime <= time) {
                float currentWeight = currentAnimation->m_weight;
                float olderWeight = 0.0f;
                for (int index = 0; index < current; index++) {
                    olderWeight += blend->m_animations.elementAt(index)->m_weight;
                }
                float scale = 0.0f;
                if (olderWeight > 0.0f) {
                    scale = (1.0f - currentWeight) / olderWeight;
                }
                for (int index = current - 1; index >= 0; index--) {
                    Animation* older = blend->m_animations.elementAt(index);
                    float weight = older->m_weight * scale;
                    if (weight < MIN_WEIGHT) {
                        older->Stop();
                    } else {
                        older->m_controller.setWeight(weight);
                    }
                }
                break;
            }
        }
        previous = blend;
        blend = next;
    }
}

void SwerveAnimations::Update()
{
    Update(TimeMS());
}

bool SwerveHelper::CalculateBounds(Bounds3& bounds, const Node& node, const Transform& transform)
{
    return CalculateBounds(bounds, node, node, transform);
}

bool SwerveHelper::CalculateBounds(Bounds3& bounds, const Node& node, const Node& reference,
                                   const Transform& transform)
{
    NodeBoundsCalculator calculator(bounds, reference, transform);
    return calculator.AddBounds_r(node);
}

bool SwerveHelper::NodeBoundsCalculator::AddBounds_r(const Node& node)
{
    bool added = false;
    Mesh mesh(node);
    if (mesh.m_interface != NULL) {
        added = AddMeshBounds(mesh);
    } else {
        Group group(node);
        if (group.m_interface != NULL) {
            int childCount = group.getChildCount();
            unsigned char childAdded = 0;
            for (int index = 0; index < childCount; index++) {
                Node child = group.getChild(index);
                childAdded |= AddBounds_r(child);
            }
            added = childAdded != 0;
        }
    }
    return added;
}

SwerveHelper::NodeBoundsCalculator::NodeBoundsCalculator(Bounds3& bounds, const Node& reference,
                                                         const Transform& transform)
    : m_bounds(bounds)
{
    m_reference = reference;
    m_transform = transform;
    m_bounds.m_corners[Bounds3::MIN_CORNER] = vec3(FLT_MAX, FLT_MAX, FLT_MAX);
    m_bounds.m_corners[Bounds3::MAX_CORNER] = vec3(-m_bounds.m_corners[Bounds3::MIN_CORNER].m_x,
                                                -m_bounds.m_corners[Bounds3::MIN_CORNER].m_y,
                                                -m_bounds.m_corners[Bounds3::MIN_CORNER].m_z);
}

bool SwerveHelper::NodeBoundsCalculator::AddMeshBounds(Mesh& mesh)
{
    Mat4x4 matrix(1.0f);
    mesh.getTransformTo(m_reference, m_transform);
    m_transform.get(matrix.ToFloatPtr());

    // positions: short components, scaled and biased
    VertexBuffer vertexBuffer = mesh.getVertexBuffer();
    float scaleBias[4];
    VertexArray positions = vertexBuffer.getPositions(scaleBias);
    int vertexCount = positions.getVertexCount();
    short* coordinates = new short[vertexCount * 3];
    positions.get(0, vertexCount, vertexCount * 3, coordinates);
    for (int index = 0; index < vertexCount; index++) {
        short* vertex = &coordinates[index * 3];
        vec3 point((float)vertex[0] * scaleBias[0] + scaleBias[1], (float)vertex[1] * scaleBias[0] + scaleBias[2],
                   (float)vertex[2] * scaleBias[0] + scaleBias[3]);
        point = matrix.TransformFast(point);
        m_bounds.Add(point);
    }
    delete[] coordinates;
    return vertexCount > 0;
}

void SwerveHelper::TransformPoint(vec3* point, vec3* result, Transform& transform)
{
    float vector[4];
    vector[0] = point->m_x;
    vector[1] = point->m_y;
    vector[2] = point->m_z;
    vector[3] = 1.0f;
    transform.transform(4, vector);
    result->m_x = vector[0];
    result->m_y = vector[1];
    result->m_z = vector[2];
}

void SwerveHelper::TransformVector(vec3* vector, vec3* result, Transform& transform)
{
    float components[4];
    components[0] = vector->m_x;
    components[1] = vector->m_y;
    components[2] = vector->m_z;
    components[3] = 0.0f;
    transform.transform(4, components);
    result->m_x = components[0];
    result->m_y = components[1];
    result->m_z = components[2];
}

Group SwerveHelper::ForEachGroupRecursive(Node node, GroupCallback callback, void* context)
{
    Group group(node);
    if (group.m_interface != NULL) {
        if (callback(group, context)) {
            return group;
        }
        int childCount = group.getChildCount();
        for (int index = 0; index < childCount; index++) {
            Node child = group.getChild(index);
            Group found = ForEachGroupRecursive(child, callback, context);
            if (found.m_interface != NULL) {
                return found;
            }
        }
    }

    // skeletons are not children of their skinned mesh
    SkinnedMesh skinnedMesh(node);
    if (skinnedMesh.m_interface != NULL) {
        Group skeleton = skinnedMesh.getSkeleton();
        Group found = ForEachGroupRecursive(skeleton, callback, context);
        if (found.m_interface != NULL) {
            return found;
        }
    }
    return Group();
}

Group SwerveHelper::ForEachGroup(Node node, GroupCallback callback, void* context)
{
    return ForEachGroupRecursive(node, callback, context);
}
