// AnimatableSwerveObject.cpp - blending animation instances onto Swerve nodes (created by b047x).
#include "AnimatableSwerveObject.h"
#include "Animation.h"
#include "AnimationInstance.h"
#include "QuaternionFixed.h"
#include "Fixed.h"
#include "DataInputStream.h"
#include "CSwerve.h"

typedef CSwerveAssertingErrorHandler ErrorHandler;

/** Projection types returned by Camera::getProjection (M3G constants). */
enum {
    PROJECTION_PARALLEL = 0x31,
    PROJECTION_PERSPECTIVE = 0x32
};

AnimatableSwerveObject::AnimatableSwerveObject(swerve::Group<ErrorHandler> group, JDataInputStream* stream)
    : AnimatableObject()
{
    m_nodes = NULL;
    m_blendWeights = NULL;
    m_channelUpdated = NULL;
    m_channelBlended = NULL;
    m_channelsCount = 0;
    m_channelTypes = NULL;
    m_typeFirstChannel = NULL;
    m_typeValues = NULL;
    m_transform = swerve::Transform<ErrorHandler>(CSwerve::GetInstance()->m_swerve);
    init(group, stream);
    for (int channel = 0; channel < m_channelsCount; channel++) {
        if (m_channelTypes[channel] == CHANNEL_ROTATION) {
            resetChannelNodeOrientation(channel);
        }
    }
}

void AnimatableSwerveObject::init(swerve::Group<ErrorHandler> group, JDataInputStream* stream)
{
    m_channelsCount = stream->readShort();
    m_nodes = new swerve::Object3D<ErrorHandler>[m_channelsCount];
    m_blendWeights = new int[m_channelsCount];
    m_channelWeights = new int[m_channelsCount];
    m_channelUpdated = new bool[m_channelsCount];
    m_channelBlended = new bool[m_channelsCount];
    m_channelTypes = new unsigned char[m_channelsCount];
    m_typeFirstChannel = new int[CHANNEL_TYPES_COUNT];
    m_typeValues = new void*[CHANNEL_TYPES_COUNT];
    for (int channel = 0; channel < m_channelsCount; channel++) {
        m_blendWeights[channel] = 0;
        m_channelWeights[channel] = 0;
        m_channelUpdated[channel] = false;
        m_channelBlended[channel] = true;
        m_channelTypes[channel] = 0;
    }
    for (int type = 0; type < CHANNEL_TYPES_COUNT; type++) {
        m_typeFirstChannel[type] = 0;
        m_typeValues[type] = NULL;
    }

    // groups of channels of one type: count, type, then the user id of each node
    int channel = 0;
    while (channel < m_channelsCount) {
        int count = stream->readShort();
        unsigned char type = stream->readByte();
        m_typeFirstChannel[type] = channel;
        switch (type) {
        case CHANNEL_POSITION:
        case CHANNEL_SCALE:
            m_typeValues[type] = new int[count * 3];
            break;
        case CHANNEL_ROTATION:
            m_typeValues[type] = new QuaternionFixed[count];
            break;
        case CHANNEL_CAMERA_FOV:
        case CHANNEL_CAMERA_NEAR:
        case CHANNEL_CAMERA_FAR:
        case CHANNEL_TEXTURE_TRANSLATION_X:
        case CHANNEL_TEXTURE_TRANSLATION_Y:
        case CHANNEL_TEXTURE_SCALE_X:
        case CHANNEL_TEXTURE_SCALE_Y:
        case CHANNEL_TEXTURE_ROTATION:
        case CHANNEL_ALPHA:
            m_typeValues[type] = new int[count];
            break;
        }
        for (int index = 0; index < count; index++) {
            int userId = stream->readInt();
            m_nodes[channel + index] = group.find(userId);
            m_channelTypes[channel + index] = type;
        }
        channel += count;
    }
}

void AnimatableSwerveObject::resetChannelNodeOrientation(int channel)
{
    swerve::Transformable<ErrorHandler> transformable(m_nodes[channel]);
    if (transformable != NULL) {
        float angleAxis[4];
        angleAxis[0] = 0.0f;
        angleAxis[1] = 0.0f;
        angleAxis[2] = 0.0f;
        angleAxis[3] = 0.0f;
        transformable.setOrientation(angleAxis);
    }
}

