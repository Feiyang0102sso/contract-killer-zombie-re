// malij297_decompressorapi.cpp - streaming zlib decompressor of the Swerve (M3G) porting layer.
// The prebuilt runtime starts a decompressor with a format tag, feeds compressed chunks and gets
// every inflated block back through a callback. Only the 'ZLIB' format exists.
#include "zlib.h"
#include "SwerveApi.h"

// ---- constants ----
/** Format tag of the only supported compression ('ZLIB' as a little-endian word). */
static const unsigned int DECOMPRESSOR_FORMAT_ZLIB = 0x5a4c4942;
/** Size of the inflate output block handed to the callback. */
static const unsigned int DECOMPRESSOR_BLOCK_SIZE = 0x400;

/** State of one decompression (0x410 bytes, allocated by DecompressOnDataStart). */
struct malij297_Decompressor {
    unsigned int m_format;                          // 0x00 DECOMPRESSOR_FORMAT_ZLIB
    void* m_userData;                               // 0x04 first callback argument
    malij297_DecompressCallback m_callback;         // 0x08
    z_stream* m_stream;                             // 0x0c
    unsigned char m_block[DECOMPRESSOR_BLOCK_SIZE]; // 0x10 inflate output
};

extern "C" {

/** Begin a decompression of `format`; stores the new state in `*handle`. Returns 1 on success. */
int malij297_DecompressOnDataStart(unsigned int format, void* userData, malij297_DecompressCallback callback,
                                   malij297_Decompressor** handle)
{
    *handle = NULL;
    if (format != DECOMPRESSOR_FORMAT_ZLIB) {
        return 0;
    }

    malij297_Decompressor* decompressor = (malij297_Decompressor*)malij297_Allocate(sizeof(malij297_Decompressor));
    if (decompressor == NULL) {
        return 0;
    }
    *handle = decompressor;
    decompressor->m_format = DECOMPRESSOR_FORMAT_ZLIB;
    decompressor->m_userData = userData;
    decompressor->m_callback = callback;
    malij297_MemSet(decompressor->m_block, 0, DECOMPRESSOR_BLOCK_SIZE);

    z_stream* stream = (z_stream*)malij297_Allocate(sizeof(z_stream));
    decompressor->m_stream = stream;
    if (stream == NULL) {
        return 0;
    }
    stream->next_in = NULL;
    stream->avail_in = 0;
    stream->zalloc = NULL;
    stream->zfree = NULL;
    stream->opaque = NULL;
    if (inflateInit(stream) != Z_OK) {
        return 0;
    }
    return 1;
}

/** Inflate one compressed chunk, handing every output block to the callback. Returns 1 on success. */
int malij297_DecompressOnData(const unsigned char* data, unsigned int size, malij297_Decompressor** handle)
{
    malij297_Decompressor* decompressor = *handle;
    if (decompressor == NULL) {
        return 0;
    }
    if (decompressor->m_format != DECOMPRESSOR_FORMAT_ZLIB) {
        return 0;
    }

    z_stream* stream = decompressor->m_stream;
    int keepGoing = 1;
    int status;
    stream->next_in = (Bytef*)data;
    stream->avail_in = size;
    do {
        stream->next_out = decompressor->m_block;
        stream->avail_out = DECOMPRESSOR_BLOCK_SIZE;
        status = inflate(stream, Z_SYNC_FLUSH);
        if (stream->avail_out != DECOMPRESSOR_BLOCK_SIZE) {
            keepGoing = decompressor->m_callback(decompressor->m_userData, DECOMPRESSOR_BLOCK_SIZE - stream->avail_out,
                                                 decompressor->m_block);
        }
    } while (keepGoing && status == Z_OK && stream->avail_out == 0);

    if (!keepGoing) {
        return 0;
    }
    if (status == Z_OK || status == Z_STREAM_END) {
        return 1;
    }
    // Z_BUF_ERROR without any output only means the chunk was used up.
    int usedUp = (status == Z_BUF_ERROR && stream->avail_out == DECOMPRESSOR_BLOCK_SIZE);
    return usedUp;
}

/** Finish the decompression and free its state; returns 1 when zlib ended cleanly. */
int malij297_DecompressOnDataEnd(malij297_Decompressor** handle)
{
    int result = 0;
    malij297_Decompressor* decompressor = *handle;
    if (decompressor == NULL) {
        return 0;
    }
    if (decompressor->m_format == DECOMPRESSOR_FORMAT_ZLIB) {
        int status = inflateEnd(decompressor->m_stream);
        malij297_Free(decompressor->m_stream);
        result = (status == Z_OK);
    }
    malij297_Free(decompressor);
    *handle = NULL;
    return result;
}

/** Abort a decompression after an error (same cleanup as DecompressOnDataEnd). */
void malij297_DecompressError(malij297_Decompressor** handle)
{
    malij297_DecompressOnDataEnd(handle);
}

/** zlib Adler-32 checksum. */
unsigned long malij297_Adler32(unsigned long adler, const unsigned char* data, unsigned int size)
{
    return adler32(adler, data, size);
}

}
