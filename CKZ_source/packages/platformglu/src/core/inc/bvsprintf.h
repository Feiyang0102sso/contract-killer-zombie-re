// bvsprintf.h - bounded printf-style formatting into char / wchar_t buffers.
// Supported: flags "-+ #0", width and precision (digits or '*'), qualifiers h/l/L (ignored),
// conversions c s U d i u o x X p n and "%%". Every argument is read as one 32-bit word.
#ifndef BVSPRINTF_H
#define BVSPRINTF_H

#include <stdarg.h>
#include <wchar.h>

/**
 * Format into `buffer`, writing at most `bufferSize - 1` characters plus the terminator.
 * Returns the number of characters written (terminator excluded); 0 when buffer or format is NULL.
 */
int bvsprintf_s(char* buffer, int bufferSize, const char* format, va_list args);
/** Wide version of bvsprintf_s; `bufferSize` counts wchar_t. */
int bwsprintf_s(wchar_t* buffer, int bufferSize, const wchar_t* format, va_list args);
/** Unbounded bvsprintf_s. */
int bvsprintf(char* buffer, const char* format, va_list args);
/** Unbounded bwsprintf_s. */
int bwsprintf(wchar_t* buffer, const wchar_t* format, va_list args);

#endif
