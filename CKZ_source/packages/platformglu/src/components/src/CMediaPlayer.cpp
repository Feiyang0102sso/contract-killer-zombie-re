// CMediaPlayer.cpp - platform independent media player: event lists, sound properties and the
// software mixer (resampling + saturating mix) used by the PCM / ADPCM / Ogg sound events.
#include "CMediaPlayer.h"
#include "CMedia.h"
#include "CInputStream.h"
#include "CArrayInputStream.h"
#include "CADPCMInputStream.h"
#include "CVorbis.h"
#include "CEventListener.h"
#include "CMathFixed.h"

namespace com { namespace glu { namespace platform { namespace components {

/** Read one source sample as signed 16 bit (8 bit samples are scaled up). */
static inline int ReadSample(const unsigned char* source, unsigned int offset, unsigned int bitsPerSample)
{
    int sample;
    if (bitsPerSample == 8) {
        sample = source[offset] << 8;
    } else {
        sample = *(const short*)(source + offset);
    }
    return sample;
}

/** `value`, but not below `minimum`. */
static inline int LimitMin(int value, int minimum)
{
    if (value < minimum) {
        value = minimum;
    }
    return value;
}

/** `value`, but not above `maximum`. */
static inline int LimitMax(int value, int maximum)
{
    if (maximum < value) {
        value = maximum;
    }
    return value;
}

/** Fixed point multiply with CMathFixed::FBits fraction bits. */
static inline int FixedMul(int a, int b)
{
    return (int)(((long long)a * (long long)b) >> math::CMathFixed::FBits);
}

// ---------------------------------------------------------------------------------------------
// CMediaEvent / CSoundEvent
// ---------------------------------------------------------------------------------------------

unsigned char CMediaEvent::GetPriority() const
{
    return m_priority;
}

void CSoundEvent::SetSoundInfo(CBinary* binary, unsigned char ownsBinary)
{
    m_binary = binary;
    m_ownsBinary = ownsBinary;
    CMedia* media = NULL;
    if (binary->m_classId == CMedia::ClassId) {
        media = (CMedia*)binary;
    }
    if (media != NULL) {
        media->GetDescription(m_channels, m_bitsPerSample, m_sampleRate, NULL);
    }
}

unsigned char CSoundEvent::SetProperty(ICMediaPlayer::SoundProperty property, int value)
{
    LimitPropertyValue(property, value);
    unsigned char result = 0;
    switch (property) {
    case ICMediaPlayer::SoundProperty_Volume:
        m_volume = value;
        result = SetGain();
        break;
    case ICMediaPlayer::SoundProperty_Pitch:
        m_pitch = value;
        result = SetPitch();
        break;
    case ICMediaPlayer::SoundProperty_Pan:
        m_pan = value;
        result = SetPan();
        break;
    case ICMediaPlayer::SoundProperty_OffsetBytes:
        result = SetOffsetBytes(value);
        break;
    case ICMediaPlayer::SoundProperty_OffsetMS:
        result = SetOffsetMS(value);
        break;
    default:
        break;
    }
    return result;
}

unsigned char CSoundEvent::SetPropertyF(ICMediaPlayer::SoundProperty property, int value)
{
    return 0;
}

unsigned char CSoundEvent::SetPropertyFV(ICMediaPlayer::SoundProperty property, int* values, int count)
{
    return 0;
}

unsigned char CSoundEvent::GetProperty(ICMediaPlayer::SoundProperty property, int& value)
{
    value = 0;
    switch (property) {
    case ICMediaPlayer::SoundProperty_Volume:
        value = m_volume;
        return 1;
    case ICMediaPlayer::SoundProperty_Pitch:
        value = m_pitch;
        return 1;
    case ICMediaPlayer::SoundProperty_Pan:
        value = m_pan;
        return 1;
    case ICMediaPlayer::SoundProperty_OffsetBytes:
        return GetOffsetBytes(value);
    case ICMediaPlayer::SoundProperty_OffsetMS:
        return GetOffsetMS(value);
    default:
        break;
    }
    return 0;
}

unsigned char CSoundEvent::GetPropertyF(ICMediaPlayer::SoundProperty property, int& value)
{
    return 0;
}

unsigned char CSoundEvent::GetPropertyFV(ICMediaPlayer::SoundProperty property, int* values, int count)
{
    return 0;
}

void CSoundEvent::LimitPropertyValue(ICMediaPlayer::SoundProperty property, int& value)
{
    switch (property) {
    case ICMediaPlayer::SoundProperty_Volume:
        value = LimitMax(value, 100);
        value = LimitMin(value, 0);
        break;
    case ICMediaPlayer::SoundProperty_Pitch:
    case ICMediaPlayer::SoundProperty_Pan:
        value = LimitMax(value, 100);
        value = LimitMin(value, -100);
        break;
    case ICMediaPlayer::SoundProperty_OffsetBytes:
        value = LimitMin(value, 0);
        value = LimitMax(value, m_binary->m_size);
        break;
    case ICMediaPlayer::SoundProperty_OffsetMS: {
        value = LimitMin(value, 0);
        // length of the sound in milliseconds
        unsigned int bytesPerSecond = (m_bitsPerSample >> 3) * m_channels * m_sampleRate;
        int duration = (int)((unsigned long long)m_binary->m_size * 1000 / bytesPerSecond);
        value = LimitMax(value, duration);
        break;
    }
    default:
        break;
    }
}

void CSoundEvent::LimitPropertyValueF(ICMediaPlayer::SoundProperty property, int& value)
{
}

void CSoundEvent::LimitPropertyValueFV(ICMediaPlayer::SoundProperty property, int* values, int count)
{
}

// ---------------------------------------------------------------------------------------------
// CSoundEventPCM
// ---------------------------------------------------------------------------------------------

CSoundEventPCM::CSoundEventPCM()
{
    m_offset = 0;
    m_lastError = 1;
    m_gainShift = 0;
    m_bufferSize = 0;
    m_buffer = NULL;
}

CSoundEventPCM::~CSoundEventPCM()
{
    Stop();
    Destroy();
}

unsigned char CSoundEventPCM::Stop()
{
    if (m_binary == NULL) {
        m_lastError = 1;
        return 0;
    }
    m_offset = m_binary->m_size;
    m_state = State_Stopped;
    m_lastError = 0;
    return 1;
}

void CSoundEventPCM::Destroy()
{
    if (m_buffer != NULL) {
        free(m_buffer);
        m_buffer = NULL;
    }
    m_bufferSize = 0;
}

unsigned char CSoundEventPCM::Initialize()
{
    unsigned char result = 0;
    Destroy();
    if (m_binary != NULL) {
        m_lastError = 0;
        SetGain();
        SetPitch();
        SetPan();
        m_position = 0;
        m_lastError = 0;
        result = 1;
    }
    return result;
}

unsigned char CSoundEventPCM::Play()
{
    unsigned char result;
    if (m_binary == NULL) {
        m_lastError = 1;
        result = 0;
    } else {
        m_offset = 0;
        m_state = State_Playing;
        m_lastError = 0;
        result = 1;
    }
    return result;
}

unsigned char CSoundEventPCM::Pause()
{
    if (m_binary == NULL) {
        m_lastError = 1;
        return 0;
    }
    m_state = State_Paused;
    m_lastError = 0;
    return 1;
}

unsigned char CSoundEventPCM::Resume()
{
    if (m_binary == NULL) {
        m_lastError = 1;
        return 0;
    }
    m_state = State_Playing;
    m_lastError = 0;
    return 1;
}

unsigned char CSoundEventPCM::HandleUpdate(int deltaTime)
{
    return 0;
}

int CSoundEventPCM::GetLastErr()
{
    return m_lastError;
}

void CSoundEventPCM::AddSamplesToBuffer(unsigned char* buffer, unsigned int size)
{
    if (m_bufferSize < size) {
        if (m_buffer != NULL) {
            free(m_buffer);
            m_buffer = NULL;
        }
        m_buffer = (unsigned char*)malloc(size);
        m_bufferSize = size;
    }

    unsigned int remaining = size;
    while (remaining != 0 && m_state == State_Playing) {
        unsigned int resampleRemaining = size;
        unsigned int produced = ResampleBuffer(m_buffer, size, resampleRemaining, m_binary->m_data, m_binary->m_size);
        if (produced == 0 || AddSamplesToBufferInternal(buffer, size, remaining, m_buffer, produced) == 0) {
            // nothing left to mix: treat as the end of the sound
            m_offset = m_binary->m_size;
        }
        if (m_offset >= m_binary->m_size) {
            if (m_loop & 1) {
                m_offset = 0;
                m_position = 0;
            } else {
                m_state = State_Stopped;
            }
        }
    }
}

unsigned int CSoundEventPCM::ResampleBuffer(unsigned char* output, unsigned int outputSize,
                                            unsigned int& outputRemaining, const unsigned char* source,
                                            unsigned int sourceSize)
{
    unsigned int channels;
    unsigned int bitsPerSample;
    unsigned int sampleRate;
    unsigned int maxSounds;
    unsigned int bufferLength;
    unsigned char useSoftwareMixer;
    ICMediaPlayer::GetInstance()->GetDescription(channels, bitsPerSample, sampleRate, maxSounds, bufferLength,
                                                 useSoftwareMixer);

    unsigned int outputFrameBytes = channels * (bitsPerSample >> 3);
    unsigned int sourceSampleBytes = m_bitsPerSample >> 3;
    unsigned int sourceFrameBytes = m_channels * sourceSampleBytes;

    if (m_channels == channels && m_bitsPerSample == bitsPerSample && m_sampleRate == sampleRate) {
        // same format: plain copy
        unsigned int copySize = sourceSize - m_offset;
        if (outputRemaining < copySize) {
            copySize = outputRemaining;
        }
        unsigned long long advance = ((unsigned long long)copySize << 16) / sourceFrameBytes;
        memcpy(output + (outputSize - outputRemaining), source + m_offset, copySize);
        m_offset += copySize;
        outputRemaining -= copySize;
        m_position += advance;
        return copySize;
    }

    // split the 16.16 position into a whole frame index and the fraction we start at
    int frameIndex = (int)(m_position >> 16);
    int fraction = (int)(m_position - ((long long)frameIndex << 16));
    unsigned int outputOffset = outputSize - outputRemaining;
    m_position -= fraction;

    int rateRatio = (int)(((unsigned long long)sampleRate << 16) / m_sampleRate);
    int step = (int)(((long long)0x10000 << math::CMathFixed::FBits) / rateRatio);

    int maxSample = 0x7fff;
    if (bitsPerSample == 8) {
        maxSample = 0x7f;
    }
    int minSample = ~maxSample;
    int outputShift = 16 - bitsPerSample;

    unsigned int produced = 0;
    unsigned int outputIndex = 0;
    while (outputRemaining >= outputFrameBytes) {
        int sourceFrame = frameIndex + (fraction >> 16);
        if (sourceFrame * sourceFrameBytes + sourceFrameBytes > sourceSize) {
            break;
        }
        unsigned int sourceOffset = sourceFrame * sourceFrameBytes;

        int left;
        int right;
        if (m_sampleRate != sampleRate) {
            // linear interpolation between this frame and the next one
            if ((sourceFrame + 1) * sourceFrameBytes + sourceFrameBytes > sourceSize) {
                break;
            }
            unsigned int nextOffset = (sourceFrame + 1) * sourceFrameBytes;
            int whole = (fraction >> math::CMathFixed::FBits) << math::CMathFixed::FBits;
            int weightNext = fraction - whole;
            int weightCurrent = 0x10000 - weightNext;

            int left0 = ReadSample(source, sourceOffset, m_bitsPerSample) << 16;
            int right0 = left0;
            if (m_channels == 2) {
                right0 = ReadSample(source, sourceOffset + sourceSampleBytes, m_bitsPerSample) << 16;
            }
            int left1 = ReadSample(source, nextOffset, m_bitsPerSample) << 16;
            int right1 = left1;
            if (m_channels == 2) {
                right1 = ReadSample(source, nextOffset + sourceSampleBytes, m_bitsPerSample) << 16;
            }

            left = (FixedMul(left1, weightNext) + FixedMul(left0, weightCurrent)) >> 16;
            if (m_channels == 2) {
                right = (FixedMul(right1, weightNext) + FixedMul(right0, weightCurrent)) >> 16;
            } else {
                right = left;
            }
        } else {
            left = ReadSample(source, sourceOffset, m_bitsPerSample);
            if (m_channels == 2) {
                right = ReadSample(source, sourceOffset + sourceSampleBytes, m_bitsPerSample);
            } else {
                right = left;
            }
        }

        if (channels == 2) {
            right = right >> outputShift;
            if (right > maxSample) {
                right = maxSample;
            } else if (right < minSample) {
                right = minSample;
            }
        } else {
            // mono output: mix both channels
            left = (left + right) / 2;
        }
        left = left >> outputShift;
        if (left > maxSample) {
            left = maxSample;
        } else if (left < minSample) {
            left = minSample;
        }

        if (bitsPerSample == 8) {
            output[outputOffset + outputIndex] = (unsigned char)left;
            outputIndex++;
            if (channels == 2) {
                output[outputOffset + outputIndex] = (unsigned char)right;
                outputIndex++;
            }
        } else {
            short* samples = (short*)(output + outputOffset);
            samples[outputIndex] = (short)left;
            outputIndex++;
            if (channels == 2) {
                samples[outputIndex] = (short)right;
                outputIndex++;
            }
        }

        outputRemaining -= outputFrameBytes;
        fraction += step;
        produced += outputFrameBytes;
    }

    m_position += fraction;
    m_offset = sourceFrameBytes * frameIndex + ((sourceFrameBytes * fraction) >> 16);
    return produced;
}

unsigned int CSoundEventPCM::AddSamplesToBufferInternal(unsigned char* output, unsigned int outputSize,
                                                        unsigned int& outputRemaining, void* source,
                                                        unsigned int sourceSize)
{
    unsigned int channels;
    unsigned int bitsPerSample;
    unsigned int sampleRate;
    unsigned int maxSounds;
    unsigned int bufferLength;
    unsigned char useSoftwareMixer;
    ICMediaPlayer::GetInstance()->GetDescription(channels, bitsPerSample, sampleRate, maxSounds, bufferLength,
                                                 useSoftwareMixer);

    unsigned int mixed = 0;
    switch (bitsPerSample) {
    case 8: {
        unsigned int count = sourceSize;
        if (outputRemaining < count) {
            count = outputRemaining;
        }
        signed char* destination = (signed char*)(output + (outputSize - outputRemaining));
        signed char* input = (signed char*)source;
        outputRemaining -= count;
        if (m_volume == 100) {
            for (unsigned int i = 0; i < count; i++) {
                int maxSample = 0x7fff;
                if (bitsPerSample == 8) {
                    maxSample = 0x7f;
                }
                int sample = destination[i] + input[i];
                if (sample > maxSample) {
                    sample = maxSample;
                } else if (sample < ~maxSample) {
                    sample = ~maxSample;
                }
                destination[i] = (signed char)sample;
            }
        } else if (m_volume > 0) {
            for (unsigned int i = 0; i < count; i++) {
                int maxSample = 0x7fff;
                if (bitsPerSample == 8) {
                    maxSample = 0x7f;
                }
                int sample = destination[i] + (input[i] >> m_gainShift);
                if (sample > maxSample) {
                    sample = maxSample;
                } else if (sample < ~maxSample) {
                    sample = ~maxSample;
                }
                destination[i] = (signed char)sample;
            }
        }
        mixed = count;
        break;
    }
    case 16: {
        unsigned int count = sourceSize;
        if (outputRemaining < count) {
            count = outputRemaining;
        }
        unsigned int sampleCount = count / 2;
        short* destination = (short*)(output + (outputSize - outputRemaining));
        short* input = (short*)source;
        outputRemaining -= sampleCount * 2;
        if (m_volume == 100) {
            for (unsigned int i = 0; i < sampleCount; i++) {
                int maxSample = 0x7fff;
                if (bitsPerSample == 8) {
                    maxSample = 0x7f;
                }
                int sample = destination[i] + input[i];
                if (sample > maxSample) {
                    sample = maxSample;
                } else if (sample < ~maxSample) {
                    sample = ~maxSample;
                }
                destination[i] = (short)sample;
            }
        } else if (m_volume > 0) {
            for (unsigned int i = 0; i < sampleCount; i++) {
                int maxSample = 0x7fff;
                if (bitsPerSample == 8) {
                    maxSample = 0x7f;
                }
                int sample = destination[i] + (input[i] >> m_gainShift);
                if (sample > maxSample) {
                    sample = maxSample;
                } else if (sample < ~maxSample) {
                    sample = ~maxSample;
                }
                destination[i] = (short)sample;
            }
        }
        mixed = sampleCount * 2;
        break;
    }
    default:
        break;
    }
    return mixed;
}

unsigned int CSoundEventPCM::ReadRawPcmSamples(unsigned char* buffer, unsigned int size)
{
    return 0;
}

unsigned int CSoundEventPCM::GetResampledPcmSamplesFromStream(unsigned char* output, unsigned int outputSize,
                                                              unsigned char* stream, unsigned int streamSize,
                                                              unsigned int& streamUsed)
{
    unsigned int total = 0;
    while (total < outputSize && m_state == State_Playing) {
        // top up the stream buffer
        unsigned int bytesRead = ReadRawPcmSamples(stream + streamUsed, streamSize - streamUsed);
        unsigned int remaining = outputSize - total;
        long long positionBefore = m_position;
        unsigned int available = streamUsed + bytesRead;
        unsigned int produced = ResampleBuffer(output, outputSize, remaining, stream, available);

        // bytes of the stream buffer the resampler went through
        unsigned int consumed;
        if (produced != 0) {
            consumed = (unsigned int)((m_position - positionBefore) >> 16) * m_bytesPerFrame;
        } else {
            consumed = 0;
            if (remaining != 0) {
                consumed = (unsigned int)(m_position >> 16) * m_bytesPerFrame;
                if (consumed > available) {
                    consumed = available;
                }
            }
        }
        if (available >= consumed) {
            memmove(stream, stream + consumed, available - consumed);
            streamUsed = available - consumed;
        }
        m_position -= (consumed << 16) / m_bytesPerFrame;
        m_offset = 0;

        total += produced;
        if (produced == 0 && bytesRead == 0) {
            break;
        }
    }
    return total;
}

unsigned char CSoundEventPCM::SetOffsetBytes(int offset)
{
    unsigned int position = offset;
    if (m_binary->m_size < position) {
        position = m_binary->m_size;
    }
    m_offset = position;
    return 1;
}

unsigned char CSoundEventPCM::SetOffsetMS(int offset)
{
    unsigned int bytesPerSecond = (m_bitsPerSample >> 3) * m_channels * m_sampleRate;
    unsigned int position = (unsigned int)((unsigned long long)bytesPerSecond * (unsigned int)offset / 1000);
    if (m_binary->m_size < position) {
        position = m_binary->m_size;
    }
    m_offset = position;
    return 1;
}

unsigned char CSoundEventPCM::GetOffsetBytes(int& offset)
{
    offset = m_offset;
    return 1;
}

unsigned char CSoundEventPCM::GetOffsetMS(int& offset)
{
    unsigned int bytesPerSecond = (m_bitsPerSample >> 3) * m_channels * m_sampleRate;
    offset = (int)((unsigned long long)m_offset * 1000 / bytesPerSecond);
    return 1;
}

unsigned char CSoundEventPCM::SetPitch()
{
    return 0;
}

unsigned char CSoundEventPCM::SetGain()
{
    // 0..7 bits of attenuation for volume 100..0
    m_gainShift = ((100 - m_volume) * 7) / 100;
    return 1;
}

unsigned char CSoundEventPCM::SetPan()
{
    return 0;
}

// ---------------------------------------------------------------------------------------------
// CSoundEventStreamingADPCM
// ---------------------------------------------------------------------------------------------

CSoundEventStreamingADPCM::CSoundEventStreamingADPCM()
{
    m_sourceStream = NULL;
    m_adpcmStream = NULL;
    m_streamBuffer = NULL;
}

CSoundEventStreamingADPCM::~CSoundEventStreamingADPCM()
{
    CSoundEventStreamingADPCM::Stop();
    CSoundEventStreamingADPCM::Destroy();
}

unsigned char CSoundEventStreamingADPCM::Stop()
{
    unsigned char result = CSoundEventPCM::Stop();
    m_adpcmStream->Skip(m_adpcmStream->Available());
    return result;
}

void CSoundEventStreamingADPCM::Destroy()
{
    CSoundEventPCM::Destroy();
    if (m_adpcmStream != NULL) {
        m_adpcmStream->Close();
        if (m_adpcmStream != NULL) {
            delete m_adpcmStream;
            m_adpcmStream = NULL;
        }
    }
    if (m_sourceStream != NULL) {
        m_sourceStream->Close();
        if (m_sourceStream != NULL) {
            delete m_sourceStream;
            m_sourceStream = NULL;
        }
    }
    if (m_streamBuffer != NULL) {
        free(m_streamBuffer);
        m_streamBuffer = NULL;
    }
}

unsigned char CSoundEventStreamingADPCM::Initialize()
{
    unsigned char result = CSoundEventPCM::Initialize();
    if (result != 0) {
        m_sourceStream = new CArrayInputStream();
        m_sourceStream->Open(m_binary->m_data, m_binary->m_size);
        m_adpcmStream = new adpcm::CADPCMInputStream();
        m_adpcmStream->Open(*m_sourceStream, m_binary->m_size, m_channels, m_bitsPerSample, m_sampleRate);
        m_bytesPerFrame = (m_channels * m_bitsPerSample) >> 3;
        m_streamBuffer = NULL;
        m_streamBufferUsed = 0;
    }
    return result;
}

unsigned char CSoundEventStreamingADPCM::Play()
{
    unsigned char result = CSoundEventPCM::Play();
    if (result != 0) {
        // restart decoding when the stream is not at its beginning
        if (m_adpcmStream->m_length != m_adpcmStream->Available()) {
            result = CSoundEventPCM::Initialize();
        }
    }
    return result;
}

void CSoundEventStreamingADPCM::AddSamplesToBuffer(unsigned char* buffer, unsigned int size)
{
    EnsureBufferCapacity(size);
    unsigned int remaining = size;
    unsigned int offset = 0;
    while (remaining != 0 && m_state == State_Playing) {
        unsigned int produced = GetResampledPcmSamplesFromStream(m_buffer, size, m_streamBuffer, size,
                                                                 m_streamBufferUsed);
        unsigned int mixed = AddSamplesToBufferInternal(buffer, size, remaining, m_buffer + offset, produced);
        if (mixed == 0) {
            m_adpcmStream->Skip(m_adpcmStream->Available());
        }
        offset += mixed;
        if (m_adpcmStream->Available() == 0) {
            if (m_loop & 1) {
                Initialize();
                EnsureBufferCapacity(size);
                m_position = 0;
                m_streamBufferUsed = 0;
                offset = 0;
            } else {
                m_state = State_Stopped;
            }
        }
    }
}

unsigned int CSoundEventStreamingADPCM::ReadRawPcmSamples(unsigned char* buffer, unsigned int size)
{
    unsigned int availableBefore = m_adpcmStream->Available();
    if (m_state == State_Playing) {
        m_adpcmStream->Read(buffer, size);
    }
    return availableBefore - m_adpcmStream->Available();
}

unsigned char CSoundEventStreamingADPCM::EnsureBufferCapacity(unsigned int size)
{
    // the stream buffer keeps its content when it grows
    unsigned char* oldStreamBuffer = m_streamBuffer;
    if (m_bufferSize < size || oldStreamBuffer == NULL) {
        unsigned char* streamBuffer = (unsigned char*)malloc(size);
        if (oldStreamBuffer != NULL) {
            memcpy(streamBuffer, m_streamBuffer, m_streamBufferUsed);
            if (m_streamBuffer != NULL) {
                free(m_streamBuffer);
                m_streamBuffer = NULL;
            }
        }
        m_streamBuffer = streamBuffer;
    }
    // the resample buffer is simply reallocated
    if (m_bufferSize < size || m_buffer == NULL) {
        if (m_buffer != NULL) {
            free(m_buffer);
            m_buffer = NULL;
        }
        m_buffer = (unsigned char*)malloc(size);
    }
    m_bufferSize = size;
    return 1;
}

void CSoundEventStreamingADPCM::LimitPropertyValue(ICMediaPlayer::SoundProperty property, int& value)
{
    switch (property) {
    case ICMediaPlayer::SoundProperty_OffsetBytes:
        value = LimitMin(value, 0);
        value = LimitMax(value, m_adpcmStream->m_length);
        break;
    case ICMediaPlayer::SoundProperty_OffsetMS: {
        value = LimitMin(value, 0);
        unsigned int bytesPerSecond = (m_bitsPerSample >> 3) * m_channels * m_sampleRate;
        int duration = (int)((unsigned long long)m_adpcmStream->m_length * 1000 / bytesPerSecond);
        value = LimitMax(value, duration);
        break;
    }
    default:
        CSoundEvent::LimitPropertyValue(property, value);
        break;
    }
}

unsigned char CSoundEventStreamingADPCM::SetOffsetBytes(int offset)
{
    if (m_adpcmStream->m_length != m_adpcmStream->Available()) {
        Initialize();
    }
    unsigned int position = offset;
    if (m_adpcmStream->m_length < position) {
        position = m_adpcmStream->m_length;
    }
    m_adpcmStream->Skip(position);
    return 1;
}

unsigned char CSoundEventStreamingADPCM::SetOffsetMS(int offset)
{
    if (m_adpcmStream->m_length != m_adpcmStream->Available()) {
        Initialize();
    }
    unsigned int bytesPerSecond = (m_bitsPerSample >> 3) * m_channels * m_sampleRate;
    unsigned int position = (unsigned int)((unsigned long long)bytesPerSecond * (unsigned int)offset / 1000);
    if (m_adpcmStream->m_length < position) {
        position = m_adpcmStream->m_length;
    }
    m_adpcmStream->Skip(position);
    return 1;
}

unsigned char CSoundEventStreamingADPCM::GetOffsetBytes(int& offset)
{
    offset = m_adpcmStream->m_length - m_adpcmStream->Available();
    return 1;
}

unsigned char CSoundEventStreamingADPCM::GetOffsetMS(int& offset)
{
    unsigned int position = m_adpcmStream->m_length - m_adpcmStream->Available();
    unsigned int bytesPerSecond = (m_bitsPerSample >> 3) * m_channels * m_sampleRate;
    offset = (int)((unsigned long long)position * 1000 / bytesPerSecond);
    return 1;
}

// ---------------------------------------------------------------------------------------------
// CSoundEventStreamingOGG
// ---------------------------------------------------------------------------------------------

CSoundEventStreamingOGG::CSoundEventStreamingOGG()
{
    m_vorbisFile = NULL;
    m_sourceStream = NULL;
    m_streamBuffer = NULL;
}

CSoundEventStreamingOGG::~CSoundEventStreamingOGG()
{
    CSoundEventStreamingOGG::Stop();
    CSoundEventStreamingOGG::Destroy();
}

unsigned char CSoundEventStreamingOGG::Stop()
{
    if (CSoundEventPCM::Stop() == 0) {
        return 0;
    }
    ov_pcm_seek(m_vorbisFile, m_totalSamples);
    return 1;
}

void CSoundEventStreamingOGG::Destroy()
{
    CSoundEventPCM::Destroy();
    if (m_sourceStream != NULL) {
        m_sourceStream->Close();
        if (m_sourceStream != NULL) {
            delete m_sourceStream;
            m_sourceStream = NULL;
        }
    }
    if (m_vorbisFile != NULL) {
        ov_clear(m_vorbisFile);
        if (m_vorbisFile != NULL) {
            delete m_vorbisFile;
            m_vorbisFile = NULL;
        }
    }
    if (m_streamBuffer != NULL) {
        free(m_streamBuffer);
        m_streamBuffer = NULL;
    }
}

unsigned char CSoundEventStreamingOGG::Initialize()
{
    unsigned char result = CSoundEventPCM::Initialize();
    if (result != 0) {
        m_vorbisFile = new OggVorbis_File;
        m_sourceStream = new CVorbisBufferStream(m_binary->m_data, m_binary->m_size);
        ov_callbacks callbacks;
        CVorbisBufferStream::GetOvCallbacks(callbacks);
        ov_open_callbacks(m_sourceStream, m_vorbisFile, NULL, 0, callbacks);
        ov_pcm_seek(m_vorbisFile, 0);
        vorbis_info* info = ov_info(m_vorbisFile, -1);
        m_channels = info->channels;
        m_bitsPerSample = 16;
        m_sampleRate = info->rate;
        m_streamBuffer = NULL;
        m_streamBufferUsed = 0;
        m_totalSamples = (unsigned int)ov_pcm_total(m_vorbisFile, -1);
        m_bytesPerFrame = (m_channels * m_bitsPerSample) >> 3;
    }
    return result;
}

unsigned char CSoundEventStreamingOGG::Play()
{
    unsigned char result = CSoundEventPCM::Play();
    if (result != 0) {
        ov_pcm_seek(m_vorbisFile, 0);
    }
    return result;
}

void CSoundEventStreamingOGG::AddSamplesToBuffer(unsigned char* buffer, unsigned int size)
{
    if (m_bufferSize < size) {
        // grow the stream buffer (keeping its content) and the resample buffer
        unsigned char* oldStreamBuffer = m_streamBuffer;
        unsigned char* streamBuffer = (unsigned char*)malloc(size);
        if (oldStreamBuffer != NULL) {
            memcpy(streamBuffer, m_streamBuffer, m_streamBufferUsed);
            if (m_streamBuffer != NULL) {
                free(m_streamBuffer);
                m_streamBuffer = NULL;
            }
        }
        m_streamBuffer = streamBuffer;
        if (m_buffer != NULL) {
            free(m_buffer);
            m_buffer = NULL;
        }
        m_buffer = (unsigned char*)malloc(size);
        m_bufferSize = size;
    }

    unsigned int remaining = size;
    unsigned int offset = 0;
    while (remaining != 0 && m_state == State_Playing) {
        unsigned int produced = GetResampledPcmSamplesFromStream(m_buffer, size, m_streamBuffer, size,
                                                                 m_streamBufferUsed);
        unsigned int mixed = AddSamplesToBufferInternal(buffer, size, remaining, m_buffer + offset, produced);
        if (mixed == 0) {
            ov_pcm_seek(m_vorbisFile, m_totalSamples);
        }
        offset += mixed;
        if (ov_pcm_tell(m_vorbisFile) == m_totalSamples) {
            if (m_loop & 1) {
                ov_pcm_seek(m_vorbisFile, 0);
                m_position = 0;
                m_streamBufferUsed = 0;
                offset = 0;
            } else {
                m_state = State_Stopped;
            }
        }
    }
}

unsigned int CSoundEventStreamingOGG::ReadRawPcmSamples(unsigned char* buffer, unsigned int size)
{
    unsigned int totalRead = 0;
    while (size != 0) {
        long bytesRead = ov_read(m_vorbisFile, (char*)buffer, size, 0, m_bitsPerSample >> 3, 1, NULL);
        totalRead += bytesRead;
        if (bytesRead == 0) {
            break;
        }
        size -= bytesRead;
        buffer += bytesRead;
    }
    return totalRead;
}

unsigned char CSoundEventStreamingOGG::SetOffsetBytes(int offset)
{
    unsigned int sample = (unsigned int)offset / m_bytesPerFrame;
    unsigned int target = m_totalSamples;
    if (sample < target) {
        target = sample;
    }
    ov_pcm_seek(m_vorbisFile, target);
    return 1;
}

unsigned char CSoundEventStreamingOGG::SetOffsetMS(int offset)
{
    unsigned int bytesPerSecond = (m_bitsPerSample >> 3) * m_channels * m_sampleRate;
    unsigned int position = (unsigned int)((unsigned long long)bytesPerSecond * (unsigned int)offset / 1000);
    unsigned int sample = position / m_bytesPerFrame;
    unsigned int target = m_totalSamples;
    if (sample < target) {
        target = sample;
    }
    ov_pcm_seek(m_vorbisFile, target);
    return 1;
}

unsigned char CSoundEventStreamingOGG::GetOffsetBytes(int& offset)
{
    offset = (int)(ov_pcm_tell(m_vorbisFile) * m_bytesPerFrame);
    return 1;
}

unsigned char CSoundEventStreamingOGG::GetOffsetMS(int& offset)
{
    unsigned int position = (unsigned int)(ov_pcm_tell(m_vorbisFile) * m_bytesPerFrame);
    unsigned int bytesPerSecond = (m_bitsPerSample >> 3) * m_channels * m_sampleRate;
    offset = (int)((unsigned long long)position * 1000 / bytesPerSecond);
    return 1;
}

void CSoundEventStreamingOGG::LimitPropertyValue(ICMediaPlayer::SoundProperty property, int& value)
{
    switch (property) {
    case ICMediaPlayer::SoundProperty_OffsetBytes:
        value = LimitMin(value, 0);
        value = LimitMax(value, m_totalSamples * m_bytesPerFrame);
        break;
    case ICMediaPlayer::SoundProperty_OffsetMS: {
        value = LimitMin(value, 0);
        unsigned int length = m_totalSamples * m_bytesPerFrame;
        unsigned int bytesPerSecond = (m_bitsPerSample >> 3) * m_channels * m_sampleRate;
        int duration = (int)((unsigned long long)length * 1000 / bytesPerSecond);
        value = LimitMax(value, duration);
        break;
    }
    default:
        CSoundEvent::LimitPropertyValue(property, value);
        break;
    }
}

// ---------------------------------------------------------------------------------------------
// CSoundEvent3d
// ---------------------------------------------------------------------------------------------

CSoundEvent3d::CSoundEvent3d()
{
    m_relativeToListener = 0;
    m_minGain = 0;
    m_maxGain = 0x10000;
    m_maxDistance = 0x7fffffff;
    m_referenceDistance = 0x10000;
    m_rolloffFactor = 0x10000;
    m_coneOuterGain = 0;
    m_coneInnerAngle = 360 << 16;
    m_coneOuterAngle = 360 << 16;
    m_position[0] = 0;
    m_position[1] = 0;
    m_position[2] = 0;
    m_velocity[0] = 0;
    m_velocity[1] = 0;
    m_velocity[2] = 0;
    m_direction[0] = 0;
    m_direction[1] = 0;
    m_direction[2] = 0;
}

CSoundEvent3d::~CSoundEvent3d()
{
}

unsigned char CSoundEvent3d::SetProperty(ICMediaPlayer::SoundProperty property, int value)
{
    unsigned char result = CSoundEvent::SetProperty(property, value);
    if (result == 0) {
        LimitPropertyValue(property, value);
        result = 0;
        if (property == ICMediaPlayer::SoundProperty_RelativeToListener) {
            m_relativeToListener = value;
            result = SetRelativeToListener();
        }
    }
    return result;
}

unsigned char CSoundEvent3d::SetPropertyF(ICMediaPlayer::SoundProperty property, int value)
{
    LimitPropertyValueF(property, value);
    unsigned char result = 0;
    switch (property) {
    case ICMediaPlayer::SoundProperty_MinGain:
        m_minGain = value;
        result = SetMinGain();
        break;
    case ICMediaPlayer::SoundProperty_MaxGain:
        m_maxGain = value;
        result = SetMaxGain();
        break;
    case ICMediaPlayer::SoundProperty_MaxDistance:
        m_maxDistance = value;
        result = SetMaxDistance();
        break;
    case ICMediaPlayer::SoundProperty_ReferenceDistance:
        m_referenceDistance = value;
        result = SetReferenceDistance();
        break;
    case ICMediaPlayer::SoundProperty_RolloffFactor:
        m_rolloffFactor = value;
        result = SetRolloffFactor();
        break;
    case ICMediaPlayer::SoundProperty_ConeOuterGain:
        m_coneOuterGain = value;
        result = SetConeOuterGain();
        break;
    case ICMediaPlayer::SoundProperty_ConeInnerAngle:
        m_coneInnerAngle = value;
        result = SetConeInnerAngle();
        break;
    case ICMediaPlayer::SoundProperty_ConeOuterAngle:
        m_coneOuterAngle = value;
        result = SetConeOuterAngle();
        break;
    default:
        break;
    }
    return result;
}

unsigned char CSoundEvent3d::SetPropertyFV(ICMediaPlayer::SoundProperty property, int* values, int count)
{
    unsigned char result = 0;
    LimitPropertyValueFV(property, values, count);
    switch (property) {
    case ICMediaPlayer::SoundProperty_Position:
        if (count == 3) {
            memcpy(m_position, values, sizeof(m_position));
            result = SetPosition();
        }
        break;
    case ICMediaPlayer::SoundProperty_Velocity:
        if (count == 3) {
            memcpy(m_velocity, values, sizeof(m_velocity));
            result = SetVelocity();
        }
        break;
    case ICMediaPlayer::SoundProperty_Direction:
        if (count == 3) {
            memcpy(m_direction, values, sizeof(m_direction));
            result = SetDirection();
        }
        break;
    default:
        break;
    }
    return result;
}

unsigned char CSoundEvent3d::GetProperty(ICMediaPlayer::SoundProperty property, int& value)
{
    unsigned char result = CSoundEvent::GetProperty(property, value);
    if (result == 0) {
        value = 0;
        if (property == ICMediaPlayer::SoundProperty_RelativeToListener) {
            value = m_relativeToListener;
            result = 1;
        }
    }
    return result;
}

unsigned char CSoundEvent3d::GetPropertyF(ICMediaPlayer::SoundProperty property, int& value)
{
    unsigned char result = 0;
    value = 0;
    switch (property) {
    case ICMediaPlayer::SoundProperty_MinGain:
        value = m_minGain;
        result = 1;
        break;
    case ICMediaPlayer::SoundProperty_MaxGain:
        value = m_maxGain;
        result = 1;
        break;
    case ICMediaPlayer::SoundProperty_MaxDistance:
        value = m_maxDistance;
        result = 1;
        break;
    case ICMediaPlayer::SoundProperty_ReferenceDistance:
        value = m_referenceDistance;
        result = 1;
        break;
    case ICMediaPlayer::SoundProperty_RolloffFactor:
        value = m_rolloffFactor;
        result = 1;
        break;
    case ICMediaPlayer::SoundProperty_ConeOuterGain:
        value = m_coneOuterGain;
        result = 1;
        break;
    case ICMediaPlayer::SoundProperty_ConeInnerAngle:
        value = m_coneInnerAngle;
        result = 1;
        break;
    case ICMediaPlayer::SoundProperty_ConeOuterAngle:
        value = m_coneOuterAngle;
        result = 1;
        break;
    default:
        break;
    }
    return result;
}

unsigned char CSoundEvent3d::GetPropertyFV(ICMediaPlayer::SoundProperty property, int* values, int count)
{
    unsigned char result = 0;
    // NOTE: the original clears values[count] (not values[i]) on every iteration
    for (int i = 0; i < count; i++) {
        values[count] = 0;
    }
    switch (property) {
    case ICMediaPlayer::SoundProperty_Position:
        if (count == 3) {
            memcpy(values, m_position, sizeof(m_position));
            result = 1;
        }
        break;
    case ICMediaPlayer::SoundProperty_Velocity:
        if (count == 3) {
            memcpy(values, m_velocity, sizeof(m_velocity));
            result = 1;
        }
        break;
    case ICMediaPlayer::SoundProperty_Direction:
        if (count == 3) {
            memcpy(values, m_direction, sizeof(m_direction));
            result = 1;
        }
        break;
    default:
        break;
    }
    return result;
}

void CSoundEvent3d::LimitPropertyValue(ICMediaPlayer::SoundProperty property, int& value)
{
    CSoundEvent::LimitPropertyValue(property, value);
    if (property == ICMediaPlayer::SoundProperty_RelativeToListener) {
        value = LimitMax(value, 1);
        value = LimitMin(value, 0);
    }
}

void CSoundEvent3d::LimitPropertyValueF(ICMediaPlayer::SoundProperty property, int& value)
{
    switch (property) {
    case ICMediaPlayer::SoundProperty_MinGain:
    case ICMediaPlayer::SoundProperty_MaxGain:
    case ICMediaPlayer::SoundProperty_ConeOuterGain:
        value = LimitMax(value, 0x10000);
        value = LimitMin(value, 0);
        break;
    case ICMediaPlayer::SoundProperty_MaxDistance:
    case ICMediaPlayer::SoundProperty_ReferenceDistance:
    case ICMediaPlayer::SoundProperty_RolloffFactor:
        value = LimitMin(value, 0);
        break;
    case ICMediaPlayer::SoundProperty_ConeInnerAngle:
    case ICMediaPlayer::SoundProperty_ConeOuterAngle:
        value = LimitMax(value, 360 << 16);
        value = LimitMin(value, 0);
        break;
    default:
        break;
    }
}

void CSoundEvent3d::LimitPropertyValueFV(ICMediaPlayer::SoundProperty property, int* values, int count)
{
}

// ---------------------------------------------------------------------------------------------
// CVibrationEvent
// ---------------------------------------------------------------------------------------------

CVibrationEvent::CVibrationEvent()
{
    m_duration = 0;
    m_elapsed = 0;
}

CVibrationEvent::~CVibrationEvent()
{
}

// ---------------------------------------------------------------------------------------------
// CMediaPlayer
// ---------------------------------------------------------------------------------------------

CMediaPlayer::CMediaPlayer()
{
    m_channels = 0;
    m_bitsPerSample = 0;
    m_sampleRate = 0;
    m_maxSounds = 0;
    m_bufferLength = 0;
    m_useSoftwareMixer = 0;
    m_eventListener = NULL;
    m_soundEnabled = 0;
    m_vibrationEnabled = 0;
    m_volume = 0;
    m_lastEventId = 0;
    m_suspended = 0;
    pthread_mutex_init(&m_soundEventsMutex, NULL);
}

CMediaPlayer::~CMediaPlayer()
{
    CMediaPlayer::Stop(0);
    CMediaPlayer::StopVibrate(0);
    if (m_eventListener != NULL) {
        m_eventListener->UnregisterAll();
        if (m_eventListener != NULL) {
            delete m_eventListener;
            m_eventListener = NULL;
        }
    }
    pthread_mutex_destroy(&m_soundEventsMutex);
}

void CMediaPlayer::Stop(unsigned int eventId)
{
    core::CLinkListNode* node = m_soundEvents.m_head;
    while (node != NULL) {
        CMediaEvent* event = (CMediaEvent*)node;
        node = node->m_next;
        if (eventId == 0 || event->m_id == eventId) {
            event->Stop();
            pthread_mutex_lock(&m_soundEventsMutex);
            m_soundEvents.Remove(event);
            pthread_mutex_unlock(&m_soundEventsMutex);
            delete event;
        }
    }
}

void CMediaPlayer::StopVibrate(unsigned int eventId)
{
    core::CLinkListNode* node = m_vibrationEvents.m_head;
    while (node != NULL) {
        CMediaEvent* event = (CMediaEvent*)node;
        node = node->m_next;
        if (eventId == 0 || event->m_id == eventId) {
            event->Stop();
            m_vibrationEvents.Remove(event);
            delete event;
        }
    }
}

unsigned char CMediaPlayer::Initialize(unsigned int channels, unsigned int bitsPerSample, unsigned int sampleRate,
                                       unsigned int maxSounds, unsigned int bufferLength,
                                       unsigned char useSoftwareMixer)
{
    m_channels = channels;
    m_bitsPerSample = bitsPerSample;
    m_sampleRate = sampleRate;
    m_maxSounds = maxSounds;
    m_bufferLength = bufferLength;
    m_useSoftwareMixer = useSoftwareMixer;
    return 1;
}

void CMediaPlayer::Destroy()
{
    m_channels = 0;
    m_bitsPerSample = 0;
    m_sampleRate = 0;
    m_maxSounds = 0;
    m_bufferLength = 0;
    m_useSoftwareMixer = 0;
}

unsigned char CMediaPlayer::GetDescription(unsigned int& channels, unsigned int& bitsPerSample,
                                           unsigned int& sampleRate, unsigned int& maxSounds,
                                           unsigned int& bufferLength, unsigned char& useSoftwareMixer)
{
    channels = m_channels;
    bitsPerSample = m_bitsPerSample;
    sampleRate = m_sampleRate;
    maxSounds = m_maxSounds;
    bufferLength = m_bufferLength;
    useSoftwareMixer = m_useSoftwareMixer;
    return 1;
}

void CMediaPlayer::SetSoundEnabled(unsigned char enabled)
{
    if (enabled) {
        InitializeSoundEngine();
        m_soundEnabled = 1;
    } else {
        Stop(0);
        DestroySoundEngine();
        m_soundEnabled = 0;
    }
}

void CMediaPlayer::SetVibrationEnabled(unsigned char enabled)
{
    if (enabled) {
        InitializeVibrationEngine();
        m_vibrationEnabled = 1;
    } else {
        StopVibrate(0);
        DestroyVibrationEngine();
        m_vibrationEnabled = 0;
    }
}

void CMediaPlayer::SetVolume(unsigned char volume)
{
    if (volume >= 10) {
        volume = 10;
    }
    m_volume = volume;
    core::CLinkListNode* node = m_soundEvents.m_head;
    while (node != NULL) {
        CSoundEvent* event = (CSoundEvent*)node;
        node = node->m_next;
        event->SetGain();
    }
}

unsigned char CMediaPlayer::GetSoundEnabled() const
{
    return m_soundEnabled;
}

unsigned char CMediaPlayer::GetVibrationEnabled() const
{
    return m_vibrationEnabled;
}

unsigned char CMediaPlayer::GetVolume() const
{
    return m_volume;
}

unsigned char CMediaPlayer::CanSetProperty(GlobalProperty property)
{
    return 0;
}

unsigned char CMediaPlayer::CanGetProperty(GlobalProperty property)
{
    return 0;
}

unsigned char CMediaPlayer::SetProperty(GlobalProperty property, int value)
{
    return 0;
}

unsigned char CMediaPlayer::SetPropertyF(GlobalProperty property, int value)
{
    return 0;
}

unsigned char CMediaPlayer::SetPropertyFV(GlobalProperty property, int* values, int count)
{
    return 0;
}

unsigned char CMediaPlayer::GetProperty(GlobalProperty property, int& value)
{
    return 0;
}

unsigned char CMediaPlayer::GetPropertyF(GlobalProperty property, int& value)
{
    return 0;
}

unsigned char CMediaPlayer::GetPropertyFV(GlobalProperty property, int* values, int count)
{
    return 0;
}

unsigned char CMediaPlayer::IsPlaying(unsigned int eventId) const
{
    core::CLinkListNode* node = m_soundEvents.m_head;
    while (node != NULL) {
        CMediaEvent* event = (CMediaEvent*)node;
        node = node->m_next;
        if (event->m_state != CMediaEvent::State_Stopped && (eventId == 0 || event->m_id == eventId)) {
            return 1;
        }
    }
    return 0;
}

unsigned char CMediaPlayer::IsPaused(unsigned int eventId) const
{
    core::CLinkListNode* node = m_soundEvents.m_head;
    while (node != NULL) {
        CMediaEvent* event = (CMediaEvent*)node;
        node = node->m_next;
        if (event->m_state == CMediaEvent::State_Paused && (eventId == 0 || event->m_id == eventId)) {
            return 1;
        }
    }
    return 0;
}

unsigned char CMediaPlayer::IsVibrating(unsigned int eventId) const
{
    core::CLinkListNode* node = m_vibrationEvents.m_head;
    while (node != NULL) {
        CMediaEvent* event = (CMediaEvent*)node;
        node = node->m_next;
        if (event->m_state != CMediaEvent::State_Stopped && (eventId == 0 || event->m_id == eventId)) {
            return 1;
        }
    }
    return 0;
}

unsigned char CMediaPlayer::IsVibrationPaused(unsigned int eventId) const
{
    core::CLinkListNode* node = m_vibrationEvents.m_head;
    while (node != NULL) {
        CMediaEvent* event = (CMediaEvent*)node;
        node = node->m_next;
        if (event->m_state == CMediaEvent::State_Paused && (eventId == 0 || event->m_id == eventId)) {
            return 1;
        }
    }
    return 0;
}

unsigned int CMediaPlayer::Play(CMedia* media, unsigned char loop, unsigned char priority)
{
    return PlayInternal(media, loop, priority, SoundStreamType_PCM);
}

unsigned int CMediaPlayer::PlayInternal(CBinary* binary, unsigned char loop, unsigned char priority,
                                        SoundStreamType type)
{
    unsigned int result = 0;
    if (m_soundEnabled != 0 && binary != NULL) {
        CSoundEvent* event = CSoundEvent::CreateInstance(type);
        if (event != NULL) {
            m_lastEventId++;
            unsigned int eventId = m_lastEventId;
            event->m_player = this;
            event->m_id = eventId;
            event->m_loop = loop;
            event->m_priority = priority;
            event->SetSoundInfo(binary, 0);
            if (PlayInternal(event)) {
                pthread_mutex_lock(&m_soundEventsMutex);
                m_soundEvents.Insert(event, NULL);
                pthread_mutex_unlock(&m_soundEventsMutex);
                result = event->m_id;
            } else {
                delete event;
                result = 0;
            }
        }
    }
    return result;
}

unsigned int CMediaPlayer::PlayStream(CBinary* binary, SoundStreamType type, unsigned char loop,
                                      unsigned char priority)
{
    unsigned int result = 0;
    // streams are decoded by the software mixer only
    if (m_useSoftwareMixer != 0) {
        result = PlayInternal(binary, loop, priority, type);
    }
    return result;
}

void CMediaPlayer::Pause(unsigned int eventId)
{
    core::CLinkListNode* node = m_soundEvents.m_head;
    while (node != NULL) {
        CMediaEvent* event = (CMediaEvent*)node;
        node = node->m_next;
        if (eventId == 0 || event->m_id == eventId) {
            event->Pause();
        }
    }
}

void CMediaPlayer::Resume(unsigned int eventId)
{
    core::CLinkListNode* node = m_soundEvents.m_head;
    while (node != NULL) {
        CMediaEvent* event = (CMediaEvent*)node;
        node = node->m_next;
        if (eventId == 0 || event->m_id == eventId) {
            event->Resume();
        }
    }
}

unsigned char CMediaPlayer::CanSetProperty(SoundProperty property)
{
    return 0;
}

unsigned char CMediaPlayer::CanGetProperty(SoundProperty property)
{
    return 0;
}

unsigned char CMediaPlayer::SetProperty(unsigned int eventId, SoundProperty property, int value)
{
    unsigned char result = 0;
    core::CLinkListNode* node = m_soundEvents.m_head;
    while (node != NULL) {
        CSoundEvent* event = (CSoundEvent*)node;
        node = node->m_next;
        if (event->m_id == eventId) {
            result = event->SetProperty(property, value);
            break;
        }
    }
    return result;
}

unsigned char CMediaPlayer::SetPropertyF(unsigned int eventId, SoundProperty property, int value)
{
    unsigned char result = 0;
    core::CLinkListNode* node = m_soundEvents.m_head;
    while (node != NULL) {
        CSoundEvent* event = (CSoundEvent*)node;
        node = node->m_next;
        if (event->m_id == eventId) {
            result = event->SetPropertyF(property, value);
            break;
        }
    }
    return result;
}

unsigned char CMediaPlayer::SetPropertyFV(unsigned int eventId, SoundProperty property, int* values, int count)
{
    unsigned char result = 0;
    core::CLinkListNode* node = m_soundEvents.m_head;
    while (node != NULL) {
        CSoundEvent* event = (CSoundEvent*)node;
        node = node->m_next;
        if (event->m_id == eventId) {
            result = event->SetPropertyFV(property, values, count);
            break;
        }
    }
    return result;
}

unsigned char CMediaPlayer::GetProperty(unsigned int eventId, SoundProperty property, int& value)
{
    unsigned char result = 0;
    core::CLinkListNode* node = m_soundEvents.m_head;
    while (node != NULL) {
        CSoundEvent* event = (CSoundEvent*)node;
        node = node->m_next;
        if (event->m_id == eventId) {
            result = event->GetProperty(property, value);
            break;
        }
    }
    return result;
}

unsigned char CMediaPlayer::GetPropertyF(unsigned int eventId, SoundProperty property, int& value)
{
    unsigned char result = 0;
    core::CLinkListNode* node = m_soundEvents.m_head;
    while (node != NULL) {
        CSoundEvent* event = (CSoundEvent*)node;
        node = node->m_next;
        if (event->m_id == eventId) {
            result = event->GetPropertyF(property, value);
            break;
        }
    }
    return result;
}

unsigned char CMediaPlayer::GetPropertyFV(unsigned int eventId, SoundProperty property, int* values, int count)
{
    unsigned char result = 0;
    core::CLinkListNode* node = m_soundEvents.m_head;
    while (node != NULL) {
        CSoundEvent* event = (CSoundEvent*)node;
        node = node->m_next;
        if (event->m_id == eventId) {
            result = event->GetPropertyFV(property, values, count);
            break;
        }
    }
    return result;
}

unsigned int CMediaPlayer::Vibrate(unsigned int duration, unsigned char priority)
{
    unsigned int result = 0;
    if (m_vibrationEnabled != 0 && duration != 0) {
        CVibrationEvent* event = CVibrationEvent::CreateInstance();
        if (event != NULL) {
            m_lastEventId++;
            unsigned int eventId = m_lastEventId;
            event->m_player = this;
            event->m_id = eventId;
            event->m_loop = 0;
            event->m_priority = priority;
            event->m_duration = duration;
            if (VibrateInternal(event)) {
                m_vibrationEvents.Insert(event, NULL);
                result = event->m_id;
            } else {
                delete event;
                result = 0;
            }
        }
    }
    return result;
}

void CMediaPlayer::PauseVibrate(unsigned int eventId)
{
    core::CLinkListNode* node = m_vibrationEvents.m_head;
    while (node != NULL) {
        CMediaEvent* event = (CMediaEvent*)node;
        node = node->m_next;
        if (eventId == 0 || event->m_id == eventId) {
            event->Pause();
        }
    }
}

void CMediaPlayer::ResumeVibrate(unsigned int eventId)
{
    core::CLinkListNode* node = m_vibrationEvents.m_head;
    while (node != NULL) {
        CMediaEvent* event = (CMediaEvent*)node;
        node = node->m_next;
        if (eventId == 0 || event->m_id == eventId) {
            event->Resume();
        }
    }
}

void CMediaPlayer::HandleSuspend()
{
    HandleSuspendInternal();
}

void CMediaPlayer::HandleSuspendInternal()
{
    m_suspended = 1;
    Stop(0);
    StopVibrate(0);
    DestroySoundEngine();
    DestroyVibrationEngine();
}

void CMediaPlayer::HandleResume()
{
    SetSoundEnabled(m_soundEnabled);
    SetVibrationEnabled(m_vibrationEnabled);
    m_suspended = 0;
}

void CMediaPlayer::HandleUpdate(int deltaTime)
{
    HandleUpdateInternal(deltaTime);
}

unsigned char CMediaPlayer::HandleUpdateInternal(int deltaTime)
{
    core::CLinkListNode* node = m_soundEvents.m_head;
    while (node != NULL) {
        CMediaEvent* event = (CMediaEvent*)node;
        node = node->m_next;
        event->HandleUpdate(deltaTime);
        if (event->m_state == CMediaEvent::State_Stopped) {
            pthread_mutex_lock(&m_soundEventsMutex);
            m_soundEvents.Remove(event);
            pthread_mutex_unlock(&m_soundEventsMutex);
            delete event;
        }
    }

    node = m_vibrationEvents.m_head;
    while (node != NULL) {
        CMediaEvent* event = (CMediaEvent*)node;
        node = node->m_next;
        event->HandleUpdate(deltaTime);
        if (event->m_state == CMediaEvent::State_Stopped) {
            m_vibrationEvents.Remove(event);
            delete event;
        }
    }
    return 0;
}

void CMediaPlayer::StopBeforeDelete(CMedia* media)
{
    core::CLinkListNode* node = m_soundEvents.m_head;
    while (node != NULL) {
        CSoundEvent* event = (CSoundEvent*)node;
        CMedia* eventMedia = NULL;
        if (event->m_binary->m_classId == CMedia::ClassId) {
            eventMedia = (CMedia*)event->m_binary;
        }
        node = node->m_next;
        if (eventMedia == media) {
            event->Stop();
            pthread_mutex_lock(&m_soundEventsMutex);
            m_soundEvents.Remove(event);
            pthread_mutex_unlock(&m_soundEventsMutex);
            delete event;
        }
    }
}

unsigned int CMediaPlayer::GetBufferSize()
{
    return m_bitsPerSample * m_sampleRate * m_bufferLength / 1000;
}

void CMediaPlayer::LimitPropertyValue(GlobalProperty property, int& value)
{
}

void CMediaPlayer::LimitPropertyValueF(GlobalProperty property, int& value)
{
}

void CMediaPlayer::LimitPropertyValueFV(GlobalProperty property, int* values, int count)
{
}

// ---------------------------------------------------------------------------------------------
// CMediaPlayer3d
// ---------------------------------------------------------------------------------------------

CMediaPlayer3d::CMediaPlayer3d()
{
    m_dopplerFactor = 0x10000;
    m_speedOfSound = 0x1574ccc;     // 343.3 m/s
    m_distanceModel = 0;
    m_listenerPosition[0] = 0;
    m_listenerPosition[1] = 0;
    m_listenerPosition[2] = 0;
    m_listenerVelocity[0] = 0;
    m_listenerVelocity[1] = 0;
    m_listenerVelocity[2] = 0;
    // looking down -z with +y up
    m_listenerOrientation[0] = 0;
    m_listenerOrientation[1] = 0;
    m_listenerOrientation[2] = -0x10000;
    m_listenerOrientation[3] = 0;
    m_listenerOrientation[4] = 0x10000;
    m_listenerOrientation[5] = 0;
}

CMediaPlayer3d::~CMediaPlayer3d()
{
}

unsigned char CMediaPlayer3d::SetProperty(GlobalProperty property, int value)
{
    LimitPropertyValue(property, value);
    unsigned char result = 0;
    if (property == GlobalProperty_DistanceModel) {
        m_distanceModel = value;
        result = SetDistanceModel();
    }
    return result;
}

unsigned char CMediaPlayer3d::SetPropertyF(GlobalProperty property, int value)
{
    LimitPropertyValueF(property, value);
    unsigned char result = 0;
    switch (property) {
    case GlobalProperty_DopplerFactor:
        m_dopplerFactor = value;
        result = SetDopplerFactor();
        break;
    case GlobalProperty_SpeedOfSound:
        m_speedOfSound = value;
        result = SetSpeedOfSound();
        break;
    default:
        break;
    }
    return result;
}

unsigned char CMediaPlayer3d::SetPropertyFV(GlobalProperty property, int* values, int count)
{
    unsigned char result = 0;
    LimitPropertyValueFV(property, values, count);
    switch (property) {
    case GlobalProperty_ListenerPosition:
        if (count == 3) {
            memcpy(m_listenerPosition, values, sizeof(m_listenerPosition));
            result = SetListenerPosition();
        }
        break;
    case GlobalProperty_ListenerVelocity:
        if (count == 3) {
            memcpy(m_listenerVelocity, values, sizeof(m_listenerVelocity));
            result = SetListenerVelocity();
        }
        break;
    case GlobalProperty_ListenerOrientation:
        if (count == 6) {
            memcpy(m_listenerOrientation, values, sizeof(m_listenerOrientation));
            result = SetListenerOrientation();
        }
        break;
    default:
        break;
    }
    return result;
}

unsigned char CMediaPlayer3d::GetProperty(GlobalProperty property, int& value)
{
    unsigned char result = 0;
    value = 0;
    if (property == GlobalProperty_DistanceModel) {
        value = m_distanceModel;
        result = 1;
    }
    return result;
}

unsigned char CMediaPlayer3d::GetPropertyF(GlobalProperty property, int& value)
{
    unsigned char result = 0;
    value = 0;
    switch (property) {
    case GlobalProperty_DopplerFactor:
        value = m_dopplerFactor;
        result = 1;
        break;
    case GlobalProperty_SpeedOfSound:
        value = m_speedOfSound;
        result = 1;
        break;
    default:
        break;
    }
    return result;
}

unsigned char CMediaPlayer3d::GetPropertyFV(GlobalProperty property, int* values, int count)
{
    unsigned char result = 0;
    // NOTE: the original clears values[count] (not values[i]) on every iteration
    for (int i = 0; i < count; i++) {
        values[count] = 0;
    }
    switch (property) {
    case GlobalProperty_ListenerPosition:
        if (count == 3) {
            memcpy(values, m_listenerPosition, sizeof(m_listenerPosition));
            result = 1;
        }
        break;
    case GlobalProperty_ListenerVelocity:
        if (count == 3) {
            memcpy(values, m_listenerVelocity, sizeof(m_listenerVelocity));
            result = 1;
        }
        break;
    case GlobalProperty_ListenerOrientation:
        if (count == 6) {
            memcpy(values, m_listenerOrientation, sizeof(m_listenerOrientation));
            result = 1;
        }
        break;
    default:
        break;
    }
    return result;
}

void CMediaPlayer3d::LimitPropertyValue(GlobalProperty property, int& value)
{
    if (property == GlobalProperty_DistanceModel) {
        value = LimitMax(value, 6);
        value = LimitMin(value, 0);
    }
}

void CMediaPlayer3d::LimitPropertyValueF(GlobalProperty property, int& value)
{
    switch (property) {
    case GlobalProperty_DopplerFactor:
        value = LimitMax(value, 0x10000);
        value = LimitMin(value, 0);
        break;
    case GlobalProperty_SpeedOfSound:
        value = LimitMin(value, 0);
        break;
    default:
        break;
    }
}

void CMediaPlayer3d::LimitPropertyValueFV(GlobalProperty property, int* values, int count)
{
}

} } } }
