// malij297_ktxload.cpp - KTX loader of the Swerve iOS porting layer.
//
// Uploads a KTX 1.1 file held in memory into a Swerve Image2D. Supported are 2D textures
// with one face: 8-bit ALPHA / LUMINANCE / LUMINANCE_ALPHA / RGB / RGBA, the packed 16-bit
// 565 / 4444 / 5551 types, and compressed formats (passed through by GL internal format).
// The header checks and the mip level walk follow the Khronos libktx loader.
// The prebuilt engine core calls the entry points with C linkage.
#include "ICStdUtil.h"
#include "SwerveApi.h"

// ---- KTX constants ----
static const unsigned int KTX_ENDIAN_REF = 0x04030201;
static const unsigned int KTX_ENDIAN_REF_REV = 0x01020304;
static const unsigned int KTX_IDENTIFIER_SIZE = 12;

// ---- GL enums used by KTX headers ----
enum {
    GL_UNSIGNED_BYTE = 0x1401,
    GL_RED = 0x1903,
    GL_GREEN = 0x1904,
    GL_BLUE = 0x1905,
    GL_ALPHA = 0x1906,
    GL_RGB = 0x1907,
    GL_RGBA = 0x1908,
    GL_LUMINANCE = 0x1909,
    GL_LUMINANCE_ALPHA = 0x190a,
    GL_UNSIGNED_SHORT_4_4_4_4 = 0x8033,
    GL_UNSIGNED_SHORT_5_5_5_1 = 0x8034,
    GL_UNSIGNED_SHORT_5_6_5 = 0x8363,
    GL_ETC1_RGB8_OES = 0x8d64
};

// ---- Swerve image formats (values passed to the Image2D interface) ----
enum {
    IMAGE_FORMAT_ALPHA = 0x60,
    IMAGE_FORMAT_LUMINANCE = 0x61,
    IMAGE_FORMAT_LUMINANCE_ALPHA = 0x62,
    IMAGE_FORMAT_RGB = 0x63,
    IMAGE_FORMAT_RGBA = 0x64,
    IMAGE_FORMAT_RGB565 = 0x65,
    IMAGE_FORMAT_RGBA5551 = 0x66,
    IMAGE_FORMAT_RGBA4444 = 0x67,
    IMAGE_FORMAT_ETC1 = 0x68,
    IMAGE_FORMAT_COMPRESSED = 0x69,         // first compressed format; one-channel compressed data
    IMAGE_FORMAT_COMPRESSED_RGB = 0x6b,
    IMAGE_FORMAT_COMPRESSED_RGBA = 0x6c
};

// Format option bit: the image has a single mip level.
static const unsigned int IMAGE_FORMAT_NO_MIPMAPS = 0x8000;

// Object type passed to the runtime's CreateObject for an Image2D.
static const int OBJECT_TYPE_IMAGE2D = 10;

// ---- KTXLoadBuffer error codes (KTXLoadInfo::m_errorCode) ----
enum {
    KTX_LOAD_OK = 0,
    KTX_LOAD_INVALID_FILE = 1,
    KTX_LOAD_UNSUPPORTED_FORMAT = 2,
    KTX_LOAD_UNEXPECTED_END = 3,
    KTX_LOAD_OUT_OF_MEMORY = 4,
    KTX_LOAD_RUNTIME_ERROR = 5
};


/** KTX file header (64 bytes). */
struct KTXHeader {
    unsigned char m_identifier[KTX_IDENTIFIER_SIZE];
    unsigned int m_endianness;
    unsigned int m_glType;
    unsigned int m_glTypeSize;
    unsigned int m_glFormat;
    unsigned int m_glInternalFormat;
    unsigned int m_glBaseInternalFormat;
    unsigned int m_pixelWidth;
    unsigned int m_pixelHeight;
    unsigned int m_pixelDepth;
    unsigned int m_numberOfArrayElements;
    unsigned int m_numberOfFaces;
    unsigned int m_numberOfMipmapLevels;
    unsigned int m_bytesOfKeyValueData;
};

/** What KTXLoadBuffer learned from the header (cleared with MemSet first). */
struct KTXLoadInfo {
    unsigned int m_mipLevels;           // 0x00 mip levels to upload (at least 1)
    unsigned int m_glFormat;            // 0x04 GL format reported for compressed images
    unsigned int m_componentCount;      // 0x08 components per element
    unsigned int m_format;              // 0x0c Swerve image format (+ IMAGE_FORMAT_NO_MIPMAPS)
    int m_errorCode;                    // 0x10 KTX_LOAD_*
    int m_swapEndian;                   // 0x14 file was written with the other byte order
    int m_reserved18;                   // 0x18 never used
    int m_compressed;                   // 0x1c glType and glFormat are both 0
    int m_nonPowerOfTwo;                // 0x20 only the base level can be uploaded
    int m_compressedFormat;             // 0x24 m_format is one of the compressed formats
};


