// NPMem.h - platform memory block helpers used by all Glu code.
#ifndef NPMEM_H
#define NPMEM_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void* np_memchr(const void* block, int value, size_t size);
int np_memcmp(const void* left, const void* right, size_t size);
void* np_memcpy(void* destination, const void* source, size_t size);
void* np_memmove(void* destination, const void* source, size_t size);
void* np_memset(void* destination, int value, size_t size);

#ifdef __cplusplus
}
#endif

#endif
