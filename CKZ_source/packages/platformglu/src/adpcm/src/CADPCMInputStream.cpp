// CADPCMInputStream.cpp - IMA ADPCM decoding input stream.
#include "CADPCMInputStream.h"

#include <string.h>
#include "CMath.h"
#include "ICMediaPlayer.h"

namespace com { namespace glu { namespace platform { namespace adpcm {

CADPCMInputStream::CADPCMInputStream()
{
    m_isOpen = 0;
    m_decodedLength = 0;
    m_decodedPosition = 0;
    m_compressed = NULL;
    m_compressedLength = 0;
    m_compressedRead = 0;
    m_pcmLength = 0;
    m_pcmPosition = 0;
}

CADPCMInputStream::~CADPCMInputStream()
{
    m_isOpen = 0;
}

unsigned char CADPCMInputStream::Open(components::CInputStream& source, unsigned int length, unsigned int& channels,
                                      unsigned int& bitsPerSample, unsigned int& sampleRate)
{
    Close();
    if (source.ReadUInt32() != FileMagic) {
        // raw ADPCM: play it in the output format of the media player
        unsigned int maxSounds;
        unsigned int bufferLength;
        unsigned char useSoftwareMixer;
        components::ICMediaPlayer::GetInstance()->GetDescription(channels, bitsPerSample, sampleRate,
                                                                 maxSounds, bufferLength, useSoftwareMixer);
        components::CInputStream::Reset(&source);
    } else {
        // Glu ADPCM header: header size, channels, bits per sample, sample rate
        unsigned int headerSize = source.ReadUInt32();
        channels = source.ReadUInt32();
        length = length - headerSize;
        bitsPerSample = source.ReadUInt32();
        sampleRate = source.ReadUInt32();
    }

    m_length = length * 4;
    m_compressed = &source;
    m_compressedLength = length;
    m_compressedRead = 0;
    m_pcmLength = length * 4;
    m_pcmPosition = 0;
    m_channels = channels;
    memset(&m_decoderState, 0, sizeof(m_decoderState));
    m_isOpen = 1;
    m_error = 0;
    return 1;
}

void CADPCMInputStream::Close()
{
    components::CInputStream::Close();
    m_isOpen = 0;
    m_length = 0;
    m_error = 0;
    m_decodedLength = 0;
    m_decodedPosition = 0;
    m_compressed = NULL;
    m_compressedLength = 0;
    m_compressedRead = 0;
    m_pcmLength = 0;
    m_pcmPosition = 0;
    m_channels = 0;
}

unsigned char CADPCMInputStream::IsOpenInternal()
{
    return m_isOpen;
}

unsigned int CADPCMInputStream::AvailableInternal()
{
    if (m_error) {
        return 0;
    }
    return m_pcmLength - m_pcmPosition;
}

void CADPCMInputStream::SkipInternal(unsigned int size)
{
    if (Available() < size) {
        m_error = 1;
        return;
    }
    while (!m_error && size != 0) {
        if (m_decodedPosition < m_decodedLength) {
            unsigned int count = math::CMath::Min(size, m_decodedLength - m_decodedPosition);
            m_decodedPosition += count;
            size -= count;
            m_pcmPosition += count;
        } else {
            ADPCMReadChunk();
            if (m_decodedLength == 0) {
                m_error = 1;
            }
        }
    }
}

void CADPCMInputStream::ADPCMReadChunk()
{
    m_decodedLength = 0;
    m_decodedPosition = 0;
    if (m_isOpen && m_compressedRead < m_compressedLength) {
        unsigned int count = math::CMath::Min((unsigned int)InputBlockSize, m_compressedLength - m_compressedRead);
        m_compressed->Read((unsigned char*)m_input, count);
        m_compressedRead += count;
        m_error = m_compressed->m_error;
        if (m_error) {
            return;
        }

        adpcm_decoder(m_input, (short*)m_output, count, m_channels, &m_decoderState);
        m_decodedLength = count * 4;
        if (m_compressedRead == m_compressedLength) {
            m_isOpen = 0;
        }
    } else {
        // nothing left to decode
        m_error = 1;
    }
}

void CADPCMInputStream::ReadInternal(unsigned char* buffer, unsigned int size)
{
    if (buffer == NULL || size == 0) {
        return;
    }
    if (Available() >= size) {
        unsigned int written = 0;
        while (!m_error && size != 0) {
            if (m_decodedPosition < m_decodedLength) {
                unsigned int count = math::CMath::Min(size, m_decodedLength - m_decodedPosition);
                memcpy(buffer + written, &m_output[m_decodedPosition], count);
                size -= count;
                written += count;
                m_decodedPosition += count;
                m_pcmPosition += count;
            } else {
                ADPCMReadChunk();
            }
        }
    } else {
        m_error = 1;
    }
    // original behaviour: on error the remaining byte count is cleared at the start of `buffer`
    if (m_error) {
        memset(buffer, 0, size);
    }
}

void CADPCMInputStream::ResetInternal()
{
}

} } } }
