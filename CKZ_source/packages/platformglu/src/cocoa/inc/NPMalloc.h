// NPMalloc.h - platform heap entry points used by all Glu code.
#ifndef NPMALLOC_H
#define NPMALLOC_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void* np_malloc(size_t size);
void* np_calloc(size_t count, size_t size);
void* np_realloc(void* pointer, size_t size);
void np_free(void* pointer);

#ifdef __cplusplus
}
#endif

#endif
