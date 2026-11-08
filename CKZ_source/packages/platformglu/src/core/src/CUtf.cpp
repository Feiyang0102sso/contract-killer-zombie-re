// CUtf.cpp - UTF-8 <-> wchar_t conversion helpers.
// The byte layouts follow java.io.DataInput's modified UTF-8:
//   0xxxxxxx                      U+0001..U+007F
//   110xxxxx 10xxxxxx             U+0000, U+0080..U+07FF
//   1110xxxx 10xxxxxx 10xxxxxx    U+0800..U+FFFF
#include "CUtf.h"

#include <stddef.h>

namespace com { namespace glu { namespace platform { namespace core {

unsigned int CUtf::Utf8ToWcs(const unsigned char* utf8, unsigned int byteCount, wchar_t* out, unsigned int outCapacity)
{
    unsigned int written = 0;

    if (utf8 == NULL || out == NULL || outCapacity == 0) {
        return written;
    }

    // keep one slot for the terminator
    unsigned int maxChars = outCapacity - 1;
    unsigned int position = 0;

    if (byteCount == 0 || maxChars == 0) {
        goto terminate;
    }

    do {
        int first = utf8[position];
        int second;
        int third;

        switch (first >> 4) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
            // 0xxxxxxx
            out[written] = first;
            position += 1;
            break;
        case 12:
        case 13:
            // 110x xxxx   10xx xxxx
            if (position + 2 > byteCount) {
                written = 0;
                goto terminate;
            }
            second = utf8[position + 1];
            if ((second & 0xC0) != 0x80) {
                written = 0;
                goto terminate;
            }
            out[written] = ((first & 0x1F) << 6) | (second & 0x3F);
            position += 2;
            break;
        case 14:
            // 1110 xxxx  10xx xxxx  10xx xxxx
            if (position + 3 > byteCount) {
                written = 0;
                goto terminate;
            }
            second = utf8[position + 1];
            third = utf8[position + 2];
            if ((second & 0xC0) != 0x80 || (third & 0xC0) != 0x80) {
                written = 0;
                goto terminate;
            }
            out[written] = ((first & 0x0F) << 12) | ((second & 0x3F) << 6) | (third & 0x3F);
            position += 3;
            break;
        default:
            // 10xx xxxx, 1111 xxxx
            written = 0;
            goto terminate;
        }
        written++;
    } while (position < byteCount && written < maxChars);

terminate:
    out[written] = 0;
    return written;
}

unsigned int CUtf::WcsToUtf8(const wchar_t* text, unsigned int length, unsigned char* out, unsigned int outCapacity)
{
    unsigned int written = 0;

    if (text == NULL || out == NULL) {
        return written;
    }

    // keep one byte for the terminator
    unsigned int maxBytes = outCapacity - 1;

    for (unsigned int index = 0; index < length && written < maxBytes; index++) {
        wchar_t character = text[index];

        if (character == 0) {
            out[written] = 0;
            break;
        }

        if (character >= 0x0001 && character <= 0x007F) {
            out[written] = (unsigned char)character;
            written += 1;
        } else if (character > 0x07FF) {
            if (written + 3 > maxBytes) {
                break;
            }
            out[written] = (unsigned char)(0xE0 | ((character >> 12) & 0x0F));
            out[written + 1] = (unsigned char)(0x80 | ((character >> 6) & 0x3F));
            out[written + 2] = (unsigned char)(0x80 | (character & 0x3F));
            written += 3;
        } else {
            if (written + 2 > maxBytes) {
                break;
            }
            out[written] = (unsigned char)(0xC0 | ((character >> 6) & 0x1F));
            out[written + 1] = (unsigned char)(0x80 | (character & 0x3F));
            written += 2;
        }
    }

    out[written] = 0;
    return written;
}

unsigned int CUtf::GetUtfLen(const wchar_t* text, unsigned int length)
{
    unsigned int byteCount = 0;

    if (text == NULL) {
        return byteCount;
    }

    for (unsigned int index = 0; index < length; index++) {
        wchar_t character = text[index];

        if (character == 0) {
            break;
        }

        if (character >= 0x0001 && character <= 0x007F) {
            byteCount += 1;
        } else if (character > 0x07FF) {
            byteCount += 3;
        } else {
            byteCount += 2;
        }
    }
    return byteCount;
}

unsigned int CUtf::GetWcsLen(const unsigned char* utf8, unsigned int byteCount)
{
    unsigned int characterCount = 0;
    unsigned int position = 0;

    if (utf8 == NULL || byteCount == 0) {
        return characterCount;
    }

    do {
        int first = utf8[position];

        switch (first >> 4) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
            position += 1;
            break;
        case 12:
        case 13:
            if (position + 2 > byteCount) {
                return 0;
            }
            if ((utf8[position + 1] & 0xC0) != 0x80) {
                return 0;
            }
            position += 2;
            break;
        case 14:
            if (position + 3 > byteCount) {
                return 0;
            }
            if ((utf8[position + 1] & 0xC0) != 0x80 || (utf8[position + 2] & 0xC0) != 0x80) {
                return 0;
            }
            position += 3;
            break;
        default:
            return 0;
        }
        characterCount++;
    } while (position < byteCount);

    return characterCount;
}

} } } }
