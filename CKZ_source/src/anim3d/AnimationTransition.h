// AnimationTransition.h - cross-fading animations on an AnimatableSwerveObject (created by b047x).
#ifndef ANIMATIONTRANSITION_H
#define ANIMATIONTRANSITION_H

#include <stddef.h>
#include "Vector_ss.h"
#include "AnimationInstance.h"

class AnimatableSwerveObject;
class AnimationImpl;

class AnimationTransition : public AnimationListener {
public:
    /** Fade states of a descriptor. */
    enum {
        STATE_FADE_IN = 0,
        STATE_PLAYING = 1,
        STATE_FADE_OUT = 2,
        STATE_FINISHED = 3
    };

    /** One animation started through this transition. */
    struct AnimationDescriptor {
        bool m_looped;                                  // 0x00
        float m_fadeSpeed;                              // 0x04 weight change per millisecond
        float m_weight;                                 // 0x08 0..1
        int m_state;                                    // 0x0c
        WeakIntrusivePtr<AnimationInstance> m_instance; // 0x10
    };

    AnimationTransition(AnimatableSwerveObject* object, AnimationListener* listener);
    virtual ~AnimationTransition();

    /** Delete every instance and forget the descriptors. */
    void shutDownAllAnims();
    void update(int elapsed);
    /** Fade the weights by `elapsed` milliseconds and drop the finished animations. */
    void updateWeights(int elapsed);
    /** Start (or restart) `animation`, fading it in over `fadeTime` milliseconds. */
    WeakIntrusivePtr<AnimationInstance> startAnimation(AnimationImpl* animation, float weight, bool looped,
                                                       int fadeTime, float speed, float startTime);
    /** Descriptor of the animation with Swerve id `id`, NULL when not started. */
    AnimationDescriptor* getAnimDesc(int id) const;
    AnimationDescriptor* getAnimDesc(WeakIntrusivePtr<AnimationInstance> instance) const;
    virtual void eventListener(const AnimationEventCK& event, WeakIntrusivePtr<AnimationInstance> instance);
    /** Fade out every animation. */
    void stopAllAnims(int fadeTime);

    Vector<AnimationDescriptor> m_descriptors;  // 0x04
    AnimatableSwerveObject* m_object;           // 0x14
    AnimationListener* m_listener;              // 0x18 receives the events (may be NULL)
};

#endif
