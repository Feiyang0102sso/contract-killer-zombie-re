// CStringUtils.h - NSString <-> CStrWChar conversion used by the iOS in-app purchase code (created by b033x).
#ifndef CSTRINGUTILS_H
#define CSTRINGUTILS_H

#include "CStrWChar.h"

#ifdef __OBJC__
@class NSString;
#endif

class CStringUtils {
public:
    /** Copy `string` (as UTF-32LE) into a new CStrWChar. */
    static com::glu::platform::components::CStrWChar NSStringToCStrWChar(NSString* string);
    /** New (retained, +1) NSString with the contents of `string`; the caller releases it. */
    static NSString* CStrWCharToNSString(const com::glu::platform::components::CStrWChar& string);
};

#endif
