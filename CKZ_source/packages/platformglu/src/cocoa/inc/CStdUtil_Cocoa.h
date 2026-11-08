// CStdUtil_Cocoa.h - iOS implementation of the standard utility services (time, wide strings, printf).
// Implemented in CStdUtil_Cocoa.cpp (time, compare, printf) and CStdUtil_Cocoa_mm.mm (case mapping via NSString).
#ifndef CSTDUTIL_COCOA_H
#define CSTDUTIL_COCOA_H

#include <wchar.h>

class CStdUtil_Cocoa {
public:
    /** Wall clock seconds since the epoch (gettimeofday tv_sec). */
    static long GetTimeSeconds();
    /** Milliseconds since the first call (mach_absolute_time based, monotonic). */
    static int GetUpTimeMS();
    /** Case-insensitive wide string compare (ASCII letters only); 0 when equal, -1 / 1 otherwise. */
    static int WcsICmp(const wchar_t* left, const wchar_t* right);
    /** vsprintf into `buffer`; `arguments` is the va_list. */
    static int VSPrintF(char* buffer, const char* format, void* arguments);
    /** Bounded vsprintf: writes at most `bufferSize - 1` characters plus the terminator. */
    static int VSPrintF_S(char* buffer, int bufferSize, const char* format, void* arguments);
    /** Wide vsprintf. */
    static int VSWPrintF(wchar_t* buffer, const wchar_t* format, void* arguments);
    /** Upper case of one character (NSString uppercaseString). */
    static wchar_t ToWUpper(wchar_t character);
    /** Lower case of one character (NSString lowercaseString). */
    static wchar_t ToWLower(wchar_t character);
};

#endif
