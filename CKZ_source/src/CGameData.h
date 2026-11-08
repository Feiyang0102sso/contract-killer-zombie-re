// PROVISIONAL (created by b055x): owner src/CGameData.cpp not written yet
// CGameData.h - game wide data singleton (20 bytes, layout from CGameData::CGameData()).
// Restored by b063; the preceding PROVISIONAL is historical.
#ifndef CGAMEDATA_H
#define CGAMEDATA_H

#include <stddef.h>
#include "CSingleton.h"

class CResBank;

class CGameData : public com::glu::platform::components::CSingleton {
public:
    enum { SingletonId = 0x17b126f6 };

    CGameData();
    virtual ~CGameData();

    /** The registered instance, created on first use (the constructor registers it). */
    static inline CGameData* GetInstance()
    {
        void* found = NULL;
        CApplet::m_pApp->m_singletons->Find(SingletonId, &found);
        CGameData* instance = (CGameData*)found;
        if (instance == NULL) {
            instance = new CGameData();
        }
        return instance;
    }

    CResBank* m_resBank;        // 0x08 resource bank of the active scene (set by CDH_RootNode::Init)
    bool m_field0c;             // 0x0c
    bool m_field0d;             // 0x0d
    int m_field10;              // 0x10
};

#endif
