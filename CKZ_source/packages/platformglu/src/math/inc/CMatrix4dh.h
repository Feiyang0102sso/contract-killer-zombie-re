// CMatrix4dh.h - 4x4 float matrix that keeps its rotation part orthonormal: after
// m_orthonormalizeInterval rotations the upper 3x3 block is re-orthonormalized.
#ifndef CMATRIX4DH_H
#define CMATRIX4DH_H

#include "CMatrix4d.h"

namespace com { namespace glu { namespace platform { namespace math {

class CMatrix4dh : public CMatrix4d {
public:
    CMatrix4dh& LoadIdentity();
    /** Gram-Schmidt on the first two columns, third column = their cross product. */
    CMatrix4dh& Orthonormalize();
    /** Rotate about Z by `degrees`. */
    CMatrix4dh& RotateZ(float degrees);

    unsigned char m_autoOrthonormalize;             // 0x40 re-orthonormalize after m_orthonormalizeInterval ops
    unsigned short m_reserved42;                    // 0x42 never touched by the known code (counters start at 0x44)
    unsigned short m_operationCount;                // 0x44 rotations since the last orthonormalization
    unsigned short m_orthonormalizeInterval;        // 0x46
};

} } } }

#endif
