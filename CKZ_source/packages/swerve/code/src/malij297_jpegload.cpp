// malij297_jpegload.cpp - JPEG loader of the Swerve iOS porting layer (IJG libjpeg 6b).
//
// Decodes a whole JPEG held in memory scanline by scanline into a Swerve Image2D
// (luminance for one-component files, RGB otherwise). libjpeg errors longjmp back into
// JPEGLoadBuffer, which then destroys the decoder and the half-built image.
// The prebuilt engine core calls the entry points with C linkage.
#include <stdio.h>
#include <setjmp.h>

extern "C" {
#include "jpeglib.h"
#include "jerror.h"
}
#include "SwerveApi.h"

// ---- Swerve image formats (values passed to the Image2D interface) ----
enum {
    IMAGE_FORMAT_LUMINANCE = 0x61,
    IMAGE_FORMAT_RGB = 0x63
};

// Object type passed to the runtime's CreateObject for an Image2D.
static const int OBJECT_TYPE_IMAGE2D = 10;


/** libjpeg error manager extended with the jump target of JPEGLoadBuffer. */
struct JPEGErrorManager {
    struct jpeg_error_mgr m_base;
    jmp_buf m_setjmpBuffer;
};


static void jpegErrorExit(j_common_ptr cinfo);
static void initSource(j_decompress_ptr cinfo);
static boolean fillInputBuffer(j_decompress_ptr cinfo);
static void skipInputData(j_decompress_ptr cinfo, long numBytes);
static void termSource(j_decompress_ptr cinfo);

/** Return 1 when the buffer starts with a JFIF header (the segment length bytes are ignored). */
int malij297_HasJPEGIdentifier(unsigned int size, const unsigned char* data)
{
    // SOI, APP0 marker, length (skipped), "JFIF\0"
    static const unsigned char jpegIdentifier[] = { 0xff, 0xd8, 0xff, 0xe0, 0x00, 0x00, 'J', 'F', 'I', 'F', 0x00 };

    if (size < sizeof(jpegIdentifier)) {
        return 0;
    }
    for (unsigned int i = 0; i < sizeof(jpegIdentifier); i++) {
        if (i == 4 || i == 5) {
            continue;
        }
        if (jpegIdentifier[i] != data[i]) {
            return 0;
        }
    }
    return 1;
}

/**
 * Decode a JPEG file and create an Image2D in *image.
 * formatHint: requested image format (0 = use the decoded format).
 * Returns 0 on success, 1 when decoding failed, or the runtime's error code.
 */
int malij297_JPEGLoadBuffer(ISwerve* runtime, unsigned int size, const unsigned char* data,
                            unsigned int formatHint, IImage2D** image)
{
    struct jpeg_decompress_struct cinfo;
    JPEGErrorManager errorManager;
    struct jpeg_source_mgr source;

    *image = NULL;
    cinfo.err = jpeg_std_error(&errorManager.m_base);
    errorManager.m_base.error_exit = jpegErrorExit;
    if (setjmp(errorManager.m_setjmpBuffer)) {
        // libjpeg or the image calls failed: drop the decoder and the half-built image
        jpeg_destroy_decompress(&cinfo);
        if (*image != NULL) {
            (*image)->m_functions->Release(*image);
            *image = NULL;
        }
        return 1;
    }

    jpeg_create_decompress(&cinfo);
    // Read straight from the memory buffer
    source.init_source = initSource;
    source.fill_input_buffer = fillInputBuffer;
    source.skip_input_data = skipInputData;
    source.resync_to_restart = jpeg_resync_to_restart;
    source.term_source = termSource;
    cinfo.src = &source;
    source.bytes_in_buffer = size;
    source.next_input_byte = data;
    jpeg_read_header(&cinfo, TRUE);
    cinfo.two_pass_quantize = FALSE;
    cinfo.dither_mode = JDITHER_ORDERED;
    jpeg_start_decompress(&cinfo);

    unsigned int imageHeight = cinfo.output_height;
    unsigned int imageWidth = cinfo.output_width;
    int components = cinfo.output_components;
    unsigned int rowStride = imageWidth * components;
    JSAMPARRAY scanline = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE, rowStride, 1);

    unsigned int format = IMAGE_FORMAT_RGB;
    if (components == 1) {
        format = IMAGE_FORMAT_LUMINANCE;
    }
    int result = runtime->m_functions->CreateObject(OBJECT_TYPE_IMAGE2D, (void**)image);
    if (result == 0) {
        unsigned int imageFormat = formatHint;
        if (formatHint == 0) {
            imageFormat = format;
        }
        result = (*image)->m_functions->Initialize(*image, imageFormat, imageWidth, imageHeight, 1);
        // Decode and upload one scanline at a time
        while (result == 0 && cinfo.output_scanline < cinfo.output_height) {
            jpeg_read_scanlines(&cinfo, scanline, 1);
            result = (*image)->m_functions->SetSubImage(*image, format, 0, 0, cinfo.output_scanline - 1,
                                                        imageWidth, 1, rowStride, scanline[0], 1);
        }
        if (result == 0) {
            result = (*image)->m_functions->Commit(*image);
        }
    }
    if (result != 0) {
        longjmp(errorManager.m_setjmpBuffer, 1);
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    return result;
}

/** libjpeg fatal error handler: print the message, record it and jump back into JPEGLoadBuffer. */
static void jpegErrorExit(j_common_ptr cinfo)
{
    JPEGErrorManager* errorManager = (JPEGErrorManager*)cinfo->err;
    (*cinfo->err->output_message)(cinfo);
    malij297_ErrorMessageAppend("Fatal error while loading a jpeg file - exiting");
    longjmp(errorManager->m_setjmpBuffer, 1);
}

/** Source manager: nothing to prepare, the whole file is already in memory. */
static void initSource(j_decompress_ptr cinfo)
{
}

/** Source manager: the buffer ran out, which can only mean a truncated file. */
static boolean fillInputBuffer(j_decompress_ptr cinfo)
{
    ERREXIT(cinfo, JERR_BAD_STATE);
    return TRUE;
}

/** Source manager: skip over uninteresting data. */
static void skipInputData(j_decompress_ptr cinfo, long numBytes)
{
    cinfo->src->next_input_byte += numBytes;
    cinfo->src->bytes_in_buffer -= numBytes;
}

/** Source manager: nothing to release. */
static void termSource(j_decompress_ptr cinfo)
{
}
