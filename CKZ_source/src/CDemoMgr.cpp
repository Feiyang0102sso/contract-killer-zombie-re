// Demo properties and upgrade URL, original 0x0dd154..0x0dd538.
#include "CDemoMgr.h"
#include "CAppProperties.h"
#include "CUtility.h"
#include "ICCore.h"
#include <stdlib.h>
#include <string.h>

using namespace com::glu::platform;
using namespace com::glu::platform::components;

// These exported property-key arrays belong to the platform configuration data owner.
namespace com { namespace glu { namespace platform { namespace components {
extern const char JADKEY__DEMO_ENABLED[];
extern const char JADKEY__DEMO_PLAY_LIMIT[];
extern const char JADKEY__DEMO_TIME_LIMIT[];
extern const char JADKEY__DEMO_GAME_LIMIT[];
extern const char JADKEY__DEMO_URL[];
} } } }


CDemoMgr::CDemoMgr() : CSingleton(SingletonId)
{
    m_demoEnabled = 0;
    m_playLimit = 0;
    m_timeLimit = 0;
    m_gameLimit = 0;
    m_flag88 = 0;
}
CDemoMgr::~CDemoMgr()
{
    // Seven string members release in reverse order; the base unregisters the singleton.
}
unsigned char CDemoMgr::Read()
{
    void* found = NULL;
    CApplet::m_pApp->m_singletons->Find(CAppProperties::SingletonId, &found);
    CAppProperties* instance = (CAppProperties*)found;
    if (instance == NULL) {
        instance = new CAppProperties();
    }
    CProperties& properties = instance->m_properties;
    m_demoEnabled = properties.GetBool(JADKEY__DEMO_ENABLED);
    m_playLimit = properties.GetInt(JADKEY__DEMO_PLAY_LIMIT);
    m_timeLimit = properties.GetInt(JADKEY__DEMO_TIME_LIMIT);
    m_gameLimit = properties.GetInt(JADKEY__DEMO_GAME_LIMIT);
    GetDemoString(m_upgradeUrl, JADKEY__DEMO_URL, NULL);
    return 1;
}
void CDemoMgr::GetDemoString(CStrWChar& text, const char* property, const char* fallback)
{
    text = "";
    void* found = NULL;
    CApplet::m_pApp->m_singletons->Find(CAppProperties::SingletonId, &found);
    CAppProperties* instance = (CAppProperties*)found;
    if (instance == NULL) {
        instance = new CAppProperties();
    }
    const char* value = instance->m_properties.GetStr(property);
    if (value != NULL && strlen(value) != 0) {
        wchar_t* wide = new wchar_t[strlen(value) + 1];
        if (wide != NULL) {
            mbstowcs(wide, value, strlen(value) + 1);
            text = wide;
            delete[] wide;
        }
    }
    if (text.GetLength() == 0 && fallback != NULL) {
        CUtility::GetString(text, fallback);
    }
}
unsigned char CDemoMgr::IsDemo() { return m_demoEnabled; }
unsigned char CDemoMgr::LaunchUpgrade()
{
    unsigned char result = 0;
    if (m_upgradeUrl.GetLength() > 0) {
        result = CApplet::m_pCore->LaunchURL(m_upgradeUrl.GetWCharPtr(), 0);
    }
    return result;
}
