// CCrc32.cpp - table driven CRC-32.
#include "CCrc32.h"

namespace com { namespace glu { namespace platform { namespace components {

CCrc32::CCrc32() : CSingleton(SingletonId)
{
    // entry 1 of a built table is never 0
    m_table[1] = 0;
}

CCrc32::~CCrc32()
{
}

unsigned int CCrc32::Crc32(const unsigned char* data, unsigned int size)
{
    if (m_table[1] == 0) {
        for (unsigned int i = 0; i < 256; i++) {
            unsigned int crc = i << 24;
            for (int bit = 0; bit < 8; bit++) {
                if (crc & 0x80000000) {
                    crc = (crc << 1) ^ 0x04C11DB7;
                } else {
                    crc = crc << 1;
                }
            }
            m_table[i] = crc;
        }
    }
    unsigned int crc = 0xFFFFFFFF;
    for (unsigned int i = 0; i < size; i++) {
        crc = m_table[(crc >> 24) ^ data[i]] ^ (crc << 8);
    }
    return ~crc;
}

} } } }
