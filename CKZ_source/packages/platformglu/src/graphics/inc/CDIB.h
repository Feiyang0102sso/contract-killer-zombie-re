// CDIB.h - device independent bitmap: an uncompressed Windows BMP (v3 header) loaded into memory.
// (taken over by b034x from b011x's PROVISIONAL; owner graphics/src/CDIB.cpp) sizeof == 0x58.
#ifndef CDIB_H
#define CDIB_H

#include "CClass.h"

namespace com { namespace glu { namespace platform {

namespace components {
class CInputStream;
}

namespace graphics {

class CDIB : public core::CClass {
public:
    enum { ClassId = 0x0275a643 };

    /** Kind of the loaded image. */
    enum Format {
        Format_None = 0,
        Format_BMP = 1
    };

    /** BITMAPFILEHEADER + BITMAPINFOHEADER as read from the stream, plus derived sizes. */
    struct BMPV3Info {
        unsigned char m_magic[2];       // 0x00 "BM"
        unsigned int m_fileSize;        // 0x04
        unsigned int m_dataOffset;      // 0x08 offset of the pixels from the file start
        unsigned int m_headerSize;      // 0x0c size of the info header
        int m_width;                    // 0x10
        int m_height;                   // 0x14
        unsigned short m_planes;        // 0x18
        unsigned short m_bitCount;      // 0x1a bits per pixel
        unsigned int m_compression;     // 0x1c 3: bit fields
        unsigned int m_imageSize;       // 0x20
        int m_xPixelsPerMeter;          // 0x24
        int m_yPixelsPerMeter;          // 0x28
        int m_colorsUsed;               // 0x2c
        int m_colorsImportant;          // 0x30
        unsigned int m_paletteOffset;   // 0x34 offset of the palette from the file start
        unsigned int m_paletteSize;     // 0x38 palette / bit field masks size in bytes
    };

    CDIB();

    virtual ~CDIB() {}

    /** Read the BMP headers; 1 when they are valid. */
    static unsigned char ParseBMPV3Info(components::CInputStream& stream, BMPV3Info& info);

    void Destroy();
    /** Decode an image of mime type `type` from `stream`. */
    unsigned char Load(components::CInputStream& stream, unsigned int type);

    int m_format;               // 0x08 Format
    BMPV3Info m_info;           // 0x0c
    int m_stride;               // 0x48 bytes from one row to the next (negative: bottom-up rows)
    unsigned char* m_palette;   // 0x4c
    unsigned int m_paletteSize; // 0x50
    unsigned char* m_pixels;    // 0x54 first (top) row
};

} } } }

#endif
