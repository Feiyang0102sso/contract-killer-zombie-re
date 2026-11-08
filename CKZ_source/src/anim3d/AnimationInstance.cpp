// AnimationInstance.cpp - time, weight and events of a playing animation (created by b047x).
#include "AnimationInstance.h"
#include "Animation.h"
#include "AnimatableObject.h"
#include "Fixed.h"

AnimationInstance::AnimationInstance(Animation* animation, AnimatableObject* object)
{
    m_animation = animation;
    m_time = 0;
    m_weight = 0;
    m_speed = 0x1000;
    m_listener = NULL;
    m_nextEventIndex = 0;
    m_stopped = false;
    m_looped = false;
    m_object = object;
}

AnimationInstance::~AnimationInstance()
{
}

void AnimationInstance::advanceTime(int elapsed)
{
    if (m_stopped) {
        return;
    }
    int duration = Fixed::fromFloat(m_animation->getDuration());
    if (m_speed == 0 || elapsed == 0) {
        return;
    }
    if (m_time >= duration) {
        return;
    }
    if (m_time <= 0 && m_listener != NULL) {
        AnimationEventCK event(AnimationEventCK::EVENT_START, NULL);
        m_listener->eventListener(event, WeakIntrusivePtr<AnimationInstance>(this));
    }
    int step = Fixed::mul(elapsed, m_speed);
    if (m_listener != NULL) {
        int eventsCount = m_animation->getEventsCount();
        while (!m_stopped && m_nextEventIndex < eventsCount && step != 0) {
            int eventIndex = m_nextEventIndex;
            int target = m_time + step;
            int eventTime = Fixed::fromFloat(m_animation->m_eventTimes.m_items[eventIndex]);
            if (target < eventTime) {
                break;
            }
            m_time = eventTime;
            m_nextEventIndex = eventIndex + 1;
            AnimationEventCK event(eventIndex, m_animation->m_eventNames.m_items[eventIndex]);
            m_listener->eventListener(event, WeakIntrusivePtr<AnimationInstance>(this));
            step = target - eventTime;
        }
    }
    m_time += step;
    if (m_time >= duration) {
        int remainder = 0;
        if (m_looped) {
            remainder = Fixed::div(m_time - duration, m_speed);
        }
        m_time = duration;
        if (m_listener != NULL) {
            AnimationEventCK event(AnimationEventCK::EVENT_END, NULL);
            m_listener->eventListener(event, WeakIntrusivePtr<AnimationInstance>(this));
        }
        if (m_looped) {
            m_time = 0;
            if (remainder != 0) {
                advanceTime(remainder);
            }
        }
    } else if (m_time < 0 && m_looped) {
        int time = m_time;
        int speed = m_speed;
        m_time = duration - 1;
        advanceTime(Fixed::div(time, speed));
    }
}

void AnimationInstance::setWeight(float weight)
{
    if (m_stopped) {
        return;
    }
    int oldWeight = m_weight;
    int newWeight = Fixed::fromFloat(weight);
    if (newWeight == oldWeight) {
        return;
    }
    int tracksCount = m_animation->getTracksCount();
    for (int track = 0; track < tracksCount; track++) {
        int channel = m_animation->getTrackTargetChannelIndex(track);
        m_object->m_channelWeights[channel] += newWeight - oldWeight;
    }
    m_weight = newWeight;
}

void AnimationInstance::setTime(float time)
{
    if (m_stopped) {
        return;
    }
    int newTime = Fixed::fromFloat(time);
    if (newTime == m_time) {
        return;
    }
    m_time = newTime;
    m_nextEventIndex = m_animation->getEventsCount();
    for (int index = 0; index < m_animation->getEventsCount(); index++) {
        if (m_animation->m_eventTimes.m_items[index] >= time) {
            m_nextEventIndex = index;
            return;
        }
    }
}

void AnimationInstance::stop()
{
    if (m_stopped) {
        return;
    }
    WeakIntrusivePtr<AnimationInstance> self(this);
    int index = m_object->m_instances.indexOf(self);
    if (index >= 0) {
        m_object->m_instances.removeElementAt(index);
    }
    m_object = NULL;
    m_animation = NULL;
    m_listener = NULL;
    m_stopped = true;
}

void AnimationInstance::setLooped(bool looped)
{
    m_looped = looped;
}
