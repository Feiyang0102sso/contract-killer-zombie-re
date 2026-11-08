// Animation.h - abstract keyframe animation with named events (created by b047x).
// Times are 20.12 fixed point milliseconds unless stated otherwise; event times are floats
// in seconds as read from the file.
#ifndef ANIMATION_H
#define ANIMATION_H

#include <stddef.h>
#include "Vector_ss.h"

class QuaternionFixed;

class Animation {
public:
    Animation();
    virtual ~Animation();

    /** Value of rotation track `track` at `time`. */
    virtual void getInterpolatedQuaternion(int track, int time, QuaternionFixed* result) const = 0;
    /** Value of a 3 component track (position, scale) at `time`. */
    virtual void getInterpolatedVector(int track, int time, int* result) const = 0;
    /** Value of a scalar track at `time`. */
    virtual int getInterpolatedValue(int track, int time) const = 0;
    virtual int getTracksCount() const = 0;
    /** Index of the AnimatableObject channel animated by `track`. */
    virtual int getTrackTargetChannelIndex(int track) const = 0;
    virtual int getDuration(int track) const = 0;
    virtual const char* getName() const = 0;
    /** Length in seconds. */
    virtual float getDuration() const = 0;

    /** Replace the events with copies of the first `count` times and names. */
    void setEvents(Vector<float>& times, Vector<const char*>& names, int count);

    int getEventsCount() const
    {
        return m_eventTimes.m_size;
    }

    Vector<float> m_eventTimes;             // 0x04
    Vector<const char*> m_eventNames;       // 0x14 owned copies
};

#endif
