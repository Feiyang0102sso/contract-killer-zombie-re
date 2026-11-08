// CPNG.cpp - PNG (v1.2) chunk header / footer parsing.
#include "CPNG.h"
#include "CInputStream.h"

namespace com { namespace glu { namespace platform { namespace graphics {

// chunk type names as they appear in the file
static const unsigned char CPNG_ChunkTypeImageHeaderBytes[4] = { 'I', 'H', 'D', 'R' };
static const unsigned char CPNG_ChunkTypePaletteBytes[4] = { 'P', 'L', 'T', 'E' };
static const unsigned char CPNG_ChunkTypeTransparencyBytes[4] = { 't', 'R', 'N', 'S' };
static const unsigned char CPNG_ChunkTypeImageDataBytes[4] = { 'I', 'D', 'A', 'T' };
static const unsigned char CPNG_ChunkTypeEndBytes[4] = { 'I', 'E', 'N', 'D' };

CPNG::PNGV12ChunkType CPNG::ParsePNGV12ChunkType(components::CInputStream& stream)
{
    unsigned char name[4];
    stream.Read(name, 4);

    if (memcmp(name, CPNG_ChunkTypeImageHeaderBytes, 4) == 0) {
        return PNGV12ChunkType_ImageHeader;
    }
    if (memcmp(name, CPNG_ChunkTypePaletteBytes, 4) == 0) {
        return PNGV12ChunkType_Palette;
    }
    if (memcmp(name, CPNG_ChunkTypeTransparencyBytes, 4) == 0) {
        return PNGV12ChunkType_Transparency;
    }
    if (memcmp(name, CPNG_ChunkTypeImageDataBytes, 4) == 0) {
        return PNGV12ChunkType_ImageData;
    }
    if (memcmp(name, CPNG_ChunkTypeEndBytes, 4) == 0) {
        return PNGV12ChunkType_End;
    }
    return PNGV12ChunkType_Unknown;
}

unsigned char CPNG::ParsePNGV12ChunkHeader(components::CInputStream& stream, PNGV12ChunkHeader& header)
{
    header.length = stream.ReadUInt32();
    header.type = ParsePNGV12ChunkType(stream);
    return stream.m_error == 0;
}

unsigned char CPNG::ParsePNGV12ChunkFooter(components::CInputStream& stream, PNGV12ChunkFooter& footer)
{
    footer.crc = stream.ReadUInt32();
    return stream.m_error == 0;
}

} } } }