AnimatableSwerveObject::~AnimatableSwerveObject()
{
    for (int type = 0; type < CHANNEL_TYPES_COUNT; type++) {
        // (the original tests m_channelTypes[type], not the type itself)
        if (m_channelTypes[type] == CHANNEL_ROTATION) {
            delete[] (QuaternionFixed*)m_typeValues[type];
        } else {
            delete[] (int*)m_typeValues[type];
        }
    }
    delete[] m_typeValues;
    delete[] m_nodes;
    delete[] m_blendWeights;
    delete[] m_channelUpdated;
    delete[] m_channelBlended;
    delete[] m_channelTypes;
    delete[] m_typeFirstChannel;
}

void AnimatableSwerveObject::setRotationChannelQauternionValue(int channel, QuaternionFixed* rotation)
{
    swerve::Transformable<ErrorHandler> transformable(m_nodes[channel]);
    if (transformable != NULL) {
        float matrix[16];
        QuaternionFixed::toMatrix(rotation, matrix);
        m_transform.set(matrix);
        transformable.setTransform(m_transform);
    }
}

void AnimatableSwerveObject::setPositionChannelVectorValue(int channel, int* position)
{
    swerve::Transformable<ErrorHandler> transformable(m_nodes[channel]);
    if (transformable != NULL) {
        float translation[3];
        translation[0] = Fixed::toFloat(position[0]);
        translation[1] = Fixed::toFloat(position[1]);
        translation[2] = Fixed::toFloat(position[2]);
        transformable.setTranslation(translation);
    }
}

void AnimatableSwerveObject::setScaleChannelVectorValue(int channel, int* scale)
{
    swerve::Transformable<ErrorHandler> transformable(m_nodes[channel]);
    if (transformable != NULL) {
        float scaleXYZ[3];
        scaleXYZ[0] = Fixed::toFloat(scale[0]);
        scaleXYZ[1] = Fixed::toFloat(scale[1]);
        scaleXYZ[2] = Fixed::toFloat(scale[2]);
        transformable.setScale(scaleXYZ);
    }
}

void AnimatableSwerveObject::setCameraPropertyChannelValue(int channel, int value, unsigned char type)
{
    swerve::Camera<ErrorHandler> camera(m_nodes[channel]);
    if (camera != NULL) {
        float params[4];
        int projection = camera.getProjection(params);
        switch (type) {
        case CHANNEL_CAMERA_FOV:
            params[0] = Fixed::toFloat(value);
            break;
        case CHANNEL_CAMERA_NEAR:
            params[2] = Fixed::toFloat(value);
            break;
        case CHANNEL_CAMERA_FAR:
            params[3] = Fixed::toFloat(value);
            break;
        }
        switch (projection) {
        case PROJECTION_PARALLEL:
            camera.setParallel(params[0], params[1], params[2], params[3]);
            break;
        case PROJECTION_PERSPECTIVE:
            camera.setPerspective(params[0], params[1], params[2], params[3]);
            break;
        }
    }
}

void AnimatableSwerveObject::setTexturePropertyChannelValue(int channel, int value, unsigned char type)
{
    swerve::Transformable<ErrorHandler> texture(m_nodes[channel]);
    if (texture != NULL) {
        float floatValue = Fixed::toFloat(value);
        // translation and scale share one 3-float buffer, the rotation has its own (original frame layout)
        float components[3];
        float angleAxis[4];
        switch (type) {
        case CHANNEL_TEXTURE_TRANSLATION_X: {
            texture.getTranslation(components);
            components[0] = floatValue;
            texture.setTranslation(components);
            break;
        }
        case CHANNEL_TEXTURE_TRANSLATION_Y: {
            texture.getTranslation(components);
            components[1] = floatValue;
            texture.setTranslation(components);
            break;
        }
        case CHANNEL_TEXTURE_SCALE_X: {
            texture.getScale(components);
            components[0] = floatValue;
            texture.setScale(components);
            break;
        }
        case CHANNEL_TEXTURE_SCALE_Y: {
            texture.getScale(components);
            components[1] = floatValue;
            texture.setScale(components);
            break;
        }
        case CHANNEL_TEXTURE_ROTATION: {
            texture.getOrientation(angleAxis);
            angleAxis[0] = floatValue;
            angleAxis[1] = 0.0f;
            angleAxis[2] = 0.0f;
            angleAxis[3] = 1.0f;
            texture.setOrientation(angleAxis);
            break;
        }
        }
    }
}

