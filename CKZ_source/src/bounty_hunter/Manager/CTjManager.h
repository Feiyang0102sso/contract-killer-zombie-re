// PROVISIONAL (created by b057x): owner src/bounty_hunter/Manager/CTjManager.cpp not written yet
// CTjManager.h - Tapjoy offers (singleton); only what src/general/App.cpp uses is declared.
// Restored by b079: singleton lifetime and featured-offer state (0x2c bytes).
#ifndef CTJMANAGER_H
#define CTJMANAGER_H

#include <stddef.h>
#include "CApplet.h"
#include "CApp.h"
#include "CHash.h"
#include "CSingleton.h"
#include "xString.h"

class CTjManager : public com::glu::platform::components::CSingleton {
public:
    /** Four shared strings and the reward count; descriptive name inferred from accesses. */
    struct FeaturedAppData {
        XString m_name;
        XString m_description;
        XString m_imageUrl;
        XString m_redirectUrl;
        int m_amount;
    };

    CTjManager();
    inline virtual ~CTjManager() {}
    enum { SingletonId = 0x70770b0e };

    static inline CTjManager* GetInstance()
    {
        void* found = NULL;
        CApplet::m_pApp->m_singletons->Find(SingletonId, &found);
        CTjManager* instance = (CTjManager*)found;
        if (instance == NULL) {
            instance = new CTjManager();
        }
        return instance;
    }

    void OnAppEnterBackground();
    void Load();
    void Save();
    /** The shipped function consists only of a return instruction. */
    void CheckFeaturedAppCondition();
    FeaturedAppData& GetFeaturedAppInfo();
    void Update(int elapsed);

    // Former m_data04[0x2c - 0x4]: layout not known yet (size from App.cpp).
    int m_queryState;              // 0x08 deliberately not initialized by the constructor
    FeaturedAppData m_featured;    // 0x0c reward count is not initialized until a reply exists
    int m_backgroundEntries;       // 0x20
    int m_elapsed;                 // 0x24
    bool m_shownAtLevelFour;       // 0x28
};

#endif
