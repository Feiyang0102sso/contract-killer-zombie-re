// CApplet.h - iOS host of the Glu application: owns the platform core and the application,
// queues system events (touch, accelerometer, gyroscope, lifecycle) and drives the frame timer.
// Its statics give global access to the running application and platform core.
// (first drafted by b005 as PROVISIONAL in cocoa/src; completed by b002, the owner)
#ifndef CAPPLET_H
#define CAPPLET_H

#include "CSystemEventQueue.h"

namespace com { namespace glu { namespace platform { namespace framework {
class CApp;
class ICCore;
} } } }

class CGyroscope_Cocoa;

class CApplet {
public:
    /** System event ids queued by the platform layer. */
    enum {
        SystemEvent_Start = 0x67542658,
        SystemEvent_Stop = 0x36754280,
        SystemEvent_Gyroscope = 0x420420ce
    };

    CApplet();
    ~CApplet();

    /** Release the application and the platform core. */
    void Destroy();
    /** Create the platform core and the application; 1 when both initialized. */
    unsigned char Initialize();
    /** Create the gyroscope and queue the start event. */
    void Start();
    /** Queue the stop event, deliver it and release the gyroscope. */
    void Stop();
    /** One frame: sample the gyroscope, deliver the queued events, run the application timer. */
    void HandleTimer();
    /** The OS terminates the application; nothing to do here. */
    void ExitApp();
    void QueueSystemEvent(unsigned int type, unsigned int param1, unsigned int param2);
    /** Accelerometer updates per second; 0 stops them. */
    void SetAccelerometerFrequency(unsigned int frequency);
    unsigned char IsGyroscopeAvailable() const;
    void SetGyroscopeFrequency(unsigned int frequency);

    static CApplet* m_pThis;                                // the running applet
    static com::glu::platform::framework::ICCore* m_pCore;  // platform core
    static com::glu::platform::framework::CApp* m_pApp;     // the running application
    static short m_UIScreenMainScreenWidth;                 // main screen size in pixels
    static short m_UIScreenMainScreenHeight;
    static void* m_eaglLayer;                               // CAEAGLLayer of the GL view

    unsigned int m_tickRate;                                        // 0x00 timer period in milliseconds
    com::glu::platform::core::CSystemEventQueue m_eventQueue;       // 0x04
    unsigned int m_accelerometerFrequency;                          // 0x14 updates per second
    unsigned char m_multipleTouchEnabled;                           // 0x18
    CGyroscope_Cocoa* m_gyroscope;                                  // 0x1c NULL until Start()
};

#endif
