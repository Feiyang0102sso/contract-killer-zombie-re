// CStringToKey.h - string hashing used to turn names into numeric keys.
#ifndef CSTRINGTOKEY_H
#define CSTRINGTOKEY_H

#include <wchar.h>

namespace com { namespace glu { namespace platform { namespace core {

/**
 * Hash a string into a 32-bit key: start from the length, then for every character rotate the
 * key left by 4 and xor the character in. With `ignoreCase` 'A'..'Z' hash like 'a'..'z'.
 */
unsigned int CStringToKey(const char* text, unsigned char ignoreCase);
/** Wide version: converted with wcstombs, then hashed like the narrow one. */
unsigned int CStringToKey(const wchar_t* text, unsigned char ignoreCase);

} } } }

#endif
