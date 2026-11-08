// AnimationImpl.h - keyframe animation loaded from the game's animation files (created by b047x).
#ifndef ANIMATIONIMPL_H
#define ANIMATIONIMPL_H

#include <stddef.h>
#include "Animation.h"
#include "CSwerve.h"

class JDataInputStream;

class AnimationImpl : public Animation {
public:
    /** One animated channel: key times and up to four value components (20.12). */
    class AnimationTrack {
    public:
        ~AnimationTrack()
        {
            if (m_times != NULL) {
                delete[] m_times;
                m_times = NULL;
            }
            if (m_values0 != NULL) {
                delete[] m_values0;
                m_values0 = NULL;
            }
            if (m_values1 != NULL) {
                delete[] m_values1;
                m_values1 = NULL;
            }
            if (m_values2 != NULL) {
                delete[] m_values2;
                m_values2 = NULL;
            }
            if (m_values3 != NULL) {
                delete[] m_values3;
                m_values3 = NULL;
            }
        }

        int* m_times;           // 0x00 key times (20.12 ms), ascending
        int* m_values0;         // 0x04 x (vectors, quaternions) or the scalar value
        int* m_values1;         // 0x08 y
        int* m_values2;         // 0x0c z
        int* m_values3;         // 0x10 w (quaternions only)
        int m_targetChannel;    // 0x14 AnimatableObject channel
        int m_keysCount;        // 0x18
        int m_duration;         // 0x1c 20.12, period of a looped track
        bool m_looped;          // 0x20
    };

    AnimationImpl(AnimationTrack** tracks, int duration, char* name, int tracksCount)
    {
        m_tracks = tracks;
        m_duration = duration;
        m_name = name;
        m_userId = CSwerve::GenerateUserID(name);
        m_tracksCount = tracksCount;
    }

    virtual ~AnimationImpl();

    /** Read all animations of `stream` and append them to `animations`. */
    static void load(JDataInputStream* stream, Vector<AnimationImpl*>& animations);
    /** A packed normalized value: 1 byte (packing 1) or 2 bytes. */
    static float unpackValue(JDataInputStream* stream, int packing);

    virtual float getDuration() const;
    virtual void getInterpolatedQuaternion(int track, int time, QuaternionFixed* result) const;
    virtual void getInterpolatedVector(int track, int time, int* result) const;
    virtual int getInterpolatedValue(int track, int time) const;
    virtual int getTracksCount() const;
    virtual const char* getName() const;
    virtual int getTrackTargetChannelIndex(int track) const;
    virtual int getDuration(int track) const;

    AnimationTrack** m_tracks;      // 0x24
    int m_duration;                 // 0x28 20.12
    char* m_name;                   // 0x2c
    int m_userId;                   // 0x30 CSwerve::GenerateUserID(m_name)
    int m_tracksCount;              // 0x34
};

#endif
