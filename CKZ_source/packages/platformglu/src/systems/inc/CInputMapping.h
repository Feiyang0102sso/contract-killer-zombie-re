// CInputMapping.h - base of the input mappings: translates input events into messages
// (created by b011x).
#ifndef CINPUTMAPPING_H
#define CINPUTMAPPING_H

#include "CExecutable.h"

namespace com { namespace glu { namespace platform { namespace systems {

class CInputMapping : public components::CExecutable {
public:
    /** Runtime class id: CStringToKey("CInputMapping"). */
    enum { ClassId = 0x0bf096f5 };

    /** Mappings run every frame until they are removed. */
    CInputMapping();
    virtual ~CInputMapping();

    /** Start listening; returns 0 on failure. */
    // b118: Init/Destroy are introduced by CKeyInputMapping; the original base class only has the inherited OnExecute slot.

    unsigned int m_targetHandle;    // 0x1c listener handle the posted messages are sent to (0 = all)
};

} } } }

#endif
