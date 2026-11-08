// CDrawUtil.h - small 2d drawing helpers on top of the platform ICGraphics2d.
#ifndef CDRAWUTIL_H
#define CDRAWUTIL_H

#include "ICGraphics2d.h"

class CDrawUtil {
public:
    /**
     * Draw the box (x1, y1)-(x2, y2) in a packed 0xAARRGGBB color.
     * primitive 1 draws the segment from (x1, y1) to (x2, y2), 3 fills the rectangle;
     * other primitives draw nothing.
     */
    static void Fill(int x1, int y1, int x2, int y2, unsigned int color,
                     com::glu::platform::graphics::ICGraphics2d::Primitive primitive);
};

#endif
