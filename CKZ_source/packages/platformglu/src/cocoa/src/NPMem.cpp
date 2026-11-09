// NPMem.cpp - platform memory block helpers over the C library.
// the prefix header maps the libc names to np_*: undo that here to reach the real functions
#undef memchr
#undef memcmp
#undef memcpy
#undef memmove
#undef memset
#include <string.h>
#include "NPMem.h"

void* np_memchr(const void* block, int value, size_t size)
{
    return const_cast<void*>(memchr(block, value, size)); // PORT: preserve the original C return type.
}

int np_memcmp(const void* left, const void* right, size_t size)
{
    return memcmp(left, right, size);
}

void* np_memcpy(void* destination, const void* source, size_t size)
{
    memcpy(destination, source, size);
    return destination;
}

void* np_memmove(void* destination, const void* source, size_t size)
{
    memmove(destination, source, size);
    return destination;
}

void* np_memset(void* destination, int value, size_t size)
{
    memset(destination, (unsigned char)value, size);
    return destination;
}
