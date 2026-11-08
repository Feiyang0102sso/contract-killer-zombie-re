// CUtf.h - UTF-8 <-> wchar_t conversion helpers (modified UTF-8 as in Java: 1..3 bytes per char).
#ifndef CUTF_H
#define CUTF_H

#include <wchar.h>

namespace com { namespace glu { namespace platform { namespace core {

/** Static UTF-8 / wide string conversions; malformed input yields an empty result. */
class CUtf {
public:
    /**
     * Decode `byteCount` UTF-8 bytes into `out` (capacity in characters, terminator included).
     * Returns the characters written (0 on malformed input); `out` is always terminated.
     */
    static unsigned int Utf8ToWcs(const unsigned char* utf8, unsigned int byteCount, wchar_t* out, unsigned int outCapacity);
    /**
     * Encode up to `length` characters (stops at L'\0') into `out` (capacity in bytes, terminator
     * included). Returns the bytes written; `out` is always terminated.
     */
    static unsigned int WcsToUtf8(const wchar_t* text, unsigned int length, unsigned char* out, unsigned int outCapacity);
    /** Number of UTF-8 bytes needed to encode `length` characters (stops at L'\0'). */
    static unsigned int GetUtfLen(const wchar_t* text, unsigned int length);
    /** Number of characters in `byteCount` UTF-8 bytes; 0 on malformed input. */
    static unsigned int GetWcsLen(const unsigned char* utf8, unsigned int byteCount);
};

} } } }

#endif
