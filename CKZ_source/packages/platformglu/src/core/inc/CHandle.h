// CHandle.h - numeric handles naming objects (header-only; created by b006, only what b006 needed).
// A named handle is CStringToKey(name) with the top bit set, so it can never collide with a
// plain numeric handle.
#ifndef CHANDLE_H
#define CHANDLE_H

#include "CStringToKey.h"

namespace com { namespace glu { namespace platform { namespace core {

class CHandle {
public:
    /** Bit marking a handle that was made from a name. */
    static const unsigned int NamedFlag = 0x80000000;

    /** Handle for the object called `name`. */
    static unsigned int FromName(const char* name)
    {
        return CStringToKey(name, 0) | NamedFlag;
    }
};

} } } }

#endif
