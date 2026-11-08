// CCRC32_gServe.cpp - reflected CRC-32 (the well known table driven implementation).
#include "CCRC32_gServe.h"

/** Standard CRC-32 polynomial (normal form). */
static const unsigned int CRC32_POLYNOMIAL = 0x04c11db7;

/** Mirrors the low `bitCount` bits of `value` (bit 0 <-> bit bitCount-1). */
static inline unsigned int Reflect(unsigned int value, char bitCount)
{
    unsigned int reflected = 0;
    for (int position = 1; position < (bitCount + 1); position++) {
        if (value & 1) {
            reflected |= (1 << (bitCount - position));
        }
        value >>= 1;
    }
    return reflected;
}

CCRC32_gServe::CCRC32_gServe()
    : CSingleton(SingletonId)
{
    m_polynomial = CRC32_POLYNOMIAL;
    Generate_Lookup_Table();
}

void CCRC32_gServe::Generate_Lookup_Table()
{
    for (int code = 0; code <= 0xFF; code++) {
        m_table[code] = Reflect(code, 8) << 24;
        for (int position = 0; position < 8; position++) {
            // ?: as in the classic implementation: an if/else feedback variable folds the shift into the eor
            m_table[code] = (m_table[code] << 1) ^ (m_table[code] & (1 << 31) ? m_polynomial : 0);
        }
        m_table[code] = Reflect(m_table[code], 32);
    }
}

unsigned int CCRC32_gServe::GenerateCRC32(unsigned char* data, unsigned int size)
{
    unsigned int crc = 0;
    if (size != 0) {
        crc = 0xffffffff;
        while (size--) {
            crc = (crc >> 8) ^ m_table[(crc & 0xFF) ^ *data++];
        }
        crc = ~crc;
    }
    return crc;
}
