// AnimatableObject.cpp - playing animations on an object (created by b047x).
#include "AnimatableObject.h"
#include "AnimationInstance.h"
#include "Fixed.h"

AnimatableObject::AnimatableObject()
{
    m_channelWeights = NULL;
}

AnimatableObject::~AnimatableObject()
{
    if (m_channelWeights != NULL) {
        delete[] m_channelWeights;
        m_channelWeights = NULL;
    }
}

WeakIntrusivePtr<AnimationInstance> AnimatableObject::play(Animation* animation, float weight, bool looped)
{
    AnimationInstance* instance = new AnimationInstance(animation, this);
    WeakIntrusivePtr<AnimationInstance> result(instance);
    result->setWeight(weight);
    result->setLooped(looped);
    m_instances.addElement(result);
    return result;
}

void AnimatableObject::advanceTime(float elapsed)
{
    int elapsedFixed = Fixed::fromFloat(elapsed);
    for (int index = 0; index < m_instances.m_size; index++) {
        m_instances.m_items[index]->advanceTime(elapsedFixed);
    }
}