/** Byte-swap `count` 16-bit values in place (libktx _ktxSwapEndian16). */
static void SwapEndian16(unsigned short* values, unsigned int count)
{
    for (unsigned int i = 0; i < count; i++) {
        unsigned short value = *values;
        *values++ = (value << 8) | (value >> 8);
    }
}

/** Byte-swap `count` 32-bit values in place (libktx _ktxSwapEndian32). */
static void SwapEndian32(unsigned int* values, unsigned int count)
{
    for (unsigned int i = 0; i < count; i++) {
        unsigned int value = *values;
        *values++ = (value << 24) | ((value & 0xff00) << 8) | ((value & 0xff0000) >> 8) | (value >> 24);
    }
}

/** Return 1 when the buffer starts with the KTX file identifier. */
int malij297_HasKTXIdentifier(unsigned int size, const unsigned char* data)
{
    // 0xAB "KTX 11" 0xBB "\r\n" 0x1A "\n"
    static const unsigned char ktxIdentifier[] = {
        0xab, 'K', 'T', 'X', ' ', '1', '1', 0xbb, '\r', '\n', 0x1a, '\n'
    };

    if (size < sizeof(ktxIdentifier)) {
        return 0;
    }
    for (unsigned int i = 0; i < sizeof(ktxIdentifier); i++) {
        if (ktxIdentifier[i] != data[i]) {
            return 0;
        }
    }
    return 1;
}

/**
 * Upload a KTX file into an Image2D created in *image.
 * formatHint is accepted for symmetry with the other loaders and not used.
 * Returns 0 on success, 1 for an invalid or unsupported file, or the runtime's error code.
 */
