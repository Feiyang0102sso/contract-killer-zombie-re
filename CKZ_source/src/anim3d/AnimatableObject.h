// AnimatableObject.h - object whose channels are driven by AnimationInstances (created by b047x).
#ifndef ANIMATABLEOBJECT_H
#define ANIMATABLEOBJECT_H

#include <stddef.h>
#include "Vector_ss.h"
#include "WeakPtr.h"

class Animation;
class AnimationInstance;
class QuaternionFixed;

class AnimatableObject {
public:
    AnimatableObject();
    virtual ~AnimatableObject();

    virtual void setRotationChannelQauternionValue(int channel, QuaternionFixed* rotation) = 0;
    virtual void setPositionChannelVectorValue(int channel, int* position) = 0;
    virtual void setScaleChannelVectorValue(int channel, int* scale) = 0;
    /** Camera field of view (type 3), near (4) or far (5) plane. */
    virtual void setCameraPropertyChannelValue(int channel, int value, unsigned char type) = 0;
    /** Texture translation (types 6, 7), scale (8, 9) or rotation (10). */
    virtual void setTexturePropertyChannelValue(int channel, int value, unsigned char type) = 0;
    virtual int getChannelsCount() const = 0;
    /** Blend the playing instances and apply the result to the channels. */
    virtual void animate() = 0;

    /** Start `animation` with blend `weight`. */
    WeakIntrusivePtr<AnimationInstance> play(Animation* animation, float weight, bool looped);
    /** Advance every playing instance by `elapsed` milliseconds. */
    void advanceTime(float elapsed);

    Vector<WeakIntrusivePtr<AnimationInstance> > m_instances;  // 0x04
    int* m_channelWeights;      // 0x14 sum of the instance weights per channel (20.12)
};

#endif
