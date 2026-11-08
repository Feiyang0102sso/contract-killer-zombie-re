// CInputMap.h - executable registry of input mappings (created by b011x).
// A CInputMap is registered in the application executor's registry under a handle; input
// mappings (e.g. CKeyInputMapping) register into it and run every frame with it.
#ifndef CINPUTMAP_H
#define CINPUTMAP_H

#include "CExecutableRegistry.h"

namespace com { namespace glu { namespace platform { namespace systems {

class CInputMap : public CExecutableRegistry {
public:
    /** Runtime class id: CStringToKey("CInputMap"). */
    enum { ClassId = 0xf9720bb7 };

    CInputMap();
    virtual ~CInputMap();

    /** Delete every registered mapping together with its element. */
    void UnregisterAll();
};

} } } }

#endif
