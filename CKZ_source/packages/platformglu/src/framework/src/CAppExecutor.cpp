// CAppExecutor.cpp - application main loop driver.
#include "CAppExecutor.h"

#include "CApplet.h"
#include "CApp.h"
#include "ICCore.h"
#include "CExecutableRegistry.h"
#include "CStdUtil_Cocoa.h"

namespace com { namespace glu { namespace platform { namespace framework {

CAppExecutor::CAppExecutor()
{
    m_classId = ClassId;
    m_app = NULL;
    m_method = Method_Timer;
    m_mode = Mode_Running;
    m_modeQualifier = ModeQualifier_Starting;
    m_registry = NULL;
    m_flag20 = 0;
    m_maxFrameMillis = 1000;
    m_frameMillis = 0;
    m_frameTime = 0;
    m_lastFrameMillis = 0;
}

CAppExecutor::~CAppExecutor()
{
    Destroy();
}

void CAppExecutor::Destroy()
{
    if (m_registry != NULL) {
        ReleaseRegistry();
        if (m_registry != NULL) {
            delete m_registry;
        }
        m_registry = NULL;
    }
    m_app = NULL;
}

unsigned char CAppExecutor::Initialize()
{
    m_app = CApplet::m_pApp;
    m_registry = new systems::CExecutableRegistry();
    if (m_registry == NULL) {
        return 0;
    }
    return InitRegistry() == 0;
}

void CAppExecutor::SetMethod(Method method)
{
    m_method = method;
}

CAppExecutor::Method CAppExecutor::GetMethod() const
{
    return (Method)m_method;
}

void CAppExecutor::SetTimerMillis(int milliseconds)
{
    ICCore* core = (ICCore*)CApplet::m_pCore;
    core->SetTickRate(milliseconds);
}

void CAppExecutor::Run()
{
    int now = 0;
    int mode = m_mode;

    if (mode == Mode_Running) {
        now = CStdUtil_Cocoa::GetUpTimeMS();
        int elapsed = now - m_lastFrameMillis;
        int frameMillis = m_maxFrameMillis;
        if (elapsed < frameMillis) {
            frameMillis = elapsed;
        }
        m_frameMillis = frameMillis;
        m_frameTime = (frameMillis << 16) / 1000;
    }

    OnExecute();

    if (mode == Mode_Running) {
        m_lastFrameMillis = now;
    }
}

void CAppExecutor::OnStart()
{
    SetMode(Mode_Running);
    SetModeQualifier(ModeQualifier_Starting);
    m_lastFrameMillis = CStdUtil_Cocoa::GetUpTimeMS();
    Run();
    SetModeQualifier(ModeQualifier_None);
}

void CAppExecutor::OnStop()
{
    SetMode(Mode_Running);
    SetModeQualifier(ModeQualifier_Stopping);
    Run();
}

void CAppExecutor::OnSuspend()
{
    SetMode(Mode_Running);
    SetModeQualifier(ModeQualifier_Suspending);
    Run();
}

void CAppExecutor::OnResume()
{
    SetMode(Mode_Resuming);
    SetModeQualifier(ModeQualifier_Resuming);
    m_lastFrameMillis = CStdUtil_Cocoa::GetUpTimeMS();
    Run();
    SetModeQualifier(ModeQualifier_None);
}

} } } }
