// PROVISIONAL (created by b057x): owner src/CHighscoreMgr.cpp not written yet
// CHighscoreMgr.h - highscores; only what src/general/App.cpp uses is declared.
// Restored by b063; original retained body only initializes the flag and five scores.
#ifndef CHIGHSCOREMGR_H
#define CHIGHSCOREMGR_H

#include <stddef.h>

class CHighscoreMgr {
public:
    CHighscoreMgr();
    virtual ~CHighscoreMgr();

    // Former m_data04[0x1c - 0x4]: layout not known yet (size from App.cpp).
    bool m_loaded;             // 0x04 initially false; name inferred from the flag.
    int m_scores[5];           // 0x08 original memset covers exactly 20 bytes.
};

#endif
