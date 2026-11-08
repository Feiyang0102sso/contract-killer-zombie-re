// CVorbis.h - Ogg Vorbis decoding of an in-memory file into PCM / WAV (created by b006).
#ifndef CVORBIS_H
#define CVORBIS_H

#include "vorbisfile.h"

namespace com { namespace glu { namespace platform { namespace components {

class COutputStream;

/** Read-only memory "file" for the vorbisfile callbacks. */
class CVorbisBufferStream {
public:
    CVorbisBufferStream(const unsigned char* data, unsigned int size);

    int Close();

    // vorbisfile callbacks; `datasource` is the CVorbisBufferStream
    static size_t _read(void* destination, size_t size, size_t count, void* datasource);
    static int _seek(void* datasource, long long offset, int whence);
    static int _close(void* datasource);
    static long _tell(void* datasource);

    static void GetOvCallbacks(ov_callbacks& callbacks);

    const unsigned char* m_data;    // 0x00
    long long m_size;               // 0x04
    long long m_position;           // 0x0c
};

/** Write a 44-byte PCM WAV header for `dataSize` bytes of sample data. */
void WriteWavHeader(COutputStream& stream, unsigned short channels, unsigned short bitsPerSample,
                    unsigned int sampleRate, unsigned int dataSize);

/**
 * Decode a whole Ogg Vorbis file to 16-bit PCM. The buffer (np_malloc'd, owned by the caller)
 * starts with a WAV header unless `rawPcm` is set. Returns 1 on success.
 */
unsigned char DecodeVorbisBitstream(const unsigned char* data, unsigned int size, unsigned char*& output,
                                    unsigned int& outputSize, unsigned char rawPcm, unsigned int& channels,
                                    unsigned int& bitsPerSample, unsigned int& sampleRate);

} } } }

#endif
