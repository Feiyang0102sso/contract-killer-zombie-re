// CColor.h - pixel color formats, color conversion helpers and color palettes (created by b005).
#ifndef CCOLOR_H
#define CCOLOR_H

#include "CClass.h"

namespace com { namespace glu { namespace platform { namespace components {

class CInputStream;

/**
 * Pixel color formats. Channel names are listed from the most significant bit down
 * (packed formats) or from the highest byte address down (byte formats, little endian),
 * e.g. R8G8B8 is stored as the bytes B, G, R.
 * The low byte of each value is the size of one pixel in bytes.
 */
enum ColorFormat {
    ColorFormat_Invalid     = 0,
    ColorFormat_R5G6B5      = 0x21302,
    ColorFormat_B5G6R5      = 0x31302,
    ColorFormat_A1R5G5B5    = 0x45402,
    ColorFormat_R5G5B5A1    = 0x55402,
    ColorFormat_A4R4G4B4    = 0x65402,
    ColorFormat_R4G4B4A4    = 0x75402,
    ColorFormat_R6G6B6      = 0x81304,  // 18 bits used out of a 32-bit pixel
    ColorFormat_R8G8B8      = 0x90303,
    ColorFormat_X8R8G8B8    = 0xb0304,
    ColorFormat_A8R8G8B8    = 0xc4404,
    ColorFormat_A8B8G8R8    = 0xf4404,
    ColorFormat_ARGB_Fixed  = 0x1e4410, // Color_ARGB_fixed: four 16.16 fixed point channels
    ColorFormat_ARGB_Float  = 0x1f4410, // four floats, a, r, g, b (1.0 = full)
    ColorFormat_RGBA_Fixed  = 0x204410  // four 16.16 fixed point channels, r, g, b, a
};

/** Converts a single pixel of any ColorFormat to a packed format. */
class CColor {
public:
    static unsigned short ConvertTo_R5G6B5(ColorFormat format, const void* pixel);
    static unsigned int ConvertTo_X8R8G8B8(ColorFormat format, const void* pixel);
    static unsigned int ConvertTo_A8R8G8B8(ColorFormat format, const void* pixel);
};

/** A packed 0xXXRRGGBB color. */
struct Color_X8R8G8B8_t {
    unsigned int value;

    static const Color_X8R8G8B8_t Magenta;
};

/** A packed 0xAARRGGBB color. */
struct Color_A8R8G8B8_t {
    unsigned int value;

    static const Color_A8R8G8B8_t Black;
};

/** A color with float channels (1.0 = full intensity). */  // added by b037x
struct Color_RGBA {  // added by b037x
    float r;    // 0x00  // added by b037x
    float g;    // 0x04  // added by b037x
    float b;    // 0x08  // added by b037x
    float a;    // 0x0c  // added by b037x
};  // added by b037x

/** A color with 16.16 fixed point channels (0x10000 = full intensity). */
struct Color_ARGB_fixed {
    int a;
    int r;
    int g;
    int b;

    /** Set from one pixel stored in `format`; unknown formats leave the color unchanged. */
    void Make(ColorFormat format, const void* pixel);

    static Color_ARGB_fixed White;
};

/** A color table (one 0xAARRGGBB entry per index), loaded from a palette resource. */
class Color_Palette : public core::CClass {
public:
    enum { ClassId = 0xa6efb9a2 };

    Color_Palette()
    {
        m_classId = ClassId;
        m_format = ColorFormat_Invalid;
        m_count = 0;
        m_colors = NULL;
    }

    virtual ~Color_Palette();

    /** Load from a PNG palette chunk stream or a raw 32-bit color table, depending on `type`. */
    unsigned char Load(CInputStream& stream, unsigned int type);
    void Destroy();

    ColorFormat m_format;           // 0x08 format of the entries in m_colors
    int m_count;                    // 0x0c number of entries
    unsigned int* m_colors;         // 0x10 owned color table
};

// The original AdMarvel ObjC++ unit contains a weak copy of this destructor; the out-of-class inline keeps the original key-function emission.
inline Color_Palette::~Color_Palette()
{
    Destroy();
}

} } } }

#endif
