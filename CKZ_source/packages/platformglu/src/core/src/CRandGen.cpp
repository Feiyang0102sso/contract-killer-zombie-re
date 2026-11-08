// CRandGen.cpp - Mersenne Twister (MT19937) random number generator.
// Generate() is genrand_int32() of the reference implementation by M. Matsumoto and T. Nishimura.
#include "CRandGen.h"

#include "CStdUtil_Cocoa.h"

namespace com { namespace glu { namespace platform { namespace core {

CRandGen::CRandGen() : CSingleton(CLASS_ID)
{
    m_index = STATE_SIZE + 1;
    m_mag01[0] = 0;
    m_mag01[1] = MATRIX_A;
    Seed(CStdUtil_Cocoa::GetTimeSeconds());
}

CRandGen::~CRandGen()
{
}

unsigned int CRandGen::Generate()
{
    unsigned int value;

    if (m_index >= STATE_SIZE) {
        // generate STATE_SIZE words at one time
        int index;

        // not seeded yet
        if (m_index == STATE_SIZE + 1) {
            Seed(CStdUtil_Cocoa::GetTimeSeconds());
        }

        for (index = 0; index < STATE_SIZE - SHIFT_SIZE; index++) {
            value = (m_state[index] & UPPER_MASK) | (m_state[index + 1] & LOWER_MASK);
            m_state[index] = m_state[index + SHIFT_SIZE] ^ (value >> 1) ^ m_mag01[value & 0x1];
        }
        for (; index < STATE_SIZE - 1; index++) {
            value = (m_state[index] & UPPER_MASK) | (m_state[index + 1] & LOWER_MASK);
            m_state[index] = m_state[index + (SHIFT_SIZE - STATE_SIZE)] ^ (value >> 1) ^ m_mag01[value & 0x1];
        }
        value = (m_state[STATE_SIZE - 1] & UPPER_MASK) | (m_state[0] & LOWER_MASK);
        m_state[STATE_SIZE - 1] = m_state[SHIFT_SIZE - 1] ^ (value >> 1) ^ m_mag01[value & 0x1];

        m_index = 0;
    }

    value = m_state[m_index++];

    // tempering
    value ^= (value >> 11);
    value ^= (value << 7) & 0x9D2C5680;
    value ^= (value << 15) & 0xEFC60000;
    value ^= (value >> 18);

    return value;
}

unsigned int CRandGen::GetRand(unsigned int range)
{
    return Generate() % range;
}

int CRandGen::GetRandRange(int low, unsigned int high)
{
    return low + Generate() % (high - low + 1);
}

} } } }
