// CDrawUtil.cpp - small 2d drawing helpers on top of the platform ICGraphics2d.
#include "CDrawUtil.h"
#include "CColor.h"
#include "CRectangle.h"
#include "CLineSegment2d.h"

using namespace com::glu::platform;
using namespace com::glu::platform::graphics;

// Primitive values as the original passes them: ICGraphics2d.h (b037x) still numbers the
// enum 0/1, the original uses 1 = lines and 3 = rectangles (see notes/b035.md).
static const int PRIMITIVE_LINES = 1;
static const int PRIMITIVE_RECTANGLES = 3;

void CDrawUtil::Fill(int x1, int y1, int x2, int y2, unsigned int color, ICGraphics2d::Primitive primitive)
{
    // Expand each 8-bit channel to 16.16 fixed point (0xff -> 0xffff), then to float.
    int alpha = color >> 24;
    int red = (color >> 16) & 0xff;
    int green = (color >> 8) & 0xff;
    int blue = color & 0xff;
    float alphaValue = (float)(alpha | (alpha << 8)) / 65536.0f;
    float redValue = (float)(red | (red << 8)) / 65536.0f;
    float greenValue = (float)(green | (green << 8)) / 65536.0f;
    float blueValue = (float)(blue | (blue << 8)) / 65536.0f;

    ICGraphics2d* graphics = ICGraphics2d::GetInstance();
    components::Color_RGBA rgba;
    rgba.r = redValue;
    rgba.g = greenValue;
    rgba.b = blueValue;
    rgba.a = alphaValue;

    switch (primitive) {
    case PRIMITIVE_RECTANGLES: {
        components::CRectangle<int> rectangle;
        rectangle.m_x = x1;
        rectangle.m_y = y1;
        rectangle.m_width = x2 - x1;
        rectangle.m_height = y2 - y1;
        graphics->Draw(rectangle, rgba);
        break;
    }
    case PRIMITIVE_LINES: {
        math::CLineSegment2d line;
        line.m_start.m_x = (float)x1;
        line.m_start.m_y = (float)y1;
        line.m_end.m_x = (float)x2;
        line.m_end.m_y = (float)y2;
        graphics->Draw(line, rgba);
        break;
    }
    }
}
