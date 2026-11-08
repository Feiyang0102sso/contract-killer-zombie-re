// malij297_pngload.cpp - PNG loader of the Swerve iOS porting layer (libpng 1.2.41 read path).
//
// Decodes a whole PNG held in memory and creates a Swerve Image2D from it:
//   - gray / gray+alpha / RGB / RGBA images become plain images (16-bit samples are stripped,
//     1/2/4-bit samples are unpacked and scaled to 0..255);
//   - a tRNS colour key turns gray into gray+alpha and RGB into RGBA (key pixels get alpha 0);
//   - palette images become paletted images (palette RGB or RGBA when tRNS is present).
// The prebuilt engine core calls the entry points with C linkage.
#include "png.h"
#include "ICDebug.h"
#include "NPMalloc.h"
#include "SwerveApi.h"

// ---- Swerve image formats (values passed to the Image2D interface) ----
enum {
    IMAGE_FORMAT_LUMINANCE = 0x61,
    IMAGE_FORMAT_LUMINANCE_ALPHA = 0x62,
    IMAGE_FORMAT_RGB = 0x63,
    IMAGE_FORMAT_RGBA = 0x64
};

// Option bits a caller may add to a requested format; masked off before comparing formats.
static const unsigned int IMAGE_FORMAT_OPTION_MASK = 0x1c000;

// Object type passed to the runtime's CreateObject for an Image2D.
static const int OBJECT_TYPE_IMAGE2D = 10;

// Length of the PNG file signature.
static const unsigned int PNG_SIGNATURE_SIZE = 8;


/** Read cursor over the in-memory PNG file (libpng io_ptr). */
struct PNGReadState {
    const unsigned char* m_current;
    const unsigned char* m_end;
};


static void PNG_error(png_structp png, png_const_charp message);
static void PNG_warning(png_structp png, png_const_charp message);
static png_voidp PNG_malloc(png_structp png, png_size_t size);
static void PNG_free(png_structp png, png_voidp pointer);
static void PNG_bufcpy(png_structp png, png_bytep destination, png_size_t length);

/** Return 1 when the buffer starts with the PNG signature. */
int malij297_HasPNGIdentifier(unsigned int size, const unsigned char* data)
{
    if (size < PNG_SIGNATURE_SIZE) {
        return 0;
    }
    return png_sig_cmp((png_bytep)data, 0, PNG_SIGNATURE_SIZE) == 0;
}

/**
 * Decode a PNG file and create an Image2D in *image.
 * formatHint: requested image format (0 = use the decoded format), may carry option bits.
 * Returns 0 on success, 1 for an invalid file, 2 when out of memory, or the runtime's error code.
 */
