// AnimationImpl.cpp - loading and sampling of keyframe animations (created by b047x).
#include "AnimationImpl.h"
#include "DataInputStream.h"
#include "Fixed.h"
#include "QuaternionFixed.h"

/** Track kinds (the AnimatableObject channel types). */
enum {
    TRACK_POSITION = 0,     // 3 components, raw floats
    TRACK_ROTATION = 1,     // quaternion, packed
    TRACK_SCALE = 2         // 3 components, packed
};

AnimationImpl::~AnimationImpl()
{
    for (int index = 0; index < m_tracksCount; index++) {
        if (m_tracks[index] != NULL) {
            delete m_tracks[index];
            m_tracks[index] = NULL;
        }
    }
    if (m_tracks != NULL) {
        delete[] m_tracks;
        m_tracks = NULL;
    }
    if (m_name != NULL) {
        delete[] m_name;
        m_name = NULL;
    }
}

void AnimationImpl::load(JDataInputStream* stream, Vector<AnimationImpl*>& animations)
{
    stream->readInt();
    int packing = stream->readByte();
    int animationsCount = stream->readShort();
    for (int animationIndex = 0; animationIndex < animationsCount; animationIndex++) {
        char* name = stream->readUTF();
        int startFrame = stream->readInt();
        int endFrame = stream->readInt();
        int duration = (endFrame << 12) - (startFrame << 12);
        // key times are stored as 16 bit frame numbers, scaled down for long animations
        unsigned int frames = Fixed::toInt(duration);
        float timeScale = 1.0f;
        if (frames > 0xffff) {
            timeScale = (float)frames / 65535.0f;
        }

        unsigned short eventsCount = stream->readUnsignedShort();
        Vector<float> eventTimes;
        Vector<const char*> eventNames;
        for (unsigned int eventIndex = 0; eventIndex < eventsCount; eventIndex++) {
            float eventTime = (float)stream->readInt();
            const char* eventName = stream->readUTF();
            eventTimes.addElement(eventTime);
            eventNames.addElement(eventName);
        }

        unsigned short tracksCount = stream->readShort();
        AnimationTrack** tracks = new AnimationTrack*[tracksCount]();
        for (unsigned int trackIndex = 0; trackIndex < tracksCount; trackIndex++) {
            int targetChannel = stream->readShort();
            char type = stream->readByte();
            int trackDuration = stream->readInt();
            char looped = stream->readByte();
            int keysCount = stream->readShort();
            int* times = new int[keysCount];
            int* values0 = new int[keysCount];
            int* values1 = NULL;
            int* values2 = NULL;
            int* values3 = NULL;
            switch (type) {
            case 0:
            case 2: {
                float scale = stream->readFloat();
                float bias = stream->readFloat();
                values1 = new int[keysCount];
                values2 = new int[keysCount];
                for (int key = 0; key < keysCount; key++) {
                    times[key] = (int)(stream->readUnsignedShort() * timeScale) << 12;
                    if (type == TRACK_POSITION) {
                        values0[key] = Fixed::fromFloat(stream->readFloat() + bias);
                        values1[key] = Fixed::fromFloat(stream->readFloat() + bias);
                        values2[key] = Fixed::fromFloat(stream->readFloat() + bias);
                    } else {
                        values0[key] = Fixed::fromFloat(unpackValue(stream, packing) * scale + bias);
                        values1[key] = Fixed::fromFloat(unpackValue(stream, packing) * scale + bias);
                        values2[key] = Fixed::fromFloat(unpackValue(stream, packing) * scale + bias);
                    }
                }
                break;
            }
            case 1:
                values3 = new int[keysCount];
                values1 = new int[keysCount];
                values2 = new int[keysCount];
                for (int key = 0; key < keysCount; key++) {
                    times[key] = (int)(stream->readUnsignedShort() * timeScale) << 12;
                    values3[key] = Fixed::fromFloat(unpackValue(stream, packing));
                    values0[key] = Fixed::fromFloat(unpackValue(stream, packing));
                    values1[key] = Fixed::fromFloat(unpackValue(stream, packing));
                    values2[key] = Fixed::fromFloat(unpackValue(stream, packing));
                }
                break;
            case 3:
            case 4:
            case 5:
            case 6:
            case 7:
            case 8:
            case 9:
            case 10:
            case 11: {
                float scale = stream->readFloat();
                float bias = stream->readFloat();
                for (int key = 0; key < keysCount; key++) {
                    times[key] = Fixed::fromFloat(stream->readUnsignedShort() * timeScale);
                    values0[key] = Fixed::fromFloat(unpackValue(stream, packing) * scale + bias);
                }
                break;
            }
            }
            AnimationTrack* track = new AnimationTrack;
            track->m_times = times;
            track->m_values0 = values0;
            track->m_values1 = values1;
            track->m_values2 = values2;
            track->m_values3 = values3;
            track->m_targetChannel = targetChannel;
            track->m_keysCount = keysCount;
            track->m_duration = trackDuration << 12;
            track->m_looped = looped != 0;
            tracks[trackIndex] = track;
        }

        AnimationImpl* animation = new AnimationImpl(tracks, duration, name, tracksCount);
        if (eventTimes.m_size != 0) {
            animation->setEvents(eventTimes, eventNames, eventsCount);
            // setEvents keeps copies: free the names read from the stream
            for (int nameIndex = 0; nameIndex < eventNames.m_size; nameIndex++) {
                if (eventNames.m_items[nameIndex] != NULL) {
                    delete[] eventNames.m_items[nameIndex];
                    eventNames.m_items[nameIndex] = NULL;
                }
            }
        }
        animations.addElement(animation);
    }
}

