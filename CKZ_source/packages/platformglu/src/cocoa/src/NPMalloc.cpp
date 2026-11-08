// NPMalloc.cpp - platform heap: thin checked wrappers over the C library allocator.
// the prefix header maps the libc names to np_*: undo that here to reach the real allocator
#undef malloc
#undef calloc
#undef realloc
#undef free
#include <stdlib.h>
#include "NPMalloc.h"

void* np_malloc(size_t size)
{
    if (size == 0) {
        return 0;
    }
    return malloc(size);
}

void* np_calloc(size_t count, size_t size)
{
    if (count == 0 || size == 0) {
        return 0;
    }
    return calloc(count, size);
}

void* np_realloc(void* pointer, size_t size)
{
    return realloc(pointer, size);
}

void np_free(void* pointer)
{
    if (pointer != 0) {
        free(pointer);
    }
}
