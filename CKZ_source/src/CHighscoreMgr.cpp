// Retained highscore constructor/destructors, original 0x0e13f0..0x0e1428.
#include "CHighscoreMgr.h"
#include <string.h>

CHighscoreMgr::CHighscoreMgr()
{
    m_loaded = false;
    memset(m_scores, 0, sizeof(m_scores));
}
CHighscoreMgr::~CHighscoreMgr()
{
    // Original non-deleting destructor at 0x0e1424 is return-only.
}
