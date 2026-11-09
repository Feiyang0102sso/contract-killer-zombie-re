// COverrideManager.cpp - reparse the shop overrides only when their CRC changes.
#include "COverrideManager.h"
#include "App.h"
#include "ResMap.h"
#include "CCrc32.h"

COverrideManager::COverrideManager()
    : m_configCrc(0)
{
    ReloadConfig();
}

void COverrideManager::ReloadConfig()
{
    ResMap* resources = ((App*)WindowApp::m_instance)->m_resMap;
    char* text = resources->loadText(XString(L"res/common/configs/shop_override.xml"));
    if (text != NULL) {
        unsigned int length = 0;
        while (text[length] != '\0') {
            length++;
        }
        unsigned int crc = com::glu::platform::components::CCrc32::GetInstance()->Crc32((const unsigned char*)text, length);
        if ((unsigned int)m_configCrc != crc) {
            m_document.Clear();
            m_document.Parse(text);
            m_configCrc = crc;
        }
        ::operator delete[](text);
    }
}
