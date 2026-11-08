// CMediaPlayer.h - platform independent part of the media player: sound / vibration events and
// the software mixer used when the platform plays everything through one PCM stream.
// The platform layer (CMediaPlayer_Cocoa, CSoundEvent_Cocoa, CVibrationEvent_Cocoa) derives from these.
#ifndef CMEDIAPLAYER_H
#define CMEDIAPLAYER_H

#include <pthread.h>
#include "ICMediaPlayer.h"
#include "CLinkList.h"
#include "vorbisfile.h"

namespace com { namespace glu { namespace platform {

namespace systems {
class CEventListener;
}

namespace adpcm {
class CADPCMInputStream;
}

namespace components {

class CArrayInputStream;
class CBinary;
class CMedia;
class CMediaPlayer;
class CVorbisBufferStream;

/** Base of everything the media player schedules (a sound or a vibration), kept in a CLinkList. */
class CMediaEvent : public core::CLinkListNode {
public:
    /** m_state values. */
    enum State {
        State_Idle = 0,
        State_Playing = 1,
        State_Paused = 2,
        State_Stopped = 4
    };

    CMediaEvent()
    {
        m_player = NULL;
        m_id = 0;
        m_loop = 0;
        m_priority = 0;
        m_state = State_Idle;
    }

    virtual ~CMediaEvent() {}

    virtual unsigned char Initialize() = 0;
    virtual void Destroy() = 0;
    virtual unsigned char Play() = 0;
    virtual unsigned char Stop() = 0;
    virtual unsigned char Pause() = 0;
    virtual unsigned char Resume() = 0;
    virtual unsigned char HandleUpdate(int deltaTime) = 0;
    virtual int GetLastErr() = 0;

    unsigned char GetPriority() const;

    CMediaPlayer* m_player;         // 0x14 owner
    unsigned int m_id;              // 0x18 event id returned by Play()/Vibrate()
    unsigned char m_loop;           // 0x1c bit 0: restart at the end
    unsigned char m_priority;       // 0x1d
    int m_state;                    // 0x20 State
};

/** A playing sound: source data plus the per-sound properties (volume, pitch, pan, offset). */
class CSoundEvent : public CMediaEvent {
public:
    CSoundEvent()
    {
        m_binary = NULL;
        m_ownsBinary = 0;
        m_volume = 100;
        m_pitch = 0;
        m_pan = 0;
        m_channels = 0;
        m_bitsPerSample = 0;
        m_sampleRate = 0;
    }

    virtual ~CSoundEvent() {}

    /** Platform factory: a software mixed event for `type`, or a hardware (3d) event. */
    static CSoundEvent* CreateInstance(ICMediaPlayer::SoundStreamType type);

    /** Attach the sound data; the PCM format is taken from it when it is a CMedia. */
    void SetSoundInfo(CBinary* binary, unsigned char ownsBinary);

    virtual unsigned char SetProperty(ICMediaPlayer::SoundProperty property, int value);
    virtual unsigned char SetPropertyF(ICMediaPlayer::SoundProperty property, int value);
    virtual unsigned char SetPropertyFV(ICMediaPlayer::SoundProperty property, int* values, int count);
    virtual unsigned char GetProperty(ICMediaPlayer::SoundProperty property, int& value);
    virtual unsigned char GetPropertyF(ICMediaPlayer::SoundProperty property, int& value);
    virtual unsigned char GetPropertyFV(ICMediaPlayer::SoundProperty property, int* values, int count);
    /** Clamp `value` into the valid range of `property`. */
    virtual void LimitPropertyValue(ICMediaPlayer::SoundProperty property, int& value);
    virtual void LimitPropertyValueF(ICMediaPlayer::SoundProperty property, int& value);
    virtual void LimitPropertyValueFV(ICMediaPlayer::SoundProperty property, int* values, int count);

    virtual unsigned char SetOffsetBytes(int offset) = 0;
    virtual unsigned char SetOffsetMS(int offset) = 0;
    virtual unsigned char GetOffsetBytes(int& offset) = 0;
    virtual unsigned char GetOffsetMS(int& offset) = 0;
    /** Apply m_pitch / m_volume / m_pan to the playing sound. */
    virtual unsigned char SetPitch() = 0;
    virtual unsigned char SetGain() = 0;
    virtual unsigned char SetPan() = 0;

