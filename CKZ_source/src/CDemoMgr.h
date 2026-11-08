// PROVISIONAL (created by b057x): owner src/CDemoMgr.cpp not written yet
// CDemoMgr.h - demo / upgrade handling (singleton); only what src/general/App.cpp uses is declared.
// Restored by b063; the preceding PROVISIONAL is historical.
#ifndef CDEMOMGR_H
#define CDEMOMGR_H

#include <stddef.h>
#include "CApplet.h"
#include "CApp.h"
#include "CHash.h"
#include "CSingleton.h"
#include "CStrWChar.h"

class CDemoMgr : public com::glu::platform::components::CSingleton {
public:
    CDemoMgr();
    virtual ~CDemoMgr();
    enum { SingletonId = 0x723bbb0e };

    static inline CDemoMgr* GetInstance()
    {
        void* found = NULL;
        CApplet::m_pApp->m_singletons->Find(SingletonId, &found);
        CDemoMgr* instance = (CDemoMgr*)found;
        if (instance == NULL) {
            instance = new CDemoMgr();
        }
        return instance;
    }

    unsigned char Read();
    void GetDemoString(com::glu::platform::components::CStrWChar& text, const char* property, const char* fallback);
    unsigned char LaunchUpgrade();
    /** True in the demo (lite) build. */  // added by b055x
    unsigned char IsDemo();  // added by b055x (original reads the byte directly at 0x0dd504)

    // Former m_data04[0x8c - 0x4]: layout not known yet (size from App.cpp).
    unsigned char m_demoEnabled;     // 0x08
    int m_playLimit;                 // 0x0c
    int m_timeLimit;                 // 0x10
    int m_gameLimit;                 // 0x14
    com::glu::platform::components::CStrWChar m_upgradeUrl; // 0x18
    com::glu::platform::components::CStrWChar m_unusedText28; // 0x28..0x78 retained strings; meanings not observed.
    com::glu::platform::components::CStrWChar m_unusedText38;
    com::glu::platform::components::CStrWChar m_unusedText48;
    com::glu::platform::components::CStrWChar m_unusedText58;
    com::glu::platform::components::CStrWChar m_unusedText68;
    com::glu::platform::components::CStrWChar m_unusedText78;
    unsigned char m_flag88;          // 0x88 initialized zero; no retained use.
};

#endif
