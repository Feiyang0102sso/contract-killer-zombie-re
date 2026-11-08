// CAppExecutor.h - drives the application main loop: frame timing, start/stop/suspend/resume
// transitions and the registry of executables run every frame.
// InitRegistry(), ReleaseRegistry() and OnExecute() are supplied by the game (src/CGameApp.cpp).
#ifndef CAPPEXECUTOR_H
#define CAPPEXECUTOR_H

#include "CPriorityClass.h"

namespace com { namespace glu { namespace platform {

namespace systems {
class CExecutableRegistry;
}

namespace framework {

class CApp;

class CAppExecutor : public core::CPriorityClass {
public:
    enum { ClassId = 0x2e3c54f6 };

    /** How the platform drives Run(). */
    enum Method {
        Method_Polled = 0,
        Method_Timer = 1        // default: the platform timer calls Run()
    };

    /** Whether frames are timed (Mode_Running) or run untimed. */
    enum Mode {
        Mode_Running = 0,       // frame timing valid
        Mode_Resuming = 1       // first frame after a resume
    };

    /** The transition the executor is in. */
    enum ModeQualifier {
        ModeQualifier_None = 0,
        ModeQualifier_Starting = 1,
        ModeQualifier_Stopping = 2,
        ModeQualifier_Suspending = 3,
        ModeQualifier_Resuming = 4
    };

    CAppExecutor();
    virtual ~CAppExecutor();

    /** Create the executable registry; 1 on success. */
    virtual unsigned char Initialize();
    virtual void Destroy();
    virtual void SetMethod(Method method);
    virtual Method GetMethod() const;
    /** Ask the platform core for a timer of `milliseconds`. */
    virtual void SetTimerMillis(int milliseconds);
    /** One frame: update the frame time (when running) and execute. */
    virtual void Run();
    virtual void OnStart();
    virtual void OnStop();
    virtual void OnSuspend();
    virtual void OnResume();
    /** The frame's work (game supplied). */
    virtual void OnExecute();
    virtual void SetMode(Mode mode);
    virtual void SetModeQualifier(ModeQualifier qualifier);

    /** Current execution mode (Mode_Running = frame timing valid). */
    int GetMode() const { return m_mode; }
    /** Duration of the last frame in 16.16 fixed-point seconds. */
    int GetFrameTime() const { return m_frameTime; }

    /** Game hooks (src/CGameApp.cpp): register / release the game's executables; 0 on success. */
    int InitRegistry();
    void ReleaseRegistry();

    CApp* m_app;                                    // 0x0c
    int m_method;                                   // 0x10 Method
    int m_mode;                                     // 0x14 Mode
    int m_modeQualifier;                            // 0x18 ModeQualifier
    systems::CExecutableRegistry* m_registry;       // 0x1c
    unsigned char m_flag20;                         // 0x20
    int m_maxFrameMillis;                           // 0x24 clamp of a frame duration (1000)
    int m_frameMillis;                              // 0x28 last frame duration in ms
    int m_frameTime;                                // 0x2c last frame duration, 16.16 seconds
    int m_lastFrameMillis;                          // 0x30 uptime at the previous frame
};

inline void CAppExecutor::SetMode(Mode mode)
{
    m_mode = mode;
}

inline void CAppExecutor::SetModeQualifier(ModeQualifier qualifier)
{
    m_modeQualifier = qualifier;
}

} } } }

#endif