int malij297_KTXLoadBuffer(ISwerve* runtime, unsigned int size, const unsigned char* data,
                           unsigned int formatHint, IImage2D** image)
{
    KTXHeader header;
    KTXLoadInfo info;
    int result = 1;
    unsigned char* buffer = NULL;
    unsigned int mipLevels;

    malij297_MemSet(&info, 0, sizeof(info));
    *image = NULL;
    if (size < sizeof(header)) {
        info.m_errorCode = KTX_LOAD_UNEXPECTED_END;
        goto cleanup;
    }
    malij297_MemCpy(&header, data, sizeof(header));

    // ---- header checks (libktx _ktxCheckHeader, restricted to 2D textures) ----
    if (header.m_endianness == KTX_ENDIAN_REF_REV) {
        info.m_swapEndian = 1;
        SwapEndian32(&header.m_glType, 12);
    } else if (header.m_endianness != KTX_ENDIAN_REF) {
        info.m_errorCode = KTX_LOAD_INVALID_FILE;
        goto cleanup;
    }
    if (header.m_glTypeSize != 1 && header.m_glTypeSize != 2 && header.m_glTypeSize != 4) {
        info.m_errorCode = KTX_LOAD_INVALID_FILE;
        goto cleanup;
    }
    // either both or none of glType and glFormat are 0
    if (header.m_glType != 0) {
        if (header.m_glFormat == 0) {
            info.m_errorCode = KTX_LOAD_INVALID_FILE;
            goto cleanup;
        }
    } else {
        if (header.m_glFormat != 0) {
            info.m_errorCode = KTX_LOAD_INVALID_FILE;
            goto cleanup;
        }
        info.m_compressed = 1;
    }
    if (header.m_pixelWidth == 0 || header.m_pixelHeight == 0) {
        info.m_errorCode = KTX_LOAD_INVALID_FILE;
        goto cleanup;
    }
    if (header.m_pixelDepth != 0 || header.m_numberOfArrayElements != 0) {
        info.m_errorCode = KTX_LOAD_INVALID_FILE;
        goto cleanup;
    }
    mipLevels = header.m_numberOfMipmapLevels;
    if (mipLevels == 1) {
        info.m_format = IMAGE_FORMAT_NO_MIPMAPS;
    }
    // 0 levels means "generate mipmaps"; only the base level is in the file (select, as in the original)
    mipLevels = (mipLevels == 0) ? 1 : mipLevels;
    info.m_mipLevels = mipLevels;
    if (mipLevels != 1) {
        // a mip chain has to go all the way down to 1x1
        unsigned int maxSize = header.m_pixelHeight;
        if (header.m_pixelWidth > maxSize) {
            maxSize = header.m_pixelWidth;
        }
        unsigned int fullLevels = 1;
        while ((maxSize >>= 1) != 0) {
            fullLevels++;
        }
        if (mipLevels != fullLevels) {
            info.m_errorCode = KTX_LOAD_INVALID_FILE;
            goto cleanup;
        }
    }
    if ((header.m_pixelWidth & (header.m_pixelWidth - 1)) != 0 ||
        (header.m_pixelHeight & (header.m_pixelHeight - 1)) != 0) {
        info.m_nonPowerOfTwo = 1;
    }

    // ---- pick the Swerve image format ----
    if (info.m_compressed) {
        switch (header.m_glBaseInternalFormat) {
        case GL_RGB: {
            unsigned int compressedFormat = IMAGE_FORMAT_COMPRESSED_RGB;
            if (header.m_glInternalFormat == GL_ETC1_RGB8_OES) {
                compressedFormat = IMAGE_FORMAT_ETC1;
            }
            info.m_format += compressedFormat;
            break;
        }
        case GL_RGBA:
            info.m_format += IMAGE_FORMAT_COMPRESSED_RGBA;
            break;
        case GL_RED:
        case GL_GREEN:
        case GL_BLUE:
        case GL_ALPHA:
        case GL_LUMINANCE:
            info.m_format += IMAGE_FORMAT_COMPRESSED;
            break;
        default:
            info.m_errorCode = KTX_LOAD_UNSUPPORTED_FORMAT;
            goto cleanup;
        }
    } else {
        info.m_componentCount = 1;
        if (header.m_glTypeSize == 1 && header.m_glType == GL_UNSIGNED_BYTE) {
            switch (header.m_glFormat) {
            case GL_ALPHA:
                info.m_format += IMAGE_FORMAT_ALPHA;
                break;
            case GL_LUMINANCE:
                info.m_format += IMAGE_FORMAT_LUMINANCE;
                break;
            case GL_LUMINANCE_ALPHA:
                info.m_format += IMAGE_FORMAT_LUMINANCE_ALPHA;
                info.m_componentCount = 2;
                break;
            case GL_RGB:
                info.m_format += IMAGE_FORMAT_RGB;
                info.m_componentCount = 3;
                break;
            case GL_RGBA:
                info.m_format += IMAGE_FORMAT_RGBA;
                info.m_componentCount = 4;
                break;
            }
        } else if (header.m_glTypeSize == 2) {
            if (header.m_glType == GL_UNSIGNED_SHORT_5_6_5 && header.m_glFormat == GL_RGB) {
                info.m_format += IMAGE_FORMAT_RGB565;
            } else if (header.m_glType == GL_UNSIGNED_SHORT_5_5_5_1 && header.m_glFormat == GL_RGBA) {
                info.m_format += IMAGE_FORMAT_RGBA5551;
            } else if (header.m_glType == GL_UNSIGNED_SHORT_4_4_4_4 && header.m_glFormat == GL_RGBA) {
                info.m_format += IMAGE_FORMAT_RGBA4444;
            } else {
                info.m_errorCode = KTX_LOAD_UNSUPPORTED_FORMAT;
                goto cleanup;
            }
        } else {
            info.m_errorCode = KTX_LOAD_UNSUPPORTED_FORMAT;
            goto cleanup;
        }
    }
    if ((int)(info.m_format & ~IMAGE_FORMAT_NO_MIPMAPS) >= IMAGE_FORMAT_COMPRESSED) {
        info.m_compressedFormat = 1;
        // select, as in the original
        info.m_glFormat = info.m_compressed ? header.m_glInternalFormat : header.m_glFormat;
    }
    if (header.m_numberOfFaces != 1) {
        info.m_errorCode = KTX_LOAD_INVALID_FILE;
        goto cleanup;
    }

    {
        // image data follows the key/value data
        unsigned int offset = sizeof(header) + header.m_bytesOfKeyValueData;
        if (offset > size) {
            info.m_errorCode = KTX_LOAD_UNEXPECTED_END;
            goto cleanup;
        }

        result = runtime->m_functions->CreateObject(OBJECT_TYPE_IMAGE2D, (void**)image);
        if (result == 0) {
            if (info.m_compressedFormat) {
                result = (*image)->m_functions->InitializeCompressed(*image, info.m_format, header.m_pixelWidth,
                                                                     header.m_pixelHeight, header.m_glType,
                                                                     info.m_glFormat);
            } else {
                result = (*image)->m_functions->Initialize(*image, info.m_format, header.m_pixelWidth,
                                                           header.m_pixelHeight, 1);
            }
        }

        // ---- upload the mip levels (non power of two images: base level only) ----
        unsigned int bufferSize = 0;
        for (unsigned int level = 0; level < info.m_mipLevels; level++) {
            if (level > 0 && info.m_nonPowerOfTwo != 0) {
                break;
            }
            if (result != 0) {
                break;
            }
            unsigned int levelWidth = header.m_pixelWidth >> level;
            if (levelWidth == 0) {
                levelWidth = 1;
            }
            unsigned int levelHeight = header.m_pixelHeight >> level;
            if (levelHeight == 0) {
                levelHeight = 1;
            }

            if (offset + 4 > size) {
                info.m_errorCode = KTX_LOAD_UNEXPECTED_END;
                goto cleanup;
            }
            unsigned int imageSize = *(const unsigned int*)(data + offset);
            if (info.m_swapEndian) {
                SwapEndian32(&imageSize, 1);
            }
            offset += 4;
            // one buffer for all levels, sized by the base level
            if (buffer == NULL) {
                buffer = (unsigned char*)malij297_Allocate(imageSize);
                if (buffer == NULL) {
                    info.m_errorCode = KTX_LOAD_OUT_OF_MEMORY;
                    goto cleanup;
                }
            } else if (imageSize > bufferSize) {
                info.m_errorCode = KTX_LOAD_INVALID_FILE;
                goto cleanup;
            }
            bufferSize = imageSize;

            for (unsigned int face = 0; face < header.m_numberOfFaces && result == 0; face++) {
                if (offset + imageSize > size) {
                    info.m_errorCode = KTX_LOAD_UNEXPECTED_END;
                    goto cleanup;
                }
                malij297_MemCpy(buffer, data + offset, imageSize);
                if (info.m_swapEndian) {
                    if (header.m_glTypeSize == 2) {
                        SwapEndian16((unsigned short*)buffer, imageSize / 2);
                    } else if (header.m_glTypeSize == 4) {
                        SwapEndian32((unsigned int*)buffer, imageSize / 4);
                    }
                }

                if (info.m_compressedFormat != 0 || header.m_glInternalFormat == GL_ETC1_RGB8_OES ||
                    ((header.m_glTypeSize * levelWidth) & 3) == 0) {
                    result = (*image)->m_functions->SetSubImage(*image, 0, level, 0, 0, levelWidth, levelHeight,
                                                                imageSize, buffer, 1);
                } else {
                    // KTX rows are padded to 4 bytes: upload row by row
                    unsigned int rowBytes = header.m_glTypeSize * levelWidth * info.m_componentCount;
                    unsigned int paddedRowBytes = (rowBytes + 3) & ~3;
                    unsigned char* row = buffer;
                    for (unsigned int y = 0; y < levelHeight && result == 0; y++) {
                        result = (*image)->m_functions->SetSubImage(*image, 0, level, 0, y, levelWidth, 1,
                                                                    rowBytes, row, 1);
                        row += paddedRowBytes;
                    }
                }
                offset += imageSize;
            }
        }
        if (result == 0) {
            result = (*image)->m_functions->Commit(*image);
        }
        if (result != 0) {
            info.m_errorCode = KTX_LOAD_RUNTIME_ERROR;
        }
    }

cleanup:
    malij297_Free(buffer);
    if (result != 0 || info.m_errorCode != KTX_LOAD_OK) {
        if (*image != NULL) {
            (*image)->m_functions->Release(*image);
            *image = NULL;
        }
        if (info.m_errorCode != KTX_LOAD_OK && info.m_errorCode != KTX_LOAD_OUT_OF_MEMORY) {
            if (info.m_compressedFormat) {
                char message[60];
                com::glu::platform::core::ICStdUtil::SPrintF_S(message, sizeof(message), "%s - GL Format = 0x%X",
                                                               "Invalid or unsupported KTX file", info.m_glFormat);
                malij297_ErrorMessageAppend(message);
            } else {
                malij297_ErrorMessageAppend("Invalid or unsupported KTX file");
            }
        }
        if (result == 0) {
            result = 1;
        }
    }
    return result;
}
