// CGenUtil.h - miscellaneous helpers of the Glu platform core.
#ifndef CGENUTIL_H
#define CGENUTIL_H

namespace com { namespace glu { namespace platform { namespace core {

/** Grab bag of small static helpers. */
class CGenUtil {
public:
    /**
     * Parse a hexadecimal string ("1F", "ff", no "0x" prefix).
     * Stops at the first character that is not a hex digit; NULL gives 0.
     */
    static unsigned int HexAToI(const char* text);
};

} } } }

#endif
