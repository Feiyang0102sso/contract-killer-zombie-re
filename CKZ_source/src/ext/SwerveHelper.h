// SwerveHelper.h - helpers around the Swerve (M3G) scene graph: loading, object creation, copying of
// animation tracks between duplicated hierarchies, bounds; SwerveAnimations plays, fades and blends
// the animation controllers of a loaded scene.
// (owner: src/ext/SwerveHelper.cpp, b051x; first drafted by b057x)
#ifndef SWERVEHELPER_H
#define SWERVEHELPER_H

#include <wchar.h>
#include "tws.h"
#include "MathLib.h"
#include "Vector_ss.h"

/** Axis aligned box (no out-of-line code in the original; layout from NodeBoundsCalculator). */
class Bounds3 {
public:
    /** Grow the box so that it contains `point`. */
    void Add(const vec3& point)
    {
        float x = point.m_x;
        float y = point.m_y;
        float z = point.m_z;
        if (m_min.m_x < x) {
            x = m_min.m_x;
        }
        if (m_min.m_y < y) {
            y = m_min.m_y;
        }
        if (m_min.m_z < z) {
            z = m_min.m_z;
        }
        m_min = vec3(x, y, z);
        x = point.m_x;
        y = point.m_y;
        z = point.m_z;
        if (m_max.m_x > x) {
            x = m_max.m_x;
        }
        if (m_max.m_y > y) {
            y = m_max.m_y;
        }
        if (m_max.m_z > z) {
            z = m_max.m_z;
        }
        m_max = vec3(x, y, z);
    }

    vec3 m_min;     // 0x00
    vec3 m_max;     // 0x0c
};

/** Static helpers for Swerve scene graphs. */
class SwerveHelper {
public:
    typedef swerve::Object3D<CSwerveAssertingErrorHandler> Object3D;
    typedef swerve::Node<CSwerveAssertingErrorHandler> Node;
    typedef swerve::Group<CSwerveAssertingErrorHandler> Group;
    typedef swerve::Mesh<CSwerveAssertingErrorHandler> Mesh;
    typedef swerve::Transform<CSwerveAssertingErrorHandler> Transform;
    typedef swerve::Transformable<CSwerveAssertingErrorHandler> Transformable;
    typedef swerve::AnimationController<CSwerveAssertingErrorHandler> AnimationController;

    /** Visitor of ForEachGroup; returning true stops the walk at that group. */
    typedef bool (*GroupCallback)(Group& group, void* context);

    /**
     * Give `target` (a duplicate of `source`) copies of the animation tracks of `source`, recursively.
     * Controllers are shared through `controllerIds` / `controllers`; the copies get the user id
     * `userIdOffset` + original id.
     */
    static void CopyAnimationTracks_r(Object3D& source, const Object3D& target, Vector<int>& controllerIds,
                                      Vector<AnimationController>& controllers, int userIdOffset);
    /** The child of `group` with user id `userID` (NULL handle when there is none). */
    static Object3D FindChild(Group group, int userID);
    /** Give every mesh below `object` its own appearances and textures (they may be animated). */
    static void SeparateAnimationAppearances_r(Object3D object);
    /** Remove all animation tracks below `object`. */
    static void RemoveAnimationTracks_r(Object3D object);
    /** Replace the animation tracks of `target` (a duplicate of `source`) by copies of those of `source`. */
    static void CopyAnimationsTracks(Group source, const Group& target, int userIdOffset);
    /** Duplicate `group` with independent animations (controller user ids + `userIdOffset`). */
    static Group FullObjectDuplicate(Group group, int userIdOffset);
    /** M3G user id of the name `name`. */
    static int Id(const char* name);
    /** The descendant of `root` called `name`. */
    static Object3D Find(Object3D root, const char* name);
    /** Identity translation, scale, orientation and transform. */
    static void ResetCompositeTransform(Transformable& transformable);
    /** Load the M3G resource `name`. */
    static Object3D LoadObject3D(const char* name);
    /** Load the M3G file `fileName` (application directory) and return its root object `index`. */
    static Object3D LoadObject3D(const wchar_t* fileName, int index);
    static Transform NewTransform();
    static swerve::Background<CSwerveAssertingErrorHandler> NewBackground();
    static Group NewGroup();
    static swerve::CompositingMode<CSwerveAssertingErrorHandler> NewCompositingMode();
    /** Copy `transform` into `matrix`. */
    static Mat4x4& SetMatrix(Mat4x4& matrix, const Transform& transform);
    static vec3 GetTranslation(const Transform& transform);
    /** Bounds of the meshes below `node` in the space of `node` transformed by `transform`; false when there is no vertex. */
    static bool CalculateBounds(Bounds3& bounds, const Node& node, const Transform& transform);
    /** Bounds of the meshes below `node` in the space of `reference` transformed by `transform`. */
    static bool CalculateBounds(Bounds3& bounds, const Node& node, const Node& reference, const Transform& transform);
    /** `result` = `transform` * (point, 1). */
    static void TransformPoint(vec3* point, vec3* result, Transform& transform);
    /** `result` = `transform` * (vector, 0). */
    static void TransformVector(vec3* vector, vec3* result, Transform& transform);
    /** Depth first walk over the groups below `node` (skeletons included); the group that stopped it. */
    static Group ForEachGroupRecursive(Node node, GroupCallback callback, void* context);
    static Group ForEachGroup(Node node, GroupCallback callback, void* context);

