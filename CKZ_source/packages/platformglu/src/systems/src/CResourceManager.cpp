// CResourceManager.cpp - resource manager interface and the common manager base.
#include "CResourceManager.h"

#include "CEventListener.h"
#include "CAppFactory.h"  // shared with the real owner, CGameApp.cpp (b063)

namespace com { namespace glu { namespace platform {

/** Game-specific factories (implemented by the game, src/CGameApp.cpp). */
// Declaration now comes from the common CAppFactory.h; the original comment is retained above.

namespace systems {

const unsigned long ICResourceManager::DefaultTableSize = 1500;
const unsigned long ICResourceManager::DefaultNumOfElementsInInitialAlloc = 150;

unsigned char CResourceManagerLegacy::m_dbgLoggingEnabled = 1;
unsigned char CResourceManagerLegacy::m_dbgVerboseLoggingEnabled = 1;

ICResourceManager* ICResourceManager::CreateInstance()
{
    return framework::CAppFactory::CreateResourceManager();
}

ICResourceManager::~ICResourceManager()
{
}

CResourceManagerLegacy::~CResourceManagerLegacy()
{
}

void CResourceManagerLegacy::Destroy()
{
    if (m_debugListener != NULL) {
        delete m_debugListener;
        m_debugListener = NULL;
    }
    m_resources.Destroy();
    m_router = NULL;
    m_version = "";
    m_versionMajor = 0;
    m_versionMinor = 0;
}

unsigned char CResourceManagerLegacy::GetResource(unsigned int handle, CResource** resource)
{
    *resource = NULL;
    if (m_resources.Find(handle, (void**)resource)) {
        return 1;
    }
    return 0;
}

const components::CStrWChar& CResourceManagerLegacy::GetVersion() const
{
    return m_version;
}

void CResourceManagerLegacy::GetVersion(unsigned int& major, unsigned int& minor) const
{
    major = m_versionMajor;
    minor = m_versionMinor;
}

} } } }