    CBinary* m_binary;              // 0x24 sound data (usually a CMedia)
    unsigned int m_channels;        // 0x28
    unsigned int m_sampleRate;      // 0x2c
    unsigned int m_bitsPerSample;   // 0x30
    unsigned char m_ownsBinary;     // 0x34 second argument of SetSoundInfo (always 0 here; meaning guessed)
    int m_volume;                   // 0x38 0..100
    int m_pitch;                    // 0x3c -100..100
    int m_pan;                      // 0x40 -100..100
};

/** Software mixed sound playing raw PCM straight from its CBinary. */
class CSoundEventPCM : public CSoundEvent {
public:
    CSoundEventPCM();
    virtual ~CSoundEventPCM();

    virtual unsigned char Initialize();
    virtual void Destroy();
    virtual unsigned char Play();
    virtual unsigned char Stop();
    virtual unsigned char Pause();
    virtual unsigned char Resume();
    virtual unsigned char HandleUpdate(int deltaTime);
    virtual int GetLastErr();

    virtual unsigned char SetOffsetBytes(int offset);
    virtual unsigned char SetOffsetMS(int offset);
    virtual unsigned char GetOffsetBytes(int& offset);
    virtual unsigned char GetOffsetMS(int& offset);
    virtual unsigned char SetPitch();
    virtual unsigned char SetGain();
    virtual unsigned char SetPan();

    /** Mix `size` bytes of this sound into `buffer` (in the player's output format). */
    virtual void AddSamplesToBuffer(unsigned char* buffer, unsigned int size);
    /** Streaming events: read up to `size` bytes of source PCM; returns the bytes read. */
    virtual unsigned int ReadRawPcmSamples(unsigned char* buffer, unsigned int size);

    /**
     * Convert source PCM (`source`, `sourceSize` bytes, starting at m_offset / m_position) to the
     * player's format into `output + (outputSize - outputRemaining)`; returns the bytes produced.
     */
    unsigned int ResampleBuffer(unsigned char* output, unsigned int outputSize, unsigned int& outputRemaining,
                                const unsigned char* source, unsigned int sourceSize);
    /** Saturating add of `source` into `output + (outputSize - outputRemaining)`; returns the bytes mixed. */
    unsigned int AddSamplesToBufferInternal(unsigned char* output, unsigned int outputSize,
                                            unsigned int& outputRemaining, void* source, unsigned int sourceSize);
    /** Streaming events: refill `stream` from ReadRawPcmSamples() and resample it into `output`. */
    unsigned int GetResampledPcmSamplesFromStream(unsigned char* output, unsigned int outputSize,
                                                  unsigned char* stream, unsigned int streamSize,
                                                  unsigned int& streamUsed);

    unsigned int m_offset;          // 0x44 read position in the source data, bytes
    int m_lastError;                // 0x48 1 = no sound data
    signed char m_gainShift;        // 0x4c attenuation: source samples are shifted right by this
    unsigned int m_bufferSize;      // 0x50 size of m_buffer
    unsigned char* m_buffer;        // 0x54 resampled PCM before mixing
    long long m_position;           // 0x58 16.16 source frame position of the resampler
    unsigned int m_bytesPerFrame;   // 0x60 source bytes per frame (streaming events)
};

/** Software mixed sound decoding IMA ADPCM on the fly. */
class CSoundEventStreamingADPCM : public CSoundEventPCM {
public:
    CSoundEventStreamingADPCM();
    virtual ~CSoundEventStreamingADPCM();

    virtual unsigned char Initialize();
    virtual void Destroy();
    virtual unsigned char Play();
    virtual unsigned char Stop();

    virtual void LimitPropertyValue(ICMediaPlayer::SoundProperty property, int& value);

    virtual unsigned char SetOffsetBytes(int offset);
    virtual unsigned char SetOffsetMS(int offset);
    virtual unsigned char GetOffsetBytes(int& offset);
    virtual unsigned char GetOffsetMS(int& offset);

