// NPNew.h - global operator new/delete routed to the platform heap (inlined at every call site).
#ifndef NPNEW_H
#define NPNEW_H

#include <stddef.h>
#include "NPMalloc.h"

inline void* operator new(size_t size)
{
    return np_malloc(size);
}

inline void* operator new[](size_t size)
{
    return np_malloc(size);
}

inline void operator delete(void* pointer)
{
    np_free(pointer);
}

inline void operator delete[](void* pointer)
{
    np_free(pointer);
}

// Placement new as the Glu code base declares it: no exception specification, so the result is not
// null-checked before the constructor runs (<new>'s throw() version would be). Found by b006 (CVector).
#ifndef GLU_PLACEMENT_NEW_DEFINED
#define GLU_PLACEMENT_NEW_DEFINED
inline void* operator new(size_t size, void* where)
{
    return where;
}
#endif

#endif
