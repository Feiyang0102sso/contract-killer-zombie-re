// ICStdUtil.h - portable printf-style helpers; each forwards to the platform implementation
// (CStdUtil_Cocoa on iOS). Header-only (created by b006; only the members b006 needed so far).
#ifndef ICSTDUTIL_H
#define ICSTDUTIL_H

#include <stdarg.h>
#include "CStdUtil_Cocoa.h"

namespace com { namespace glu { namespace platform { namespace core {

class ICStdUtil {
public:
    /** sprintf into `buffer` (no size check). */
    static int SPrintF(char* buffer, const char* format, ...)
    {
        va_list arguments;
        va_start(arguments, format);
        int written = CStdUtil_Cocoa::VSPrintF(buffer, format, arguments);
        va_end(arguments);
        return written;
    }

    /** swprintf into `buffer` (no size check); defined out of line (gserve CNGSUtil.cpp). */  // added by b011x
    static int SWPrintF(wchar_t* buffer, const wchar_t* format, ...);  // added by b011x
    /** Bounded sprintf (at most `size` bytes with the terminator); defined in ICStdUtil_inl.h. */  // added by b040x
    static int SPrintF_S(char* buffer, int size, const char* format, ...);  // added by b040x
};

} } } }

#endif
#include "ICStdUtil_inl.h"  // added by b016x: inline SWPrintF (weak in the original objects)
