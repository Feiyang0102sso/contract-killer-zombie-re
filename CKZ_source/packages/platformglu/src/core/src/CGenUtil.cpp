// CGenUtil.cpp - miscellaneous helpers of the Glu platform core.
#include "CGenUtil.h"

namespace com { namespace glu { namespace platform { namespace core {

unsigned int CGenUtil::HexAToI(const char* text)
{
    unsigned int value = 0;

    if (text == NULL) {
        return value;
    }

    while (*text != '\0') {
        char character = *text;
        unsigned char digit;

        if (character >= '0' && character <= '9') {
            digit = character - '0';
        } else if (character == 'A' || character == 'a') {
            digit = 10;
        } else if (character == 'B' || character == 'b') {
            digit = 11;
        } else if (character == 'C' || character == 'c') {
            digit = 12;
        } else if (character == 'D' || character == 'd') {
            digit = 13;
        } else if (character == 'E' || character == 'e') {
            digit = 14;
        } else if (character == 'F' || character == 'f') {
            digit = 15;
        } else {
            // not a hex digit: stop parsing
            return value;
        }

        value = (value << 4) | digit;
        text++;
    }

    return value;
}

} } } }
