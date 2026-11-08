// PROVISIONAL (created by b055x): owner src/bounty_hunter/CIntermediateState.cpp not written yet
// CIntermediateState.h - transition screen between the map and the 3D game; only what src/game uses.
// b074i promoted: original 11d008/11d030 complete owner, Window base + ac frame counter, total size b0.
#ifndef CINTERMEDIATESTATE_H
#define CINTERMEDIATESTATE_H

#include "WindowBase.h"

class CIntermediateState : public Window {
public:
    CIntermediateState();
    virtual ~CIntermediateState() {} // Original 11d058/11d068 only call the Window destructor.
    virtual void Update();

    // original m_dataac[0xb0-0xac], layout not known yet (size from CGameScreen::OnCreateState), now promoted per the instruction.
    int m_updateCount; // +ac; the original constructor sets 0; when it equals 2, calls virtual SetInterrupt(0), then rereads and adds 1.
};

#endif
