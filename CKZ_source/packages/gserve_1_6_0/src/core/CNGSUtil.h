// CNGSUtil.h - small helpers of the gServe code: Base64, wide/narrow conversion, message lookups
// and the XOR obfuscation of saved strings.
#ifndef CNGSUTIL_H
#define CNGSUTIL_H

#include "CStrChar.h"
#include "CStrWChar.h"

class CObjectMap;

namespace com { namespace glu { namespace platform { namespace network {

/**
 * Raw buffer handed between the gServe helpers (the original header of this struct is unknown;
 * defined here by b016x).
 */
struct sDataBlock {
    unsigned int m_size;    // +0x00 bytes
    void* m_data;           // +0x04 malloc'd, owned by the receiver
};

} } } }

class CNGSUtil {
public:
    /** Base64 text of `data` (malloc'd, NUL-terminated; the caller frees it). */
    static char* Base64Encode(const unsigned char* data, unsigned int length);
    /** Decode `length` Base64 chars of `text` into `output`; returns the byte count. */
    static int Base64Decode(unsigned char* output, const unsigned char* text, int length);
    static com::glu::platform::components::CStrChar WStrToCStr(const com::glu::platform::components::CStrWChar& text);
    /** Decimal text of `value`. */
    static void IntegerToString(int value, com::glu::platform::components::CStrWChar& text);
    /** String at `path` (split by `separator`) in `message`; empty when absent or not a string. */
    static com::glu::platform::components::CStrWChar GetMessageDataString(CObjectMap* message,
        const com::glu::platform::components::CStrWChar& path,
        const com::glu::platform::components::CStrWChar& separator);
    /**
     * XOR `text` with `key` (and the key with the result), byte-swap both and store
     * [length, keyLength, key..., text...] in `block`. 0 = ok, 1 = bad input, 2 = out of memory.
     */
    static unsigned char ObfuscateString(com::glu::platform::network::sDataBlock& block, const wchar_t* text, const wchar_t* key);
    /** Inverse of ObfuscateString for a block of `textLength` + `keyLength` characters. */
    static unsigned char DeObfuscateString(com::glu::platform::network::sDataBlock& block, const wchar_t* text,
                                           const wchar_t* key, unsigned int textLength, unsigned int keyLength);
};

#endif
