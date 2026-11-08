// PROVISIONAL (created by b035): header-only math type, no owning unit yet (inline users: graphics/2d/src/CGraphics2d_OGLES.cpp)
// CLineSegment2d.h - 2d line segment between two float points.
#ifndef CLINESEGMENT2D_H
#define CLINESEGMENT2D_H

#include "CVector2d.h"

namespace com { namespace glu { namespace platform { namespace math {

class CLineSegment2d {
public:
    CVector2d m_start;  // 0x00
    CVector2d m_end;    // 0x08
};

} } } }

#endif
