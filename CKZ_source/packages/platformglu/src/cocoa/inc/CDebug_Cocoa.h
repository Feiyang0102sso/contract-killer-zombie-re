// CDebug_Cocoa.h - iOS debug output (the shipping build logs nothing).
#ifndef CDEBUG_COCOA_H
#define CDEBUG_COCOA_H

class CDebug_Cocoa {
public:
    /** Print a printf-style message; `arguments` is the va_list. Compiled out on device. */
    static void LogMessage(const char* format, void* arguments);
};

#endif
