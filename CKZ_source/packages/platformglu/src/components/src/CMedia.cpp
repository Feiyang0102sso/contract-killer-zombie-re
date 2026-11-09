// CMedia.cpp - sound resource: decodes WAV / Ogg Vorbis / IMA ADPCM into PCM and announces itself
// to the application with messages.
#include "CMedia.h"
#include "CInputStream.h"
#include "CStrWChar.h"
#include "CMediaPlayer.h"
#include "CVorbis.h"
#include "CADPCMInputStream.h"
#include "CMessage.h"

namespace com { namespace glu { namespace platform { namespace components {

extern const unsigned char kWAVChunkTypeRiffBytes[4] = { 'R', 'I', 'F', 'F' };
extern const unsigned char kWAVFormatWaveBytes[4] = { 'W', 'A', 'V', 'E' };
extern const unsigned char kWAVChunkTypeFormatBytes[4] = { 'f', 'm', 't', ' ' };
extern const unsigned char kWAVChunkTypeDataBytes[4] = { 'd', 'a', 't', 'a' };

unsigned char CMedia::ReadWavHeader(CInputStream& stream, WavFormat& format)
{
    unsigned char chunkType[4];
    stream.Read(chunkType, 4);
    unsigned int chunkSize = stream.ReadUInt32();
    if (memcmp(chunkType, kWAVChunkTypeRiffBytes, 4) != 0) {
        return 0;
    }
    stream.Read(chunkType, 4);
    if (memcmp(chunkType, kWAVFormatWaveBytes, 4) != 0) {
        return 0;
    }

    // skip chunks up to "fmt "
    while (!stream.m_error && memcmp(chunkType, kWAVChunkTypeFormatBytes, 4) != 0) {
        stream.Read(chunkType, 4);
        chunkSize = stream.ReadUInt32();
        if (memcmp(chunkType, kWAVChunkTypeFormatBytes, 4) != 0) {
            stream.Skip(chunkSize);
        }
    }
    if (memcmp(chunkType, kWAVChunkTypeFormatBytes, 4) != 0 || chunkSize != 16) {
        return 0;
    }
    format.m_formatTag = stream.ReadUInt16();
    format.m_channels = stream.ReadUInt16();
    format.m_sampleRate = stream.ReadUInt32();
    format.m_byteRate = stream.ReadUInt32();
    format.m_blockAlign = stream.ReadUInt16();
    format.m_bitsPerSample = stream.ReadUInt16();
    m_channels = format.m_channels;
    m_bitsPerSample = format.m_bitsPerSample;
    m_sampleRate = format.m_sampleRate;
    if (format.m_formatTag != 1) {
        return 0;
    }

    // skip chunks up to "data"
    while (!stream.m_error && memcmp(chunkType, kWAVChunkTypeDataBytes, 4) != 0) {
        stream.Read(chunkType, 4);
        chunkSize = stream.ReadUInt32();
        if (memcmp(chunkType, kWAVChunkTypeDataBytes, 4) != 0) {
            stream.Skip(chunkSize);
        } else {
            format.m_dataSize = chunkSize;
        }
    }
    return memcmp(chunkType, kWAVChunkTypeDataBytes, 4) == 0;
}

CMedia::~CMedia()
{
    // the player must not keep playing data that is about to go away
    CMediaPlayer* player = (CMediaPlayer*)ICMediaPlayer::GetInstance();
    if (player != NULL) {
        player->StopBeforeDelete(this);
    }
    CMedia::Destroy();
}

void CMedia::Destroy()
{
    if (m_isLoaded) {
        systems::CMessage* message = new systems::CMessage(this, MessageDestroyed, 0, 1, ClassId, 1, this);
        message->Run();
    }
    if (m_name != NULL) {
        delete m_name;
        m_name = NULL;
    }
    m_isLoaded = 0;
    CBinary::Destroy();
}

unsigned char CMedia::Load(CInputStream& stream, unsigned int type, const wchar_t* name)
{
    unsigned int available = stream.Available();
    Destroy();
    m_channels = 0;
    m_bitsPerSample = 0;
    m_sampleRate = 0;
    unsigned char result = 0;
    if (available != 0) {
        switch (type) {
        case MimeAdpcm: {
            result = 0;
            adpcm::CADPCMInputStream decoder;
            if (decoder.Open(stream, stream.Available(), m_channels, m_bitsPerSample, m_sampleRate)) {
                result = CBinary::Load(decoder, MimePcm);
            }
            break;
        }
        case MimeOgg: {
            result = 0;
            unsigned char* encoded = (unsigned char*)malloc(available);
            stream.Read(encoded, available);
            if (!stream.m_error) {
                result = DecodeVorbisBitstream(encoded, available, m_data, m_size, 1, m_channels, m_bitsPerSample,
                                               m_sampleRate);
                if (result) {
                    m_type = MimePcm;
                } else {
                    result = 0;
                    m_data = NULL;
                    m_size = 0;
                }
            }
            free(encoded);
            break;
        }
        case MimeWav: {
            WavFormat format;
            result = ReadWavHeader(stream, format);
            if (result && format.m_dataSize != 0) {
                m_data = (unsigned char*)malloc(format.m_dataSize);
                stream.Read(m_data, format.m_dataSize);
                if (!stream.m_error) {
                    result = 1;
                    m_size = format.m_dataSize;
                    m_type = MimePcm;
                } else {
                    result = 0;
                    Destroy();
                }
            }
            break;
        }
        default:
            result = CBinary::Load(stream, type);
            break;
        }
    }
    if (name != NULL) {
        m_name = new CStrWChar(name);
    }
    if (result) {
        m_isLoaded = 1;
        systems::CMessage* message = new systems::CMessage(this, MessageLoaded, 0, 1, ClassId, 1, this);
        message->Run();
    }
    return result;
}

unsigned char CMedia::GetDescription(unsigned int& channels, unsigned int& bitsPerSample, unsigned int& sampleRate,
                                     const CStrWChar** name)
{
    channels = m_channels;
    bitsPerSample = m_bitsPerSample;
    sampleRate = m_sampleRate;
    if (name != NULL) {
        *name = m_name;
    }
    return 1;
}

} } } }
