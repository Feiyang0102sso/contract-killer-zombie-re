// ICDebug_inl.h - iOS inline bodies of ICDebug (created by b040x; the original N_SOL lists this
// header for swerve CSwerve.cpp, which carries a weak copy of LogMessage).
#ifndef ICDEBUG_INL_H
#define ICDEBUG_INL_H

#include <stdarg.h>
#include "CDebug_Cocoa.h"

namespace com { namespace glu { namespace platform { namespace core {

inline void ICDebug::LogMessage(const char* format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    CDebug_Cocoa::LogMessage(format, arguments);
    va_end(arguments);
}

} } } }

#endif