    virtual void AddSamplesToBuffer(unsigned char* buffer, unsigned int size);
    virtual unsigned int ReadRawPcmSamples(unsigned char* buffer, unsigned int size);
    /** Grow m_streamBuffer (keeping its content) and m_buffer to `size` bytes. */
    virtual unsigned char EnsureBufferCapacity(unsigned int size);

    CArrayInputStream* m_sourceStream;          // 0x64 reads m_binary
    adpcm::CADPCMInputStream* m_adpcmStream;    // 0x68 decodes m_sourceStream
    unsigned char* m_streamBuffer;              // 0x6c decoded PCM waiting to be resampled
    unsigned int m_streamBufferUsed;            // 0x70
};

/** Software mixed sound decoding Ogg Vorbis on the fly. */
class CSoundEventStreamingOGG : public CSoundEventPCM {
public:
    CSoundEventStreamingOGG();
    virtual ~CSoundEventStreamingOGG();

    virtual unsigned char Initialize();
    virtual void Destroy();
    virtual unsigned char Play();
    virtual unsigned char Stop();

    virtual void LimitPropertyValue(ICMediaPlayer::SoundProperty property, int& value);

    virtual unsigned char SetOffsetBytes(int offset);
    virtual unsigned char SetOffsetMS(int offset);
    virtual unsigned char GetOffsetBytes(int& offset);
    virtual unsigned char GetOffsetMS(int& offset);

    virtual void AddSamplesToBuffer(unsigned char* buffer, unsigned int size);
    virtual unsigned int ReadRawPcmSamples(unsigned char* buffer, unsigned int size);

    OggVorbis_File* m_vorbisFile;               // 0x64
    CVorbisBufferStream* m_sourceStream;        // 0x68 reads m_binary for m_vorbisFile
    unsigned char* m_streamBuffer;              // 0x6c decoded PCM waiting to be resampled
    unsigned int m_streamBufferUsed;            // 0x70
    unsigned int m_totalSamples;                // 0x74 ov_pcm_total
};

/** Positional sound (OpenAL style); the platform implements the Set* hooks. */
class CSoundEvent3d : public CSoundEvent {
public:
    CSoundEvent3d();
    virtual ~CSoundEvent3d();

    virtual unsigned char SetProperty(ICMediaPlayer::SoundProperty property, int value);
    virtual unsigned char SetPropertyF(ICMediaPlayer::SoundProperty property, int value);
    virtual unsigned char SetPropertyFV(ICMediaPlayer::SoundProperty property, int* values, int count);
    virtual unsigned char GetProperty(ICMediaPlayer::SoundProperty property, int& value);
    virtual unsigned char GetPropertyF(ICMediaPlayer::SoundProperty property, int& value);
    virtual unsigned char GetPropertyFV(ICMediaPlayer::SoundProperty property, int* values, int count);
    virtual void LimitPropertyValue(ICMediaPlayer::SoundProperty property, int& value);
    virtual void LimitPropertyValueF(ICMediaPlayer::SoundProperty property, int& value);
    virtual void LimitPropertyValueFV(ICMediaPlayer::SoundProperty property, int* values, int count);

    virtual unsigned char SetRelativeToListener() = 0;
    virtual unsigned char SetMinGain() = 0;
    virtual unsigned char SetMaxGain() = 0;
    virtual unsigned char SetMaxDistance() = 0;
    virtual unsigned char SetReferenceDistance() = 0;
    virtual unsigned char SetRolloffFactor() = 0;
    virtual unsigned char SetConeOuterGain() = 0;
    virtual unsigned char SetConeInnerAngle() = 0;
    virtual unsigned char SetConeOuterAngle() = 0;
    virtual unsigned char SetPosition() = 0;
    virtual unsigned char SetVelocity() = 0;
    virtual unsigned char SetDirection() = 0;

