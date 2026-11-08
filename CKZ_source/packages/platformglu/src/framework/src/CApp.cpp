// CApp.cpp - base application: default configuration, subsystem ownership, system events.
#include "CApp.h"

#include <string.h>
#include "CAppConfig.h"
#include "CAppExecutor.h"
#include "CAppProperties.h"
#include "CRegistryAccelerateHandleQuery.h"
#include "CResourceManager_v2.h"
#include "CSystemElement.h"
#include "CRegistryItr.h"
#include "CHash.h"
#include "CSingleton.h"
#include "CMessage.h"
#include "CStrChar.h"
#include "CStrWChar.h"
#include "ICGraphics.h"
#include "AppGluStrings.h"

namespace com { namespace glu { namespace platform { namespace framework {

static const wchar_t PROPERTIES_FILE[] = L"properties.dat";

/** Argument type tag of the event parameters in the system event message. */
static const unsigned int SYSTEM_EVENT_ARGUMENT_TYPE = 0x013f9702;
/** ClearBuffers() flags used when the application starts: every buffer. */
static const unsigned int CLEAR_ALL_BUFFERS = 0x70000;

CAppConfig* CApp_CreateDefaultConfig()
{
    graphics::ICGraphics::InitParam graphicsParams[1] = { { 0, 0 } };
    unsigned char flag30;   // passed on uninitialized, as in the original
    components::CStrWChar propertiesFile;
    propertiesFile = PROPERTIES_FILE;
    components::CStrWChar resourceFile;
    components::CStrChar appName(APP_GLU_NAME_STR);
    resourceFile = (appName + ".big").GetCharPtr();

    CAppConfig* config = new CAppConfig();
    // graphics mode 5, mono 16-bit 8 kHz audio, 10584-byte buffer, 15 sounds, hardware mixing
    config->Copy(5, graphicsParams, 1, 16, 8000, 10584, 15, 0, propertiesFile, flag30, resourceFile);
    return config;
}

CApp::CApp() : core::CClass(ClassId)
{
    m_config = CApp_CreateDefaultConfig();
    m_subsystem0c = NULL;
    m_executor = NULL;
    m_subsystem14 = NULL;
    m_subsystem18 = NULL;
    m_singletons = NULL;
    m_subsystem24 = NULL;
    m_initialized = 0;
    m_cached2c = NULL;
    m_resourceManager = NULL;
    m_fileMgr = NULL;
    m_cached38 = NULL;
    m_mediaPlayer = NULL;
    m_moviePlayer = NULL;
    m_graphicsAbstractionManager = NULL;
    m_cached48 = NULL;
    m_cached4c = NULL;
}

CApp::~CApp()
{
    if (m_subsystem24 != NULL) {
        delete m_subsystem24;
        m_subsystem24 = NULL;
    }
    if (m_executor != NULL) {
        delete m_executor;
        m_executor = NULL;
    }
    if (m_subsystem14 != NULL) {
        delete m_subsystem14;
        m_subsystem14 = NULL;
    }
    if (m_subsystem18 != NULL) {
        delete m_subsystem18;
        m_subsystem18 = NULL;
    }
    if (m_subsystem0c != NULL) {
        delete m_subsystem0c;
        m_subsystem0c = NULL;
    }

    // delete every singleton still registered, then the table itself
    if (m_singletons != NULL) {
        int bucketCount = m_singletons->m_bucketCount;
        for (int bucket = 0; bucket < bucketCount; bucket++) {
            for (components::CHash::Entry* entry = m_singletons->m_buckets[bucket]; entry != NULL; entry = entry->m_next) {
                if (entry->m_value != NULL) {
                    delete (components::CSingleton*)entry->m_value;
                    entry->m_value = NULL;
                }
            }
        }
        // the original re-checks the pointer (safe-delete idiom) and skips the store when NULL
        components::CHash* singletons = m_singletons;
        if (singletons != NULL) {
            delete singletons;
            m_singletons = NULL;
        }
    }

    if (m_config != NULL) {
        delete m_config;
        m_config = NULL;
    }
}

int CApp::Initialize()
{
    int result = OnInit();
    if (result == 0) {
        m_initialized = 1;
    }
    return result;
}

void CApp::Destroy()
{
    if (m_initialized) {
        OnFree();
        m_initialized = 0;
    }
}

void CApp::OnTimer()
{
    if (m_executor != NULL) {
        m_executor->Run();
    }
}

void CApp::OnSystemEvent(unsigned int eventId, unsigned int param1, unsigned int param2)
{
    // let every listener see the event first
    systems::CMessage* message = new systems::CMessage(this, eventId, 0, 2,
                                                       SYSTEM_EVENT_ARGUMENT_TYPE, 0, param1,
                                                       SYSTEM_EVENT_ARGUMENT_TYPE, 0, param2);
    message->Run();

    // b117-va: case order Start, Stop, Suspend, Resume reproduces the original block layout
    switch (eventId) {
    case SystemEvent_Start:
        graphics::ICGraphics::GetInstance()->ClearBuffers(CLEAR_ALL_BUFFERS);
        m_executor->OnStart();
        break;
    case SystemEvent_Stop:
        m_executor->OnStop();
        break;
    case SystemEvent_Suspend:
        m_executor->OnSuspend();
        break;
    case SystemEvent_Resume:
        m_executor->OnResume();
        break;
    }
}

unsigned char CApp::FrameworkHandleMessage(systems::CMessage& message)
{
    unsigned char handled = 0;
    unsigned int eventId = message.m_eventId;
    if (eventId == SystemEvent_Stop || eventId == SystemEvent_Suspend || eventId == SystemEvent_Start
        || eventId == SystemEvent_Resume) {
        handled = 1;
    }
    return handled;
}

} } } }
