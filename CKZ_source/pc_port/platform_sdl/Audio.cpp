// Play the original game's decoded PCM resources through OpenAL Soft.
#include <stdio.h>
#include <AL/al.h>
#include <AL/alc.h>
#include "CMediaPlayer.h"
#include "CBinary.h"

using namespace com::glu::platform::components;

namespace {

float Fixed(int value) { return static_cast<float>(value) / 65536.0f; }

class SoundEventPC final : public CSoundEvent3d {
public:
    ~SoundEventPC() override { Destroy(); }

    unsigned char Initialize() override
    {
        if (m_binary == nullptr || m_sampleRate == 0) {
            return 0;
        }

        ALenum format = AL_FORMAT_MONO16;
        if (m_channels == 2) {
            format = AL_FORMAT_STEREO16;
        }
        if (m_bitsPerSample == 8) {
            format = AL_FORMAT_MONO8;
            if (m_channels == 2) {
                format = AL_FORMAT_STEREO8;
            }
        }
        alGetError();
        alGenBuffers(1, &m_buffer);
        alGenSources(1, &m_source);
        alBufferData(m_buffer, format, m_binary->m_data, m_binary->m_size, m_sampleRate);
        alSourcei(m_source, AL_BUFFER, static_cast<ALint>(m_buffer));
        alSourcei(m_source, AL_LOOPING, m_loop != 0);
        SetRelativeToListener();
        SetPitch();
        SetGain();
        SetPosition();
        return GetLastErr() == AL_NO_ERROR;
    }

    void Destroy() override
    {
        if (m_source != 0) {
            alDeleteSources(1, &m_source);
            m_source = 0;
        }
        if (m_buffer != 0) {
            alDeleteBuffers(1, &m_buffer);
            m_buffer = 0;
        }
    }

    unsigned char Play() override
    {
        alSourcePlay(m_source);
        m_state = State_Playing;
        return GetLastErr() == AL_NO_ERROR;
    }
    unsigned char Stop() override
    {
        alSourceStop(m_source);
        m_state = State_Stopped;
        return GetLastErr() == AL_NO_ERROR;
    }
    unsigned char Pause() override
    {
        alSourcePause(m_source);
        m_state = State_Paused;
        return GetLastErr() == AL_NO_ERROR;
    }
    unsigned char Resume() override { return Play(); }
    unsigned char HandleUpdate(int) override
    {
        ALint state = 0;
        alGetSourcei(m_source, AL_SOURCE_STATE, &state);
        if (state == AL_STOPPED) {
            m_state = State_Stopped;
        }
        return 1;
    }
    int GetLastErr() override { return alGetError(); }

    unsigned char SetOffsetBytes(int offset) override
    {
        alSourcei(m_source, AL_BYTE_OFFSET, offset);
        return GetLastErr() == AL_NO_ERROR;
    }
    unsigned char SetOffsetMS(int offset) override { return SetFloat(AL_SEC_OFFSET, offset / 1000.0f); }
    unsigned char GetOffsetBytes(int& offset) override
    {
        alGetSourcei(m_source, AL_BYTE_OFFSET, &offset);
        return GetLastErr() == AL_NO_ERROR;
    }
    unsigned char GetOffsetMS(int& offset) override
    {
        ALfloat seconds = 0.0f;
        alGetSourcef(m_source, AL_SEC_OFFSET, &seconds);
        offset = static_cast<int>(seconds * 1000.0f);
        return GetLastErr() == AL_NO_ERROR;
    }
    unsigned char SetPitch() override
    {
        float pitch = 1.0f + m_pitch / 100.0f;
        if (m_pitch < 0) {
            pitch = 1.0f + m_pitch / 200.0f;
        }
        return SetFloat(AL_PITCH, pitch);
    }
    unsigned char SetGain() override { return SetFloat(AL_GAIN, m_volume / 100.0f); }
    unsigned char SetPan() override
    {
        alSource3f(m_source, AL_POSITION, m_pan / 100.0f, 0, 0);
        return GetLastErr() == AL_NO_ERROR;
    }
    unsigned char SetRelativeToListener() override
    {
        alSourcei(m_source, AL_SOURCE_RELATIVE, m_relativeToListener);
        return GetLastErr() == AL_NO_ERROR;
    }
    unsigned char SetMinGain() override { return SetFloat(AL_MIN_GAIN, Fixed(m_minGain)); }
    unsigned char SetMaxGain() override { return SetFloat(AL_MAX_GAIN, Fixed(m_maxGain)); }
    unsigned char SetMaxDistance() override { return SetFloat(AL_MAX_DISTANCE, Fixed(m_maxDistance)); }
    unsigned char SetReferenceDistance() override { return SetFloat(AL_REFERENCE_DISTANCE, Fixed(m_referenceDistance)); }
    unsigned char SetRolloffFactor() override { return SetFloat(AL_ROLLOFF_FACTOR, Fixed(m_rolloffFactor)); }
    unsigned char SetConeOuterGain() override { return SetFloat(AL_CONE_OUTER_GAIN, Fixed(m_coneOuterGain)); }
    unsigned char SetConeInnerAngle() override { return SetFloat(AL_CONE_INNER_ANGLE, Fixed(m_coneInnerAngle)); }
    unsigned char SetConeOuterAngle() override { return SetFloat(AL_CONE_OUTER_ANGLE, Fixed(m_coneOuterAngle)); }
    unsigned char SetPosition() override { return SetVector(AL_POSITION, m_position); }
    unsigned char SetVelocity() override { return SetVector(AL_VELOCITY, m_velocity); }
    unsigned char SetDirection() override { return SetVector(AL_DIRECTION, m_direction); }

private:
    unsigned char SetFloat(ALenum property, float value)
    {
        alSourcef(m_source, property, value);
        return GetLastErr() == AL_NO_ERROR;
    }
    unsigned char SetVector(ALenum property, const int* value)
    {
        alSource3f(m_source, property, Fixed(value[0]), Fixed(value[1]), Fixed(value[2]));
        return GetLastErr() == AL_NO_ERROR;
    }

