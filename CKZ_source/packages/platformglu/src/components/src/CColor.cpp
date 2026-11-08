// CColor.cpp - pixel format conversions and palette loading.
#include "CColor.h"
#include "CInputStream.h"
#include "CPNG.h"

namespace com { namespace glu { namespace platform { namespace components {

static const float FIXED_ONE = 65536.0f;              // 1.0 in 16.16 fixed point

// resource types accepted by Color_Palette::Load
static const unsigned int PALETTE_TYPE_PNG = 0x40546f85;   // PNG chunks: PLTE (+ optional tRNS)
static const unsigned int PALETTE_TYPE_RAW = 0x40547da2;   // raw table of 32-bit colors

const Color_X8R8G8B8_t Color_X8R8G8B8_t::Magenta = { 0xffff00ff };
const Color_A8R8G8B8_t Color_A8R8G8B8_t::Black = { 0xff000000 };
Color_ARGB_fixed Color_ARGB_fixed::White = { 0x10000, 0x10000, 0x10000, 0x10000 };

// Packed pixel layouts, fields listed from the least significant bit up.
struct Pixel_B5G6R5 {
    unsigned short r : 5;
    unsigned short g : 6;
    unsigned short b : 5;
};

struct Pixel_R5G5B5A1 {
    short a : 1;                // signed: 1 -> -1 -> 0xff as an 8-bit channel
    unsigned short b : 5;
    unsigned short g : 5;
    unsigned short r : 5;
};

struct Pixel_R4G4B4A4 {
    unsigned short a : 4;
    unsigned short b : 4;
    unsigned short g : 4;
    unsigned short r : 4;
};

struct Pixel_R6G6B6 {
    unsigned int b : 6;
    unsigned int g : 6;
    unsigned int r : 6;
};

/** Four float channels, 1.0 = full intensity (ColorFormat_ARGB_Float). */
struct Color_ARGB_float {
    float a;
    float r;
    float g;
    float b;
};

/** 8-bit channel -> 16.16 fixed point (0xff -> 0xffff). */
static inline int ByteToFixed(unsigned char channel)
{
    return channel | (channel << 8);
}

/** 16.16 fixed point channel -> 8-bit channel, clamped to 0..255. */
static inline unsigned char FixedToByte(int fixed)
{
    int channel = fixed >> 8;
    if (channel < 0) {
        return 0;
    }
    if (channel > 255) {
        return 255;
    }
    return channel;
}

/** 6-bit channel -> 8-bit channel (the low 2 bits are repeated). */
static inline unsigned char Expand6To8(unsigned char channel)
{
    return (channel << 2) | (channel & 3);
}

/** 8-bit channels -> packed 0bRRRRRGGGGGGBBBBB. */
static inline unsigned short PackR5G6B5(unsigned char red, unsigned char green, unsigned char blue)
{
    return (blue >> 3) | ((green & 0xfc) << 3) | ((red & 0xf8) << 8);
}

void Color_Palette::Destroy()
{
    if (m_colors != NULL) {
        free(m_colors);
    }
    m_format = ColorFormat_Invalid;
    m_count = 0;
    m_colors = NULL;
}

void Color_ARGB_fixed::Make(ColorFormat format, const void* pixel)
{
    const unsigned char* bytes = (const unsigned char*)pixel;
    const int* fixedChannels = (const int*)pixel;
    const Color_ARGB_float* floatChannels = (const Color_ARGB_float*)pixel;
    const Pixel_R6G6B6* pixel666 = (const Pixel_R6G6B6*)pixel;
    Pixel_B5G6R5 pixel565;
    Pixel_R5G5B5A1 pixel5551;
    Pixel_R4G4B4A4 pixel4444;
    unsigned short color;
    unsigned int alpha;
    unsigned int red;
    unsigned int green;
    unsigned int blue;

    switch (format) {
    case ColorFormat_R5G6B5:
        color = *(const unsigned short*)pixel;
        a = 0xffff;
        r = ByteToFixed(((color & 0xf800) >> 8) | ((color & 0xf800) >> 13));
        g = ByteToFixed(((color & 0x07e0) >> 3) | ((color & 0x07e0) >> 8));
        b = ByteToFixed(((color & 0x001f) << 3) | ((color & 0x001f) >> 2));
        break;

    case ColorFormat_B5G6R5:
        pixel565 = *(const Pixel_B5G6R5*)pixel;
        a = 0xffff;
        r = ByteToFixed((pixel565.r << 3) | (pixel565.r & 7));
        g = ByteToFixed((pixel565.g << 2) | (pixel565.g & 3));
        b = ByteToFixed((pixel565.b << 3) | (pixel565.b & 7));
        break;

    case ColorFormat_A1R5G5B5:
        color = *(const unsigned short*)pixel;
        a = ByteToFixed((short)color >> 15);
        r = ByteToFixed(((color & 0x7c00) >> 8) | ((color & 0x7c00) >> 13));
        g = ByteToFixed(((color & 0x03e0) >> 3) | ((color & 0x03e0) >> 9));
        b = ByteToFixed(((color & 0x001f) << 3) | ((color & 0x001f) >> 2));
        break;

    case ColorFormat_R5G5B5A1:
        pixel5551 = *(const Pixel_R5G5B5A1*)pixel;
        a = ByteToFixed(pixel5551.a);
        r = ByteToFixed((pixel5551.r << 3) | (pixel5551.r & 7));
        g = ByteToFixed((pixel5551.g << 3) | (pixel5551.g & 7));
        b = ByteToFixed((pixel5551.b << 3) | (pixel5551.b & 7));
        break;

    case ColorFormat_A4R4G4B4:
        color = *(const unsigned short*)pixel;
        a = ByteToFixed(((color & 0xf000) >> 8) | ((color & 0xf000) >> 12));
        r = ByteToFixed(((color & 0x0f00) >> 4) | ((color & 0x0f00) >> 8));
        g = ByteToFixed((color & 0x00f0) | ((color & 0x00f0) >> 4));
        b = ByteToFixed((color & 0x000f) | ((color & 0x000f) << 4));
        break;

    case ColorFormat_R6G6B6:
        red = pixel666->r;
        green = pixel666->g;
        blue = pixel666->b;
        a = 0xffff;
        r = ByteToFixed((red << 2) | (red & 3));
        g = ByteToFixed((green << 2) | (green & 3));
        b = ByteToFixed((blue << 2) | (blue & 3));
        break;

    case ColorFormat_R8G8B8:
    case ColorFormat_X8R8G8B8:
        blue = bytes[0];
        green = bytes[1];
        red = bytes[2];
        a = 0xffff;
        r = ByteToFixed(red);
        g = ByteToFixed(green);
        b = ByteToFixed(blue);
        break;

    case ColorFormat_A8R8G8B8:
        blue = bytes[0];
        green = bytes[1];
        red = bytes[2];
        alpha = bytes[3];
        a = ByteToFixed(alpha);
        r = ByteToFixed(red);
        g = ByteToFixed(green);
        b = ByteToFixed(blue);
        break;

    case ColorFormat_A8B8G8R8:
        red = bytes[0];
        green = bytes[1];
        blue = bytes[2];
        alpha = bytes[3];
        a = ByteToFixed(alpha);
        r = ByteToFixed(red);
        g = ByteToFixed(green);
        b = ByteToFixed(blue);
        break;

    case ColorFormat_ARGB_Fixed:
        alpha = fixedChannels[0];
        red = fixedChannels[1];
        green = fixedChannels[2];
        blue = fixedChannels[3];
        a = alpha;
        r = red;
        g = green;
        b = blue;
        break;

    case ColorFormat_ARGB_Float:
        alpha = (int)(floatChannels->a * FIXED_ONE);
        red = (int)(floatChannels->r * FIXED_ONE);
        green = (int)(floatChannels->g * FIXED_ONE);
        blue = (int)(floatChannels->b * FIXED_ONE);
        a = alpha;
        r = red;
        g = green;
        b = blue;
        break;

    case ColorFormat_RGBA_Fixed:
        red = fixedChannels[0];
        green = fixedChannels[1];
        blue = fixedChannels[2];
        alpha = fixedChannels[3];
        a = alpha;
        r = red;
        g = green;
        b = blue;
        break;

    case ColorFormat_R4G4B4A4:
        pixel4444 = *(const Pixel_R4G4B4A4*)pixel;
        a = ByteToFixed((pixel4444.a << 4) | pixel4444.a);
        r = ByteToFixed((pixel4444.r << 4) | pixel4444.r);
        g = ByteToFixed((pixel4444.g << 4) | pixel4444.g);
        b = ByteToFixed((pixel4444.b << 4) | pixel4444.b);
        break;

    default:
        break;
    }
}

unsigned char Color_Palette::Load(CInputStream& stream, unsigned int type)
{
    unsigned char result = 0;
    graphics::CPNG::PNGV12ChunkHeader header;
    graphics::CPNG::PNGV12ChunkFooter footer;

    Destroy();

    if (type == PALETTE_TYPE_RAW) {
        m_format = ColorFormat_A8R8G8B8;
        m_count = stream.Available() / 4;
        m_colors = (unsigned int*)malloc(stream.Available());
        stream.Read((unsigned char*)m_colors, stream.Available());
        for (int i = 0; i < m_count; i++) {
            m_colors[i] |= 0xff000000;
        }
        result = !stream.m_error;
    } else if (type == PALETTE_TYPE_PNG) {
        m_format = ColorFormat_A8R8G8B8;
        stream.m_isBigEndian = 1;

        // PLTE chunk: 3 bytes (r, g, b) per entry
        if (graphics::CPNG::ParsePNGV12ChunkHeader(stream, header)
            && header.type == graphics::CPNG::PNGV12ChunkType_Palette
            && header.length % 3 == 0) {
            m_count = header.length / 3;
            m_colors = (unsigned int*)malloc(m_count * sizeof(unsigned int));
            for (unsigned int i = 0; i < (unsigned int)m_count; i++) {
                unsigned int red = stream.ReadUInt8();
                unsigned int green = stream.ReadUInt8();
                unsigned int blue = stream.ReadUInt8();
                m_colors[i] = (red << 16) | (green << 8) | blue;
            }

            result = !stream.m_error;
            if (result) {
                result = graphics::CPNG::ParsePNGV12ChunkFooter(stream, footer);
            }
            if (result) {
                if (stream.Available() == 0) {
                    // no tRNS chunk: every entry is opaque
                    for (unsigned int i = 0; i < (unsigned int)m_count; i++) {
                        m_colors[i] |= 0xff000000;
                    }
                    return 1;
                }

                // tRNS chunk: one alpha byte per entry, missing entries are opaque
                result = 0;
                if (graphics::CPNG::ParsePNGV12ChunkHeader(stream, header)
                    && header.type == graphics::CPNG::PNGV12ChunkType_Transparency) {
                    for (unsigned int i = 0; i < header.length && i < (unsigned int)m_count; i++) {
                        m_colors[i] |= stream.ReadUInt8() << 24;
                    }
                    for (unsigned int i = header.length; i < (unsigned int)m_count; i++) {
                        m_colors[i] |= 0xff000000;
                    }
                    result = !stream.m_error;
                    if (result) {
                        result = graphics::CPNG::ParsePNGV12ChunkFooter(stream, footer);
                    }
                }
            }
        }
    }

    if (!result) {
        Destroy();
    }
    return result;
}

unsigned short CColor::ConvertTo_R5G6B5(ColorFormat format, const void* pixel)
{
    const unsigned char* bytes = (const unsigned char*)pixel;
    const int* fixedChannels = (const int*)pixel;
    const Pixel_R6G6B6* pixel666 = (const Pixel_R6G6B6*)pixel;
    unsigned int result = 0;

    switch (format) {
    case ColorFormat_R5G6B5:
        result = *(const unsigned short*)pixel;
        break;

    case ColorFormat_R6G6B6:
        result = PackR5G6B5(Expand6To8(pixel666->r), Expand6To8(pixel666->g), Expand6To8(pixel666->b));
        break;

    case ColorFormat_R8G8B8:
    case ColorFormat_X8R8G8B8:
    case ColorFormat_A8R8G8B8:
        result = PackR5G6B5(bytes[2], bytes[1], bytes[0]);
        break;

    case ColorFormat_A8B8G8R8:
        result = PackR5G6B5(bytes[0], bytes[1], bytes[2]);
        break;

    case ColorFormat_ARGB_Fixed:
        result = (FixedToByte(fixedChannels[3]) >> 3)
               | ((FixedToByte(fixedChannels[2]) & 0xfc) << 3)
               | ((FixedToByte(fixedChannels[1]) & 0xf8) << 8);
        break;

    case ColorFormat_RGBA_Fixed:
        result = (FixedToByte(fixedChannels[2]) >> 3)
               | ((FixedToByte(fixedChannels[1]) & 0xfc) << 3)
               | ((FixedToByte(fixedChannels[0]) & 0xf8) << 8);
        break;

    default:
        break;
    }
    return result;
}

unsigned int CColor::ConvertTo_X8R8G8B8(ColorFormat format, const void* pixel)
{
    const unsigned char* bytes = (const unsigned char*)pixel;
    const int* fixedChannels = (const int*)pixel;
    const Pixel_R6G6B6* pixel666 = (const Pixel_R6G6B6*)pixel;
    unsigned int result = 0;
    unsigned short color;
    unsigned int red;
    unsigned int green;
    unsigned int blue;

    switch (format) {
    case ColorFormat_R5G6B5:
        color = *(const unsigned short*)pixel;
        red = ((color & 0xf800) >> 8) | ((color & 0xf800) >> 13);
        green = ((color & 0x07e0) >> 3) | ((color & 0x07e0) >> 8);
        blue = ((color & 0x001f) << 3) | ((color & 0x001f) >> 2);
        result = blue | (green << 8) | (red << 16) | 0xff000000;
        break;

    case ColorFormat_R8G8B8:
    case ColorFormat_A8R8G8B8:
        result = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | 0xff000000;
        break;

    case ColorFormat_R6G6B6:
        red = pixel666->r;
        green = pixel666->g;
        blue = pixel666->b;
        red = (red << 2) | (red & 3);
        green = (green << 2) | (green & 3);
        blue = (blue << 2) | (blue & 3);
        result = blue | (green << 8) | (red << 16) | 0xff000000;
        break;

    case ColorFormat_X8R8G8B8:
        result = *(const unsigned int*)pixel;
        break;

    case ColorFormat_A8B8G8R8:
        result = bytes[2] | (bytes[1] << 8) | (bytes[0] << 16) | 0xff000000;
        break;

    case ColorFormat_ARGB_Fixed:
        result = 0xff000000
               | FixedToByte(fixedChannels[3])
               | (FixedToByte(fixedChannels[2]) << 8)
               | (FixedToByte(fixedChannels[1]) << 16);
        break;

    case ColorFormat_RGBA_Fixed:
        result = 0xff000000
               | FixedToByte(fixedChannels[2])
               | (FixedToByte(fixedChannels[1]) << 8)
               | (FixedToByte(fixedChannels[0]) << 16);
        break;

    default:
        break;
    }
    return result;
}

unsigned int CColor::ConvertTo_A8R8G8B8(ColorFormat format, const void* pixel)
{
    const unsigned char* bytes = (const unsigned char*)pixel;
    const int* fixedChannels = (const int*)pixel;
    const Pixel_R6G6B6* pixel666 = (const Pixel_R6G6B6*)pixel;
    unsigned int result = 0;
    unsigned short color;
    unsigned int red;
    unsigned int green;
    unsigned int blue;

    switch (format) {
    case ColorFormat_R5G6B5:
        color = *(const unsigned short*)pixel;
        red = ((color & 0xf800) >> 8) | ((color & 0xf800) >> 13);
        green = ((color & 0x07e0) >> 3) | ((color & 0x07e0) >> 8);
        blue = ((color & 0x001f) << 3) | ((color & 0x001f) >> 2);
        result = blue | (green << 8) | (red << 16) | 0xff000000;
        break;

    case ColorFormat_R8G8B8:
    case ColorFormat_X8R8G8B8:
        result = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | 0xff000000;
        break;

    case ColorFormat_A8R8G8B8:
        result = *(const unsigned int*)pixel;
        break;

    case ColorFormat_A8B8G8R8:
        result = bytes[2] | (bytes[1] << 8) | (bytes[0] << 16) | (bytes[3] << 24);
        break;

    case ColorFormat_ARGB_Fixed:
        result = FixedToByte(fixedChannels[3])
               | (FixedToByte(fixedChannels[2]) << 8)
               | (FixedToByte(fixedChannels[1]) << 16)
               | (FixedToByte(fixedChannels[0]) << 24);
        break;

    case ColorFormat_RGBA_Fixed:
        result = FixedToByte(fixedChannels[2])
               | (FixedToByte(fixedChannels[1]) << 8)
               | (FixedToByte(fixedChannels[0]) << 16)
               | (FixedToByte(fixedChannels[3]) << 24);
        break;

    case ColorFormat_R6G6B6:
        red = pixel666->r;
        green = pixel666->g;
        blue = pixel666->b;
        red = (red << 2) | (red & 3);
        green = (green << 2) | (green & 3);
        blue = (blue << 2) | (blue & 3);
        result = blue | (green << 8) | (red << 16) | 0xff000000;
        break;

    default:
        break;
    }
    return result;
}

} } } }