    int m_relativeToListener;       // 0x44 0 / 1
    int m_minGain;                  // 0x48 16.16
    int m_maxGain;                  // 0x4c 16.16
    int m_maxDistance;              // 0x50 16.16
    int m_referenceDistance;        // 0x54 16.16
    int m_rolloffFactor;            // 0x58 16.16
    int m_coneOuterGain;            // 0x5c 16.16
    int m_coneInnerAngle;           // 0x60 16.16 degrees
    int m_coneOuterAngle;           // 0x64 16.16 degrees
    int m_position[3];              // 0x68 16.16
    int m_velocity[3];              // 0x74 16.16
    int m_direction[3];             // 0x80 16.16
};

/** A vibration; the platform implements it. */
class CVibrationEvent : public CMediaEvent {
public:
    CVibrationEvent();
    virtual ~CVibrationEvent();

    /** Platform factory. */
    static CVibrationEvent* CreateInstance();

    unsigned int m_duration;        // 0x24 argument of Vibrate()
    int m_elapsed;                  // 0x28
};

/** Media player keeping the lists of sound / vibration events; the platform supplies the engines. */
class CMediaPlayer : public ICMediaPlayer {
public:
    CMediaPlayer();
    virtual ~CMediaPlayer();

    virtual unsigned char Initialize(unsigned int channels, unsigned int bitsPerSample, unsigned int sampleRate,
                                     unsigned int maxSounds, unsigned int bufferLength,
                                     unsigned char useSoftwareMixer);
    virtual void Destroy();
    virtual unsigned char GetDescription(unsigned int& channels, unsigned int& bitsPerSample, unsigned int& sampleRate,
                                         unsigned int& maxSounds, unsigned int& bufferLength,
                                         unsigned char& useSoftwareMixer);
    virtual void SetSoundEnabled(unsigned char enabled);
    virtual void SetVibrationEnabled(unsigned char enabled);
    virtual void SetVolume(unsigned char volume);
    virtual unsigned char GetSoundEnabled() const;
    virtual unsigned char GetVibrationEnabled() const;
    virtual unsigned char GetVolume() const;

    virtual unsigned char CanSetProperty(GlobalProperty property);
    virtual unsigned char CanGetProperty(GlobalProperty property);
    virtual unsigned char SetProperty(GlobalProperty property, int value);
    virtual unsigned char SetPropertyF(GlobalProperty property, int value);
    virtual unsigned char SetPropertyFV(GlobalProperty property, int* values, int count);
    virtual unsigned char GetProperty(GlobalProperty property, int& value);
    virtual unsigned char GetPropertyF(GlobalProperty property, int& value);
    virtual unsigned char GetPropertyFV(GlobalProperty property, int* values, int count);

    virtual unsigned char IsPlaying(unsigned int eventId) const;
    virtual unsigned char IsPaused(unsigned int eventId) const;
    virtual unsigned char IsVibrating(unsigned int eventId) const;
    virtual unsigned char IsVibrationPaused(unsigned int eventId) const;

    virtual unsigned int Play(CMedia* media, unsigned char loop, unsigned char priority);
    virtual unsigned int PlayStream(CBinary* binary, SoundStreamType type, unsigned char loop,
                                    unsigned char priority);
    virtual void Stop(unsigned int eventId);
    virtual void Pause(unsigned int eventId);
    virtual void Resume(unsigned int eventId);

    virtual unsigned char CanSetProperty(SoundProperty property);
    virtual unsigned char CanGetProperty(SoundProperty property);
    virtual unsigned char SetProperty(unsigned int eventId, SoundProperty property, int value);
    virtual unsigned char SetPropertyF(unsigned int eventId, SoundProperty property, int value);
    virtual unsigned char SetPropertyFV(unsigned int eventId, SoundProperty property, int* values, int count);
    virtual unsigned char GetProperty(unsigned int eventId, SoundProperty property, int& value);
    virtual unsigned char GetPropertyF(unsigned int eventId, SoundProperty property, int& value);
    virtual unsigned char GetPropertyFV(unsigned int eventId, SoundProperty property, int* values, int count);

    virtual unsigned int Vibrate(unsigned int duration, unsigned char priority);
    virtual void StopVibrate(unsigned int eventId);
    virtual void PauseVibrate(unsigned int eventId);
    virtual void ResumeVibrate(unsigned int eventId);

