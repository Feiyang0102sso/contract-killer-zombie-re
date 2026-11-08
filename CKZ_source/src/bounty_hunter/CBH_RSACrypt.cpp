// b074: original 0x1192e4..0x1193a6; chained XOR, despite the source filename.
#include "CBH_RSACrypt.h"

// Writable globals are loaded on every iteration in the shipped implementation.
// Original 3fce01 complete NUL-terminated literal; KEY_LENGTH only selects the first 20 bytes and does not truncate the original global content.
const char* KEY = "QWEIORDHGKJGHSDVBWEROWEUTERIOTKJASLHFHSDHJFGAHGSNMXBVPQOIWRYEWUITYE";
int KEY_LENGTH = 20;

int CBH_XorCrypt::Decypher(char* data, int size, unsigned char key)
{
    for (int index = 0; index < size; ++index) {
        unsigned char encrypted = (unsigned char)data[index];
        data[index] = encrypted ^ key;
        data[index] ^= KEY[index % KEY_LENGTH];
        key = encrypted;
    }
    return 0;
}

char CBH_XorCrypt::Cipher(char* data, int size)
{
    unsigned char checksum = 0;
    for (int index = 0; index < size; ++index) {
        checksum ^= (unsigned char)data[index];
    }
    unsigned char previous = checksum;
    for (int index = 0; index < size; ++index) {
        previous = KEY[index % KEY_LENGTH] ^ previous ^ (unsigned char)data[index];
        data[index] = previous;
    }
    return (signed char)checksum;
}