void AnimatableSwerveObject::animate()
{
    for (int channel = 0; channel < m_channelsCount; channel++) {
        m_channelUpdated[channel] = false;
    }

    // blend every playing instance into the channel values
    int instancesCount = m_instances.m_size;
    for (int instanceIndex = 0; instanceIndex < instancesCount; instanceIndex++) {
        WeakIntrusivePtr<AnimationInstance> instance = m_instances.m_items[instanceIndex];
        Animation* animation = instance->m_animation;
        int time = instance->m_time;
        for (int track = 0; track < animation->getTracksCount(); track++) {
            if (instance->m_weight <= 0) {
                continue;
            }
            int channel = animation->getTrackTargetChannelIndex(track);
            int totalWeight = m_channelWeights[channel];
            if (totalWeight <= 0) {
                continue;
            }
            unsigned char type = m_channelTypes[channel];
            switch (type) {
            case CHANNEL_POSITION:
            case CHANNEL_SCALE: {
                int* values = (int*)m_typeValues[type];
                int first = m_typeFirstChannel[type];
                int vector[3];
                animation->getInterpolatedVector(track, time, vector);
                int* current = &values[(channel - first) * 3];
                int weight = Fixed::div(instance->m_weight, m_channelWeights[channel]);
                if (!m_channelUpdated[channel] || !m_channelBlended[channel]) {
                    current[0] = vector[0];
                    current[1] = vector[1];
                    current[2] = vector[2];
                    m_blendWeights[channel] = weight;
                    m_channelUpdated[channel] = true;
                } else {
                    int blendWeight = m_blendWeights[channel] + weight;
                    if (blendWeight != 0) {
                        int factor = Fixed::div(weight, blendWeight);
                        current[0] = current[0] + Fixed::mul(vector[0] - current[0], factor);
                        current[1] = current[1] + Fixed::mul(vector[1] - current[1], factor);
                        current[2] = current[2] + Fixed::mul(vector[2] - current[2], factor);
                    }
                    m_blendWeights[channel] = blendWeight;
                }
                break;
            }
            case CHANNEL_ROTATION: {
                QuaternionFixed* values = (QuaternionFixed*)m_typeValues[type];
                int index = channel - m_typeFirstChannel[type];
                int weight = Fixed::div(instance->m_weight, totalWeight);
                if (!m_channelUpdated[channel] || !m_channelBlended[channel]) {
                    if (m_channelBlended[channel]) {
                        animation->getInterpolatedQuaternion(track, time, &values[index]);
                    }
                    m_blendWeights[channel] = weight;
                    m_channelUpdated[channel] = true;
                } else {
                    QuaternionFixed rotation;
                    animation->getInterpolatedQuaternion(track, time, &rotation);
                    int blendWeight = m_blendWeights[channel] + weight;
                    if (blendWeight != 0) {
                        int factor = Fixed::div(weight, blendWeight);
                        if (QuaternionFixed::dot(&values[index], &rotation) < 0) {
                            QuaternionFixed::inverse(&rotation);
                        }
                        QuaternionFixed::interpolate(&values[index], &rotation, factor);
                    }
                    m_blendWeights[channel] = blendWeight;
                }
                break;
            }
            case CHANNEL_CAMERA_FOV:
            case CHANNEL_CAMERA_NEAR:
            case CHANNEL_CAMERA_FAR:
            case CHANNEL_TEXTURE_TRANSLATION_X:
            case CHANNEL_TEXTURE_TRANSLATION_Y:
            case CHANNEL_TEXTURE_SCALE_X:
            case CHANNEL_TEXTURE_SCALE_Y:
            case CHANNEL_TEXTURE_ROTATION:
            case CHANNEL_ALPHA: {
                int* values = (int*)m_typeValues[type];
                int first = m_typeFirstChannel[type];
                int value = animation->getInterpolatedValue(track, time);
                int index = channel - first;
                int weight = Fixed::div(instance->m_weight, m_channelWeights[channel]);
                if (!m_channelUpdated[channel] || !m_channelBlended[channel]) {
                    values[index] = value;
                    m_blendWeights[channel] = weight;
                    m_channelUpdated[channel] = true;
                } else {
                    int blendWeight = m_blendWeights[channel] + weight;
                    if (blendWeight != 0) {
                        int factor = Fixed::div(weight, blendWeight);
                        values[index] = values[index] + Fixed::mul(value - values[index], factor);
                    }
                    m_blendWeights[channel] = blendWeight;
                }
                break;
            }
            }
        }
    }

    // apply the blended values to the nodes
    for (int channel = 0; channel < m_channelsCount; channel++) {
        if (!m_channelUpdated[channel] || !m_channelBlended[channel]) {
            continue;
        }
        unsigned char type = m_channelTypes[channel];
        if (type > CHANNEL_ALPHA) {
            continue;
        }
        void* values = m_typeValues[type];
        int index = channel - m_typeFirstChannel[type];
        switch (type) {
        case CHANNEL_POSITION:
        case CHANNEL_SCALE: {
            int vector[3];
            vector[0] = ((int*)values)[index * 3];
            vector[1] = ((int*)values)[index * 3 + 1];
            vector[2] = ((int*)values)[index * 3 + 2];
            if (type == CHANNEL_SCALE) {
                setScaleChannelVectorValue(channel, vector);
            } else if (type == CHANNEL_POSITION) {
                setPositionChannelVectorValue(channel, vector);
            }
            break;
        }
        case CHANNEL_ROTATION:
            setRotationChannelQauternionValue(channel, &((QuaternionFixed*)values)[index]);
            break;
        case CHANNEL_CAMERA_FOV:
        case CHANNEL_CAMERA_NEAR:
        case CHANNEL_CAMERA_FAR:
            setCameraPropertyChannelValue(channel, ((int*)values)[index], type);
            break;
        case CHANNEL_TEXTURE_TRANSLATION_X:
        case CHANNEL_TEXTURE_TRANSLATION_Y:
        case CHANNEL_TEXTURE_SCALE_X:
        case CHANNEL_TEXTURE_SCALE_Y:
        case CHANNEL_TEXTURE_ROTATION:
            setTexturePropertyChannelValue(channel, ((int*)values)[index], type);
            break;
        case CHANNEL_ALPHA: {
            swerve::Mesh<ErrorHandler> mesh(m_nodes[channel]);
            swerve::Sprite3D<ErrorHandler> sprite(m_nodes[channel]);
            if (mesh != NULL || sprite != NULL) {
                int alpha = ((int*)values)[index];
                if (alpha > 0x1000) {
                    alpha = 0x1000;
                } else if (alpha < 0) {
                    alpha = 0;
                }
                if (mesh != NULL) {
                    mesh.setRenderingEnable(alpha != 0);
                    float alphaFactor = Fixed::toFloat(alpha);
                    mesh.setAlphaFactor(alphaFactor);
                    swerve::Node<ErrorHandler> parent = mesh.getParent();
                    float parentAlpha = alphaFactor + 0.4960784316f;
                    if (parentAlpha > 1.0f) {
                        parentAlpha = 1.0f;
                    }
                    parent.setAlphaFactor(parentAlpha);
                    parent.setRenderingEnable(parentAlpha >= 0.5f);
                } else {
                    sprite.setRenderingEnable(alpha != 0);
                    float alphaFactor = Fixed::toFloat(alpha);
                    sprite.setAlphaFactor(alphaFactor);
                    swerve::Node<ErrorHandler> parent = sprite.getParent();
                    float parentAlpha = alphaFactor + 0.4960784316f;
                    if (parentAlpha > 1.0f) {
                        parentAlpha = 1.0f;
                    }
                    parent.setAlphaFactor(parentAlpha);
                    parent.setRenderingEnable(parentAlpha >= 0.5f);
                }
            }
            break;
        }
        }
    }
}

int AnimatableSwerveObject::getChannel(const swerve::Object3D<ErrorHandler>& node) const
{
    for (int channel = 0; channel < m_channelsCount; channel++) {
        if (m_nodes[channel] == node) {
            return channel;
        }
    }
    return -1;
}
