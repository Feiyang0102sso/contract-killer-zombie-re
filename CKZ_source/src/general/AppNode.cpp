// AppNode.cpp - node of the application node tree that hosts the game application (App) and
// forwards the platform events, updates and rendering to it.
#include "AppNode.h"
#include "App.h"

// Platform events (CStringToKey of the platform event names; names known only for some).
static const unsigned int EVENT_SYSTEM_START = 0x67542658;
static const unsigned int EVENT_SYSTEM_STOP = 0x36754280;
static const unsigned int EVENT_APP_RESUME = 0x75424363;
static const unsigned int EVENT_KEY_PRESSED = 0xc7424be6;
static const unsigned int EVENT_KEY_RELEASED = 0x43a39818;
static const unsigned int EVENT_ACCELEROMETER = 0xad174cef;

AppNode::AppNode()
{
    m_app = NULL;
}

AppNode::~AppNode()
{
    // The original body was not empty (clang keeps the vtable store of a destructor only then);
    // whatever it held compiled to nothing in the release build (e.g. a disabled debug macro).
    ;
}

int AppNode::Start()
{
    int result = CNode::Start();
    m_app = new App(this);
    m_app->HandleCreate();
    return result;
}

void AppNode::Stop()
{
    m_app->HandleDestroy();
    if (m_app != NULL) {
        delete m_app;
        m_app = NULL;
    }
    CNode::Stop();
}

void AppNode::Activate()
{
    CNode::Activate();
}

void AppNode::Deactivate()
{
    CNode::Deactivate();
}

int AppNode::HandleEvent(unsigned int eventId, unsigned int param1, unsigned int param2)
{
    if (m_app == NULL) {
        return 1;
    }
    switch (eventId) {
    case EVENT_SYSTEM_START:
    case EVENT_SYSTEM_STOP:
    case 0x75794125:
    case 0x518e1a3b:
    case 0x24314258:
    case 0x7b8428cd: {
        // The original stores 2 into an otherwise unused stack variable here (kept only when it is volatile).
        volatile int ignored = 2;
        break;
    }
    case 0x8408f369:
    case 0x54246de3:
    case 0x5152a73b:
        m_app->HandlePause();
        break;
    case EVENT_APP_RESUME:
    case 0x6740942f:
        m_app->HandleResume();
        break;
    case 0x1e96b3a4:
        m_app->HandleEnterBackground();
        break;
    case 0x0096b3eb:
        m_app->HandleEnterForeground();
        break;
    case EVENT_KEY_PRESSED:
        WindowApp::PutEvent(Event::KEY_PRESSED, param1, 0, 0, 0);
        break;
    case EVENT_KEY_RELEASED:
        WindowApp::PutEvent(Event::KEY_RELEASED, param1, 0, 0, 0);
        break;
    // single touch: position packed as 16 + 16 bits
    case 0x8251b8de:
        if (WindowApp::PointerCapture(0) != NULL) {
            WindowApp::PutEvent(Event::POINTER_DRAGGED, 0, param2 & 0xffff, param2 >> 16, -1);
        }
        break;
    case 0x8918794c:
        WindowApp::PutEvent(Event::POINTER_PRESSED, 0, param2 & 0xffff, param2 >> 16, -1);
        break;
    case 0x52a91971:
        WindowApp::PutEvent(Event::POINTER_RELEASED, 0, param2 & 0xffff, param2 >> 16, -1);
        break;
    // multi touch: pointer index in param1, position packed as 14 + 14 bits
    case 0x35c43e56:
        WindowApp::PutEvent(Event::POINTER_PRESSED, param1, param2 & 0x3fff, (param2 >> 14) & 0x3fff, -1);
        break;
    case 0x35cb8f1c:
        WindowApp::PutEvent(Event::POINTER_DRAGGED, param1, param2 & 0x3fff, (param2 >> 14) & 0x3fff, -1);
        break;
    case 0x35c38d1c:
    case 0x5707b968:
        WindowApp::PutEvent(Event::POINTER_RELEASED, param1, param2 & 0x3fff, (param2 >> 14) & 0x3fff, -1);
        break;
    case EVENT_ACCELEROMETER: {
        // three signed 21-bit 16.16 fixed point values packed into param1:param2
        int fixedX = (int)(param1 << 11) >> 11;
        int fixedY = (int)((param2 << 22) | (param1 >> 10)) >> 11;
        int fixedZ = (int)(param2 << 1) >> 11;
        float x = (float)fixedX / 65536.0f;
        float y = (float)fixedY / 65536.0f;
        float z = (float)fixedZ / 65536.0f;
        // the command parameters carry the raw float bits
        WindowApp::PutEvent(Event::COMMAND, EVENT_ACCELEROMETER, *(int*)&x, *(int*)&y, *(int*)&z);
        break;
    }
    case 0x71fd83f5:
    case 0x7214ff0d:
        WindowApp::PutEvent(Event::COMMAND, 0x7214ff0d, 0, 0, 0);
        break;
    case 0x20174fb7:
    case 0x3f972120:
        WindowApp::PutEvent(Event::COMMAND, 0x3f972120, 0, 0, 0);
        break;
    case 0x172e9965:
    case 0x2074d009:
        WindowApp::PutEvent(Event::COMMAND, 0x2074d009, 0, 0, 0);
        break;
    case 0x172ebf3e:
    case 0x972028c1:
        WindowApp::PutEvent(Event::COMMAND, 0x972028c1, 0, 0, 0);
        break;
    case 0x172ec5fa:
    case 0x97204784:
        WindowApp::PutEvent(Event::COMMAND, 0x97204784, 0, 0, 0);
        break;
    case 0x172e3434:
    case 0x9720a3cb:
        WindowApp::PutEvent(Event::COMMAND, 0x9720a3cb, 0, 0, 0);
        break;
    case 0x9720b34a:
        WindowApp::PutEvent(Event::COMMAND, 0x9720b34a, 0, 0, 0);
        break;
    case 0x97200e9a:
        WindowApp::PutEvent(Event::COMMAND, 0x97200e9a, 0, 0, 0);
        break;
    case 0x5f61a84a:
        WindowApp::PutEvent(Event::COMMAND, 0x5f61a84a, 0, 0, 0);
        break;
    case 0xd45f52de:
        WindowApp::PutEvent(Event::COMMAND, 0xd45f52de, 0, 0, 0);
        break;
    case 0xfd4836a5:
        WindowApp::PutEvent(Event::COMMAND, 0xfd4836a5, 0, 0, 0);
        break;
    case 0x747ab255:
        m_app->HandleDeviceOrientationChanged();
        // and forward it as a command as well
    default:
        WindowApp::PutEvent(Event::COMMAND, eventId, 0, 0, 0);
        break;
    }
    return 1;
}

int AppNode::HandleUpdate(int elapsed)
{
    m_app->HandleUpdate(elapsed);
    return 1;
}

int AppNode::HandleRender()
{
    RenderBegin();
    m_app->HandleRender();
    RenderEnd();
    return 1;
}

void AppNode::SetInterrupt(int interrupt)
{
    CNode::SetInterrupt(interrupt);
}

int AppNode::GetInterrupt()
{
    return CNode::GetInterrupt();
}

void AppNode::ClearInterrupt()
{
    CNode::ClearInterrupt();
}
