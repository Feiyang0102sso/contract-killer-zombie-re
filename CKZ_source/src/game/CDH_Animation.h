// CDH_Animation.h - named animation controllers of one Swerve object (weapon models etc.).
#ifndef CDH_ANIMATION_H
#define CDH_ANIMATION_H

#include "tws.h"
#include "xString.h"
#include "Vector_ss.h"

/** Longest keyframe sequence (ms) driven by `controller` in `object` and its descendants. */
int getControllerAnimDuration(swerve::Object3D<CSwerveAssertingErrorHandler> object,
                              swerve::AnimationController<CSwerveAssertingErrorHandler> controller);

/**
 * A set of animation controllers found by name in one Swerve object. One controller at a time is
 * "current" (weight 1, the others 0); durations are kept to drive the controllers by relative position.
 */
class CDH_Animation {
public:
    typedef swerve::Object3D<CSwerveAssertingErrorHandler> Object3D;
    typedef swerve::AnimationController<CSwerveAssertingErrorHandler> AnimationController;
    typedef swerve::AnimationTrack<CSwerveAssertingErrorHandler> AnimationTrack;

    CDH_Animation(Object3D object);
    virtual ~CDH_Animation();

    /** First animation track of `object` (or a descendant) driven by `controller`; NULL handle when none. */
    static AnimationTrack getFirstControlledTrack(Object3D object, AnimationController controller);
    /** True when `object` contains an animation controller named `name`. */
    static bool HaveController(Object3D object, const char* name);

    /** Find the controller named `name` and append it (weight 0) with its duration. */
    void addController(const XString& name);
    /** Duration (ms) of controller `index` at speed 1. */
    int getControllerBaseDuration(int index);
    /** Play controller `index` in `duration` ms. */
    void setControllerDuration(int index, int duration);
    void setControllerSpeed(int index, float speed);
    /** Speed of controller `index` (1 when out of range). */
    float getControllerSpeed(int index);
    /** Position of controller `index` relative to its duration (0..1). */
    float getControllerPosition(int index);
    /** Move controller `index` to `position` (relative to its duration). */
    void setControllerPosition(int index, float position);
    /** Make `index` the only playing controller, from the start (or from the end backwards when `reverse`). */
    void setCurrentController(int index, bool reverse);
    /** Weight 0 for every controller. */
    void stopAllAnimation();
    /** Animate the object to the current world time. */
    void animate();
    /** Every controller to `position` (relative to its duration). */
    void setRelativePosition(float position);
    /** Every controller to `position` (relative to its duration), speed 0. */
    void setRelativePositionAndStop(float position);
    /** True when the current controller has played its whole duration (or there is none). */
    bool isAnimationComplete() const;

    Object3D m_object;                          // 0x04 animated object
    Vector<AnimationController> m_controllers;  // 0x08
    Vector<int> m_durations;                    // 0x18 base duration (ms) per controller
    int m_currentController;                    // 0x28 -1: none
    int m_startTime;                            // 0x2c world time when the current controller started (-1: none)
};

#endif
