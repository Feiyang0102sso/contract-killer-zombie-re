// CRenderSurfaceBuffer.cpp - pixel buffers of the render surfaces and their image decoders
// (PNG through libpng, JPEG through libjpeg, BMP, M3G compressed textures and SBI).
#include "CRenderSurfaceBuffer.h"
#include "CInputStream.h"
#include "CArrayInputStream.h"
#include "CColor.h"
#include "CBlit.h"
#include "CBlitUtil.h"
#include "CDIB.h"
#include "ICStdUtil.h"
#include "png.h"
// libjpeg 6b's jpeglib.h has no C++ guard
extern "C" {
#include "jpeglib.h"
}

namespace com { namespace glu { namespace platform { namespace graphics {

// mime type keys of the image formats Initialize decodes
static const unsigned int ImageType_PNG = 0xb7178678;
static const unsigned int ImageType_DIB = 0xb717945f;
static const unsigned int ImageType_SBI = 0x0c20040b;
static const unsigned int ImageType_M3G = 0x6e0412f4;
static const unsigned int ImageType_JPEG = 0x7179dacc;
// largest image dimension
static const unsigned int MaxImageDimension = 0x1000;
// size of the buffer the decoders read palettes into
static const unsigned int PaletteBufferSize = 0x400;
// color the decoders fill transparent palette entries / the background with
static const unsigned int ColorKeyMagenta = 0xff00ff;
// size of the BITMAPFILEHEADER in front of the BMP info header
static const unsigned int BMPFileHeaderSize = 14;
// a source format of Convert that is copied as it is
static const unsigned int RSBFormat_Raw = 0x5c0104;

/** Bytes the PNG reader buffers in front of libpng. */
static const int PNGStreamBufferSize = 0x200;

/** Read state of the libpng read callback (png_struct::io_ptr points to a pointer to it). */
struct CRenderSurfaceBuffer_PNGStream {
    unsigned char m_buffer[PNGStreamBufferSize];    // 0x000 the last m_buffered bytes are unread
    components::CInputStream* m_stream;             // 0x200
    int m_buffered;                                 // 0x204
};

/** libjpeg error manager that only records that an error happened. */
struct CRenderSurfaceBuffer_JPEGError {
    jpeg_error_mgr m_manager;       // 0x00
    unsigned char m_valid;          // 0x84 cleared by error_exit
};

/** libjpeg source manager reading from a CInputStream. */
struct CRenderSurfaceBuffer_JPEGSource {
    jpeg_source_mgr m_manager;          // 0x00
    components::CInputStream* m_stream; // 0x1c
    JOCTET m_buffer[0x1000];            // 0x20
};

static int CRenderSurfaceBuffer_AdjustToPowOfTwoDimension(int dimension);
void jpeg_stream_src(jpeg_decompress_struct* cinfo, components::CInputStream* stream);
static void init_source(jpeg_decompress_struct* cinfo);
static boolean fill_input_buffer(jpeg_decompress_struct* cinfo);
static void skip_input_data(jpeg_decompress_struct* cinfo, long count);
static void term_source(jpeg_decompress_struct* cinfo);
static void error_exit(jpeg_common_struct* cinfo);
static void output_message(jpeg_common_struct* cinfo);
static void format_message(jpeg_common_struct* cinfo, char* buffer);
void CRenderSurfaceBuffer_InitializeFromPNG_Config(RSBFormat& format, const CRenderSurfaceBuffer::SourceStreamDesc* desc,
                                                   int colorKind, int bitDepth, unsigned char& colorKeyEnabled,
                                                   unsigned int& colorKey, unsigned char* trans, unsigned char transRed,
                                                   unsigned char transGreen, unsigned char transBlue, int transCount,
                                                   RSBFormat& sourceFormat, int& alphaKind);
void* CRenderSurfaceBuffer_InitializeFromPNG_Malloc(png_struct_def* png, unsigned long size);
void CRenderSurfaceBuffer_InitializeFromPNG_Free(png_struct_def* png, void* pointer);
void CRenderSurfaceBuffer_InitializeFromPNG_InputStream(png_struct_def* png, unsigned char* data, unsigned long length);

void CRenderSurfaceBuffer::Destroy()
{
    DestroyBufferData();
    m_format = RSBFormat_None;
    m_flags = 0;
    m_pitch = 0;
    m_width = 0;
    m_height = 0;
    m_imageWidth = 0;
    m_imageHeight = 0;
}

unsigned char CRSBFrag::Convert(RSBFormat sourceFormat, const void* source, RSBFormat destinationFormat,
                                void* destination)
{
    switch (destinationFormat) {
    case RSBFormat_R5G6B5: {
        unsigned short value;
        if (sourceFormat == (RSBFormat)RSBFormat_Raw) {
            value = ((const unsigned short*)source)[1];
        } else {
            value = components::CColor::ConvertTo_R5G6B5((components::ColorFormat)sourceFormat, source);
        }
        *(unsigned short*)destination = value;
        return 1;
    }
    case RSBFormat_X8R8G8B8: {
        unsigned int value;
        if (sourceFormat == (RSBFormat)RSBFormat_Raw) {
            value = *(const unsigned int*)source;
        } else {
            value = components::CColor::ConvertTo_X8R8G8B8((components::ColorFormat)sourceFormat, source);
        }
        *(unsigned int*)destination = value;
        return 1;
    }
    case RSBFormat_A8R8G8B8: {
        unsigned int value;
        if (sourceFormat == (RSBFormat)RSBFormat_Raw) {
            value = *(const unsigned int*)source;
        } else {
            value = components::CColor::ConvertTo_A8R8G8B8((components::ColorFormat)sourceFormat, source);
        }
        *(unsigned int*)destination = value;
        return 1;
    }
    case RSBFormat_ARGB_Fixed:
        if (sourceFormat == (RSBFormat)RSBFormat_Raw) {
            int* channels = (int*)destination;
            channels[0] = *(const int*)source;
            channels[1] = *(const int*)source;
            channels[2] = *(const int*)source;
            channels[3] = *(const int*)source;
        } else {
            ((components::Color_ARGB_fixed*)destination)->Make((components::ColorFormat)sourceFormat, source);
        }
        return 1;
    }
    return 0;
}

unsigned char CRenderSurfaceBuffer::Initialize(RSBFormat format, unsigned short width, unsigned short height,
                                               unsigned char powerOfTwo, void** palette, const SourceStreamDesc* desc,
                                               PitchCondition pitchCondition, short pitch, void* pixels)
{
    unsigned char paletteBuffer[PaletteBufferSize];
    Destroy();
    m_format = format;
    unsigned short sizeFlags;
    unsigned short externalFlags;
    unsigned short allocatedWidth;
    unsigned short allocatedHeight;
    if (powerOfTwo) {
        allocatedWidth = CRenderSurfaceBuffer_AdjustToPowOfTwoDimension(width);
        m_width = allocatedWidth;
        allocatedHeight = CRenderSurfaceBuffer_AdjustToPowOfTwoDimension(height);
        externalFlags = Flag_ExternalData | Flag_PowerOfTwo;
        sizeFlags = Flag_PowerOfTwo;
        m_height = allocatedHeight;
        m_imageWidth = width;
        m_imageHeight = height;
    } else {
        externalFlags = Flag_ExternalData;
        allocatedHeight = height;
        allocatedWidth = width;
        sizeFlags = 0;
        m_width = width;
        m_height = height;
        m_imageWidth = width;
        m_imageHeight = height;
    }
    m_flags = sizeFlags;

    unsigned char result = 0;
    if (allocatedWidth <= MaxImageDimension && allocatedHeight <= MaxImageDimension) {
        unsigned short rowPitch = 0;
        if (format != RSBFormat_None && allocatedWidth != 0 && allocatedHeight != 0) {
            if (pitchCondition == PitchCondition_Given) {
                rowPitch = pitch;
                m_pitch = rowPitch;
            } else if ((unsigned int)pitchCondition <= PitchCondition_Negative) {
                // The original range check is unsigned (BHI at 0x80ba6).
                rowPitch = (format & RSBFormat_BytesPerPixelMask) * allocatedWidth;
                m_pitch = rowPitch;
            }
        }
        if (allocatedWidth != 0 && pixels != NULL && allocatedHeight != 0) {
            if (rowPitch == 0) {
                rowPitch = 0;
                result = 0;
            } else {
                // the caller's memory
                m_data = pixels;
                if (pitchCondition == PitchCondition_Negative) {
                    rowPitch = -rowPitch;
                    m_pitch = rowPitch;
                }
                result = 1;
                m_flags = externalFlags;
            }
        }
        if (desc != NULL) {
            switch (desc->m_type) {
            case ImageType_SBI:
                result = InitializeFromSBI(paletteBuffer, palette, desc, pitchCondition, 0, pixels);
                break;
            case ImageType_JPEG:
                result = InitializeFromJPEG(paletteBuffer, palette, desc, pitchCondition, 0, pixels);
                break;
            case ImageType_M3G:
                result = InitializeFromM3G(paletteBuffer, palette, desc, pitchCondition, 0, pixels);
                break;
            case ImageType_PNG:
                result = InitializeFromPNG(paletteBuffer, palette, desc, pitchCondition, 0, pixels);
                break;
            case ImageType_DIB:
                result = InitializeFromDIB(paletteBuffer, palette, desc, pitchCondition, 0, pixels);
                break;
            default:
                result = 0;
                break;
            }
        } else if (pixels == NULL) {
            // allocate the pixels
            if (format != RSBFormat_None && allocatedWidth != 0 && height != 0 && rowPitch != 0) {
                unsigned int absolutePitch = (short)rowPitch;
                if ((short)rowPitch < 0) {
                    absolutePitch = -(short)rowPitch;
                }
                if (absolutePitch >= (format & RSBFormat_BytesPerPixelMask) * allocatedWidth) {
                    unsigned int size = allocatedHeight * absolutePitch;
                    m_data = malloc(size);
                    memset(m_data, 0, size);
                    if (pitchCondition == PitchCondition_Negative || m_pitch < 0) {
                        MakePitchNegative();
                    }
                    m_flags = (m_flags & ~(Flag_OwnsData | Flag_ExternalData)) | Flag_OwnsData;
                    return 1;
                }
            }
        }
        if (result) {
            return result;
        }
    }
    Destroy();
    return 0;
}

static int CRenderSurfaceBuffer_AdjustToPowOfTwoDimension(int dimension)
{
    if (dimension == 0) {
        return 0;
    }
    if (dimension == 1) {
        return 1;
    }
    if (dimension <= 0x2) {
        return 0x2;
    }
    if (dimension <= 0x4) {
        return 0x4;
    }
    if (dimension <= 0x8) {
        return 0x8;
    }
    if (dimension <= 0x10) {
        return 0x10;
    }
    if (dimension <= 0x20) {
        return 0x20;
    }
    if (dimension <= 0x40) {
        return 0x40;
    }
    if (dimension <= 0x80) {
        return 0x80;
    }
    if (dimension <= 0x100) {
        return 0x100;
    }
    if (dimension <= 0x200) {
        return 0x200;
    }
    if (dimension <= 0x400) {
        return 0x400;
    }
    if (dimension <= 0x800) {
        return 0x800;
    }
    if (dimension <= 0x1000) {
        return 0x1000;
    }
    if (dimension <= 0x2000) {
        return 0x2000;
    }
    if (dimension <= 0x4000) {
        return 0x4000;
    }
    if (dimension <= 0x8000) {
        return 0x8000;
    }
    if (dimension <= 0x10000) {
        return 0x10000;
    }
    return 0;
}

unsigned char CRenderSurfaceBuffer::InitializeFromDIB(unsigned char* buffer, void** palette, const SourceStreamDesc* desc,
                                                      PitchCondition pitchCondition, short pitch, void* pixels)
{
    if (desc == NULL) {
        return 0;
    }
    components::CInputStream* stream = desc->m_stream;
    if (stream == NULL) {
        return 0;
    }
    CDIB::BMPV3Info info;
    if (!CDIB::ParseBMPV3Info(*stream, info)) {
        return 0;
    }

    // bytes of one source row (rows are padded to 4 bytes)
    unsigned int bitCount = info.m_bitCount;
    int rowBytes;
    unsigned char ok;
    if (bitCount < 8) {
        rowBytes = 0;
        ok = 0;
        if (8 % bitCount == 0) {
            int bits = info.m_width * bitCount;
            rowBytes = bits / 8 + ((bits & 7) != 0);
            ok = 1;
        }
    } else {
        rowBytes = info.m_width * (bitCount >> 3);
        ok = 1;
    }
    if (info.m_width > (int)MaxImageDimension || info.m_height > (int)MaxImageDimension || !ok) {
        return 0;
    }
    RSBFormat sourceFormat = RSBFormat_R8G8B8;
    if (info.m_paletteSize != 0) {
        sourceFormat = RSBFormat_P256X8R8G8B8;
        if (bitCount < 8) {
            sourceFormat = RSBFormat_P16X8R8G8B8;
        }
    }
    int paddedRowBytes = rowBytes + (-rowBytes & 3);

    RSBFormat format = m_format;
    if (format == RSBFormat_None) {
        // the format map of the description picks the destination format
        int index;
        for (index = 0;; index++) {
            if (index > 9) {
                return 0;
            }
            if (desc->m_formatMap[desc->m_colorKeyEnabled != 0][index].m_from == sourceFormat) {
                break;
            }
        }
        format = desc->m_formatMap[desc->m_colorKeyEnabled != 0][index].m_to;
        if (format == RSBFormat_P16X8R8G8B8) {
            format = RSBFormat_P256X8R8G8B8;
        }
        if (format == RSBFormat_P16A8R8G8B8) {
            format = RSBFormat_P256A8R8G8B8;
        }
        ok = 1;
        m_format = format;
    }

    unsigned char* paletteData;
    unsigned int paletteSkip;
    if (info.m_paletteSize == 0) {
        paletteData = NULL;
        paletteSkip = 0;
    } else {
        bool readPalette;
        if (palette == NULL) {
            readPalette = true;
            paletteData = buffer;
        } else {
            paletteData = (unsigned char*)*palette;
            readPalette = false;
            if (paletteData == NULL) {
                if ((format & 0x8000) == 0) {
                    readPalette = true;
                    paletteData = buffer;
                } else {
                    // a palettized buffer keeps the palette
                    paletteData = (unsigned char*)malloc(PaletteBufferSize);
                    *palette = paletteData;
                    if (paletteData == NULL) {
                        ok = 0;
                        paletteData = buffer;
                    }
                    readPalette = true;
                }
            }
        }
        if (!ok) {
            return 0;
        }
        paletteSkip = info.m_paletteOffset - BMPFileHeaderSize - info.m_headerSize;
        stream->Skip(paletteSkip);
        if (readPalette) {
            stream->Read(paletteData, info.m_paletteSize);
        } else {
            stream->Skip(info.m_paletteSize);
        }
        if (stream->m_error) {
            return 0;
        }
        ok = 1;
        if (!desc->m_colorKeyEnabled) {
            if ((m_format == RSBFormat_P16X8R8G8B8 || m_format == RSBFormat_P256X8R8G8B8) && info.m_paletteSize > 3) {
                for (unsigned int entry = 0; entry < info.m_paletteSize >> 2; entry++) {
                    paletteData[entry * 4 + 3] = 0xff;
                }
            }
        } else if (info.m_paletteSize > 3) {
            for (unsigned int entry = 0; entry < info.m_paletteSize >> 2; entry++) {
                if (((*(unsigned int*)(paletteData + entry * 4) ^ desc->m_colorKey) & 0xffffff) == 0) {
                    // the color key becomes transparent magenta
                    paletteData[entry * 4] = 0xff;
                    paletteData[entry * 4 + 1] = 0;
                    paletteData[entry * 4 + 2] = 0xff;
                    paletteData[entry * 4 + 3] = 0;
                } else {
                    paletteData[entry * 4 + 3] = 0xff;
                }
            }
        }
    }

    unsigned int imageWidth = info.m_width;
    unsigned int imageHeight = info.m_height;
    unsigned int allocatedWidth;
    unsigned int allocatedHeight;
    if (m_flags & Flag_PowerOfTwo) {
        allocatedWidth = CRenderSurfaceBuffer_AdjustToPowOfTwoDimension(imageWidth);
        allocatedHeight = CRenderSurfaceBuffer_AdjustToPowOfTwoDimension(imageHeight);
    } else {
        allocatedWidth = imageWidth;
        allocatedHeight = imageHeight;
    }
    unsigned short previousWidth = m_width;
    unsigned short previousHeight = m_height;
    unsigned int rowWidth;
    if (!desc->m_transpose) {
        m_width = allocatedWidth;
        m_height = allocatedHeight;
        m_imageWidth = imageWidth;
        m_imageHeight = imageHeight;
        if (previousWidth != 0 && previousWidth != allocatedWidth) {
            ok = 0;
        }
        if (previousHeight != allocatedHeight && previousHeight != 0) {
            ok = 0;
        }
        rowWidth = allocatedWidth;
    } else {
        m_width = allocatedHeight;
        m_height = allocatedWidth;
        m_imageWidth = imageHeight;
        m_imageHeight = imageWidth;
        if (previousWidth != 0 && previousWidth != allocatedHeight) {
            ok = 0;
        }
        if (previousHeight != allocatedWidth && previousHeight != 0) {
            ok = 0;
        }
        rowWidth = allocatedHeight;
    }
    if (!ok) {
        return 0;
    }

    int rowPitch;
    if ((m_format & RSBFormat_BytesPerPixelMask) == 0) {
        // 16 entry palettes: 4 bits per pixel
        int bitsPerPixel = 0;
        if (m_format == RSBFormat_P16X8R8G8B8 || m_format == RSBFormat_P16A8R8G8B8) {
            bitsPerPixel = 4;
        }
        ok = 1;
        unsigned int bits = bitsPerPixel * (unsigned short)rowWidth;
        rowPitch = bits / 8 + ((bits & 7) != 0);
    } else {
        rowPitch = (m_format & RSBFormat_BytesPerPixelMask) * (unsigned short)rowWidth;
    }
    if (m_pitch != 0) {
        int existingPitch = m_pitch;
        if (existingPitch < 0) {
            existingPitch = -existingPitch;
        }
        if (existingPitch != rowPitch) {
            return 0;
        }
    }
    m_pitch = (short)rowPitch;
    if (!ok) {
        return 0;
    }
    stream->Skip(info.m_dataOffset - BMPFileHeaderSize - paletteSkip - info.m_headerSize - info.m_paletteSize);
    if (pixels == NULL) {
        unsigned int absolutePitch = m_pitch;
        if (m_pitch < 0) {
            absolutePitch = -m_pitch;
        }
        if (absolutePitch < m_width * (m_format & RSBFormat_BytesPerPixelMask)) {
            return 0;
        }
        unsigned int size = m_height * absolutePitch;
        m_data = malloc(size);
        memset(m_data, 0, size);
        if (pitchCondition == PitchCondition_Negative || m_pitch < 0) {
            MakePitchNegative();
        }
        m_flags = (m_flags & ~(Flag_OwnsData | Flag_ExternalData)) | Flag_OwnsData;
    } else {
        m_data = pixels;
        if (pitchCondition == PitchCondition_Negative) {
            m_pitch = -m_pitch;
        }
        m_flags = (m_flags & ~(Flag_OwnsData | Flag_ExternalData)) | Flag_ExternalData;
    }

    CBlitUtil::BlitOpDesc blitOp;
    blitOp.m_source = sourceFormat;
    blitOp.m_destination = m_format;
    blitOp.m_colorKeyTest = (CBlitUtil::ColorKeyTest)(desc->m_colorKeyEnabled != 0);
    blitOp.m_alphaTest = CBlitUtil::AlphaTest_None;
    blitOp.m_blend = CBlitUtil::Blend_None;
    blitOp.m_opId = CBlitUtil::BlitOpId_Stream;
    CBlitUtil::GetBlitOp(blitOp);
    CBlit::StreamOpDesc decode;
    decode.m_width = info.m_width;
    decode.m_height = info.m_height;
    *(unsigned int*)decode.m_colorKey = desc->m_colorKey;
    decode.m_colorKeyEnabled = desc->m_colorKeyEnabled;
    decode.m_transpose = desc->m_transpose;
    decode.m_flipX = desc->m_flipX;
    decode.m_flipY = desc->m_flipY;
    decode.m_destinationPitch = m_pitch;
    decode.m_destination = m_data;
    if (decode.m_destinationPitch > 0) {
        // BMP rows are stored bottom-up
        decode.m_destination = (unsigned char*)decode.m_destination + (m_imageHeight - 1) * decode.m_destinationPitch;
        decode.m_destinationPitch = -decode.m_destinationPitch;
        if (decode.m_transpose) {
            decode.m_flipX = !decode.m_flipX;
            decode.m_flipY = !decode.m_flipY;
        }
    }
    if (blitOp.m_function == NULL) {
        return 0;
    }
    decode.m_stream = stream;
    int absolutePaddedRowBytes = paddedRowBytes;
    if (absolutePaddedRowBytes < 0) {
        absolutePaddedRowBytes = -absolutePaddedRowBytes;
    }
    decode.m_rowSkip = absolutePaddedRowBytes - rowBytes;
    decode.m_palette = (const unsigned int*)paletteData;
    ((void (*)(const CBlit::StreamOpDesc&))blitOp.m_function)(decode);
    return ok;
}

unsigned char CRenderSurfaceBuffer::InitializeFromJPEG(unsigned char* buffer, void** palette, const SourceStreamDesc* desc,
                                                       PitchCondition pitchCondition, short pitch, void* pixels)
{
    if (desc == NULL) {
        return 0;
    }
    CRenderSurfaceBuffer_JPEGError error;
    jpeg_decompress_struct cinfo;
    error.m_valid = 1;
    cinfo.err = jpeg_std_error(&error.m_manager);
    error.m_manager.error_exit = error_exit;
    error.m_manager.output_message = output_message;
    error.m_manager.format_message = format_message;
    jpeg_CreateDecompress(&cinfo, JPEG_LIB_VERSION, sizeof(jpeg_decompress_struct));
    jpeg_stream_src(&cinfo, desc->m_stream);
    jpeg_read_header(&cinfo, 1);
    unsigned char ok = 0;
    if (error.m_valid) {
        jpeg_start_decompress(&cinfo);
        ok = error.m_valid;
    }

    unsigned int imageWidth = cinfo.output_width;
    RSBFormat format = m_format;
    if (format == RSBFormat_None) {
        for (unsigned int index = 0; index < 10; index++) {
            if (desc->m_formatMap[desc->m_colorKeyEnabled != 0][index].m_from == RSBFormat_R8G8B8) {
                format = desc->m_formatMap[desc->m_colorKeyEnabled != 0][index].m_to;
                m_format = format;
                break;
            }
        }
    }

    unsigned char result = 0;
    if (ok) {
        if (cinfo.output_width > MaxImageDimension || cinfo.output_height > MaxImageDimension) {
            ok = 0;
        }
        unsigned int imageHeight = cinfo.output_height;
        unsigned int allocatedWidth;
        unsigned int allocatedHeight;
        if (m_flags & Flag_PowerOfTwo) {
            allocatedWidth = CRenderSurfaceBuffer_AdjustToPowOfTwoDimension(cinfo.output_width);
            allocatedHeight = CRenderSurfaceBuffer_AdjustToPowOfTwoDimension(cinfo.output_height);
        } else {
            allocatedWidth = cinfo.output_width;
            allocatedHeight = cinfo.output_height;
        }
        unsigned short previousWidth = m_width;
        unsigned short previousHeight = m_height;
        unsigned int rowWidth;
        unsigned int rowCount;
        if (!desc->m_transpose) {
            m_width = allocatedWidth;
            m_height = allocatedHeight;
            m_imageWidth = imageWidth;
            m_imageHeight = imageHeight;
            if (previousWidth != 0 && previousWidth != allocatedWidth) {
                ok = 0;
            }
            if (previousHeight != allocatedHeight && previousHeight != 0) {
                ok = 0;
            }
            rowWidth = allocatedWidth;
            rowCount = allocatedHeight;
        } else {
            m_width = allocatedHeight;
            m_height = allocatedWidth;
            m_imageWidth = imageHeight;
            m_imageHeight = imageWidth;
            if (previousWidth != 0 && previousWidth != allocatedHeight) {
                ok = 0;
            }
            rowWidth = allocatedHeight;
            rowCount = allocatedWidth;
            if (previousHeight != allocatedWidth && previousHeight != 0) {
                ok = 0;
            }
        }
        if (ok) {
            unsigned int bytesPerPixel = format & RSBFormat_BytesPerPixelMask;
            int rowPitch;
            if (bytesPerPixel == 0) {
                int bitsPerPixel = 0;
                if (format == RSBFormat_P16X8R8G8B8 || format == RSBFormat_P16A8R8G8B8) {
                    bitsPerPixel = 4;
                }
                ok = 1;
                unsigned int bits = (unsigned short)rowWidth * bitsPerPixel;
                rowPitch = bits / 8 + ((bits & 7) != 0);
            } else {
                rowPitch = bytesPerPixel * (unsigned short)rowWidth;
            }
            short existingPitch = m_pitch;
            if (existingPitch != 0) {
                if (existingPitch < 0) {
                    existingPitch = -existingPitch;
                }
                if (existingPitch != rowPitch) {
                    goto done;
                }
            }
            m_pitch = (short)rowPitch;
            if (ok) {
                if (pixels == NULL) {
                    unsigned int absolutePitch = (short)rowPitch;
                    if ((short)rowPitch < 0) {
                        absolutePitch = -(short)rowPitch;
                    }
                    if (absolutePitch < (unsigned short)rowWidth * bytesPerPixel) {
                        goto done;
                    }
                    unsigned int size = (unsigned short)rowCount * absolutePitch;
                    m_data = malloc(size);
                    memset(m_data, 0, size);
                    if (pitchCondition == PitchCondition_Negative || m_pitch < 0) {
                        MakePitchNegative();
                    }
                    m_flags = (m_flags & ~(Flag_OwnsData | Flag_ExternalData)) | Flag_OwnsData;
                } else {
                    m_data = pixels;
                    if (pitchCondition == PitchCondition_Negative) {
                        m_pitch = -(short)rowPitch;
                    }
                    m_flags = (m_flags & ~(Flag_OwnsData | Flag_ExternalData)) | Flag_ExternalData;
                }

                CBlitUtil::BlitOpDesc blitOp;
                blitOp.m_source = RSBFormat_R8G8B8;
                blitOp.m_destination = m_format;
                blitOp.m_colorKeyTest = (CBlitUtil::ColorKeyTest)(desc->m_colorKeyEnabled != 0);
                blitOp.m_alphaTest = CBlitUtil::AlphaTest_None;
                blitOp.m_blend = CBlitUtil::Blend_None;
                blitOp.m_opId = CBlitUtil::BlitOpId_Stream;
                CBlitUtil::GetBlitOp(blitOp);
                components::CArrayInputStream rowStream;
                CBlit::StreamOpDesc decode;
                decode.m_rowSkip = 0;
                decode.m_palette = NULL;
                decode.m_width = cinfo.output_width;
                decode.m_height = 1;
                *(unsigned int*)decode.m_colorKey = desc->m_colorKey;
                decode.m_colorKeyEnabled = desc->m_colorKeyEnabled;
                decode.m_transpose = desc->m_transpose;
                if (!decode.m_transpose) {
                    decode.m_flipX = desc->m_flipX;
                    decode.m_flipY = 0;
                } else {
                    decode.m_flipX = 0;
                    decode.m_flipY = desc->m_flipY;
                }
                decode.m_destinationPitch = m_pitch;
                decode.m_stream = &rowStream;
                if (blitOp.m_function != NULL) {
                    unsigned int lastRow = m_imageHeight;
                    unsigned int lastColumn = m_imageWidth;
                    JSAMPARRAY scanline = cinfo.mem->alloc_sarray((j_common_ptr)&cinfo, JPOOL_IMAGE,
                                                                  cinfo.output_width * cinfo.output_components, 1);
                    result = ok;
                    while (cinfo.output_scanline < cinfo.output_height) {
                        rowStream.Open(scanline[0], cinfo.output_components * cinfo.output_width);
                        jpeg_read_scanlines(&cinfo, scanline, 1);
                        result = error.m_valid;
                        if (error.m_valid) {
                            // libjpeg delivers R, G, B: swap to the B, G, R byte order of R8G8B8
                            for (int column = 0; column < (int)cinfo.output_width; column++) {
                                unsigned char* pixel = scanline[0] + column * 3;
                                unsigned char red = pixel[2];
                                pixel[2] = pixel[0];
                                pixel[0] = red;
                            }
                            unsigned int destinationIndex;
                            unsigned int step;
                            if (!desc->m_transpose) {
                                destinationIndex = lastRow - cinfo.output_scanline;
                                if (!desc->m_flipY) {
                                    destinationIndex = cinfo.output_scanline - 1;
                                }
                                step = m_pitch;
                            } else {
                                destinationIndex = lastColumn - cinfo.output_scanline;
                                if (!desc->m_flipX) {
                                    destinationIndex = cinfo.output_scanline - 1;
                                }
                                step = m_format & RSBFormat_BytesPerPixelMask;
                            }
                            decode.m_destination = (unsigned char*)m_data + destinationIndex * step;
                            ((void (*)(const CBlit::StreamOpDesc&))blitOp.m_function)(decode);
                        }
                        rowStream.Close();
                        if (!result) {
                            break;
                        }
                    }
                }
            }
        }
    }
done:
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    return result;
}

unsigned char CRenderSurfaceBuffer::InitializeFromM3G(unsigned char* buffer, void** palette, const SourceStreamDesc* desc,
                                                      PitchCondition pitchCondition, short pitch, void* pixels)
{
    // file identifiers of an M3G file and of a compressed texture container
    static const unsigned char image2D[12] = { 0xab, 0x4a, 0x53, 0x52, 0x31, 0x38, 0x34, 0xbb, 0x0d, 0x0a, 0x1a, 0x0a };
    static const unsigned char compressedImage2D[12] = { 0xbb, 0x53, 0x57, 0x45, 0x52, 0x56, 0x45, 0xab, 0x0d, 0x0a, 0x1a,
                                                         0x0a };
    unsigned char result = 0;
    if (desc == NULL) {
        return result;
    }
    components::CInputStream* stream = desc->m_stream;
    bool isCompressed = true;
    bool isImage = true;
    for (int index = 0; index < 12; index++) {
        unsigned int value = stream->ReadUInt8();
        if (isCompressed && value != compressedImage2D[index]) {
            isCompressed = false;
        }
        if (isImage && value != image2D[index]) {
            isImage = false;
        }
    }
    if (!isImage && !isCompressed) {
        return result;
    }

    // section 1 holds the header object, section 2 the texture
    for (int sectionsLeft = 2; sectionsLeft != 0; sectionsLeft--) {
        int compression = stream->ReadUInt8();
        if (sectionsLeft == 2 && compression != 0) {
            continue;
        }
        stream->Skip(4);
        unsigned int uncompressedLength = stream->ReadUInt32();
        if (uncompressedLength != 0) {
            int objectType = stream->ReadUInt8();
            int objectLength = stream->ReadUInt32();
            if (objectType == 0 && sectionsLeft == 2 && objectLength != 0) {
                stream->Skip(11);
                stream->Skip(objectLength - 11);
            } else if (objectType == 0x19 && sectionsLeft == 1 && objectLength != 0) {
                stream->Skip(12);
                int glFormat = stream->ReadUInt32();
                RSBFormat format;
                switch (glFormat) {
                case 0x8c00:
                    format = RSBFormat_PVRTC_RGB4;
                    break;
                case 0x8c01:
                    format = RSBFormat_PVRTC_RGB2;
                    break;
                case 0x8c02:
                    format = RSBFormat_PVRTC_RGBA4;
                    break;
                case 0x8c03:
                    format = RSBFormat_PVRTC_RGBA2;
                    break;
                case 0x8c92:
                    format = (RSBFormat)0x803c1300;     // ATC RGB
                    break;
                case 0x8c93:
                    format = (RSBFormat)0x803d5400;     // ATC explicit alpha
                    break;
                default:
                    format = RSBFormat_None;
                    break;
                }
                m_format = format;
                if (format != RSBFormat_None) {
                    stream->Skip(1);
                    m_width = stream->ReadUInt32();
                    unsigned short height = stream->ReadUInt32();
                    m_height = height;
                    unsigned short width = m_width;
                    unsigned char sizeOk = result;
                    if (width <= 0x400 && height <= 0x400) {
                        sizeOk = 1;
                    }
                    result = sizeOk;
                    if (sizeOk && width == CRenderSurfaceBuffer_AdjustToPowOfTwoDimension(width) &&
                        height == CRenderSurfaceBuffer_AdjustToPowOfTwoDimension(height)) {
                        m_imageWidth = width;
                        m_imageHeight = height;
                        unsigned int size = stream->ReadUInt32();
                        m_pitch = size / m_height;
                        if (pixels == NULL) {
                            m_data = malloc(m_height * m_pitch);
                            stream->Read((unsigned char*)m_data, size);
                            m_flags = (m_flags & ~(Flag_OwnsData | Flag_ExternalData)) | Flag_OwnsData;
                        } else {
                            m_data = pixels;
                            m_flags = (m_flags & ~(Flag_OwnsData | Flag_ExternalData)) | Flag_ExternalData;
                        }
                        result = 1;
                    }
                }
            }
        }
        stream->Skip(4);
    }
    return result;
}

unsigned char CRenderSurfaceBuffer::InitializeFromPNG(unsigned char* buffer, void** palette, const SourceStreamDesc* desc,
                                                      PitchCondition pitchCondition, short pitch, void* pixels)
{
    if (desc == NULL) {
        return 0;
    }
    CRenderSurfaceBuffer_PNGStream pngStream;
    pngStream.m_stream = desc->m_stream;
    pngStream.m_stream->m_isBigEndian = 1;
    if (pngStream.m_stream == NULL) {
        return 0;
    }
    CRenderSurfaceBuffer_PNGStream* pngStreamPointer = &pngStream;
    png_structp png = NULL;
    png_infop info = NULL;
    png_uint_32 width = 0;
    png_uint_32 height = 0;
    int bitDepth = 0;
    int colorType = 0;
    int interlace = 0;
    int paletteCount = 0;
    pngStream.m_buffered = 0;
    unsigned int streamSize = pngStream.m_stream->Available();
    png = png_create_read_struct_2(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL, NULL,
                                   (png_malloc_ptr)CRenderSurfaceBuffer_InitializeFromPNG_Malloc,
                                   CRenderSurfaceBuffer_InitializeFromPNG_Free);
    if (png == NULL) {
        return 0;
    }
    unsigned char result = 0;
    info = png_create_info_struct(png);
    if (info != NULL) {
        png_set_read_fn(png, &pngStreamPointer, (png_rw_ptr)CRenderSurfaceBuffer_InitializeFromPNG_InputStream);
        unsigned char signature[8];
        CRenderSurfaceBuffer_InitializeFromPNG_InputStream(png, signature, 8);
        if (png_check_sig(signature, 8)) {
            png_set_sig_bytes(png, 8);
            png_read_info(png, info);
            png_get_IHDR(png, info, &width, &height, &bitDepth, &colorType, &interlace, NULL, NULL);
            int passes = 1;
            if (interlace == PNG_INTERLACE_ADAM7) {
                passes = png_set_interlace_handling(png);
            }
            unsigned char ok = 1;
            png_read_update_info(png, info);
            if (bitDepth < 8) {
                ok = 0;
                if (8 % bitDepth == 0) {
                    ok = 1;
                }
            }
            if (width <= MaxImageDimension && height <= MaxImageDimension && ok) {
                unsigned int colorKey = desc->m_colorKey;
                unsigned char colorKeyEnabled = desc->m_colorKeyEnabled;
                unsigned char transGreen = 0;
                png_bytep trans = NULL;
                int transCount = 0;
                png_color_16p transColor = NULL;
                png_get_tRNS(png, info, &trans, &transCount, &transColor);
                unsigned char transRed = 0;
                unsigned char transBlue = 0;
                int colorKind;
                if (transCount != 0 && colorType == PNG_COLOR_TYPE_PALETTE) {
                    transRed = 0;
                    transGreen = 0;
                    transBlue = 0;
                    colorKind = 0;
                } else {
                    if (transCount != 0) {
                        transBlue = (unsigned char)transColor->blue;
                        transGreen = (unsigned char)transColor->green;
                        transRed = (unsigned char)transColor->red;
                    }
                    // 0 palette, 1 rgb, 2 rgb with alpha
                    colorKind = 0;
                    if (colorType != PNG_COLOR_TYPE_PALETTE) {
                        colorKind = colorType != PNG_COLOR_TYPE_RGB_ALPHA;
                        if (colorType == PNG_COLOR_TYPE_RGB_ALPHA) {
                            colorKind = 2;
                        }
                    }
                }
                int alphaKind = 0;
                RSBFormat sourceFormat;
                CRenderSurfaceBuffer_InitializeFromPNG_Config(m_format, desc, colorKind, bitDepth, colorKeyEnabled,
                                                              colorKey, trans, transRed, transGreen, transBlue,
                                                              transCount, sourceFormat, alphaKind);
                if (ok) {
                    void* paletteData = NULL;
                    if (colorType == PNG_COLOR_TYPE_PALETTE) {
                        bool fillPalette;
                        if (palette == NULL) {
                            fillPalette = true;
                            paletteData = buffer;
                        } else {
                            paletteData = *palette;
                            if (paletteData == NULL) {
                                if ((m_format & 0x8000) == 0) {
                                    fillPalette = true;
                                    paletteData = buffer;
                                } else {
                                    paletteData = malloc(PaletteBufferSize);
                                    *palette = paletteData;
                                    if (paletteData == NULL) {
                                        ok = 0;
                                        paletteData = buffer;
                                    }
                                    fillPalette = true;
                                }
                            } else {
                                // a palette of the caller: only mark the transparent entries
                                fillPalette = false;
                                for (unsigned short entry = 0; entry < info->num_trans; entry++) {
                                    if (trans[entry] == 0 && ((unsigned int*)paletteData)[entry] != ColorKeyMagenta) {
                                        ((unsigned int*)paletteData)[entry] = ColorKeyMagenta;
                                    }
                                }
                            }
                        }
                        if (ok) {
                            png_colorp plte = NULL;
                            png_get_PLTE(png, info, &plte, &paletteCount);
                            if (fillPalette) {
                                for (int entry = 0; entry < paletteCount; entry++) {
                                    unsigned int red = plte[entry].red;
                                    unsigned int green = plte[entry].green;
                                    unsigned int blue = plte[entry].blue;
                                    unsigned char alpha;
                                    bool transparent;
                                    if (entry < transCount) {
                                        alpha = trans[entry];
                                        transparent = alpha == 0;
                                    } else {
                                        transparent = false;
                                        alpha = 0xff;
                                    }
                                    if ((colorKeyEnabled && (green << 8 | red << 16 | blue) == (colorKey & 0xffffff)) ||
                                        transparent) {
                                        green = 0;
                                        blue = 0xff;
                                        red = 0xff;
                                        alpha = 0;
                                    }
                                    ((unsigned int*)paletteData)[entry] = blue | green << 8 | red << 16 | alpha << 24;
                                }
                            }
                            if (colorKeyEnabled) {
                                colorKey = ColorKeyMagenta;
                            }
                        }
                    }
                    if (ok) {
                        if (m_format & 0x8000) {
                            // palettized buffers keep the palette indices: no color key
                            colorKeyEnabled = 0;
                        }
                        components::CArrayInputStream rowStreamUnused;
                        unsigned int rowBytes = png_get_rowbytes(png, info);
                        unsigned int rowCount = height;
                        if (passes < 2) {
                            rowCount = 1;
                        }
                        png_bytep* rows = new png_bytep[rowCount];
                        for (unsigned int row = 0; row < rowCount; row++) {
                            rows[row] = (png_bytep)malloc(rowBytes);
                        }

                        unsigned int imageWidth = width;
                        unsigned int imageHeight = height;
                        unsigned int allocatedWidth;
                        unsigned int allocatedHeight;
                        if (m_flags & Flag_PowerOfTwo) {
                            allocatedWidth = CRenderSurfaceBuffer_AdjustToPowOfTwoDimension(width);
                            allocatedHeight = CRenderSurfaceBuffer_AdjustToPowOfTwoDimension(height);
                        } else {
                            allocatedWidth = width;
                            allocatedHeight = height;
                        }
                        unsigned short previousWidth = m_width;
                        unsigned short previousHeight = m_height;
                        unsigned int rowWidth;
                        if (!desc->m_transpose) {
                            m_width = allocatedWidth;
                            m_height = allocatedHeight;
                            m_imageWidth = imageWidth;
                            m_imageHeight = imageHeight;
                            if (previousWidth != 0 && previousWidth != allocatedWidth) {
                                ok = 0;
                            }
                            if (previousHeight != allocatedHeight && previousHeight != 0) {
                                ok = 0;
                            }
                            rowWidth = allocatedWidth;
                        } else {
                            m_width = allocatedHeight;
                            m_height = allocatedWidth;
                            m_imageWidth = imageHeight;
                            m_imageHeight = imageWidth;
                            if (previousWidth != 0 && previousWidth != allocatedHeight) {
                                ok = 0;
                            }
                            if (previousHeight != allocatedWidth && previousHeight != 0) {
                                ok = 0;
                            }
                            rowWidth = allocatedHeight;
                        }
                        if (ok) {
                            int rowPitch;
                            if ((m_format & RSBFormat_BytesPerPixelMask) == 0) {
                                int bitsPerPixel = 0;
                                if (m_format == RSBFormat_P16X8R8G8B8 || m_format == RSBFormat_P16A8R8G8B8) {
                                    bitsPerPixel = 4;
                                }
                                ok = 1;
                                unsigned int bits = bitsPerPixel * (unsigned short)rowWidth;
                                rowPitch = bits / 8 + ((bits & 7) != 0);
                            } else {
                                rowPitch = (m_format & RSBFormat_BytesPerPixelMask) * (unsigned short)rowWidth;
                            }
                            bool pitchOk = true;
                            if (m_pitch != 0) {
                                int existingPitch = m_pitch;
                                if (existingPitch < 0) {
                                    existingPitch = -existingPitch;
                                }
                                if (existingPitch != rowPitch) {
                                    pitchOk = false;
                                }
                            }
                            if (pitchOk) {
                                m_pitch = (short)rowPitch;
                                if (ok) {
                                    bool allocated = true;
                                    if (pixels == NULL) {
                                        unsigned int absolutePitch = m_pitch;
                                        if (m_pitch < 0) {
                                            absolutePitch = -m_pitch;
                                        }
                                        if (absolutePitch < (m_format & RSBFormat_BytesPerPixelMask) * m_width) {
                                            allocated = false;
                                        } else {
                                            m_data = malloc(m_height * absolutePitch);
                                            if (pitchCondition == PitchCondition_Negative || m_pitch < 0) {
                                                MakePitchNegative();
                                            }
                                            m_flags = (m_flags & ~(Flag_OwnsData | Flag_ExternalData)) | Flag_OwnsData;
                                        }
                                    } else {
                                        m_data = pixels;
                                        if (pitchCondition == PitchCondition_Negative) {
                                            m_pitch = -m_pitch;
                                        }
                                        m_flags = (m_flags & ~(Flag_OwnsData | Flag_ExternalData)) | Flag_ExternalData;
                                    }
                                    if (allocated) {
                                        // transparent background for the pixels the decoder skips
                                        unsigned int background = ColorKeyMagenta;
                                        Fill(RSBFormat_A8R8G8B8, &background);
                                        CBlitUtil::BlitOpDesc blitOp;
                                        blitOp.m_source = sourceFormat;
                                        blitOp.m_destination = m_format;
                                        blitOp.m_colorKeyTest = (CBlitUtil::ColorKeyTest)(colorKeyEnabled != 0);
                                        blitOp.m_alphaTest = CBlitUtil::AlphaTest_None;
                                        blitOp.m_blend = CBlitUtil::Blend_None;
                                        blitOp.m_opId = CBlitUtil::BlitOpId_Stream;
                                        CBlitUtil::GetBlitOp(blitOp);
                                        components::CArrayInputStream rowStream;
                                        CBlit::StreamOpDesc decode;
                                        decode.m_rowSkip = 0;
                                        decode.m_width = width;
                                        decode.m_height = 1;
                                        *(unsigned int*)decode.m_colorKey = colorKey;
                                        decode.m_colorKeyEnabled = colorKeyEnabled;
                                        decode.m_transpose = desc->m_transpose;
                                        if (!decode.m_transpose) {
                                            decode.m_flipX = desc->m_flipX;
                                            decode.m_flipY = 0;
                                        } else {
                                            decode.m_flipX = 0;
                                            decode.m_flipY = desc->m_flipY;
                                        }
                                        decode.m_destinationPitch = m_pitch;
                                        decode.m_stream = &rowStream;
                                        decode.m_palette = (const unsigned int*)paletteData;
                                        if (blitOp.m_function != NULL) {
                                            unsigned int rowFromBottom = m_imageHeight;
                                            unsigned int columnFromRight = m_imageWidth;
                                            if (passes > 1) {
                                                for (int pass = passes; pass != 0; pass--) {
                                                    png_read_rows(png, rows, NULL, height);
                                                }
                                            }
                                            for (unsigned int row = 0; (int)row < (int)height; row++) {
                                                rowFromBottom--;
                                                columnFromRight--;
                                                rowStream.Open(rows[passes - 1], rowBytes);
                                                png_bytep* current;
                                                if (passes < 2) {
                                                    rowStream.Open(rows[passes - 1], rowBytes);
                                                    png_read_row(png, rows[0], NULL);
                                                    current = rows;
                                                } else {
                                                    rowStream.Open(rows[row], rowBytes);
                                                    current = &rows[row];
                                                }
                                                if (colorType != PNG_COLOR_TYPE_PALETTE) {
                                                    // libpng delivers R, G, B: swap to B, G, R
                                                    int step = 3;
                                                    if (colorType == PNG_COLOR_TYPE_RGB_ALPHA) {
                                                        step = 4;
                                                    }
                                                    unsigned char* pixel = *current + 2;
                                                    for (int column = 0; column < (int)width; column++) {
                                                        unsigned char blue = *pixel;
                                                        *pixel = pixel[-2];
                                                        pixel[-2] = blue;
                                                        pixel += step;
                                                    }
                                                }
                                                unsigned int destinationIndex;
                                                if (!desc->m_transpose) {
                                                    destinationIndex = rowFromBottom;
                                                    if (!desc->m_flipY) {
                                                        destinationIndex = row;
                                                    }
                                                    decode.m_destination = (unsigned char*)m_data + destinationIndex * m_pitch;
                                                } else {
                                                    destinationIndex = columnFromRight;
                                                    if (!desc->m_flipX) {
                                                        destinationIndex = row;
                                                    }
                                                    decode.m_destination = (unsigned char*)m_data +
                                                        destinationIndex * (m_format & RSBFormat_BytesPerPixelMask);
                                                }
                                                ((void (*)(const CBlit::StreamOpDesc&))blitOp.m_function)(decode);
                                                rowStream.Close();
                                            }
                                            result = ok;
                                        }
                                    }
                                }
                            }
                        }
                        for (unsigned int row = 0; row < rowCount; row++) {
                            free(rows[row]);
                        }
                        delete[] rows;
                    }
                }
            }
        }
        png_destroy_read_struct(&png, &info, NULL);
        return result;
    }
    png_destroy_read_struct(&png, NULL, NULL);
    return result;
}

unsigned char CRenderSurfaceBuffer::InitializeFromSBI(unsigned char* buffer, void** palette, const SourceStreamDesc* desc,
                                                      PitchCondition pitchCondition, short pitch, void* pixels)
{
    if (desc == NULL) {
        return 0;
    }
    components::CInputStream* stream = desc->m_stream;
    if (stream == NULL) {
        return 0;
    }
    short version = stream->ReadInt16();
    stream->ReadInt16();
    RSBFormat sourceFormat = RSBFormat_None;
    unsigned int imageWidth = 0;
    unsigned int imageHeight = 0;
    int rowBytes = 0;
    unsigned char ok = 0;
    if (version == 1) {
        imageWidth = stream->ReadUInt16();
        imageHeight = stream->ReadUInt16();
        stream->ReadInt16();
        stream->ReadUInt16();
        sourceFormat = (RSBFormat)stream->ReadUInt32();
    }
    // bytes of one source row
    unsigned int bytesPerPixel = sourceFormat & RSBFormat_BytesPerPixelMask;
    unsigned int bitsPerPixel;
    if (bytesPerPixel != 0) {
        bitsPerPixel = bytesPerPixel * 8;
    } else if (sourceFormat == RSBFormat_P16X8R8G8B8 || sourceFormat == RSBFormat_P16A8R8G8B8) {
        bitsPerPixel = 4;
    } else {
        bitsPerPixel = 0;
    }
    if (bitsPerPixel >= 8) {
        rowBytes = bytesPerPixel * imageWidth;
        ok = 1;
    } else if (8 % bitsPerPixel == 0) {
        int bits = imageWidth * bitsPerPixel;
        rowBytes = bits / 8 + ((bits & 7) != 0);
        ok = 1;
    }
    if (imageHeight > MaxImageDimension || imageWidth > MaxImageDimension) {
        ok = 0;
    }
    if (!ok) {
        return 0;
    }

    RSBFormat format = m_format;
    unsigned char colorKeyEnabled = desc->m_colorKeyEnabled;
    unsigned int colorKey = desc->m_colorKey;
    if (!colorKeyEnabled && ((sourceFormat >> 14) & 1)) {
        // sources with alpha: magenta is transparent
        colorKeyEnabled = 1;
        colorKey = ColorKeyMagenta;
    }
    if (format == RSBFormat_None) {
        ok = 0;
        for (unsigned int index = 0; index < 10; index++) {
            if (sourceFormat == desc->m_formatMap[colorKeyEnabled != 0][index].m_from) {
                format = desc->m_formatMap[colorKeyEnabled != 0][index].m_to;
                if (format == RSBFormat_P16X8R8G8B8) {
                    format = RSBFormat_P256X8R8G8B8;
                }
                if (format == RSBFormat_P16A8R8G8B8) {
                    format = RSBFormat_P256A8R8G8B8;
                }
                ok = 1;
                m_format = format;
                break;
            }
        }
    }
    if (format & 0x8000) {
        // palettized destinations are not supported
        ok = 0;
    }
    if (!ok) {
        return 0;
    }
    if (format & 0x8000) {
        colorKeyEnabled = 0;
    }

    unsigned int allocatedWidth = imageWidth;
    unsigned int allocatedHeight = imageHeight;
    if (m_flags & Flag_PowerOfTwo) {
        allocatedWidth = CRenderSurfaceBuffer_AdjustToPowOfTwoDimension(imageWidth);
        allocatedHeight = CRenderSurfaceBuffer_AdjustToPowOfTwoDimension(imageHeight);
    }
    unsigned short previousWidth = m_width;
    unsigned short previousHeight = m_height;
    m_width = allocatedHeight;
    m_height = allocatedWidth;
    m_imageWidth = imageWidth;
    m_imageHeight = imageHeight;
    if (previousWidth != 0 && previousWidth != allocatedHeight) {
        ok = 0;
    }
    if (previousHeight != 0 && previousHeight != allocatedWidth) {
        ok = 0;
    }
    if (!ok) {
        return 0;
    }

    int rowPitch;
    unsigned int destinationBytesPerPixel = format & RSBFormat_BytesPerPixelMask;
    if (destinationBytesPerPixel == 0) {
        int destinationBitsPerPixel = 0;
        if (format == RSBFormat_P16X8R8G8B8 || format == RSBFormat_P16A8R8G8B8) {
            destinationBitsPerPixel = 4;
        }
        ok = 1;
        unsigned int bits = (unsigned short)allocatedHeight * destinationBitsPerPixel;
        rowPitch = bits / 8 + ((bits & 7) != 0);
    } else {
        rowPitch = destinationBytesPerPixel * (unsigned short)allocatedHeight;
    }
    if (m_pitch != 0) {
        int existingPitch = m_pitch;
        if (existingPitch < 0) {
            existingPitch = -existingPitch;
        }
        if (existingPitch != rowPitch) {
            return 0;
        }
    }
    m_pitch = (short)rowPitch;
    if (!ok) {
        return 0;
    }
    if (pixels == NULL) {
        unsigned int absolutePitch = (short)rowPitch;
        if ((short)rowPitch < 0) {
            absolutePitch = -(short)rowPitch;
        }
        if (absolutePitch < (unsigned short)allocatedHeight * destinationBytesPerPixel) {
            return 0;
        }
        m_data = malloc((unsigned short)allocatedWidth * absolutePitch);
        if (pitchCondition == PitchCondition_Negative || m_pitch < 0) {
            MakePitchNegative();
        }
        m_flags = (m_flags & ~(Flag_OwnsData | Flag_ExternalData)) | Flag_OwnsData;
    } else {
        m_data = pixels;
        if (pitchCondition == PitchCondition_Negative) {
            m_pitch = -(short)rowPitch;
        }
        m_flags = (m_flags & ~(Flag_OwnsData | Flag_ExternalData)) | Flag_ExternalData;
    }
    unsigned int background = ColorKeyMagenta;
    Fill(RSBFormat_A8R8G8B8, &background);

    CBlitUtil::BlitOpDesc blitOp;
    blitOp.m_source = sourceFormat;
    blitOp.m_destination = m_format;
    blitOp.m_colorKeyTest = (CBlitUtil::ColorKeyTest)(colorKeyEnabled != 0);
    blitOp.m_alphaTest = CBlitUtil::AlphaTest_None;
    blitOp.m_blend = CBlitUtil::Blend_None;
    blitOp.m_opId = CBlitUtil::BlitOpId_Stream;
    CBlitUtil::GetBlitOp(blitOp);
    CBlit::StreamOpDesc decode;
    decode.m_palette = NULL;
    decode.m_width = m_width;
    decode.m_height = m_height;
    decode.m_colorKeyEnabled = colorKeyEnabled;
    decode.m_transpose = desc->m_transpose;
    decode.m_flipX = desc->m_flipX;
    decode.m_flipY = desc->m_flipY;
    decode.m_destinationPitch = m_pitch;
    decode.m_destination = m_data;
    unsigned char result = 0;
    if (blitOp.m_function != NULL) {
        decode.m_stream = stream;
        int absoluteRowBytes = rowBytes;
        if (absoluteRowBytes < 0) {
            absoluteRowBytes = -absoluteRowBytes;
        }
        decode.m_rowSkip = absoluteRowBytes - rowBytes;
        *(unsigned int*)decode.m_colorKey = colorKey;
        ((void (*)(const CBlit::StreamOpDesc&))blitOp.m_function)(decode);
        result = ok;
    }
    return result;
}

unsigned char CRenderSurfaceBuffer::DestroyBufferData()
{
    unsigned char result = 0;
    if (m_flags & Flag_OwnsData) {
        result = 1;
        if (m_data != NULL) {
            void* data = m_data;
            if (m_pitch < 0) {
                // m_data is the last row in memory
                data = (unsigned char*)data + (m_height - 1) * m_pitch;
            }
            free(data);
            result = 1;
        }
    }
    m_data = NULL;
    return result;
}

void CRenderSurfaceBuffer::MakePitchPositive()
{
    if (m_data != NULL) {
        if (m_pitch < 0) {
            m_data = (unsigned char*)m_data + (m_height - 1) * m_pitch;
            m_pitch = -m_pitch;
        }
    }
}

void CRenderSurfaceBuffer::MakePitchNegative()
{
    if (m_data != NULL) {
        if (m_pitch > 0) {
            m_data = (unsigned char*)m_data + (m_height - 1) * m_pitch;
            m_pitch = -m_pitch;
        }
    }
}

unsigned char CRenderSurfaceBuffer::Fill(RSBFormat format, void* color)
{
    unsigned char result = 0;
    if (m_data != NULL) {
        RSBFormat destinationFormat = m_format;
        if (destinationFormat != RSBFormat_None && (destinationFormat & 0x8000) == 0) {
            unsigned short height = m_imageHeight;
            unsigned short width = m_imageWidth;
            CBlitUtil::BlitOpDesc blitOp;
            blitOp.m_destination = destinationFormat;
            blitOp.m_source = format;
            blitOp.m_colorKeyTest = CBlitUtil::ColorKeyTest_None;
            blitOp.m_alphaTest = CBlitUtil::AlphaTest_None;
            blitOp.m_blend = CBlitUtil::Blend_None;
            blitOp.m_opId = CBlitUtil::BlitOpId_Rectangle;
            CBlitUtil::GetBlitOp(blitOp);
            CBlit::RectangleOpDesc fill;
            fill.m_destination = m_data;
            fill.m_destinationPitch = m_pitch;
            fill.m_width = width;
            fill.m_height = height;
            fill.m_colorFormat = format;
            fill.m_color = color;
            if (blitOp.m_function != NULL) {
                result = 1;
                ((void (*)(const CBlit::RectangleOpDesc&))blitOp.m_function)(fill);
            }
        }
    }
    return result;
}

void* CRenderSurfaceBuffer::GetLevel(unsigned int level) const
{
    void* data = NULL;
    if (level == 0) {
        data = m_data;
    }
    return data;
}

void jpeg_stream_src(jpeg_decompress_struct* cinfo, components::CInputStream* stream)
{
    if (cinfo->src == NULL) {
        cinfo->src = (jpeg_source_mgr*)cinfo->mem->alloc_small((j_common_ptr)cinfo, JPOOL_PERMANENT,
                                                               sizeof(CRenderSurfaceBuffer_JPEGSource));
    }
    CRenderSurfaceBuffer_JPEGSource* source = (CRenderSurfaceBuffer_JPEGSource*)cinfo->src;
    source->m_manager.init_source = init_source;
    source->m_manager.fill_input_buffer = fill_input_buffer;
    source->m_manager.skip_input_data = skip_input_data;
    source->m_manager.resync_to_restart = jpeg_resync_to_restart;
    source->m_manager.term_source = term_source;
    source->m_stream = stream;
}

static void init_source(jpeg_decompress_struct* cinfo)
{
    cinfo->src->bytes_in_buffer = 0;
}

static boolean fill_input_buffer(jpeg_decompress_struct* cinfo)
{
    CRenderSurfaceBuffer_JPEGSource* source = (CRenderSurfaceBuffer_JPEGSource*)cinfo->src;
    unsigned int count;
    if (source->m_stream->Available() == 0) {
        // end of the data: insert an EOI marker
        count = 2;
        source->m_buffer[0] = 0xff;
        source->m_buffer[1] = 0xd9;
    } else {
        count = source->m_stream->Available();
        if (count > sizeof(source->m_buffer)) {
            count = sizeof(source->m_buffer);
        }
        source->m_stream->Read(source->m_buffer, count);
    }
    source->m_manager.next_input_byte = source->m_buffer;
    source->m_manager.bytes_in_buffer = count;
    return 1;
}

static void skip_input_data(jpeg_decompress_struct* cinfo, long count)
{
    if (count > 0) {
        jpeg_source_mgr* source = cinfo->src;
        while (count > (long)source->bytes_in_buffer) {
            count -= source->bytes_in_buffer;
            fill_input_buffer(cinfo);
        }
        source->next_input_byte += count;
        source->bytes_in_buffer -= count;
    }
}

static void term_source(jpeg_decompress_struct* cinfo)
{
}

static void error_exit(jpeg_common_struct* cinfo)
{
    // no longjmp: the decoder checks the flag after each step
    ((CRenderSurfaceBuffer_JPEGError*)cinfo->err)->m_valid = 0;
    (*cinfo->err->output_message)(cinfo);
}

static void output_message(jpeg_common_struct* cinfo)
{
    char message[JMSG_LENGTH_MAX];
    (*cinfo->err->format_message)(cinfo, message);
}

static void format_message(jpeg_common_struct* cinfo, char* buffer)
{
    jpeg_error_mgr* error = cinfo->err;
    int code = error->msg_code;
    const char* format = NULL;
    if (code > 0 && code <= error->last_jpeg_message) {
        format = error->jpeg_message_table[code];
    } else if (error->addon_message_table != NULL && code >= error->first_addon_message &&
               code <= error->last_addon_message) {
        format = error->addon_message_table[code - error->first_addon_message];
    }
    if (format == NULL) {
        error->msg_parm.i[0] = code;
        format = error->jpeg_message_table[0];
    }
    // a %s parameter?
    bool isString = false;
    const char* character = format;
    char value;
    while ((value = *character++) != 0) {
        if (value == '%') {
            if (*character == 's') {
                isString = true;
            }
            break;
        }
    }
    if (isString) {
        core::ICStdUtil::SPrintF(buffer, format, error->msg_parm.s);
    } else {
        core::ICStdUtil::SPrintF(buffer, format, error->msg_parm.i[0], error->msg_parm.i[1], error->msg_parm.i[2],
                                 error->msg_parm.i[3], error->msg_parm.i[4], error->msg_parm.i[5],
                                 error->msg_parm.i[6], error->msg_parm.i[7]);
    }
}

void CRenderSurfaceBuffer_InitializeFromPNG_Config(RSBFormat& format, const CRenderSurfaceBuffer::SourceStreamDesc* desc,
                                                   int colorKind, int bitDepth, unsigned char& colorKeyEnabled,
                                                   unsigned int& colorKey, unsigned char* trans, unsigned char transRed,
                                                   unsigned char transGreen, unsigned char transBlue, int transCount,
                                                   RSBFormat& sourceFormat, int& alphaKind)
{
    alphaKind = 0;
    if (colorKind == 0) {
        // palette
        if (transCount != 0) {
            alphaKind = 1;
            for (int entry = 0; entry < transCount; entry++) {
                // anything but fully transparent / opaque needs real alpha
                if ((unsigned char)(trans[entry] + 1) > 1) {
                    alphaKind = 2;
                }
            }
            if (!colorKeyEnabled) {
                colorKey = ColorKeyMagenta;
                colorKeyEnabled = 1;
            }
            if (alphaKind == 2) {
                sourceFormat = RSBFormat_P256A8R8G8B8;
                if (bitDepth < 8) {
                    sourceFormat = RSBFormat_P16A8R8G8B8;
                }
            } else {
                sourceFormat = RSBFormat_P256X8R8G8B8;
                if (bitDepth < 8) {
                    sourceFormat = RSBFormat_P16X8R8G8B8;
                }
            }
        } else {
            sourceFormat = RSBFormat_P256X8R8G8B8;
            if (bitDepth < 8) {
                sourceFormat = RSBFormat_P16X8R8G8B8;
            }
        }
    } else {
        if (transCount != 0) {
            alphaKind = 1;
            if (!colorKeyEnabled) {
                colorKey = transBlue | transRed << 16 | transGreen << 8;
                colorKeyEnabled = 1;
            }
        }
        if (colorKind == 2) {
            alphaKind = 2;
            sourceFormat = RSBFormat_A8R8G8B8;
        } else {
            sourceFormat = RSBFormat_R8G8B8;
        }
    }
    if (format == RSBFormat_None) {
        for (unsigned int index = 0; index < 10; index++) {
            if (sourceFormat == desc->m_formatMap[colorKeyEnabled != 0][index].m_from) {
                RSBFormat mapped = desc->m_formatMap[colorKeyEnabled != 0][index].m_to;
                if (mapped == RSBFormat_P16X8R8G8B8) {
                    mapped = RSBFormat_P256X8R8G8B8;
                }
                format = mapped;
                if (mapped == RSBFormat_P16A8R8G8B8) {
                    format = RSBFormat_P256A8R8G8B8;
                }
                return;
            }
        }
    }
}

void* CRenderSurfaceBuffer_InitializeFromPNG_Malloc(png_struct_def* png, unsigned long size)
{
    return malloc(size);
}

void CRenderSurfaceBuffer_InitializeFromPNG_Free(png_struct_def* png, void* pointer)
{
    free(pointer);
}

void CRenderSurfaceBuffer_InitializeFromPNG_InputStream(png_struct_def* png, unsigned char* data, unsigned long length)
{
    CRenderSurfaceBuffer_PNGStream* stream = *(CRenderSurfaceBuffer_PNGStream**)png->io_ptr;
    int buffered = stream->m_buffered;
    if (buffered != 0) {
        // the rest of the buffer first
        const unsigned char* source = stream->m_buffer + (PNGStreamBufferSize - buffered);
        int count = buffered;
        if ((int)length < buffered) {
            count = length;
        }
        switch (count) {
        case 8:
            *data++ = *source++;
        case 7:
            *data++ = *source++;
        case 6:
            *data++ = *source++;
        case 5:
            *data++ = *source++;
        case 4:
            *data++ = *source++;
        case 3:
            *data++ = *source++;
        case 2:
            *data++ = *source++;
        case 1:
            *data++ = *source++;
            break;
        default:
            memcpy(data, source, count);
            data += count;
            break;
        }
        length -= count;
        stream->m_buffered -= count;
    }
    if (length != 0) {
        if (length < (unsigned long)PNGStreamBufferSize) {
            int available = stream->m_stream->Available();
            if (available > PNGStreamBufferSize) {
                available = PNGStreamBufferSize;
            }
            stream->m_buffered = available;
            stream->m_stream->Read(stream->m_buffer + (PNGStreamBufferSize - available), available);
            int count = stream->m_buffered;
            if ((int)length < count) {
                count = length;
            }
            memcpy(data, stream->m_buffer + (PNGStreamBufferSize - stream->m_buffered), count);
            stream->m_buffered -= count;
        } else {
            stream->m_stream->Read(data, length);
        }
    }
}

unsigned char CRenderSurfaceBufferMipmap::Initialize(RSBFormat format, unsigned short width, unsigned short height,
                                                     unsigned char powerOfTwo, void** palette,
                                                     const SourceStreamDesc* desc, PitchCondition pitchCondition,
                                                     short pitch, void* pixels)
{
    unsigned char result = CRenderSurfaceBuffer::Initialize(format, width, height, powerOfTwo, palette, desc,
                                                            pitchCondition, pitch, pixels);
    memset(m_levels, 0, sizeof(m_levels));
    m_levels[0] = m_data;
    m_levelCount = 1;
    m_flags |= Flag_Mipmap;
    return result;
}

void CRenderSurfaceBufferMipmap::MakePitchPositive()
{
    if (m_data == NULL) {
        return;
    }
    if (m_pitch < 0) {
        CRenderSurfaceBuffer::MakePitchPositive();
    }
}

void CRenderSurfaceBufferMipmap::MakePitchNegative()
{
    if (m_data == NULL) {
        return;
    }
    if (m_pitch > 0) {
        CRenderSurfaceBuffer::MakePitchNegative();
    }
}

void* CRenderSurfaceBufferMipmap::GetLevel(unsigned int level) const
{
    void* data = NULL;
    if (level < MaxLevels) {
        data = m_levels[level];
    }
    return data;
}

} } } }
