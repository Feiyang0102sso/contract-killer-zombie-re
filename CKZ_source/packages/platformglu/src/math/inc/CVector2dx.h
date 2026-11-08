// PROVISIONAL (created by b002): header-only math type, no owning unit yet (inline users: graphics/src/CGraphics.cpp)
// CVector2dx.h - 2-component 16.16 fixed point vector.
#ifndef CVECTOR2DX_H
#define CVECTOR2DX_H

namespace com { namespace glu { namespace platform { namespace math {

/** Two 16.16 fixed point components (8 bytes). */
class CVector2dx {
public:
    int x;      // 0x00
    int y;      // 0x04
};

} } } }

#endif
