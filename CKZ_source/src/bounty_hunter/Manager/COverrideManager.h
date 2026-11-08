// PROVISIONAL (created by b057x): owner src/bounty_hunter/Manager/COverrideManager.cpp not written yet
// COverrideManager.h - server overrides of the game config; only what src/general/App.cpp uses is declared.
// b078a: constructor and CRC-based reload restored; the historical notes above are retained.
#ifndef COVERRIDEMANAGER_H
#define COVERRIDEMANAGER_H

#include <stddef.h>
#include "tinyxml.h"

class COverrideManager {
public:
    COverrideManager();
    void ReloadConfig();

    int m_configCrc;                  // 0x00
    TiXmlDocument m_document;       // 0x04
};

#endif
