// CGyroscope_Cocoa.h - device attitude sampling through CoreMotion (CMMotionManager device motion).
// Owned by CApplet (created in CApplet::Start when the device has a gyroscope).
#ifndef CGYROSCOPE_COCOA_H
#define CGYROSCOPE_COCOA_H

#include "CVector3d.h"

#ifdef __OBJC__
@class CMMotionManager;
#else
typedef void CMMotionManager;
#endif

class CGyroscope_Cocoa {
public:
    /** Allocate a gyroscope; check IsAvailable() before use. */
    static CGyroscope_Cocoa* Create();

    CGyroscope_Cocoa();
    virtual ~CGyroscope_Cocoa();

    /**
     * Sample the attitude when a new sample is due for the current frequency.
     * Returns 1 when m_attitude was updated.
     */
    virtual unsigned char CanSample();

    /** Samples per second; 0 stops the device motion updates. */
    void SetGyroscopeFrequency(unsigned int frequency);

    /** The device has a gyroscope (the motion manager survived construction). */
    bool IsAvailable() const
    {
        return m_motionManager != NULL;
    }

    /** Last sampled attitude: pitch, roll, yaw in 16.16 fixed point radians (stored as float). */
    const com::glu::platform::math::CVector3d& GetAttitude() const
    {
        return m_attitude;
    }

private:
    int m_lastSampleTime;                           // 0x04 GetUpTimeMS() of the last sample
    unsigned int m_frequency;                       // 0x08 samples per second
    com::glu::platform::math::CVector3d m_attitude; // 0x0c pitch, roll, yaw
    CMMotionManager* m_motionManager;               // 0x18 nil without a gyroscope
};

#endif