    /** Accumulates the bounds of the meshes below a node. */
    class NodeBoundsCalculator {
    public:
        /** Reset `bounds`; the vertices are mapped into the space of `reference`, then by `transform`. */
        NodeBoundsCalculator(Bounds3& bounds, const Node& reference, const Transform& transform);

        bool AddBounds_r(const Node& node);
        bool AddMeshBounds(Mesh& mesh);

        Node m_reference;       // 0x00
        Transform m_transform;  // 0x04
        Bounds3& m_bounds;      // 0x08
    };
};

/** Plays the animation controllers of a scene: start / pause / fade per controller, cross fading per channel. */
class SwerveAnimations {
public:
    typedef swerve::AnimationController<CSwerveAssertingErrorHandler> AnimationController;
    typedef swerve::AnimationTrack<CSwerveAssertingErrorHandler> AnimationTrack;
    typedef swerve::Object3D<CSwerveAssertingErrorHandler> Object3D;

    /** One animation controller of the scene (all tracks with the same controller user id). */
    class Animation {
    public:
        Animation()
        {
            m_pausedSpeed = 0.0f;
            m_owner = NULL;
            m_durationMS = 0;
            m_durationSec = 0.0f;
            m_looping = false;
            m_blendable = false;
            m_paused = false;
            m_groupId = -1;
            m_channelId = -1;
            m_fadeStartTime = 0;
            m_fadeEndTime = 0;
            m_weight = 0.0f;
            m_fadeStartWeight = 0.0f;
            m_fadeEndWeight = 1.0f;
            m_next = NULL;
            m_stopTime = 0;
        }

        virtual ~Animation();

        /** Started, not stopped and with a non zero weight. */
        bool IsPlaying() const;
        /**
         * Start after `delaySec` seconds at `speed`, fading the weight from `startWeight` to `endWeight`
         * in `fadeSec` seconds.
         */
        void Start(float delaySec, float speed, float fadeSec, float startWeight, float endWeight);
        /** Start at world time `startTime` (ms), fading the weight until `endTime`. */
        void StartAtTimeMS(int startTime, float speed, int endTime, float startWeight, float endWeight);
        /** Set the weight; an (almost) zero weight stops the animation. */
        void SetWeight(float weight);
        void SetPositionSec(float seconds);
        void Pause(int time);
        void Pause();
        /** Stop at the end of a non looping sequence and apply the weight fade. */
        void Update(int time);

        /** Stop playing (weight 0). */
        void Stop()
        {
            m_paused = false;
            m_stopTime = 0;
            m_fadeEndTime = 0;
            m_fadeStartTime = 0;
            m_weight = 0.0f;
            m_fadeEndWeight = 0.0f;
            m_fadeStartWeight = 0.0f;
            m_controller.setWeight(0.0f);
        }

        SwerveAnimations* m_owner;          // 0x04
        Object3D m_object;                  // 0x08 (never set in SwerveHelper.cpp)
        AnimationController m_controller;   // 0x0c
        int m_durationMS;                   // 0x10 longest keyframe sequence
        float m_durationSec;                // 0x14 m_durationMS / 1000
        bool m_looping;                     // 0x18 one of the sequences loops
        bool m_blendable;                   // 0x19 cross fades with the other animations of its channel
        bool m_paused;                      // 0x1a
        float m_pausedSpeed;                // 0x1c speed before Pause()
        int m_fadeStartTime;                // 0x20 world time (ms)
        int m_fadeEndTime;                  // 0x24
        float m_fadeStartWeight;            // 0x28
        float m_fadeEndWeight;              // 0x2c
        float m_weight;                     // 0x30
        int m_groupId;                      // 0x34 animations of one group ...
        int m_channelId;                    // 0x38 ... on different channels stop each other (-1: none)
        int m_stopTime;                     // 0x3c stop at the next update (0: playing)
        Animation* m_next;                  // 0x40
    };

    /** Cross fade state of one (group, channel): the newest animation fades the others out. */
    struct Blend {
        int m_groupId;                      // 0x00
        int m_channelId;                    // 0x04
        Vector<Animation*> m_animations;    // 0x08 oldest first
        bool m_started;                     // 0x18 the newest animation has started fading in
        Blend* m_next;                      // 0x1c
    };

    SwerveAnimations();
    virtual ~SwerveAnimations();
    /** Current world time (ms). */
    virtual int TimeMS();

    /** Forget all animations and the scene. */
    void Reset();
    /** The animation of the controller of `track` (created on first use); NULL when it has no user id. */
    Animation* RegisterAnimationTrack(const AnimationTrack& track);
    /** The animation whose controller has user id `userID`. */
    Animation* Anim(int userID) const;
    /** Collect the animations of `root`; returns their count. */
    int LoadFrom(Object3D root);
    void LoadFrom_r(Object3D& object);
    /** Make `animation` the newest of its (group, channel) cross fade. */
    void RegisterBlending(Animation* animation);
    void Update(int time);
    void Update();

    Object3D m_root;                // 0x04
    Animation* m_animations;        // 0x08
    int m_animationCount;           // 0x0c
    Blend* m_blends;                // 0x10
};

#endif
