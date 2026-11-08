// CExecutableRegistry.h - registry whose OnExecute() runs every registered executable.
// (first drafted by b000x as PROVISIONAL; completed by b011x, the owner)
#ifndef CEXECUTABLEREGISTRY_H
#define CEXECUTABLEREGISTRY_H

#include "CRegistry.h"

namespace com { namespace glu { namespace platform { namespace systems {

class CExecutableRegistry : public CRegistry {
public:
    /** Runtime class id: CStringToKey("CExecutableRegistry"). */
    enum { ClassId = 0x773d40d7 };

    CExecutableRegistry()
    {
        m_classId = ClassId;
    }

    virtual ~CExecutableRegistry() {}

    /**
     * Run every element's CExecutable once. An element without object names a resource whose
     * data is the executable; executables that deleted themselves are unregistered.
     */
    virtual void OnExecute();
};

} } } }

#endif