    virtual void HandleSuspend();
    virtual void HandleResume();
    virtual void HandleUpdate(int deltaTime);
    virtual unsigned int GetBufferSize();

    virtual void LimitPropertyValue(GlobalProperty property, int& value);
    virtual void LimitPropertyValueF(GlobalProperty property, int& value);
    virtual void LimitPropertyValueFV(GlobalProperty property, int* values, int count);

    // platform engines
    virtual void InitializeSoundEngine() = 0;
    virtual void DestroySoundEngine() = 0;
    virtual unsigned char InitializeVibrationEngine() = 0;  // return type corrected by b003 (Cocoa returns 1)
    virtual void DestroyVibrationEngine() = 0;
    /** Start `event` on the platform; 0 when it cannot be played. */
    virtual unsigned char PlayInternal(CSoundEvent* event) = 0;
    virtual unsigned char VibrateInternal(CVibrationEvent* event) = 0;

    /** Create, start and register a sound event for `binary`; returns its id, 0 on failure. */
    unsigned int PlayInternal(CBinary* binary, unsigned char loop, unsigned char priority, SoundStreamType type);
    void HandleSuspendInternal();
    unsigned char HandleUpdateInternal(int deltaTime);
    /** Stop and delete every sound event playing `media` (called before the media goes away). */
    void StopBeforeDelete(CMedia* media);

    unsigned int m_channels;                    // 0x08 output format
    unsigned int m_bitsPerSample;               // 0x0c
    unsigned int m_sampleRate;                  // 0x10
    unsigned int m_maxSounds;                   // 0x14 Initialize() argument 4 (meaning guessed)
    unsigned int m_bufferLength;                // 0x18 mixing buffer length, ms (see GetBufferSize)
    unsigned char m_useSoftwareMixer;           // 0x1c sounds are mixed by CSoundEventPCM & co
    systems::CEventListener* m_eventListener;   // 0x20
    unsigned char m_soundEnabled;               // 0x24
    unsigned char m_vibrationEnabled;           // 0x25
    unsigned char m_volume;                     // 0x26 0..10
    unsigned int m_lastEventId;                 // 0x28
    core::CLinkList m_soundEvents;              // 0x2c CSoundEvent nodes
    core::CLinkList m_vibrationEvents;          // 0x3c CVibrationEvent nodes
    unsigned char m_suspended;                  // 0x4c
    pthread_mutex_t m_soundEventsMutex;         // 0x50 guards m_soundEvents against the mixer thread
};

/** Media player with an OpenAL style listener; the platform implements the Set* hooks. */
class CMediaPlayer3d : public CMediaPlayer {
public:
    CMediaPlayer3d();
    virtual ~CMediaPlayer3d();

    virtual unsigned char SetProperty(GlobalProperty property, int value);
    virtual unsigned char SetPropertyF(GlobalProperty property, int value);
    virtual unsigned char SetPropertyFV(GlobalProperty property, int* values, int count);
    virtual unsigned char GetProperty(GlobalProperty property, int& value);
    virtual unsigned char GetPropertyF(GlobalProperty property, int& value);
    virtual unsigned char GetPropertyFV(GlobalProperty property, int* values, int count);

    virtual void LimitPropertyValue(GlobalProperty property, int& value);
    virtual void LimitPropertyValueF(GlobalProperty property, int& value);
    virtual void LimitPropertyValueFV(GlobalProperty property, int* values, int count);

    virtual unsigned char SetDopplerFactor() = 0;
    virtual unsigned char SetSpeedOfSound() = 0;
    virtual unsigned char SetDistanceModel() = 0;
    virtual unsigned char SetListenerPosition() = 0;
    virtual unsigned char SetListenerVelocity() = 0;
    virtual unsigned char SetListenerOrientation() = 0;

    int m_dopplerFactor;                // 0x7c 16.16
    int m_speedOfSound;                 // 0x80 16.16
    int m_distanceModel;                // 0x84 0..6
    int m_listenerPosition[3];          // 0x88 16.16
    int m_listenerVelocity[3];          // 0x94 16.16
    int m_listenerOrientation[6];       // 0xa0 16.16 "at" then "up"
};

} } } }

#endif
