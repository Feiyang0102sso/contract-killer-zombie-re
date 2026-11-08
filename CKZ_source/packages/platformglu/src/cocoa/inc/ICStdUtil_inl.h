// ICStdUtil_inl.h - iOS inline bodies of ICStdUtil (created by b016x; the original N_SOL lists this
// header for gserve CNGS.cpp, and CNGSUtil.o carries a weak copy of SWPrintF).
#ifndef ICSTDUTIL_INL_H
#define ICSTDUTIL_INL_H

#include <stdarg.h>
#include "CStdUtil_Cocoa.h"

namespace com { namespace glu { namespace platform { namespace core {

/** swprintf into `buffer` (no size check). Variadic, so every user keeps a weak copy. */
inline int ICStdUtil::SWPrintF(wchar_t* buffer, const wchar_t* format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    int written = CStdUtil_Cocoa::VSWPrintF(buffer, format, arguments);
    va_end(arguments);
    return written;
}

/** Bounded sprintf. Variadic, so every user keeps a weak copy. */  // added by b040x
inline int ICStdUtil::SPrintF_S(char* buffer, int size, const char* format, ...)  // added by b040x
{  // added by b040x
    va_list arguments;  // added by b040x
    va_start(arguments, format);  // added by b040x
    int written = CStdUtil_Cocoa::VSPrintF_S(buffer, size, format, arguments);  // added by b040x
    va_end(arguments);  // added by b040x
    return written;  // added by b040x
}  // added by b040x

} } } }

#endif
