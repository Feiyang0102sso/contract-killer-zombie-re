// CCRC32_gServe.h - table driven CRC-32 singleton of gserve (the classic reflected CRC-32,
// polynomial 0x04c11db7). Owned by util/CCRC32_gServe.cpp (b032, written by b027x).
#ifndef CCRC32_GSERVE_H
#define CCRC32_GSERVE_H

#include "CSingleton.h"

class CCRC32_gServe : public com::glu::platform::components::CSingleton {
public:
    /** Singleton key in CApp::m_singletons. */
    enum { SingletonId = 0x792d4305 };

    CCRC32_gServe();
    /** Inline: the original emits weak D1/D0 in every including unit. */
    virtual ~CCRC32_gServe() {}

    /** The registered instance, created on first use (CFileUtil_gServe::GetCRCFromFileInCache).
        `void* found` then cast gives the original cmp+bne (verified by b020). */
    static inline CCRC32_gServe* GetInstance()
    {
        void* found = NULL;
        CApplet::m_pApp->m_singletons->Find(SingletonId, &found);
        CCRC32_gServe* instance = (CCRC32_gServe*)found;
        if (instance == NULL) {
            instance = new CCRC32_gServe();
        }
        return instance;
    }

    /** Fills m_table with the reflected CRC-32 table of m_polynomial. */
    void Generate_Lookup_Table();
    /** CRC-32 of `size` bytes (0 for an empty buffer). */
    unsigned int GenerateCRC32(unsigned char* data, unsigned int size);

    unsigned int m_polynomial;      // 0x08 0x04c11db7
    unsigned int m_table[256];      // 0x0c lookup table; object size 0x40c
};

#endif