float AnimationImpl::unpackValue(JDataInputStream* stream, int packing)
{
    if (packing == 1) {
        return (float)stream->readByte() / 127.0f;
    }
    return (float)stream->readShort() / 32767.0f;
}

float AnimationImpl::getDuration() const
{
    return Fixed::toFloat(m_duration);
}

void AnimationImpl::getInterpolatedQuaternion(int track, int time, QuaternionFixed* result) const
{
    AnimationTrack* animationTrack = m_tracks[track];
    if (animationTrack->m_looped) {
        time = time % animationTrack->m_duration;
    }
    int* times = animationTrack->m_times;
    if (time < times[0]) {
        QuaternionFixed::set(result, animationTrack->m_values3[0], animationTrack->m_values0[0],
                             animationTrack->m_values1[0], animationTrack->m_values2[0]);
        return;
    }
    int high = animationTrack->m_keysCount - 1;
    if (time >= times[high]) {
        QuaternionFixed::set(result, animationTrack->m_values3[high], animationTrack->m_values0[high],
                             animationTrack->m_values1[high], animationTrack->m_values2[high]);
        return;
    }
    // binary search of the key interval holding `time`
    int low = 0;
    while (low < high - 1) {
        int middle = (low + high) >> 1;
        if (times[middle] > time) {
            high = middle;
        } else {
            low = middle;
        }
    }
    if (time == times[low]) {
        QuaternionFixed::set(result, animationTrack->m_values3[low], animationTrack->m_values0[low],
                             animationTrack->m_values1[low], animationTrack->m_values2[low]);
        return;
    }
    QuaternionFixed next;
    int nextKey = low + 1;
    int factor = Fixed::div(time - times[low], times[nextKey] - times[low]);
    QuaternionFixed::set(result, animationTrack->m_values3[low], animationTrack->m_values0[low],
                         animationTrack->m_values1[low], animationTrack->m_values2[low]);
    QuaternionFixed::set(&next, animationTrack->m_values3[nextKey], animationTrack->m_values0[nextKey],
                         animationTrack->m_values1[nextKey], animationTrack->m_values2[nextKey]);
    if (QuaternionFixed::dot(result, &next) < 0) {
        QuaternionFixed::inverse(&next);
    }
    QuaternionFixed::interpolate(result, &next, factor);
}

void AnimationImpl::getInterpolatedVector(int track, int time, int* result) const
{
    AnimationTrack* animationTrack = m_tracks[track];
    if (animationTrack->m_looped) {
        time = time % animationTrack->m_duration;
    }
    int* times = animationTrack->m_times;
    if (time < times[0]) {
        result[0] = animationTrack->m_values0[0];
        result[1] = animationTrack->m_values1[0];
        result[2] = animationTrack->m_values2[0];
        return;
    }
    int high = animationTrack->m_keysCount - 1;
    if (time >= times[high]) {
        result[0] = animationTrack->m_values0[high];
        result[1] = animationTrack->m_values1[high];
        result[2] = animationTrack->m_values2[high];
        return;
    }
    int low = 0;
    while (low < high - 1) {
        int middle = (low + high) >> 1;
        if (times[middle] > time) {
            high = middle;
        } else {
            low = middle;
        }
    }
    if (time == times[low]) {
        result[0] = animationTrack->m_values0[low];
        result[1] = animationTrack->m_values1[low];
        result[2] = animationTrack->m_values2[low];
        return;
    }
    int nextKey = low + 1;
    // Snapshot the lower key before publishing output that may overlap key storage.
    int value0 = animationTrack->m_values0[low];
    int value1 = animationTrack->m_values1[low];
    int value2 = animationTrack->m_values2[low];
    int factor = Fixed::div(time - times[low], times[nextKey] - times[low]);
    result[0] = value0 + Fixed::mul(animationTrack->m_values0[nextKey] - value0, factor);
    result[1] = value1 + Fixed::mul(animationTrack->m_values1[nextKey] - value1, factor);
    result[2] = value2 + Fixed::mul(animationTrack->m_values2[nextKey] - value2, factor);
}

int AnimationImpl::getInterpolatedValue(int track, int time) const
{
    AnimationTrack* animationTrack = m_tracks[track];
    if (animationTrack->m_looped) {
        time = time % animationTrack->m_duration;
    }
    int* times = animationTrack->m_times;
    if (time < times[0]) {
        return animationTrack->m_values0[0];
    }
    int last = animationTrack->m_keysCount - 1;
    if (time < times[last]) {
        int low = 0;
        int high = last;
        while (low < high - 1) {
            int middle = (low + high) >> 1;
            if (times[middle] > time) {
                high = middle;
            } else {
                low = middle;
            }
        }
        if (time == times[low]) {
            return animationTrack->m_values0[low];
        }
        int nextKey = low + 1;
        int factor = Fixed::div(time - times[low], times[nextKey] - times[low]);
        return animationTrack->m_values0[low] + Fixed::mul(animationTrack->m_values0[nextKey] - animationTrack->m_values0[low], factor);
    }
    return animationTrack->m_values0[last];
}

int AnimationImpl::getTracksCount() const
{
    return m_tracksCount;
}

const char* AnimationImpl::getName() const
{
    return m_name;
}

int AnimationImpl::getTrackTargetChannelIndex(int track) const
{
    return m_tracks[track]->m_targetChannel;
}

int AnimationImpl::getDuration(int track) const
{
    return m_tracks[track]->m_duration;
}
