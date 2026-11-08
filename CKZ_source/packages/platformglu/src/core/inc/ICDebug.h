// ICDebug.h - portable debug output; the bodies are platform inline headers (ICDebug_inl.h).
// (created by b040x: header-only class, first used by swerve CSwerve.cpp)
#ifndef ICDEBUG_H
#define ICDEBUG_H

namespace com { namespace glu { namespace platform { namespace core {

class ICDebug {
public:
    /** printf-style debug message. Variadic, so every user keeps a weak copy. */
    static void LogMessage(const char* format, ...);
};

} } } }

#include "ICDebug_inl.h"

#endif
