// CApp.h - base application: owns the configuration, the executor, the subsystems, the
// singleton table and cached platform interfaces. The game derives from it (CGameApp) and
// supplies OnInit()/OnFree() and CApp::CreateInstance().
#ifndef CAPP_H
#define CAPP_H

#include "CClass.h"

namespace com { namespace glu { namespace platform {

namespace components {
class CHash;
class ICFileMgr;
class ICMediaPlayer;
class ICMoviePlayer;  // added by fx2
}

namespace systems {
class CMessage;
}

namespace framework {

class CAppConfig;
class CAppExecutor;

/** Default start-up configuration (audio format, properties file, "<app>.big" archive). */
CAppConfig* CApp_CreateDefaultConfig();

class CApp : public core::CClass {
public:
    enum { ClassId = 0x0275a643 };

    /** System event ids passed to OnSystemEvent() by the platform layer. */
    enum SystemEvent {
        SystemEvent_Start = 0x67542658,
        SystemEvent_Stop = 0x36754280,
        SystemEvent_Suspend = 0x54246de3,
        SystemEvent_Resume = 0x75424363
    };

    /** The game's application object (src/CGameApp.cpp). */
    static CApp* CreateInstance();

    CApp();
    virtual ~CApp();

    /** Run OnInit(); 0 on success (the application is then initialized). */
    virtual int Initialize();
    /** Run OnFree() when initialized. */
    virtual void Destroy();
    /** Platform timer tick: run one frame of the executor. */
    virtual void OnTimer();
    /** Broadcast a system event as a message and drive the executor transitions. */
    virtual void OnSystemEvent(unsigned int eventId, unsigned int param1, unsigned int param2);
    /** Game start-up; 0 on success. */
    virtual int OnInit() = 0;
    virtual void OnFree() = 0;
    virtual void OnUpdate() {}
    virtual void OnRender() {}
    /** True for the system events the framework itself handles. */
    virtual unsigned char FrameworkHandleMessage(systems::CMessage& message);

    CAppExecutor* GetExecutor() const { return m_executor; }

    CAppConfig* m_config;                           // 0x08
    core::CClass* m_subsystem0c;                    // 0x0c owned, deleted last of the subsystems
    CAppExecutor* m_executor;                       // 0x10 owned
    core::CClass* m_subsystem14;                    // 0x14 owned
    core::CClass* m_subsystem18;                    // 0x18 owned
    void* m_subsystem1c;                            // 0x1c not touched by CApp
    components::CHash* m_singletons;                // 0x20 CSingleton id -> instance, owned
    core::CClass* m_subsystem24;                    // 0x24 owned, deleted first
    unsigned char m_initialized;                    // 0x28 OnInit() succeeded
    void* m_cached2c;                               // 0x2c
    void* m_resourceManager;                        // 0x30 ICResourceManager* cache
    components::ICFileMgr* m_fileMgr;               // 0x34 ICFileMgr* cache
    void* m_cached38;                               // 0x38 ICLicenseMgr* cache
    components::ICMediaPlayer* m_mediaPlayer;       // 0x3c ICMediaPlayer* cache
    components::ICMoviePlayer* m_moviePlayer;       // 0x40 ICMoviePlayer* cache  // changed by fx2 (was void*)
    void* m_graphicsAbstractionManager;             // 0x44 ICGraphicsAbstractionManager* cache
    void* m_cached48;                               // 0x48
    void* m_cached4c;                               // 0x4c
};

} } } }

#endif
