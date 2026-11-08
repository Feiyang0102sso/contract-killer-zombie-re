// PROVISIONAL (created by b055x): owner src/CResBank.cpp not written yet
// CResBank.h - bank of loaded resource key sets owned by a screen (36 bytes, layout from CResBank::CResBank()).
// Restored by b063; the preceding PROVISIONAL is historical.
#ifndef CRESBANK_H
#define CRESBANK_H

#include "CClass.h"
#include "Vector_ss.h"

class CResBank {
public:
    /** Twelve-byte ownership record used by removeAllKeysets; names inferred from releases. */
    struct LoadedKeyset {
        const char* m_resourceName;
        com::glu::platform::core::CClass* m_object;
        void* m_ownedData;
    };
    CResBank();
    virtual ~CResBank();

    /** Release every key set of the bank. */
    void removeAllKeysets();

    // Former m_data04[0x24 - 0x04]: layout not known yet (0x14 count, 0x1c = 4, 0x20 buffer).
    int m_cacheState[4];                    // 0x04 unused retained state, cleared when the bank is emptied.
    Vector<LoadedKeyset> m_keysets;         // 0x14 three-pointer records, 12-byte stride.
};

#endif
