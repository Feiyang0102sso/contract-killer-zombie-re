// CRandGen.h - Mersenne Twister (MT19937) random number generator, registered as a singleton.
#ifndef CRANDGEN_H
#define CRANDGEN_H

#include "CSingleton.h"

namespace com { namespace glu { namespace platform { namespace core {

/** MT19937 generator seeded from the wall clock (0x9D4 bytes). */
class CRandGen : public components::CSingleton {
public:
    // ---- constants ----
    static const unsigned int CLASS_ID = 0x64780132;    // key in CApp::m_singletons
    static const int STATE_SIZE = 624;                  // N
    static const int SHIFT_SIZE = 397;                  // M
    static const unsigned int MATRIX_A = 0x9908B0DF;    // constant vector a
    static const unsigned int UPPER_MASK = 0x80000000;  // most significant w-r bits
    static const unsigned int LOWER_MASK = 0x7FFFFFFF;  // least significant r bits

    CRandGen();
    virtual ~CRandGen();

    /** The shared instance, created on first use. */
    static inline CRandGen* GetInstance()  // changed by b025x: explicit inline + `void* found` (inlined into CNGSJSONData::SaveToDisk, cmp+bne)
    {
        void* found = NULL;  // changed by b025x
        CApplet::m_pApp->m_singletons->Find(CLASS_ID, &found);  // changed by b025x
        CRandGen* instance = (CRandGen*)found;  // changed by b025x
        if (instance == NULL) {
            instance = new CRandGen();
        }
        return instance;
    }

    /** Next 32-bit random number. */
    unsigned int Generate();
    /** Random number in [0, range). */
    unsigned int GetRand(unsigned int range);
    /** Random number in [low, high]. */
    int GetRandRange(int low, unsigned int high);

    int m_index;                            // +0x08 next state word; STATE_SIZE + 1 = not seeded
    unsigned int m_mag01[2];                // +0x0C { 0, MATRIX_A }
    unsigned int m_state[STATE_SIZE];       // +0x14

private:
    /** init_genrand: fill the state from `seed`. */
    void Seed(unsigned int seed)
    {
        int index;

        m_state[0] = seed;
        for (index = 1; index < STATE_SIZE; index++) {
            m_state[index] = 1812433253U * (m_state[index - 1] ^ (m_state[index - 1] >> 30)) + index;
        }
        m_index = STATE_SIZE;
    }
};

} } } }

#endif
