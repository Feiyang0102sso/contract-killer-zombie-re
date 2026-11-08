// PROVISIONAL (created by b057x): owner src/bounty_hunter/Manager/CTipManager.cpp not written yet
// CTipManager.h - loading tips; only what src/general/App.cpp uses is declared.
// Restored by b079: owns the XML tips in a 16-byte Vector (total size 0x14).
#ifndef CTIPMANAGER_H
#define CTIPMANAGER_H

#include <stddef.h>
#include "Vector_ss.h"

class CTip;

class CTipManager {
public:
    CTipManager();
    virtual ~CTipManager();

    /** NULL when no tips exist; otherwise choose a clamped random index. */
    CTip* GetRandomTip() const;

    // Former m_data04[0x14 - 0x4]: layout not known yet (size from App.cpp).
    Vector<CTip*> m_tips;                  // 0x04
};

#endif
