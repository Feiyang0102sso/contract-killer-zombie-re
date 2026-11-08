// CPriorityClass.h - CClass with an ordering priority (registries keep their elements sorted
// by it; lower values run first).
#ifndef CPRIORITYCLASS_H
#define CPRIORITYCLASS_H

#include "CClass.h"

namespace com { namespace glu { namespace platform { namespace core {

class CPriorityClass : public CClass {
public:
    /** Priority of objects that never set one. */
    enum { DefaultPriority = 0x3fffffff };

    CPriorityClass()
    {
        m_priority = DefaultPriority;
    }

    int GetPriority() const
    {
        return m_priority;
    }

    void SetPriority(int priority)
    {
        m_priority = priority;
    }

    int m_priority;     // 0x08
};

} } } }

#endif
