// CGyroscope_Cocoa_mm.mm - attitude sampling through CMMotionManager (looked up at run time).
#include "CGyroscope_Cocoa.h"

#import <CoreMotion/CoreMotion.h>

#include "CStdUtil_Cocoa.h"

using namespace com::glu::platform::math;

/** Radians to 16.16 fixed point. */
static inline int RadiansToFixed(double radians)
{
    return (int)(radians * 65536.0);
}

CGyroscope_Cocoa* CGyroscope_Cocoa::Create()
{
    return new CGyroscope_Cocoa();
}

CGyroscope_Cocoa::CGyroscope_Cocoa()
    : m_lastSampleTime(0),
      m_frequency(0),
      m_attitude(0.0f, 0.0f, 0.0f),
      m_motionManager(nil)
{
    // CoreMotion may be missing on old firmware: look the class up by name
    m_motionManager = [[NSClassFromString(@"CMMotionManager") alloc] init];
    if (m_motionManager != nil && ![m_motionManager isGyroAvailable]) {
        [m_motionManager release];
        m_motionManager = nil;
    }
}

CGyroscope_Cocoa::~CGyroscope_Cocoa()
{
    if (m_motionManager != nil) {
        [m_motionManager release];
    }
}

unsigned char CGyroscope_Cocoa::CanSample()
{
    if (m_motionManager == nil) {
        return 0;
    }
    if (![m_motionManager isDeviceMotionActive]) {
        return 0;
    }

    int now = CStdUtil_Cocoa::GetUpTimeMS();
    if ((now - m_lastSampleTime) * m_frequency < 1000) {
        return 0;
    }
    m_lastSampleTime = now;

    CMDeviceMotion* motion = [m_motionManager deviceMotion];
    if (motion == nil) {
        return 0;
    }

    m_attitude = CVector3d(RadiansToFixed([[motion attitude] pitch]),
                           RadiansToFixed([[motion attitude] roll]),
                           RadiansToFixed([[motion attitude] yaw]));
    return 1;
}

void CGyroscope_Cocoa::SetGyroscopeFrequency(unsigned int frequency)
{
    if (m_motionManager == nil) {
        return;
    }

    m_frequency = frequency;
    BOOL active = [m_motionManager isDeviceMotionActive];
    if (frequency == 0) {
        if (active) {
            [m_motionManager stopDeviceMotionUpdates];
        }
    } else {
        if (!active) {
            [m_motionManager startDeviceMotionUpdates];
        }
    }
}
