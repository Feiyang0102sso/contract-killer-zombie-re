// AnimationInstance.h - one playing Animation on an AnimatableObject (created by b047x).
#ifndef ANIMATIONINSTANCE_H
#define ANIMATIONINSTANCE_H

#include <stddef.h>
#include "WeakPtr.h"
#include "Fixed.h"

class Animation;
class AnimatableObject;
class AnimationInstance;

/** Event passed to AnimationListener::eventListener. */
struct AnimationEventCK {
    /** Ids of the two implicit events; other ids are indices of the animation's events. */
    enum {
        EVENT_END = 0x7ffffffe,     // the animation reached its end
        EVENT_START = 0x7fffffff    // the animation starts
    };

    AnimationEventCK(int id, const char* name)
    {
        m_id = id;
        m_name = name;
    }

    int m_id;               // 0x00
    const char* m_name;     // 0x04 event name (NULL for start / end)
};

/** Receives the events of animation instances. */
class AnimationListener {
public:
    virtual ~AnimationListener()
    {
    }

    virtual void eventListener(const AnimationEventCK& event, WeakIntrusivePtr<AnimationInstance> instance) = 0;
};

class AnimationInstance : public Referenced<AnimationInstance> {
public:
    AnimationInstance(Animation* animation, AnimatableObject* object);
    virtual ~AnimationInstance();

    /** Advance by `elapsed` (20.12 ms), firing events and looping. */
    void advanceTime(int elapsed);
    /** Blend weight (0..1); the owner's channel weight totals follow the change. */
    void setWeight(float weight);
    /** Jump to `time` (ms); the next event is the first one not before it. */
    void setTime(float time);
    /** Remove from the owner and stop for good. */
    void stop();
    void setLooped(bool looped);

    /** Playback speed factor (1.0: normal). */
    void setSpeed(float speed)
    {
        m_speed = Fixed::fromFloat(speed);
    }

    void setListener(AnimationListener* listener)
    {
        if (!m_stopped) {
            m_listener = listener;
        }
    }

    Animation* m_animation;             // 0x0c
    AnimatableObject* m_object;         // 0x10
    int m_time;                         // 0x14 20.12 ms
    int m_weight;                       // 0x18 20.12
    int m_speed;                        // 0x1c 20.12 (1.0 by default)
    AnimationListener* m_listener;      // 0x20
    int m_nextEventIndex;               // 0x24
    bool m_stopped;                     // 0x28
    bool m_looped;                      // 0x29
};

#endif
