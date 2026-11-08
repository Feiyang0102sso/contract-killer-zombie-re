// CPNG.h - minimal PNG (v1.2) chunk parsing helpers.
// (created by b005 as PROVISIONAL, completed by b037x)
#ifndef CPNG_H
#define CPNG_H

namespace com { namespace glu { namespace platform { namespace components {
class CInputStream;
} } } }

namespace com { namespace glu { namespace platform { namespace graphics {

class CPNG {
public:
    /** Chunk types recognised by ParsePNGV12ChunkType (0 = anything else). */
    enum PNGV12ChunkType {
        PNGV12ChunkType_Unknown         = 0,
        PNGV12ChunkType_ImageHeader     = 1,    // IHDR
        PNGV12ChunkType_Palette         = 2,    // PLTE
        PNGV12ChunkType_Transparency    = 3,    // tRNS
        PNGV12ChunkType_ImageData       = 4,    // IDAT
        PNGV12ChunkType_End             = 5     // IEND
    };

    /** Chunk length + type, read before the chunk data. */
    struct PNGV12ChunkHeader {
        unsigned int length;        // 0x00 size of the chunk data in bytes
        PNGV12ChunkType type;       // 0x04
    };

    /** CRC read after the chunk data. */
    struct PNGV12ChunkFooter {
        unsigned int crc;           // 0x00
    };

    /** Read the 4-byte chunk type name and classify it. */
    static PNGV12ChunkType ParsePNGV12ChunkType(components::CInputStream& stream);
    /** Returns 1 if the stream had no read error. */
    static unsigned char ParsePNGV12ChunkHeader(components::CInputStream& stream, PNGV12ChunkHeader& header);
    static unsigned char ParsePNGV12ChunkFooter(components::CInputStream& stream, PNGV12ChunkFooter& footer);
};

} } } }

#endif