int malij297_PNGLoadBuffer(ISwerve* runtime, unsigned int size, const unsigned char* data,
                           unsigned int formatHint, IImage2D** image)
{
    // Scale factor from an n-bit sample to 0..255, indexed by bit depth (1, 2, 4 and 8 occur).
    // The original symbol spans 48 bytes; entries past 8 are zero or alignment padding.
    static const int depthFactors[] = { 0, 255, 85, 36, 17, 8, 4, 2, 1 };

    int result = 1;
    *image = NULL;
    if (data == NULL || size < PNG_SIGNATURE_SIZE) {
        return result;
    }
    if (png_sig_cmp((png_bytep)data, 0, PNG_SIGNATURE_SIZE) != 0) {
        return result;
    }

    png_structp png = png_create_read_struct_2(PNG_LIBPNG_VER_STRING, NULL, PNG_error, PNG_warning,
                                               NULL, PNG_malloc, PNG_free);
    png_infop info = png_create_info_struct(png);
    if (png == NULL || info == NULL) {
        return result;
    }

    int bitDepth = 8;
    unsigned char* pixels = NULL;
    unsigned char* palette = NULL;
    unsigned int paletteSize = 0;
    png_bytepp rows = NULL;
    if (setjmp(png_jmpbuf(png))) {
        // libpng reported an error (PNG_error) and jumped back here
        result = 1;
        goto cleanup;
    }

    {
        PNGReadState readState;
        readState.m_current = data;
        readState.m_end = data + size;
        png_uint_32 width;
        png_uint_32 height;
        int colorType;
        png_bytep trans = NULL;
        png_color_16p transValues = NULL;
        int numTrans = 0;
        png_colorp pngPalette;
        int numPalette;

        png_set_read_fn(png, &readState, PNG_bufcpy);
        png_read_info(png, info);
        png_get_IHDR(png, info, &width, &height, &bitDepth, &colorType, NULL, NULL, NULL);
        unsigned int imageWidth = width;
        int imageHeight = height;
        if (png_get_valid(png, info, PNG_INFO_tRNS)) {
            png_get_tRNS(png, info, &trans, &numTrans, &transValues);
        }
        if (bitDepth == 16) {
            png_set_strip_16(png);
        }
        if (bitDepth < 8) {
            png_set_packing(png);
        }

        // Pick the image format; a colour key adds an alpha channel filled in after decoding.
        unsigned int format;
        unsigned int rowBytes;
        int hasColorKey;
        unsigned char keyRed;           // red key, or the gray key of a gray image
        unsigned char keyGreen;
        unsigned char keyBlue;
        switch (colorType) {
        case PNG_COLOR_TYPE_GRAY:
            rowBytes = width;
            if (numTrans > 0 && bitDepth != 16) {
                keyRed = transValues->gray * depthFactors[bitDepth];
                rowBytes = rowBytes * 2;
                format = IMAGE_FORMAT_LUMINANCE_ALPHA;
                hasColorKey = 1;
            } else {
                format = IMAGE_FORMAT_LUMINANCE;
                hasColorKey = 0;
            }
            break;
        case PNG_COLOR_TYPE_PALETTE: {
            png_get_PLTE(png, info, &pngPalette, &numPalette);
            if (numTrans > 0) {
                paletteSize = numPalette * 4;
                format = IMAGE_FORMAT_RGBA;
            } else {
                paletteSize = numPalette * 3;
                format = IMAGE_FORMAT_RGB;
            }
            palette = (unsigned char*)malij297_Allocate(paletteSize);
            if (palette == NULL) {
                goto cleanup;
            }
            rowBytes = width;
            // Copy the palette, with alpha from tRNS (opaque past the tRNS entries)
            unsigned char* entry = palette;
            for (int index = 0; index < numPalette; index++) {
                *entry++ = pngPalette->red;
                *entry++ = pngPalette->green;
                *entry++ = pngPalette->blue;
                if (numTrans > 0) {
                    unsigned char alpha = 0xff;
                    if (index < numTrans) {
                        alpha = *trans++;
                    }
                    *entry++ = alpha;
                }
                pngPalette++;
            }
            hasColorKey = 0;
            break;
        }
        case PNG_COLOR_TYPE_RGB:
            rowBytes = width * 3;
            if (numTrans > 0 && bitDepth != 16) {
                int factor = depthFactors[bitDepth];
                rowBytes = width * 4;
                format = IMAGE_FORMAT_RGBA;
                keyRed = transValues->red * factor;
                keyGreen = transValues->green * factor;
                keyBlue = transValues->blue * factor;
                hasColorKey = 1;
            } else {
                format = IMAGE_FORMAT_RGB;
                hasColorKey = 0;
            }
            break;
        case PNG_COLOR_TYPE_RGB_ALPHA:
            rowBytes = width * 4;
            format = IMAGE_FORMAT_RGBA;
            hasColorKey = 0;
            break;
        case PNG_COLOR_TYPE_GRAY_ALPHA:
            rowBytes = width * 2;
            format = IMAGE_FORMAT_LUMINANCE_ALPHA;
            hasColorKey = 0;
            break;
        default:
            goto cleanup;
        }

        unsigned int imageSize = height * rowBytes;
        pixels = (unsigned char*)malij297_Allocate(imageSize);
        if (pixels == NULL) {
            result = 2;
            goto cleanup;
        }
        rows = (png_bytepp)malij297_Allocate(height * sizeof(png_bytep));
        if (rows == NULL) {
            result = 2;
            goto cleanup;
        }
        for (int y = 0; y < imageHeight; y++) {
            rows[y] = pixels + y * rowBytes;
        }
        png_read_image(png, rows);
        png_read_end(png, info);

        if (palette != NULL) {
            // Paletted image: pixels hold palette indices
            result = runtime->m_functions->CreateObject(OBJECT_TYPE_IMAGE2D, (void**)image);
            if (result == 0) {
                unsigned int imageFormat = formatHint;
                if (formatHint == 0) {
                    imageFormat = format;
                }
                int mode = 1;
                if (format == (formatHint & ~IMAGE_FORMAT_OPTION_MASK) || formatHint == 0) {
                    mode = 2;
                }
                result = (*image)->m_functions->Initialize(*image, imageFormat, imageWidth, imageHeight, mode);
                if (result == 0) {
                    result = (*image)->m_functions->SetPalettedImage(*image, format, 0, imageSize, pixels,
                                                                     paletteSize, palette);
                }
                if (result == 0) {
                    result = (*image)->m_functions->Commit(*image);
                } else {
                    (*image)->m_functions->Release(*image);
                    *image = NULL;
                }
            }
            goto cleanup;
        }

        // Scale unpacked 1/2/4-bit samples to 0..255
        if (bitDepth < 8) {
            for (unsigned int i = 0; i < imageSize; i++) {
                pixels[i] = pixels[i] * depthFactors[bitDepth];
            }
        }

        // Expand colour-keyed rows in place, from the end of each row backwards
        if (hasColorKey) {
            if (format == IMAGE_FORMAT_LUMINANCE_ALPHA) {
                unsigned char* row = pixels;
                for (int y = 0; y < imageHeight; y++) {
                    unsigned char* source = row + imageWidth - 1;
                    unsigned char* destination = row + rowBytes - 1;
                    for (unsigned int x = 0; x < imageWidth; x++) {
                        if (keyRed == *source) {
                            *destination-- = 0x00;
                        } else {
                            *destination-- = 0xff;
                        }
                        *destination-- = *source--;
                    }
                    row += rowBytes;
                }
            } else if (format == IMAGE_FORMAT_RGBA) {
                unsigned char* row = pixels;
                for (int y = 0; y < imageHeight; y++) {
                    unsigned char* source = row + imageWidth * 3 - 1;
                    unsigned char* destination = row + rowBytes - 1;
                    for (unsigned int x = 0; x < imageWidth; x++) {
                        unsigned char red = source[-2];
                        unsigned char green = source[-1];
                        unsigned char blue = source[0];
                        unsigned char alpha = 0xff;
                        if (keyRed == red && keyGreen == green && keyBlue == blue) {
                            alpha = 0x00;
                        }
                        destination[0] = alpha;
                        destination[-1] = blue;
                        destination[-2] = green;
                        destination[-3] = red;
                        source -= 3;
                        destination -= 4;
                    }
                    row += rowBytes;
                }
            }
        }

        result = runtime->m_functions->CreateObject(OBJECT_TYPE_IMAGE2D, (void**)image);
        if (result == 0) {
            unsigned int imageFormat = formatHint;
            if (formatHint == 0) {
                imageFormat = format;
            }
            result = (*image)->m_functions->Initialize(*image, imageFormat, imageWidth, imageHeight, 1);
            if (result == 0) {
                result = (*image)->m_functions->SetSubImage(*image, format, 0, 0, 0, imageWidth, imageHeight,
                                                            imageSize, pixels, 1);
            }
            if (result == 0) {
                (*image)->m_functions->Commit(*image);
            } else {
                (*image)->m_functions->Release(*image);
                *image = NULL;
            }
        }
    }

cleanup:
    malij297_Free(pixels);
    malij297_Free(palette);
    malij297_Free(rows);
    png_destroy_read_struct(&png, &info, NULL);
    return result;
}

/** libpng error callback: log the message (libpng then longjmps back into PNGLoadBuffer). */
static void PNG_error(png_structp png, png_const_charp message)
{
    com::glu::platform::core::ICDebug::LogMessage("ERROR malij297_PNGLoadBuffer: %s", message);
}

/** libpng warning callback: log the message. */
static void PNG_warning(png_structp png, png_const_charp message)
{
    com::glu::platform::core::ICDebug::LogMessage("WARNING malij297_PNGLoadBuffer: %s", message);
}

/** libpng allocator. */
static png_voidp PNG_malloc(png_structp png, png_size_t size)
{
    return np_malloc(size);
}

/** libpng deallocator. */
static void PNG_free(png_structp png, png_voidp pointer)
{
    np_free(pointer);
}

/** libpng read callback: copy the next `length` bytes of the in-memory file. */
static void PNG_bufcpy(png_structp png, png_bytep destination, png_size_t length)
{
    PNGReadState* state = (PNGReadState*)png->io_ptr;
    if (state == NULL || state->m_current + length > state->m_end) {
        png_error(png, "Read Error");
    }
    malij297_MemCpy(destination, state->m_current, length);
    state->m_current += length;
}
