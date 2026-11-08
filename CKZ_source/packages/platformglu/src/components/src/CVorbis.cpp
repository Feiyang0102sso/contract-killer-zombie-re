// CVorbis.cpp - Ogg Vorbis decoding of an in-memory file into PCM / WAV.
#include <stdio.h>
#include "CVorbis.h"
#include "COutputStream.h"
#include "CArrayOutputStream.h"

namespace com { namespace glu { namespace platform { namespace components {

/** Size of the WAV header written in front of the PCM data. */
static const unsigned int kWavHeaderSize = 44;
/** Decoded sample width. */
static const unsigned int kBitsPerSample = 16;

CVorbisBufferStream::CVorbisBufferStream(const unsigned char* data, unsigned int size)
{
    m_data = data;
    m_size = size;
    m_position = 0;
}

int CVorbisBufferStream::Close()
{
    return 0;
}

size_t CVorbisBufferStream::_read(void* destination, size_t size, size_t count, void* datasource)
{
    CVorbisBufferStream* stream = (CVorbisBufferStream*)datasource;
    size_t bytes = size * count;
    size_t remaining = (size_t)(stream->m_size - stream->m_position);
    if (bytes > remaining) {
        bytes = remaining;
    }
    memcpy(destination, stream->m_data + (size_t)stream->m_position, bytes);
    stream->m_position += bytes;
    return bytes;
}

int CVorbisBufferStream::_seek(void* datasource, long long offset, int whence)
{
    CVorbisBufferStream* stream = (CVorbisBufferStream*)datasource;
    switch (whence) {
    case SEEK_SET:
        stream->m_position = offset;
        break;
    case SEEK_CUR:
        stream->m_position += offset;
        break;
    case SEEK_END:
        stream->m_position = stream->m_size - offset;
        break;
    }
    return 0;
}

int CVorbisBufferStream::_close(void* datasource)
{
    return 0;
}

long CVorbisBufferStream::_tell(void* datasource)
{
    CVorbisBufferStream* stream = (CVorbisBufferStream*)datasource;
    return (long)stream->m_position;
}

void CVorbisBufferStream::GetOvCallbacks(ov_callbacks& callbacks)
{
    callbacks.read_func = _read;
    callbacks.seek_func = _seek;
    callbacks.close_func = _close;
    callbacks.tell_func = _tell;
}

void WriteWavHeader(COutputStream& stream, unsigned short channels, unsigned short bitsPerSample,
                    unsigned int sampleRate, unsigned int dataSize)
{
    stream.Write((const unsigned char*)"RIFF", 4);
    stream.WriteUInt32(dataSize + 36);
    stream.Write((const unsigned char*)"WAVE", 4);
    stream.Write((const unsigned char*)"fmt ", 4);
    stream.WriteUInt32(16);                     // fmt chunk size
    stream.WriteUInt16(1);                      // PCM
    stream.WriteUInt16(channels);
    stream.WriteUInt32(sampleRate);
    unsigned short blockAlign = channels * (bitsPerSample / 8);
    stream.WriteUInt32(sampleRate * blockAlign);
    stream.WriteUInt16(blockAlign);
    stream.WriteUInt16(bitsPerSample);
    stream.Write((const unsigned char*)"data", 4);
    stream.WriteUInt32(dataSize);
}

unsigned char DecodeVorbisBitstream(const unsigned char* data, unsigned int size, unsigned char*& output,
                                    unsigned int& outputSize, unsigned char rawPcm, unsigned int& channels,
                                    unsigned int& bitsPerSample, unsigned int& sampleRate)
{
    OggVorbis_File vorbisFile;
    output = NULL;
    outputSize = 0;

    CVorbisBufferStream stream(data, size);
    ov_callbacks callbacks;
    CVorbisBufferStream::GetOvCallbacks(callbacks);
    if (ov_open_callbacks(&stream, &vorbisFile, NULL, 0, callbacks) < 0) {
        return 0;
    }

    unsigned char decoded = 0;
    vorbis_info* info = ov_info(&vorbisFile, -1);
    if (info != NULL) {
        channels = info->channels;
        bitsPerSample = kBitsPerSample;
        sampleRate = info->rate;

        unsigned int pcmSize = (unsigned int)((double)ov_pcm_total(&vorbisFile, -1) * info->channels * (bitsPerSample / 8));
        if (info->channels == 2) {
            pcmSize &= ~1u;
        }
        unsigned int totalSize = pcmSize + kWavHeaderSize;
        if (rawPcm) {
            totalSize = pcmSize;
        }

        unsigned char* buffer = (unsigned char*)malloc(totalSize);
        if (buffer != NULL) {
            unsigned char* pcm = buffer;
            unsigned char headerWritten = 1;
            if (!rawPcm) {
                CArrayOutputStream header;
                headerWritten = 0;
                if (header.Open(buffer, kWavHeaderSize)) {
                    WriteWavHeader(header, info->channels, bitsPerSample, info->rate, pcmSize);
                    if (!header.m_error) {
                        pcm = buffer + kWavHeaderSize;
                        headerWritten = 1;
                    }
                }
            }

            if (headerWritten) {
                int bytesPerSample = bitsPerSample / 8;
                unsigned int filled = 0;
                long read = ov_read(&vorbisFile, (char*)pcm, pcmSize, 0, bytesPerSample, 1, NULL);
                while (read > 0) {
                    pcm += read;
                    filled += read;
                    read = ov_read(&vorbisFile, (char*)pcm, pcmSize - filled, 0, bytesPerSample, 1, NULL);
                }
                output = buffer;
                outputSize = totalSize;
                decoded = 1;
            } else {
                free(buffer);
            }
        }
    }
    ov_clear(&vorbisFile);
    return decoded;
}

} } } }
