// CMatrix2dx.h - 2x2 16.16 fixed-point matrix with periodic re-orthonormalization bookkeeping.
#ifndef CMATRIX2DX_H
#define CMATRIX2DX_H

namespace com { namespace glu { namespace platform { namespace math {

class CMatrix2dx {
public:
    /** Leaves the elements uninitialized; only the bookkeeping is reset. */
    CMatrix2dx();

    int m[4];                                       // 0x00 row-major elements, 16.16
    unsigned char m_autoOrthonormalize;             // 0x10 re-orthonormalize after m_orthonormalizeInterval ops
    unsigned short m_reserved12;                    // 0x12 never touched by the known code (counters start at 0x14)
    unsigned short m_operationCount;                // 0x14 operations since the last orthonormalization
    unsigned short m_orthonormalizeInterval;        // 0x16
};

} } } }

#endif
