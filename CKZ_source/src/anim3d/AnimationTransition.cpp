// AnimationTransition.cpp - starting, cross-fading and stopping animations (created by b047x).
#include "AnimationTransition.h"
#include "AnimatableSwerveObject.h"
#include "AnimationImpl.h"
#include "Animation.h"
#include "Fixed.h"
#include "DGHelper.h"

AnimationTransition::AnimationTransition(AnimatableSwerveObject* object, AnimationListener* listener)
{
    m_object = object;
    m_listener = listener;
    m_descriptors.ensureCapacity(5);
}

AnimationTransition::~AnimationTransition()
{
    shutDownAllAnims();
}

void AnimationTransition::shutDownAllAnims()
{
    for (int index = 0; index < m_descriptors.m_size; index++) {
        WeakIntrusivePtr<AnimationInstance> instance = m_descriptors.m_items[index].m_instance;
        if (instance.get() != NULL) {
            delete instance.get();
            instance = WeakIntrusivePtr<AnimationInstance>();
        }
    }
    m_descriptors.removeAllElements();
}

void AnimationTransition::update(int elapsed)
{
    updateWeights(elapsed);
}

void AnimationTransition::updateWeights(int elapsed)
{
    for (int index = 0; index < m_descriptors.m_size; index++) {
        AnimationDescriptor* descriptor = &m_descriptors.m_items[index];
        float change = (float)elapsed * descriptor->m_fadeSpeed;
        if (descriptor->m_state == STATE_FADE_IN) {
            descriptor->m_weight += change;
            if (descriptor->m_weight >= 1.0f) {
                descriptor->m_weight = 1.0f;
                descriptor->m_state++;
            }
        } else if (descriptor->m_state == STATE_FADE_OUT) {
            descriptor->m_weight -= change;
            if (descriptor->m_weight <= 0.0f) {
                descriptor->m_weight = 0.0f;
                descriptor->m_state++;
            }
        }
        if (descriptor->m_instance.get() != NULL) {
            descriptor->m_instance->setWeight(descriptor->m_weight);
        }
    }

    // drop the finished animations: the last descriptor moves into the freed place
    int count = m_descriptors.m_size;
    int index = 0;
    while (index < count) {
        AnimationDescriptor* descriptor = &m_descriptors.m_items[index];
        if (descriptor->m_state != STATE_FINISHED) {
            index++;
            continue;
        }
        if (descriptor->m_instance.get() != NULL) {
            descriptor->m_instance->stop();
            AnimationDescriptor* stopped = &m_descriptors.m_items[index];
            if (stopped->m_instance.get() != NULL) {
                delete stopped->m_instance.get();
                stopped->m_instance = WeakIntrusivePtr<AnimationInstance>();
            }
        }
        if (count > 1) {
            m_descriptors.m_items[index] = m_descriptors.m_items[count - 1];
        }
        count--;
    }
    m_descriptors.setSize(count);
}

WeakIntrusivePtr<AnimationInstance> AnimationTransition::startAnimation(AnimationImpl* animation, float weight,
                                                                        bool looped, int fadeTime, float speed,
                                                                        float startTime)
{
    AnimationDescriptor* descriptor = getAnimDesc(DGHelper::getSwerveID(animation->getName()));
    if (descriptor != NULL) {
        // already started: fade it in again from the start
        descriptor->m_state = STATE_FADE_IN;
        descriptor->m_fadeSpeed = 1.0f / (float)fadeTime;
        if (descriptor->m_instance.get() != NULL) {
            descriptor->m_instance->setTime(0.0f);
            descriptor->m_instance->setSpeed(speed);
            descriptor->m_instance->setLooped(looped);
        }
        return descriptor->m_instance;
    }

    WeakIntrusivePtr<AnimationInstance> instance = m_object->play(animation, weight, looped);
    if (m_listener != NULL || looped == true) {
        instance->setListener(this);
    }
    instance->setSpeed(speed);
    instance->setLooped(looped);
    if (startTime != 0.0f) {
        instance->setTime((float)((int)startTime % (int)instance->m_animation->getDuration()));
    }

    AnimationDescriptor newDescriptor;
    newDescriptor.m_instance = instance;
    newDescriptor.m_looped = looped;
    newDescriptor.m_fadeSpeed = 1.0f / (float)fadeTime;
    newDescriptor.m_weight = 0.0f;
    newDescriptor.m_state = STATE_FADE_IN;
    m_descriptors.addElement(newDescriptor);
    return instance;
}

AnimationTransition::AnimationDescriptor* AnimationTransition::getAnimDesc(int id) const
{
    for (int index = 0; index < m_descriptors.m_size; index++) {
        if (DGHelper::getSwerveID(m_descriptors.m_items[index].m_instance->m_animation->getName()) == id) {
            return &m_descriptors.m_items[index];
        }
    }
    return NULL;
}

AnimationTransition::AnimationDescriptor* AnimationTransition::getAnimDesc(WeakIntrusivePtr<AnimationInstance> instance) const
{
    for (int index = 0; index < m_descriptors.m_size; index++) {
        AnimationDescriptor* descriptor = &m_descriptors.m_items[index];
        if (descriptor->m_instance == instance) {
            return descriptor;
        }
    }
    return NULL;
}

void AnimationTransition::eventListener(const AnimationEventCK& event, WeakIntrusivePtr<AnimationInstance> instance)
{
    AnimationDescriptor* descriptor = getAnimDesc(instance);
    // looped animations do not report their implicit start / end events
    if (!descriptor->m_looped || (event.m_id != AnimationEventCK::EVENT_END && event.m_id != AnimationEventCK::EVENT_START)) {
        if (m_listener != NULL) {
            m_listener->eventListener(event, instance);
        }
    }
}

void AnimationTransition::stopAllAnims(int fadeTime)
{
    for (int index = 0; index < m_descriptors.m_size; index++) {
        AnimationDescriptor* descriptor = &m_descriptors.m_items[index];
        if (descriptor != NULL) {
            descriptor->m_state = STATE_FADE_OUT;
        }
    }
}
