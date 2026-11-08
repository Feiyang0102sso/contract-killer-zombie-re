// CDIB.cpp - device independent bitmap: uncompressed BMP loading.
#include <stdlib.h>
#include "CDIB.h"
#include "CInputStream.h"

namespace com { namespace glu { namespace platform { namespace graphics {

// ---- constants ----
static const unsigned int BMP_FileHeaderSize = 14;     // BITMAPFILEHEADER
static const unsigned int BMP_CompressionBitFields = 3;

unsigned char CDIB::ParseBMPV3Info(components::CInputStream& stream, BMPV3Info& info)
{
    if (stream.Available() == 0) {
        return 0;
    }
    info.m_magic[0] = stream.ReadUInt8();
    info.m_magic[1] = stream.ReadUInt8();
    if (info.m_magic[0] != 'B' || info.m_magic[1] != 'M') {
        return 0;
    }
    info.m_fileSize = stream.ReadUInt32();
    stream.Skip(4);     // reserved
    info.m_dataOffset = stream.ReadUInt32();
    info.m_headerSize = stream.ReadUInt32();
    info.m_width = stream.ReadInt32();
    info.m_height = stream.ReadInt32();
    info.m_planes = stream.ReadUInt16();
    info.m_bitCount = stream.ReadUInt16();
    info.m_compression = stream.ReadUInt32();
    info.m_imageSize = stream.ReadUInt32();
    info.m_xPixelsPerMeter = stream.ReadInt32();
    info.m_yPixelsPerMeter = stream.ReadInt32();
    info.m_colorsUsed = stream.ReadInt32();
    info.m_colorsImportant = stream.ReadInt32();
    info.m_paletteOffset = info.m_headerSize + BMP_FileHeaderSize;

    if (info.m_bitCount <= 8) {
        // indexed: 4 bytes per palette entry
        int colors = info.m_colorsUsed;
        if (colors == 0) {
            colors = 1 << info.m_bitCount;
        }
        info.m_paletteSize = colors * 4;
    } else if ((info.m_bitCount == 16 || info.m_bitCount == 32) && info.m_compression == BMP_CompressionBitFields) {
        // three channel masks
        info.m_paletteSize = 12;
    } else {
        info.m_paletteSize = 0;
    }

    unsigned char valid = 0;
    if (!stream.m_error) {
        if (info.m_fileSize != 0) {
            valid = 1;
        }
    }
    return valid;
}

CDIB::CDIB()
    : core::CClass(ClassId)
{
    m_format = Format_None;
    m_stride = 0;
    m_palette = NULL;
    m_paletteSize = 0;
    m_pixels = NULL;
}

void CDIB::Destroy()
{
    if (m_format == Format_BMP) {
        delete[] m_palette;
        delete[] m_pixels;
    }
    m_format = Format_None;
    m_stride = 0;
    m_palette = NULL;
    m_paletteSize = 0;
    m_pixels = NULL;
}

unsigned char CDIB::Load(components::CInputStream& stream, unsigned int type)
{
    unsigned int available = stream.Available();
    Destroy();
    if (available == 0) {
        return 0;
    }
    m_format = Format_BMP;
    if (!ParseBMPV3Info(stream, m_info)) {
        return 0;
    }

    // rows are 4 byte aligned and stored bottom-up
    m_stride = -((((int)(m_info.m_width * m_info.m_bitCount) + 31) >> 3) & ~3);

    int paletteGap = 0;
    m_paletteSize = m_info.m_paletteSize;
    if (m_paletteSize != 0) {
        paletteGap = m_info.m_paletteOffset - BMP_FileHeaderSize - m_info.m_headerSize;
        if (paletteGap > 0) {
            stream.Skip(paletteGap);
        }
        m_palette = new unsigned char[m_paletteSize];
        stream.Read(m_palette, m_paletteSize);
    }

    int pixelGap = m_info.m_dataOffset - BMP_FileHeaderSize - paletteGap - m_info.m_headerSize - m_paletteSize;
    if (pixelGap > 0) {
        stream.Skip(pixelGap);
    }

    unsigned int pixelSize = abs(m_stride) * m_info.m_height;
    m_pixels = new unsigned char[pixelSize];
    stream.Read(m_pixels, pixelSize);
    if (!stream.m_error) {
        // point at the top row
        m_pixels += (m_info.m_height - 1) * m_stride;
        return 1;
    }
    Destroy();
    return 0;
}

} } } }