    ALuint m_buffer = 0;
    ALuint m_source = 0;
};

class MediaPlayerPC final : public CMediaPlayer3d {
public:
    ~MediaPlayerPC() override { Destroy(); }
    void Destroy() override
    {
        Stop(0);
        DestroySoundEngine();
        CMediaPlayer::Destroy();
    }

    void InitializeSoundEngine() override
    {
        if (m_context != nullptr) {
            return;
        }
        m_device = alcOpenDevice(nullptr);
        if (m_device != nullptr) {
            m_context = alcCreateContext(m_device, nullptr);
        }
        if (m_context == nullptr || !alcMakeContextCurrent(m_context)) {
            fprintf(stderr, "OpenAL output unavailable\n");
            DestroySoundEngine();
            return;
        }
        alListenerf(AL_GAIN, m_volume / 10.0f);
        alDistanceModel(AL_NONE);
        fprintf(stderr, "OpenAL output: %s\n", alcGetString(m_device, ALC_DEVICE_SPECIFIER));
    }
    void DestroySoundEngine() override
    {
        if (m_context != nullptr) {
            alcMakeContextCurrent(nullptr);
            alcDestroyContext(m_context);
            m_context = nullptr;
        }
        if (m_device != nullptr) {
            alcCloseDevice(m_device);
            m_device = nullptr;
        }
    }
    void SetVolume(unsigned char volume) override
    {
        CMediaPlayer::SetVolume(volume);
        if (m_context != nullptr) {
            alListenerf(AL_GAIN, m_volume / 10.0f);
        }
    }
    unsigned char CanSetProperty(GlobalProperty) override { return 1; }
    unsigned char CanGetProperty(GlobalProperty) override { return 1; }
    unsigned char CanSetProperty(SoundProperty) override { return 1; }
    unsigned char CanGetProperty(SoundProperty) override { return 1; }
    unsigned char PlayInternal(CSoundEvent* event) override
    {
        return m_context != nullptr && event->Initialize() && event->Play();
    }
    unsigned char InitializeVibrationEngine() override { return 0; }
    void DestroyVibrationEngine() override {}
    unsigned char VibrateInternal(CVibrationEvent*) override { return 0; }
    unsigned char SetDopplerFactor() override
    {
        alDopplerFactor(Fixed(m_dopplerFactor));
        return alGetError() == AL_NO_ERROR;
    }
    unsigned char SetSpeedOfSound() override
    {
        alSpeedOfSound(Fixed(m_speedOfSound));
        return alGetError() == AL_NO_ERROR;
    }
    unsigned char SetDistanceModel() override
    {
        const ALenum modes[] = { AL_NONE, AL_INVERSE_DISTANCE, AL_INVERSE_DISTANCE_CLAMPED,
            AL_LINEAR_DISTANCE, AL_LINEAR_DISTANCE_CLAMPED, AL_EXPONENT_DISTANCE, AL_EXPONENT_DISTANCE_CLAMPED };
        alDistanceModel(modes[m_distanceModel]);
        return alGetError() == AL_NO_ERROR;
    }
    unsigned char SetListenerPosition() override { return SetListenerVector(AL_POSITION, m_listenerPosition); }
    unsigned char SetListenerVelocity() override { return SetListenerVector(AL_VELOCITY, m_listenerVelocity); }
    unsigned char SetListenerOrientation() override
    {
        ALfloat values[6];
        for (int index = 0; index < 6; index++) {
            values[index] = Fixed(m_listenerOrientation[index]);
        }
        alListenerfv(AL_ORIENTATION, values);
        return alGetError() == AL_NO_ERROR;
    }

private:
    unsigned char SetListenerVector(ALenum property, const int* values)
    {
        alListener3f(property, Fixed(values[0]), Fixed(values[1]), Fixed(values[2]));
        return alGetError() == AL_NO_ERROR;
    }

    ALCdevice* m_device = nullptr;
    ALCcontext* m_context = nullptr;
};

} // namespace

ICMediaPlayer* ICMediaPlayer::CreateInstance() { return new MediaPlayerPC; }

CSoundEvent* CSoundEvent::CreateInstance(ICMediaPlayer::SoundStreamType type)
{
    if (type == ICMediaPlayer::SoundStreamType_PCM) {
        return new SoundEventPC;
    }
    return nullptr;
}

CVibrationEvent* CVibrationEvent::CreateInstance() { return nullptr; }
