#pragma once

// Pull in UCRT declarations before the game aliases the memory functions.
#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wchar.h>
#ifdef __cplusplus
#include <new>
#include <SwerveApi.h>
#endif

#include "NPMalloc.h"
#include "NPMem.h"

#define malloc  np_malloc
#define calloc  np_calloc
#define realloc np_realloc
#define free    np_free
#define memchr  np_memchr
#define memcmp  np_memcmp
#define memcpy  np_memcpy
#define memmove np_memmove
#define memset  np_memset
