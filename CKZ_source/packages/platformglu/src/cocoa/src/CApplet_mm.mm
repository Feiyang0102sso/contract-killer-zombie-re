// CApplet_mm.mm - iOS host of the Glu application (see CApplet.h).
#include "CApplet.h"
#include "CApp.h"
#include "ICCore.h"
#include "CGyroscope_Cocoa.h"

using namespace com::glu::platform::core;
using namespace com::glu::platform::framework;

CApplet* CApplet::m_pThis;
ICCore* CApplet::m_pCore;
CApp* CApplet::m_pApp;
short CApplet::m_UIScreenMainScreenWidth;
short CApplet::m_UIScreenMainScreenHeight;
void* CApplet::m_eaglLayer;

CApplet::CApplet()
{
    m_pThis = this;
    m_pApp = NULL;
    m_pCore = NULL;
    m_tickRate = 1;
    m_accelerometerFrequency = 0;
    m_multipleTouchEnabled = 0;
    m_gyroscope = NULL;
}

CApplet::~CApplet()
{
    Destroy();
    m_pThis = NULL;
}

void CApplet::Destroy()
{
    if (m_pApp != NULL) {
        m_pApp->Destroy();
    }
    if (m_pCore != NULL) {
        delete m_pCore;
        m_pCore = NULL;
    }
    if (m_pApp != NULL) {
        delete m_pApp;
        m_pApp = NULL;
    }
}

unsigned char CApplet::Initialize()
{
    m_pCore = ICCore::CreateInstance();
    m_pApp = CApp::CreateInstance();
    if (m_pCore == NULL || m_pApp == NULL) {
        return 0;
    }
    return m_pApp->Initialize() == 0;
}

void CApplet::Start()
{
    m_gyroscope = CGyroscope_Cocoa::Create();
    m_eventQueue.Queue(SystemEvent_Start, 0, 0);
}

void CApplet::Stop()
{
    m_eventQueue.Queue(SystemEvent_Stop, 0, 0);
    HandleTimer();
    if (m_gyroscope != NULL) {
        delete m_gyroscope;
        m_gyroscope = NULL;
    }
}

void CApplet::HandleTimer()
{
    // gyroscope: three 21-bit attitude angles packed into the two event parameters
    if (m_gyroscope != NULL && m_gyroscope->CanSample()) {
        int pitch = (int)m_gyroscope->GetAttitude().x;
        int roll = (int)m_gyroscope->GetAttitude().y;
        int yaw = (int)m_gyroscope->GetAttitude().z;
        unsigned int param1 = (pitch & 0x1fffff) | (roll << 21);
        unsigned int param2 = ((roll & 0x1fffff) >> 11) | ((yaw & 0x1fffff) << 10);
        m_eventQueue.Queue(SystemEvent_Gyroscope, param1, param2);
    }

    // deliver everything queued since the last frame
    for (int index = 0; index < m_eventQueue.GetCount(); index++) {
        CSystemEventQueue::SystemEvent event;
        event.type = 0;
        event.param1 = 0;
        event.param2 = 0;
        m_eventQueue.GetEvent(event, index);
        m_pApp->OnSystemEvent(event.type, event.param1, event.param2);
    }
    m_eventQueue.Clear();

    m_pApp->OnTimer();
}

void CApplet::ExitApp()
{
}

void CApplet::QueueSystemEvent(unsigned int type, unsigned int param1, unsigned int param2)
{
    m_eventQueue.Queue(type, param1, param2);
}

void CApplet::SetAccelerometerFrequency(unsigned int frequency)
{
    m_accelerometerFrequency = frequency;
    if (frequency != 0) {
        [[UIAccelerometer sharedAccelerometer] setUpdateInterval:1.0 / m_accelerometerFrequency];
        [[UIAccelerometer sharedAccelerometer] setDelegate:(id<UIAccelerometerDelegate>)[[UIApplication sharedApplication] delegate]];
    } else {
        [[UIAccelerometer sharedAccelerometer] setDelegate:nil];
    }
}

unsigned char CApplet::IsGyroscopeAvailable() const
{
    if (m_gyroscope == NULL) {
        return 0;
    }
    return m_gyroscope->IsAvailable();
}

void CApplet::SetGyroscopeFrequency(unsigned int frequency)
{
    if (m_gyroscope != NULL) {
        m_gyroscope->SetGyroscopeFrequency(frequency);
    }
}
