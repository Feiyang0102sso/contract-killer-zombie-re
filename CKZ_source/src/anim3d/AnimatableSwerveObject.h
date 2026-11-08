// AnimatableSwerveObject.h - AnimatableObject whose channels are Swerve scene graph nodes (created by b047x).
// Every channel has a type: 0 position, 1 rotation, 2 scale, 3..5 camera properties, 6..10 texture
// transform properties, 11 alpha. The current values of all channels of one type live in one array
// (m_typeValues[type]) indexed by channel - m_typeFirstChannel[type].
#ifndef ANIMATABLESWERVEOBJECT_H
#define ANIMATABLESWERVEOBJECT_H

#include <stddef.h>
#include "AnimatableObject.h"
#include "tws.h"

class JDataInputStream;

class AnimatableSwerveObject : public AnimatableObject {
public:
    /** Channel types. */
    enum {
        CHANNEL_POSITION = 0,
        CHANNEL_ROTATION = 1,
        CHANNEL_SCALE = 2,
        CHANNEL_CAMERA_FOV = 3,
        CHANNEL_CAMERA_NEAR = 4,
        CHANNEL_CAMERA_FAR = 5,
        CHANNEL_TEXTURE_TRANSLATION_X = 6,
        CHANNEL_TEXTURE_TRANSLATION_Y = 7,
        CHANNEL_TEXTURE_SCALE_X = 8,
        CHANNEL_TEXTURE_SCALE_Y = 9,
        CHANNEL_TEXTURE_ROTATION = 10,
        CHANNEL_ALPHA = 11,
        CHANNEL_TYPES_COUNT = 12
    };

    /** Bind the channels described by `stream` to the descendants of `group`. */
    AnimatableSwerveObject(swerve::Group<CSwerveAssertingErrorHandler> group, JDataInputStream* stream);
    void init(swerve::Group<CSwerveAssertingErrorHandler> group, JDataInputStream* stream);
    /** Clear the orientation of the node of `channel`. */
    void resetChannelNodeOrientation(int channel);
    virtual ~AnimatableSwerveObject();

    virtual void setRotationChannelQauternionValue(int channel, QuaternionFixed* rotation);
    virtual void setPositionChannelVectorValue(int channel, int* position);
    virtual void setScaleChannelVectorValue(int channel, int* scale);
    virtual void setCameraPropertyChannelValue(int channel, int value, unsigned char type);
    virtual void setTexturePropertyChannelValue(int channel, int value, unsigned char type);

    virtual int getChannelsCount() const
    {
        return m_channelsCount;
    }

    virtual void animate();
    /** Channel of `node`, -1 when it is not animated. */
    int getChannel(const swerve::Object3D<CSwerveAssertingErrorHandler>& node) const;

    swerve::Object3D<CSwerveAssertingErrorHandler>* m_nodes;   // 0x18 node of each channel
    int* m_blendWeights;            // 0x1c weight blended into each channel so far this frame
    bool* m_channelUpdated;         // 0x20 channel got a value this frame
    bool* m_channelBlended;         // 0x24 channel blends several instances (true by default)
    swerve::Transform<CSwerveAssertingErrorHandler> m_transform;  // 0x28 scratch for rotations
    int m_channelsCount;            // 0x2c
    unsigned char* m_channelTypes;  // 0x30
    int* m_typeFirstChannel;        // 0x34 [CHANNEL_TYPES_COUNT]
    void** m_typeValues;            // 0x38 [CHANNEL_TYPES_COUNT]
};

#endif
