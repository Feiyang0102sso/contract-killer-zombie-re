// Resource-bank ownership and cleanup, original 0x0e14f0..0x0e1604.
#include "CResBank.h"
#include "App.h"
#include "CDH_ResourceManager.h"

CResBank::CResBank()
{
    m_cacheState[0] = 0;
    m_cacheState[1] = 0;
    m_cacheState[2] = 0;
    m_cacheState[3] = 0;
}
CResBank::~CResBank()
{
    removeAllKeysets();
}
void CResBank::removeAllKeysets()
{
    for (int index = 0; index < m_keysets.size(); ++index) {
        com::glu::platform::core::CClass* object = m_keysets[index].m_object;
        void* ownedData = m_keysets[index].m_ownedData;
        if (ownedData != NULL) {
            if (object != NULL) {
                delete object;
            }
            free(ownedData);
        } else {
            if (object != NULL) {
                ((App*)WindowApp::m_instance)->m_resourceManager->releaseResource(m_keysets[index].m_resourceName);
            }
        }
    }
    m_keysets.removeAllElements();
    m_cacheState[0] = 0;
    m_cacheState[1] = 0;
    m_cacheState[2] = 0;
    m_cacheState[3] = 0;
}
