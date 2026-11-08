// CBlit.h - software blit loops: buffer to buffer copies (optionally flipped and scaled), solid
// rectangle fills and stream decoding into a buffer (optionally color keyed, flipped, transposed).
// (drafted by b037x as PROVISIONAL; completed by b034x, the writer of graphics/src/CBlit.cpp)
// The loops are registered with CBlitUtil::AddBlitOp and found with CBlitUtil::GetBlitOp.
#ifndef CBLIT_H
#define CBLIT_H

#include "CRenderSurfaceBuffer.h"

namespace com { namespace glu { namespace platform {

namespace components {
class CInputStream;
}

namespace graphics {

class CBlit {
public:
    /** Copy a source buffer into a destination buffer. */
    struct BufferOpDesc {
        short m_sourcePitch;            // 0x00 bytes per source row
        const void* m_source;           // 0x04 first source pixel
        unsigned int m_reserved08;      // 0x08
        short m_destinationPitch;       // 0x0c bytes per destination row
        void* m_destination;            // 0x10 first destination pixel
        int m_width;                    // 0x14 pixels per row (destination)
        int m_height;                   // 0x18 rows (destination)
        unsigned char m_flipX;          // 0x1c read the source rows right to left
        unsigned char m_flipY;          // 0x1d read the source rows bottom to top
        int m_scaleX;                   // 0x20 16.16 source step per destination pixel (0x10000: 1)
        int m_scaleY;                   // 0x24
    };

    /** Fill a rectangle of a buffer with one color. */
    struct RectangleOpDesc {
        short m_destinationPitch;       // 0x00
        void* m_destination;            // 0x04
        RSBFormat m_colorFormat;        // 0x08 format of *m_color
        const void* m_color;            // 0x0c
        int m_width;                    // 0x10
        int m_height;                   // 0x14
    };

    /** Decode pixels read from a stream into a buffer. */
    struct StreamOpDesc {
        components::CInputStream* m_stream;     // 0x00
        unsigned int m_rowSkip;                 // 0x04 bytes skipped after every source row
        const unsigned int* m_palette;          // 0x08 X8R8G8B8 entries of paletted sources
        short m_destinationPitch;               // 0x0c
        void* m_destination;                    // 0x10
        int m_width;                            // 0x14 pixels per source row
        int m_height;                           // 0x18 source rows
        unsigned char m_colorKey[4];            // 0x1c B, G, R (as stored in X8R8G8B8): pixels of this color are skipped
        unsigned char m_colorKeyEnabled;        // 0x20
        unsigned char m_transpose;              // 0x21 source rows become destination columns
        unsigned char m_flipX;                  // 0x22
        unsigned char m_flipY;                  // 0x23
    };

    static void Buffer_R5G6B5_To_R5G6B5(const BufferOpDesc& op);
    static void Buffer_R5G5B5A1_To_R5G5B5A1(const BufferOpDesc& op);
    static void Buffer_R4G4B4A4_To_R4G4B4A4(const BufferOpDesc& op);
    static void Buffer_B8G8R8_To_B8G8R8(const BufferOpDesc& op);
    static void Buffer_A8B8G8R8_To_R5G6B5(const BufferOpDesc& op);
    static void Buffer_A8B8G8R8_To_R5G5B5A1(const BufferOpDesc& op);
    static void Buffer_A8B8G8R8_To_R4G4B4A4(const BufferOpDesc& op);
    static void Buffer_A8B8G8R8_To_A8B8G8R8(const BufferOpDesc& op);
    static void Rectangle_To_R5G6B5(const RectangleOpDesc& op);
    static void Rectangle_To_R5G5B5A1(const RectangleOpDesc& op);
    static void Rectangle_To_R4G4B4A4(const RectangleOpDesc& op);
    static void Rectangle_To_B8G8R8(const RectangleOpDesc& op);
    static void Rectangle_To_A8B8G8R8(const RectangleOpDesc& op);
    static void Stream_R8G8B8_To_R5G6B5_ColorKeyE(const StreamOpDesc& op);
    static void Stream_R8G8B8_To_R5G5B5A1_ColorKeyE(const StreamOpDesc& op);
    static void Stream_R8G8B8_To_B8G8R8_ColorKeyE(const StreamOpDesc& op);
    static void Stream_R8G8B8_To_A8B8G8R8_ColorKeyE(const StreamOpDesc& op);
    static void Stream_A8R8G8B8_To_R5G6B5_ColorKeyE(const StreamOpDesc& op);
    static void Stream_A8R8G8B8_To_R4G4B4A4_ColorKeyE(const StreamOpDesc& op);
    static void Stream_A8R8G8B8_To_A8B8G8R8_ColorKeyE(const StreamOpDesc& op);
    static void Stream_A8B8G8R8_To_R4G4B4A4_ColorKeyE(const StreamOpDesc& op);
    static void Stream_A8B8G8R8_To_A8B8G8R8_ColorKeyE(const StreamOpDesc& op);
    static void Stream_P16X8R8G8B8_To_R5G6B5_ColorKeyE(const StreamOpDesc& op);
    static void Stream_P16X8R8G8B8_To_R5G5B5A1_ColorKeyE(const StreamOpDesc& op);
    static void Stream_P16X8R8G8B8_To_B8G8R8_ColorKeyE(const StreamOpDesc& op);
    static void Stream_P16X8R8G8B8_To_A8B8G8R8_ColorKeyE(const StreamOpDesc& op);
    static void Stream_P256X8R8G8B8_To_R5G6B5_ColorKeyE(const StreamOpDesc& op);
    static void Stream_P256X8R8G8B8_To_R5G5B5A1_ColorKeyE(const StreamOpDesc& op);
    static void Stream_P256X8R8G8B8_To_B8G8R8_ColorKeyE(const StreamOpDesc& op);
    static void Stream_P256X8R8G8B8_To_A8B8G8R8_ColorKeyE(const StreamOpDesc& op);
};

} } } }

#endif
