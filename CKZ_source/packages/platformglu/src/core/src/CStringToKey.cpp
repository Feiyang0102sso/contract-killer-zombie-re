// CStringToKey.cpp - string hashing used to turn names into numeric keys.
#include "CStringToKey.h"

#include <stdlib.h>
#include <string.h>

namespace com { namespace glu { namespace platform { namespace core {

unsigned int CStringToKey(const char* text, unsigned char ignoreCase)
{
    unsigned int length = strlen(text);
    unsigned int key = length;

    if (ignoreCase) {
        for (unsigned int index = 0; index < length; index++) {
            int character = text[index];
            if (character >= 'A' && character <= 'Z') {
                character = character + ('a' - 'A');
            }
            // rotate left by 4, then mix the character in
            key = ((key << 4) | (key >> 28)) ^ character;
        }
    } else {
        for (unsigned int index = 0; index < length; index++) {
            key = ((key << 4) | (key >> 28)) ^ text[index];
        }
    }
    return key;
}

unsigned int CStringToKey(const wchar_t* text, unsigned char ignoreCase)
{
    int length = wcslen(text);
    char* narrow = (char*)malloc(length + 1);
    unsigned int key;

    if (wcstombs(narrow, text, length + 1) == (size_t)-1) {
        // conversion failed: hash the empty string
        narrow[0] = '\0';
    } else {
        narrow[length] = '\0';
    }

    key = CStringToKey(narrow, ignoreCase);
    if (narrow != NULL) {
        free(narrow);
    }
    return key;
}

} } } }
