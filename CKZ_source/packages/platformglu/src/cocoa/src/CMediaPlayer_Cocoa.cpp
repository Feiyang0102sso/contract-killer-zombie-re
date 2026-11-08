// CMediaPlayer_Cocoa.cpp - iOS media player.
//  - Sounds: one OpenAL source + buffer per sound (CSoundEvent_Cocoa, pairs taken from a pool), or,
//    with the software mixer, one OpenAL stream refilled by a thread from the CSoundEventPCM mixers.
//  - Vibration: the system vibrate sound (CVibrationEvent_Cocoa).
//  - Music: an AudioQueue playing a compressed file (BackgroundTrackMgr, after Apple's SoundEngine sample).
// The music code (from Apple's sample) uses the C library allocator directly: the original object
// calls malloc / free, not the np_* wrappers the prefix header maps them to.
#undef malloc
#undef free
#include <stdlib.h>

#include "CMediaPlayer_Cocoa.h"

#include <unistd.h>
#include <wchar.h>
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#include <AudioToolbox/AudioToolbox.h>

#include "CApplet.h"
#include "CApp.h"
#include "CHash.h"
#include "CBinary.h"
#include "ICCore.h"

using namespace com::glu::platform::components;
using namespace com::glu::platform::core;

/** Share OpenAL buffer / source pairs between sounds (disabled with the software mixer). */
static unsigned char g_enablePoolObjects = 1;

/** OpenAL buffer / source pairs created up front for the pool. */
static const int kPoolObjectCount = 32;
/** 16.16 fixed point to float. */
static const float kFixedToFloat = 1.0f / 65536.0f;

/** One pooled OpenAL buffer / source pair. */
class CALPoolObject : public CLinkListNode {
public:
    CALPoolObject();
    virtual ~CALPoolObject();

    ALuint m_buffer;        // 0x14
    ALuint m_source;        // 0x18
};

class BackgroundTrackMgr;
/** The music track playing, NULL when none. */
static BackgroundTrackMgr* s_backgroundTrack = NULL;

/** OpenAL sample format for a PCM layout. */
static inline ALenum GetALFormat(unsigned char channels, unsigned char bitsPerSample)
{
    if (channels == 1) {
        if (bitsPerSample == 8) {
            return AL_FORMAT_MONO8;
        }
        return AL_FORMAT_MONO16;
    }
    if (bitsPerSample == 8) {
        return AL_FORMAT_STEREO8;
    }
    return AL_FORMAT_STEREO16;
}

/** Audio session interruptions (phone call, alarm): drop the OpenAL context while interrupted. */
void InterruptionListenerCB(void* clientData, UInt32 interruptionState)
{
    CMediaPlayer_Cocoa* player = (CMediaPlayer_Cocoa*)clientData;
    if (interruptionState == kAudioSessionBeginInterruption) {
        player->DestroySoundEngine();
    } else if (interruptionState == kAudioSessionEndInterruption) {
        if (player->GetSoundEnabled()) {
            player->InitializeSoundEngine();
        }
    }
}

void CMediaPlayer_Cocoa::PublicDestroySoundEngine()
{
    DestroySoundEngine();
}

void CMediaPlayer_Cocoa::PublicInitializeSoundEngine()
{
    InitializeSoundEngine();
}

void SetAudioSessionCategory(CMediaPlayer_Cocoa::eAudioMode mode)
{
    UInt32 category = 0;
    switch (mode) {
    case CMediaPlayer_Cocoa::AudioMode_Solo:
        category = kAudioSessionCategory_SoloAmbientSound;
        break;
    case CMediaPlayer_Cocoa::AudioMode_Ambient:
        category = kAudioSessionCategory_AmbientSound;
        break;
    }
    AudioSessionSetProperty(kAudioSessionProperty_AudioCategory, sizeof(category), &category);
}

// ---------------------------------------------------------------------------------------------
// Audio file helpers (Apple SoundEngine sample)
// ---------------------------------------------------------------------------------------------

OSStatus OpenFile(const char* filePath, AudioFileID& fileID)
{
    CFURLRef fileURL = CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault, (const UInt8*)filePath,
                                                               strlen(filePath), false);
    OSStatus result = AudioFileOpenURL(fileURL, kAudioFileReadPermission, 0, &fileID);
    CFRelease(fileURL);
    return result;
}

OSStatus LoadFileDataInfo(const char* filePath, AudioFileID& fileID, AudioStreamBasicDescription& fileFormat,
                          UInt64& dataSize)
{
    UInt32 propertySize = sizeof(fileFormat);
    OSStatus result = OpenFile(filePath, fileID);
    if (result == noErr) {
        result = AudioFileGetProperty(fileID, kAudioFilePropertyDataFormat, &propertySize, &fileFormat);
        if (result == noErr) {
            propertySize = sizeof(dataSize);
            result = AudioFileGetProperty(fileID, kAudioFilePropertyAudioDataByteCount, &propertySize, &dataSize);
        }
    }
    return result;
}

void CalculateBytesForTime(AudioStreamBasicDescription& format, UInt32 maxPacketSize, Float64 seconds,
                           UInt32* bufferSize, UInt32* numPackets)
{
    static const UInt32 maxBufferSize = 0x10000;   // 64K
    static const UInt32 minBufferSize = 0x4000;    // 16K

    if (format.mFramesPerPacket != 0) {
        Float64 numPacketsForTime = format.mSampleRate / format.mFramesPerPacket * seconds;
        *bufferSize = numPacketsForTime * maxPacketSize;
    } else {
        // No predictable packet duration: default size (?: as in Apple's sample, keeps the select)
        *bufferSize = maxBufferSize > maxPacketSize ? maxBufferSize : maxPacketSize;
    }

    if (*bufferSize > maxBufferSize && *bufferSize > maxPacketSize) {
        *bufferSize = maxBufferSize;
    } else if (*bufferSize < minBufferSize) {
        *bufferSize = minBufferSize;
    }
    *numPackets = *bufferSize / maxPacketSize;
}

// ---------------------------------------------------------------------------------------------
// CSoundEvent_Cocoa: hardware (OpenAL) sound
// ---------------------------------------------------------------------------------------------

class CSoundEvent_Cocoa : public CSoundEvent3d {
public:
    CSoundEvent_Cocoa()
    {
        m_format = AL_FORMAT_MONO8;
        m_buffer = 0;
        m_source = 0;
        m_lastError = 0;
    }

    virtual ~CSoundEvent_Cocoa();

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
    virtual unsigned char SetRelativeToListener();
    virtual unsigned char SetMinGain();
    virtual unsigned char SetMaxGain();
    virtual unsigned char SetMaxDistance();
    virtual unsigned char SetReferenceDistance();
    virtual unsigned char SetRolloffFactor();
    virtual unsigned char SetConeOuterGain();
    virtual unsigned char SetConeInnerAngle();
    virtual unsigned char SetConeOuterAngle();
    virtual unsigned char SetPosition();
    virtual unsigned char SetVelocity();
    virtual unsigned char SetDirection();

private:
    ALenum m_format;        // 0x8c
    ALuint m_buffer;        // 0x90
    ALuint m_source;        // 0x94
    ALenum m_lastError;     // 0x98 last alGetError()
};

CSoundEvent* CSoundEvent::CreateInstance(ICMediaPlayer::SoundStreamType type)
{
    ICMediaPlayer* player = ICMediaPlayer::GetInstance();

    unsigned int channels;
    unsigned int bitsPerSample;
    unsigned int sampleRate;
    unsigned int maxSounds;
    unsigned int bufferLength;
    unsigned char useSoftwareMixer;
    player->GetDescription(channels, bitsPerSample, sampleRate, maxSounds, bufferLength, useSoftwareMixer);

    CSoundEvent* event = NULL;
    if (useSoftwareMixer) {
        // Software mixed events decode their data themselves
        switch (type) {
        case ICMediaPlayer::SoundStreamType_PCM:
            event = new CSoundEventPCM();
            break;
        case ICMediaPlayer::SoundStreamType_ADPCM:
            event = new CSoundEventStreamingADPCM();
            break;
        case ICMediaPlayer::SoundStreamType_OGG:
            event = new CSoundEventStreamingOGG();
            break;
        }
    } else {
        event = new CSoundEvent_Cocoa();
    }
    return event;
}

CSoundEvent_Cocoa::~CSoundEvent_Cocoa()
{
    CSoundEvent_Cocoa::Stop();
    CSoundEvent_Cocoa::Destroy();
}

unsigned char CSoundEvent_Cocoa::Stop()
{
    alGetError();
    alSourceStop(m_source);
    m_lastError = alGetError();
    if (m_lastError == AL_NO_ERROR) {
        m_state = State_Stopped;
    }
    return m_lastError == AL_NO_ERROR;
}

void CSoundEvent_Cocoa::Destroy()
{
    if (g_enablePoolObjects) {
        // The pair goes back to the pool
        if (m_source != 0) {
            alSourceStop(m_source);
            alSourcei(m_source, AL_BUFFER, 0);
        }
        if (m_buffer != 0) {
            ((CMediaPlayer_Cocoa*)m_player)->ReleaseBufferAndSource(m_buffer, m_source);
        }
        m_buffer = 0;
        m_source = 0;
    } else {
        if (m_source != 0) {
            alSourceStop(m_source);
            alSourcei(m_source, AL_BUFFER, 0);
            alDeleteSources(1, &m_source);
            m_source = 0;
        }
        if (m_buffer != 0) {
            alDeleteBuffers(1, &m_buffer);
            m_buffer = 0;
        }
    }
    alGetError();
}

unsigned char CSoundEvent_Cocoa::Initialize()
{
    Destroy();
    if (m_binary == NULL) {
        return 0;
    }

    m_lastError = AL_NO_ERROR;
    alGetError();
    m_format = GetALFormat(m_channels, m_bitsPerSample);

    unsigned char haveBuffer;
    if (g_enablePoolObjects) {
        haveBuffer = ((CMediaPlayer_Cocoa*)m_player)->AquireBufferAndSource(m_buffer, m_source);
    } else {
        alGenBuffers(1, &m_buffer);
        m_lastError = alGetError();
        if (m_lastError == AL_NO_ERROR) {
            alGenSources(1, &m_source);
            m_lastError = alGetError();
        }
        haveBuffer = (m_lastError == AL_NO_ERROR);
    }

    if (haveBuffer) {
        alBufferData(m_buffer, m_format, m_binary->m_data, m_binary->m_size, m_sampleRate);
        m_lastError = alGetError();
    }
    if (m_lastError == AL_NO_ERROR) {
        alSourcei(m_source, AL_BUFFER, m_buffer);
        m_lastError = alGetError();
        if (m_lastError == AL_NO_ERROR) {
            alSourcei(m_source, AL_LOOPING, m_loop & 1);
            m_lastError = alGetError();
            if (m_lastError == AL_NO_ERROR) {
                // Push every property to the new source
                SetGain();
                SetPitch();
                SetPan();
                SetRelativeToListener();
                SetMinGain();
                SetMaxGain();
                SetMaxDistance();
                SetReferenceDistance();
                SetRolloffFactor();
                SetConeOuterGain();
                SetConeInnerAngle();
                SetConeOuterAngle();
                SetPosition();
                SetVelocity();
                SetDirection();
                m_lastError = AL_NO_ERROR;
            }
        }
    }
    return m_lastError == AL_NO_ERROR;
}

unsigned char CMediaPlayer_Cocoa::AquireBufferAndSource(unsigned int& buffer, unsigned int& source)
{
    buffer = 0;
    source = 0;

    CALPoolObject* object = (CALPoolObject*)m_freePool.GetHead();
    if (object == NULL) {
        return 0;
    }
    buffer = object->m_buffer;
    source = object->m_source;
    m_freePool.Remove(object);
    m_usedPool.Insert(object, NULL);
    return 1;
}

unsigned char CMediaPlayer_Cocoa::ReleaseBufferAndSource(unsigned int buffer, unsigned int source)
{
    unsigned char released = 0;
    if (buffer != 0) {
        CLinkListNode* node = m_usedPool.GetHead();
        while (released == 0 && node != NULL) {
            CLinkListNode* next = node->GetNext();
            if (((CALPoolObject*)node)->m_buffer == buffer) {
                m_usedPool.Remove(node);
                m_freePool.InsertAtEnd(node, NULL);
                released = 1;
            }
            node = next;
        }
    }
    return released;
}

unsigned char CSoundEvent_Cocoa::Play()
{
    alGetError();
    alSourcePlay(m_source);
    m_lastError = alGetError();
    if (m_lastError == AL_NO_ERROR) {
        m_state = State_Playing;
    }
    return m_lastError == AL_NO_ERROR;
}

unsigned char CSoundEvent_Cocoa::Pause()
{
    alGetError();
    alSourcePause(m_source);
    m_lastError = alGetError();
    if (m_lastError == AL_NO_ERROR) {
        m_state = State_Paused;
    }
    return m_lastError == AL_NO_ERROR;
}

unsigned char CSoundEvent_Cocoa::Resume()
{
    alGetError();
    alSourcePlay(m_source);
    m_lastError = alGetError();
    if (m_lastError == AL_NO_ERROR) {
        m_state = State_Playing;
    }
    return m_lastError == AL_NO_ERROR;
}

unsigned char CSoundEvent_Cocoa::HandleUpdate(int deltaTime)
{
    alGetError();
    if (m_state == State_Playing) {
        // A finished (non looping) source becomes "stopped"
        ALint sourceState = 0;
        alGetSourcei(m_source, AL_SOURCE_STATE, &sourceState);
        m_lastError = alGetError();
        if (m_lastError != AL_NO_ERROR || sourceState == AL_STOPPED) {
            Stop();
        }
    }
    return 1;
}

int CSoundEvent_Cocoa::GetLastErr()
{
    return m_lastError;
}

unsigned char CSoundEvent_Cocoa::SetOffsetBytes(int offset)
{
    alGetError();
    alSourcei(m_source, AL_BYTE_OFFSET, offset);
    m_lastError = alGetError();
    return m_lastError == AL_NO_ERROR;
}

unsigned char CSoundEvent_Cocoa::SetOffsetMS(int offset)
{
    alGetError();
    alSourcef(m_source, AL_SEC_OFFSET, (float)offset / 1000.0f);
    m_lastError = alGetError();
    return m_lastError == AL_NO_ERROR;
}

unsigned char CSoundEvent_Cocoa::GetOffsetBytes(int& offset)
{
    // Original behaviour kept: the offset is never read back (alSourcei instead of alGetSourcei)
    offset = 0;
    ALint value = 0;
    alGetError();
    alSourcei(m_source, AL_BYTE_OFFSET, value);
    m_lastError = alGetError();
    if (m_lastError == AL_NO_ERROR) {
        offset = value;
    }
    return m_lastError == AL_NO_ERROR;
}

unsigned char CSoundEvent_Cocoa::GetOffsetMS(int& offset)
{
    // Original behaviour kept: the offset is never read back (alSourcef instead of alGetSourcef)
    offset = 0;
    ALfloat value = 0.0f;
    alGetError();
    alSourcef(m_source, AL_SEC_OFFSET, value);
    m_lastError = alGetError();
    if (m_lastError == AL_NO_ERROR) {
        offset = (int)value;
    }
    return m_lastError == AL_NO_ERROR;
}

unsigned char CSoundEvent_Cocoa::SetPitch()
{
    alGetError();

    // -100..100 -> 0.5..2.0
    float pitch;
    if (m_pitch > 0) {
        pitch = (float)(m_pitch + 100) / 100.0f;
    } else if (m_pitch < 0) {
        pitch = (float)(m_pitch + 100) / 200.0f + 0.5f;
    } else {
        pitch = 1.0f;
    }
    alSourcef(m_source, AL_PITCH, pitch);
    m_lastError = alGetError();
    return m_lastError == AL_NO_ERROR;
}

unsigned char CSoundEvent_Cocoa::SetGain()
{
    alGetError();
    // Player volume 0..10 times sound volume 0..100
    unsigned char playerVolume = m_player->GetVolume();
    alSourcef(m_source, AL_GAIN, (float)playerVolume * (float)m_volume / 1000.0f);
    m_lastError = alGetError();
    return m_lastError == AL_NO_ERROR;
}

unsigned char CSoundEvent_Cocoa::SetPan()
{
    alGetError();
    alSource3f(m_source, AL_POSITION, (float)m_pan / 100.0f, 0.0f, 0.0f);
    m_lastError = alGetError();
    return m_lastError == AL_NO_ERROR;
}

unsigned char CSoundEvent_Cocoa::SetRelativeToListener()
{
    alGetError();
    alSourcei(m_source, AL_SOURCE_RELATIVE, m_relativeToListener != 0);
    m_lastError = alGetError();
    return m_lastError == AL_NO_ERROR;
}

unsigned char CSoundEvent_Cocoa::SetMinGain()
{
    alGetError();
    alSourcef(m_source, AL_MIN_GAIN, (float)m_minGain * kFixedToFloat);
    m_lastError = alGetError();
    return m_lastError == AL_NO_ERROR;
}

unsigned char CSoundEvent_Cocoa::SetMaxGain()
{
    alGetError();
    alSourcef(m_source, AL_MAX_GAIN, (float)m_maxGain * kFixedToFloat);
    m_lastError = alGetError();
    return m_lastError == AL_NO_ERROR;
}

unsigned char CSoundEvent_Cocoa::SetMaxDistance()
{
    alGetError();
    alSourcef(m_source, AL_MAX_DISTANCE, (float)m_maxDistance * kFixedToFloat);
    m_lastError = alGetError();
    return m_lastError == AL_NO_ERROR;
}

unsigned char CSoundEvent_Cocoa::SetReferenceDistance()
{
    alGetError();
    alSourcef(m_source, AL_REFERENCE_DISTANCE, (float)m_referenceDistance * kFixedToFloat);
    m_lastError = alGetError();
    return m_lastError == AL_NO_ERROR;
}

unsigned char CSoundEvent_Cocoa::SetRolloffFactor()
{
    alGetError();
    // Original behaviour kept: written to AL_MAX_DISTANCE, not AL_ROLLOFF_FACTOR
    alSourcef(m_source, AL_MAX_DISTANCE, (float)m_rolloffFactor * kFixedToFloat);
    m_lastError = alGetError();
    return m_lastError == AL_NO_ERROR;
}

unsigned char CSoundEvent_Cocoa::SetConeOuterGain()
{
    alGetError();
    alSourcef(m_source, AL_CONE_OUTER_GAIN, (float)m_coneOuterGain * kFixedToFloat);
    m_lastError = alGetError();
    return m_lastError == AL_NO_ERROR;
}

unsigned char CSoundEvent_Cocoa::SetConeInnerAngle()
{
    alGetError();
    alSourcef(m_source, AL_CONE_INNER_ANGLE, (float)m_coneInnerAngle * kFixedToFloat);
    m_lastError = alGetError();
    return m_lastError == AL_NO_ERROR;
}

unsigned char CSoundEvent_Cocoa::SetConeOuterAngle()
{
    alGetError();
    alSourcef(m_source, AL_CONE_OUTER_ANGLE, (float)m_coneOuterAngle * kFixedToFloat);
    m_lastError = alGetError();
    return m_lastError == AL_NO_ERROR;
}

unsigned char CSoundEvent_Cocoa::SetPosition()
{
    alGetError();
    alSource3f(m_source, AL_POSITION, (float)m_position[0] * kFixedToFloat, (float)m_position[1] * kFixedToFloat,
               (float)m_position[2] * kFixedToFloat);
    m_lastError = alGetError();
    return m_lastError == AL_NO_ERROR;
}

unsigned char CSoundEvent_Cocoa::SetVelocity()
{
    alGetError();
    // Original behaviour kept: written to AL_POSITION, not AL_VELOCITY
    alSource3f(m_source, AL_POSITION, (float)m_velocity[0] * kFixedToFloat, (float)m_velocity[1] * kFixedToFloat,
               (float)m_velocity[2] * kFixedToFloat);
    m_lastError = alGetError();
    return m_lastError == AL_NO_ERROR;
}

unsigned char CSoundEvent_Cocoa::SetDirection()
{
    alGetError();
    // Original behaviour kept: written to AL_POSITION, not AL_DIRECTION
    alSource3f(m_source, AL_POSITION, (float)m_direction[0] * kFixedToFloat, (float)m_direction[1] * kFixedToFloat,
               (float)m_direction[2] * kFixedToFloat);
    m_lastError = alGetError();
    return m_lastError == AL_NO_ERROR;
}

// ---------------------------------------------------------------------------------------------
// CVibrationEvent_Cocoa: the system vibrate sound
// ---------------------------------------------------------------------------------------------

/** The vibration finished: stop the event passed as client data. */
void MyAudioServicesSystemSoundCompletionProc(SystemSoundID soundID, void* clientData)
{
    if (soundID == kSystemSoundID_Vibrate) {
        ((CMediaEvent*)clientData)->Stop();
    }
}

class CVibrationEvent_Cocoa : public CVibrationEvent {
public:
    CVibrationEvent_Cocoa()
    {
        m_lastError = 0;
    }

    virtual ~CVibrationEvent_Cocoa();

    virtual unsigned char Initialize();
    virtual void Destroy();
    virtual unsigned char Play();
    virtual unsigned char Stop();
    virtual unsigned char Pause();
    virtual unsigned char Resume();
    virtual unsigned char HandleUpdate(int deltaTime);
    virtual int GetLastErr();

private:
    int m_lastError;        // 0x2c always 0
};

CVibrationEvent* CVibrationEvent::CreateInstance()
{
    return new CVibrationEvent_Cocoa();
}

CVibrationEvent_Cocoa::~CVibrationEvent_Cocoa()
{
    AudioServicesRemoveSystemSoundCompletion(kSystemSoundID_Vibrate);
}

unsigned char CVibrationEvent_Cocoa::Stop()
{
    return 0;
}

void CVibrationEvent_Cocoa::Destroy()
{
    AudioServicesRemoveSystemSoundCompletion(kSystemSoundID_Vibrate);
}

unsigned char CVibrationEvent_Cocoa::Initialize()
{
    AudioServicesAddSystemSoundCompletion(kSystemSoundID_Vibrate, NULL, NULL,
                                          MyAudioServicesSystemSoundCompletionProc, this);
    return 1;
}

unsigned char CVibrationEvent_Cocoa::Play()
{
    AudioServicesPlaySystemSound(kSystemSoundID_Vibrate);
    m_state = State_Playing;
    return 1;
}

unsigned char CVibrationEvent_Cocoa::Pause()
{
    return 0;
}

unsigned char CVibrationEvent_Cocoa::Resume()
{
    return 0;
}

unsigned char CVibrationEvent_Cocoa::HandleUpdate(int deltaTime)
{
    if (m_state == State_Playing) {
        m_elapsed = m_elapsed + deltaTime;
        if (m_elapsed > (int)m_duration) {
            Stop();
        }
    }
    return 1;
}

int CVibrationEvent_Cocoa::GetLastErr()
{
    return m_lastError;
}

// ---------------------------------------------------------------------------------------------
// CALPoolObject: one pooled OpenAL buffer / source pair
// ---------------------------------------------------------------------------------------------

CALPoolObject::CALPoolObject()
{
    m_buffer = 0;
    m_source = 0;
    alGenBuffers(1, &m_buffer);
    alGenSources(1, &m_source);
}

CALPoolObject::~CALPoolObject()
{
    if (m_source != 0) {
        alDeleteSources(1, &m_source);
    }
    if (m_buffer != 0) {
        alDeleteBuffers(1, &m_buffer);
    }
}

// ---------------------------------------------------------------------------------------------
// CMediaPlayer_Cocoa
// ---------------------------------------------------------------------------------------------

ICMediaPlayer* ICMediaPlayer::CreateInstance()
{
    return new CMediaPlayer_Cocoa();
}

CMediaPlayer_Cocoa::CMediaPlayer_Cocoa()
{
    m_soundEngineInitialized = 0;
    m_streamFormat = AL_FORMAT_MONO16;
    m_streamData = NULL;
    memset(&m_streamThread, 0, sizeof(m_streamThread));
    memset(m_streamBuffers, 0, sizeof(m_streamBuffers));
    m_streamSource = 0;
    m_streaming = 0;
    m_streamThreadStopped = 0;
    m_audioMode = AudioMode_Ambient;
    m_musicVolume = 0;
    g_enablePoolObjects = 1;

    AudioSessionInitialize(NULL, NULL, InterruptionListenerCB, this);
    SetAudioSessionCategory(m_audioMode);
}

CMediaPlayer_Cocoa::~CMediaPlayer_Cocoa()
{
    CMediaPlayer::Stop(0);
    CMediaPlayer::StopVibrate(0);
    StopMusic();
    CMediaPlayer_Cocoa::DestroySoundEngine();
}

void CMediaPlayer_Cocoa::DestroySoundEngine()
{
    if (!m_soundEngineInitialized) {
        return;
    }

    Stop(0);
    StopVibrate(0);
    if (m_useSoftwareMixer) {
        StreamStop();
    }
    if (g_enablePoolObjects) {
        while (m_freePool.GetHead() != NULL) {
            CLinkListNode* node = m_freePool.GetHead();
            node->RemoveFromList();
            delete node;
        }
    }
    StopMusic();

    ALCcontext* context = alcGetCurrentContext();
    ALCdevice* device = alcGetContextsDevice(context);
    alcMakeContextCurrent(NULL);
    alcDestroyContext(context);
    alcCloseDevice(device);
    AudioSessionSetActive(false);
    m_soundEngineInitialized = 0;
}

void CMediaPlayer_Cocoa::DestroyVibrationEngine()
{
}

unsigned char CMediaPlayer_Cocoa::Initialize(unsigned int channels, unsigned int bitsPerSample, unsigned int sampleRate,
                                             unsigned int maxSounds, unsigned int bufferLength,
                                             unsigned char useSoftwareMixer)
{
    // The software mixer owns a single OpenAL stream: no pooled pairs
    g_enablePoolObjects = (useSoftwareMixer == 0);
    CMediaPlayer::Initialize(channels, bitsPerSample, sampleRate, maxSounds, bufferLength, useSoftwareMixer);

    if (useSoftwareMixer) {
        m_streamData = new unsigned char*[StreamBufferCount];
        for (int index = 0; index < StreamBufferCount; index++) {
            int bufferSize = GetBufferSize();
            m_streamData[index] = new unsigned char[bufferSize];
        }
    }
    return 1;
}

void CMediaPlayer_Cocoa::Destroy()
{
    if (m_useSoftwareMixer && m_streamData != NULL) {
        for (int index = 0; index < StreamBufferCount; index++) {
            delete[] m_streamData[index];
        }
        delete[] m_streamData;
    }
    CMediaPlayer::Destroy();
}

void CMediaPlayer_Cocoa::HandleUpdate(int deltaTime)
{
    CMediaPlayer::HandleUpdate(deltaTime);

    if (!m_useSoftwareMixer) {
        return;
    }

    // Start the stream when there is something to hear (a vibration silences it on iPhones)
    if (!m_suspended && m_soundEnabled && !m_streaming &&
        (((com::glu::platform::framework::ICCore*)CApplet::m_pCore)->m_canVibrate || m_vibrationEvents.GetCount() == 0)) {
        StreamStart();
    }
    if (m_streaming) {
        if (!m_streamThreadStopped && m_streamSource != 0) {
            ALint sourceState = 0;
            alGetSourcei(m_streamSource, AL_SOURCE_STATE, &sourceState);
            if (alGetError() == AL_NO_ERROR && sourceState == AL_PLAYING) {
                return;
            }
        }
        StreamStop();
    }
}

unsigned char CMediaPlayer_Cocoa::StreamStart()
{
    alGetError();
    alGenBuffers(StreamBufferCount, m_streamBuffers);
    if (alGetError() == AL_NO_ERROR) {
        alGenSources(1, &m_streamSource);
    }
    if (alGetError() == AL_NO_ERROR) {
        // A plain stereo stream at the listener
        alSource3f(m_streamSource, AL_POSITION, 0.0f, 0.0f, 0.0f);
        alSource3f(m_streamSource, AL_VELOCITY, 0.0f, 0.0f, 0.0f);
        alSource3f(m_streamSource, AL_DIRECTION, 0.0f, 0.0f, 0.0f);
        alSourcef(m_streamSource, AL_ROLLOFF_FACTOR, 0.0f);
        alSourcei(m_streamSource, AL_SOURCE_RELATIVE, AL_TRUE);

        bool queued = true;
        for (int index = 0; index < StreamBufferCount; index++) {
            FillBuffer(index);
            alBufferData(m_streamBuffers[index], m_streamFormat, m_streamData[index], GetBufferSize(), m_sampleRate);
            if (alGetError() == AL_NO_ERROR) {
                alSourceQueueBuffers(m_streamSource, 1, &m_streamBuffers[index]);
            }
            if (alGetError() != AL_NO_ERROR) {
                queued = false;
                break;
            }
        }
        if (queued) {
            m_streamThreadStopped = 0;
            pthread_create(&m_streamThread, NULL, ThreadProc, this);
        }
    }

    alSourcePlay(m_streamSource);
    m_streaming = (alGetError() == AL_NO_ERROR);
    if (!m_streaming) {
        StreamStop();
    }
    return m_streaming;
}

void CMediaPlayer_Cocoa::StreamStop()
{
    pthread_cancel(m_streamThread);
    if (m_streamSource != 0) {
        alSourceStop(m_streamSource);
        ALint queuedCount;
        alGetSourcei(m_streamSource, AL_BUFFERS_QUEUED, &queuedCount);
        while (queuedCount-- != 0) {
            ALuint buffer;
            alSourceUnqueueBuffers(m_streamSource, 1, &buffer);
        }
        alDeleteSources(1, &m_streamSource);
        m_streamSource = 0;
    }
    alDeleteBuffers(StreamBufferCount, m_streamBuffers);
    memset(m_streamBuffers, 0, sizeof(m_streamBuffers));
    alGetError();
    m_streaming = 0;
}

void CMediaPlayer_Cocoa::SetVolume(unsigned char volume)
{
    CMediaPlayer::SetVolume(volume);
    if (m_useSoftwareMixer) {
        SetStreamVolume();
    }
}

unsigned char CMediaPlayer_Cocoa::SetStreamVolume()
{
    if (m_streamSource == 0) {
        return 0;
    }
    alSourcef(m_streamSource, AL_GAIN, (float)m_volume / 10.0f);
    return alGetError() == AL_NO_ERROR;
}

void* CMediaPlayer_Cocoa::ThreadProc(void* player)
{
    CMediaPlayer_Cocoa* mediaPlayer = (CMediaPlayer_Cocoa*)player;
    if (mediaPlayer != NULL && mediaPlayer->m_useSoftwareMixer) {
        mediaPlayer->OnThreadProc();
    }
    return NULL;
}

void CMediaPlayer_Cocoa::OnThreadProc()
{
    // Refill every processed stream buffer; never returns (cancelled by StreamStop)
    for (;;) {
        ALint processed;
        alGetSourcei(m_streamSource, AL_BUFFERS_PROCESSED, &processed);
        if (processed <= 0) {
            usleep(10000);
            continue;
        }

        while (processed-- != 0) {
            ALuint buffer;
            alSourceUnqueueBuffers(m_streamSource, 1, &buffer);
            if (alGetError() != AL_NO_ERROR) {
                break;
            }

            int index = 0;
            while (index < StreamBufferCount && buffer != m_streamBuffers[index]) {
                index++;
            }
            if (index >= StreamBufferCount) {
                break;
            }

            FillBuffer(index);
            alBufferData(buffer, m_streamFormat, m_streamData[index], GetBufferSize(), m_sampleRate);
            if (alGetError() == AL_NO_ERROR) {
                alSourceQueueBuffers(m_streamSource, 1, &buffer);
            }
            if (alGetError() != AL_NO_ERROR) {
                break;
            }
        }
    }
}

unsigned char CMediaPlayer_Cocoa::CanSetProperty(GlobalProperty property)
{
    // The listener exists only without the software mixer
    unsigned char result = 0;
    if (!m_useSoftwareMixer) {
        result = 1;
        if ((unsigned int)property > GlobalProperty_ListenerOrientation) {
            result = 0;
        }
    }
    return result;
}

unsigned char CMediaPlayer_Cocoa::CanGetProperty(GlobalProperty property)
{
    // The listener exists only without the software mixer
    unsigned char result = 0;
    if (!m_useSoftwareMixer) {
        result = 1;
        if ((unsigned int)property > GlobalProperty_ListenerOrientation) {
            result = 0;
        }
    }
    return result;
}

unsigned char CMediaPlayer_Cocoa::CanSetProperty(SoundProperty property)
{
    if (m_useSoftwareMixer) {
        switch (property) {
        case SoundProperty_Volume:
        case SoundProperty_OffsetBytes:
        case SoundProperty_OffsetMS:
            return 1;
        default:
            return 0;
        }
    }
    switch (property) {
    case SoundProperty_Volume:
    case SoundProperty_Pitch:
    case SoundProperty_Pan:
    case SoundProperty_OffsetBytes:
    case SoundProperty_OffsetMS:
    case SoundProperty_MinGain:
    case SoundProperty_MaxGain:
    case SoundProperty_MaxDistance:
    case SoundProperty_ReferenceDistance:
    case SoundProperty_RolloffFactor:
    case SoundProperty_ConeOuterGain:
    case SoundProperty_ConeInnerAngle:
    case SoundProperty_ConeOuterAngle:
    case SoundProperty_Position:
    case SoundProperty_Velocity:
    case SoundProperty_Direction:
        return 1;
    default:
        return 0;
    }
}

unsigned char CMediaPlayer_Cocoa::CanGetProperty(SoundProperty property)
{
    if (m_useSoftwareMixer) {
        switch (property) {
        case SoundProperty_Volume:
        case SoundProperty_OffsetBytes:
        case SoundProperty_OffsetMS:
            return 1;
        default:
            return 0;
        }
    }
    switch (property) {
    case SoundProperty_Volume:
    case SoundProperty_Pitch:
    case SoundProperty_Pan:
    case SoundProperty_OffsetBytes:
    case SoundProperty_OffsetMS:
    case SoundProperty_MinGain:
    case SoundProperty_MaxGain:
    case SoundProperty_MaxDistance:
    case SoundProperty_ReferenceDistance:
    case SoundProperty_RolloffFactor:
    case SoundProperty_ConeOuterGain:
    case SoundProperty_ConeInnerAngle:
    case SoundProperty_ConeOuterAngle:
    case SoundProperty_Position:
    case SoundProperty_Velocity:
    case SoundProperty_Direction:
        return 1;
    default:
        return 0;
    }
}

unsigned char CMediaPlayer_Cocoa::IsUserMusicPlaying()
{
    UInt32 otherAudioIsPlaying;
    UInt32 propertySize = sizeof(UInt32);
    AudioSessionGetProperty(kAudioSessionProperty_OtherAudioIsPlaying, &propertySize, &otherAudioIsPlaying);
    return otherAudioIsPlaying != 0;
}

void CMediaPlayer_Cocoa::SetAudioMode(eAudioMode mode)
{
    m_audioMode = mode;
    DestroySoundEngine();
    SetAudioSessionCategory(mode);
    if (m_soundEnabled) {
        InitializeSoundEngine();
    }
}

// ---------------------------------------------------------------------------------------------
// BackgroundTrackMgr: music through an AudioQueue (from Apple's SoundEngine sample, one file)
// ---------------------------------------------------------------------------------------------

class BackgroundTrackMgr {
public:
    struct BG_FileInfo {
        const char* mFilePath;                      // 0x20
        AudioFileID mAFID;                          // 0x24
        AudioStreamBasicDescription mFileFormat;    // 0x28
        UInt64 mFileDataSize;                       // 0x50
    };

    BackgroundTrackMgr()
    {
        mFileInfo.mFilePath = NULL;
        mFileInfo.mAFID = 0;
        mFileInfo.mFileDataSize = 0;
        mQueue = 0;
        mBufferByteSize = 0;
        mCurrentPacket = 0;
        mNumPacketsToRead = 0;
        mVolume = 1.0f;
        mPacketDescs = NULL;
        mStopAtEnd = false;
    }

    ~BackgroundTrackMgr()
    {
        if (mQueue != 0) {
            AudioQueueDispose(mQueue, true);
        }
        if (mPacketDescs != NULL) {
            delete[] mPacketDescs;
        }
        if (mFileInfo.mAFID != 0) {
            AudioFileClose(mFileInfo.mAFID);
        }
    }

    /** Open `filePath` and prepare the queue. */
    OSStatus LoadTrack(const char* filePath)
    {
        mFileInfo.mFilePath = filePath;
        OSStatus result = LoadFileDataInfo(mFileInfo.mFilePath, mFileInfo.mAFID, mFileInfo.mFileFormat,
                                           mFileInfo.mFileDataSize);
        if (result == noErr) {
            result = SetupQueue(&mFileInfo);
            if (result == noErr) {
                result = SetupBuffers(&mFileInfo);
            }
        }
        return result;
    }

    OSStatus Start()
    {
        OSStatus result = AudioQueuePrime(mQueue, 1, NULL);
        if (result == noErr) {
            result = AudioQueueStart(mQueue, NULL);
        }
        return result;
    }

    OSStatus SetVolume(Float32 volume)
    {
        mVolume = volume;
        return AudioQueueSetParameter(mQueue, kAudioQueueParam_Volume, mVolume);
    }

    OSStatus SetupQueue(BG_FileInfo* fileInfo)
    {
        OSStatus result = AudioQueueNewOutput(&fileInfo->mFileFormat, QueueCallback, this, CFRunLoopGetCurrent(),
                                              kCFRunLoopCommonModes, 0, &mQueue);
        if (result != noErr) {
            return result;
        }

        // The file magic cookie, if any, goes to the queue
        UInt32 size = sizeof(UInt32);
        result = AudioFileGetPropertyInfo(fileInfo->mAFID, kAudioFilePropertyMagicCookieData, &size, NULL);
        if (result == noErr && size != 0) {
            char* cookie = new char[size];
            result = noErr;
            if (cookie != NULL) {
                result = AudioFileGetProperty(fileInfo->mAFID, kAudioFilePropertyMagicCookieData, &size, cookie);
                if (result == noErr) {
                    result = AudioQueueSetProperty(mQueue, kAudioQueueProperty_MagicCookie, cookie, size);
                }
                delete[] cookie;
            }
            if (result != noErr) {
                return result;
            }
        }

        // Channel layout
        result = AudioFileGetPropertyInfo(fileInfo->mAFID, kAudioFilePropertyChannelLayout, &size, NULL);
        if (result == noErr && size != 0) {
            AudioChannelLayout* layout = (AudioChannelLayout*)malloc(size);
            if (layout != NULL) {
                result = AudioFileGetProperty(fileInfo->mAFID, kAudioFilePropertyChannelLayout, &size, layout);
                if (result != noErr) {
                    free(layout);
                    return result;
                }
                result = AudioQueueSetProperty(mQueue, kAudioQueueProperty_ChannelLayout, layout, size);
                free(layout);
                if (result != noErr) {
                    return result;
                }
            }
        }

        result = AudioQueueAddPropertyListener(mQueue, kAudioQueueProperty_IsRunning, QueueStoppedProc, this);
        if (result == noErr) {
            result = SetVolume(mVolume);
        }
        return result;
    }

    OSStatus SetupBuffers(BG_FileInfo* fileInfo)
    {
        UInt32 maxPacketSize;
        UInt32 size = sizeof(maxPacketSize);
        OSStatus result = AudioFileGetProperty(fileInfo->mAFID, kAudioFilePropertyPacketSizeUpperBound, &size,
                                               &maxPacketSize);
        if (result != noErr) {
            return result;
        }

        // About half a second per buffer
        CalculateBytesForTime(fileInfo->mFileFormat, maxPacketSize, 0.5, &mBufferByteSize, &mNumPacketsToRead);
        mNumPacketsToRead = mBufferByteSize / maxPacketSize;

        // Variable bit rate data needs packet descriptions
        bool isFormatVBR = (fileInfo->mFileFormat.mBytesPerPacket == 0 || fileInfo->mFileFormat.mFramesPerPacket == 0);
        if (isFormatVBR) {
            mPacketDescs = new AudioStreamPacketDescription[mNumPacketsToRead];
        }

        result = AudioQueueAllocateBuffer(mQueue, mBufferByteSize, &mBuffer);
        if (result == noErr) {
            QueueCallback(this, mQueue, mBuffer);
            result = noErr;
        }
        return result;
    }

    /** Read the next packets into the finished buffer and enqueue it; loops unless mStopAtEnd. */
    static void QueueCallback(void* userData, AudioQueueRef queue, AudioQueueBufferRef buffer)
    {
        BackgroundTrackMgr* track = (BackgroundTrackMgr*)userData;
        UInt32 numBytes = 0;
        UInt32 numPackets;

        do {
            numPackets = track->mNumPacketsToRead;
            OSStatus result = AudioFileReadPackets(track->mFileInfo.mAFID, false, &numBytes, track->mPacketDescs,
                                                   track->mCurrentPacket, &numPackets, buffer->mAudioData);
            if (result != noErr) {
                return;
            }
            buffer->mAudioDataByteSize = numBytes;

            if (numPackets == 0) {
                // End of file: rewind, or stop
                track->mCurrentPacket = 0;
                if (track->mStopAtEnd) {
                    AudioQueueStop(queue, false);
                    return;
                }
            } else {
                UInt32 descriptionCount = numPackets;
                if (track->mPacketDescs == NULL) {
                    descriptionCount = 0;
                }
                result = AudioQueueEnqueueBuffer(queue, buffer, descriptionCount, track->mPacketDescs);
                if (result == noErr) {
                    track->mCurrentPacket = track->mCurrentPacket + numPackets;
                }
            }
        } while (numPackets == 0);
    }

    /** The queue stopped running: the track is finished, delete it. */
    static void QueueStoppedProc(void* userData, AudioQueueRef queue, AudioQueuePropertyID propertyID)
    {
        UInt32 isRunning;
        UInt32 size = sizeof(UInt32);
        AudioQueueGetProperty(queue, kAudioQueueProperty_IsRunning, &isRunning, &size);
        if (isRunning == 0 && s_backgroundTrack != NULL) {
            delete s_backgroundTrack;
            s_backgroundTrack = NULL;
        }
    }

    AudioQueueRef mQueue;                           // 0x00
    AudioQueueBufferRef mBuffer;                    // 0x04
    UInt32 mBufferByteSize;                         // 0x08
    SInt64 mCurrentPacket;                          // 0x0c
    UInt32 mNumPacketsToRead;                       // 0x14
    Float32 mVolume;                                // 0x18
    AudioStreamPacketDescription* mPacketDescs;     // 0x1c NULL for constant bit rate data
    BG_FileInfo mFileInfo;                          // 0x20
    Boolean mStopAtEnd;                             // 0x58 play once
};

unsigned char CMediaPlayer_Cocoa::PlayMusic(const wchar_t* fileName, unsigned char loop)
{
    if (s_backgroundTrack != NULL) {
        delete s_backgroundTrack;
        s_backgroundTrack = NULL;
    }

    bool started = false;
    if (m_soundEnabled) {
        char filePath[256];
        wcstombs(filePath, fileName, wcslen(fileName) + 1);

        s_backgroundTrack = new BackgroundTrackMgr();
        if (s_backgroundTrack != NULL && s_backgroundTrack->LoadTrack(filePath) == noErr) {
            float volume = 0.0f;
            if (m_musicVolume != 0) {
                volume = (float)m_musicVolume / 10.0f;
            }
            s_backgroundTrack->SetVolume(volume);
            started = (s_backgroundTrack->Start() == noErr);
            if (started && !loop) {
                s_backgroundTrack->mStopAtEnd = true;
            }
        }
    }
    return started;
}

void CMediaPlayer_Cocoa::StopMusic()
{
    if (s_backgroundTrack != NULL) {
        delete s_backgroundTrack;
        s_backgroundTrack = NULL;
    }
}

void CMediaPlayer_Cocoa::SetMusicVolume(unsigned char volume)
{
    m_musicVolume = volume;
    if (s_backgroundTrack != NULL) {
        float trackVolume = 0.0f;
        if (volume != 0) {
            trackVolume = (float)volume / 10.0f;
        }
        s_backgroundTrack->SetVolume(trackVolume);
    }
}

unsigned char CMediaPlayer_Cocoa::GetMusicVolume() const
{
    return m_musicVolume;
}

unsigned char CMediaPlayer_Cocoa::IsMusicPlaying() const
{
    return s_backgroundTrack != NULL;
}

void CMediaPlayer_Cocoa::HandleSuspend()
{
    CMediaPlayer::HandleSuspend();
    StopMusic();
}

void CMediaPlayer_Cocoa::InitializeSoundEngine()
{
    if (m_soundEngineInitialized) {
        return;
    }

    SetAudioSessionCategory(m_audioMode);
    AudioSessionSetActive(true);
    if (m_useSoftwareMixer) {
        m_streamFormat = GetALFormat(m_channels, m_bitsPerSample);
    }

    ALCdevice* device = alcOpenDevice(NULL);
    if (device != NULL) {
        ALCcontext* context = alcCreateContext(device, NULL);
        alcMakeContextCurrent(context);
    }
    alListenerf(AL_GAIN, 1.0f);

    SetDopplerFactor();
    SetSpeedOfSound();
    SetDistanceModel();
    SetListenerPosition();
    SetListenerVelocity();
    SetListenerOrientation();

    if (g_enablePoolObjects) {
        for (int count = kPoolObjectCount; count != 0; count--) {
            CALPoolObject* object = new CALPoolObject();
            m_freePool.Insert(object, NULL);
        }
    }
    m_soundEngineInitialized = 1;
}

unsigned char CMediaPlayer_Cocoa::InitializeVibrationEngine()
{
    return 1;
}

unsigned char CMediaPlayer_Cocoa::PlayInternal(CSoundEvent* event)
{
    // Phones without a vibration motor: a sound replaces a vibration of lower priority
    if (m_soundEvents.GetCount() < 1 && m_vibrationEvents.GetCount() > 0 &&
        !((com::glu::platform::framework::ICCore*)CApplet::m_pCore)->m_canVibrate) {
        CMediaEvent* vibration = (CMediaEvent*)m_vibrationEvents.GetTail();
        if (event->GetPriority() > vibration->GetPriority()) {
            return 0;
        }
        StopVibrate(0);
    }

    if (event->Initialize() && event->Play()) {
        return 1;
    }
    return 0;
}

unsigned char CMediaPlayer_Cocoa::VibrateInternal(CVibrationEvent* event)
{
    if (m_soundEvents.GetCount() > 0) {
        if (!((com::glu::platform::framework::ICCore*)CApplet::m_pCore)->m_canVibrate) {
            // Without a vibration motor the vibration is a sound: it replaces the playing sounds
            CMediaEvent* sound = (CMediaEvent*)m_soundEvents.GetHead();
            if (sound != NULL) {
                // Original behaviour kept: starts at 1, so the priorities never refuse the vibration
                unsigned char replaceSounds = 1;
                while (sound != NULL) {
                    CMediaEvent* next = (CMediaEvent*)sound->GetNext();
                    replaceSounds = replaceSounds | (event->GetPriority() <= sound->GetPriority());
                    sound = next;
                }
                if (!replaceSounds) {
                    return 0;
                }
            }
            Stop(0);
            if (m_useSoftwareMixer) {
                StreamStop();
            }
        }
    } else if (m_vibrationEvents.GetCount() > 0) {
        return 0;
    }

    if (event->Initialize() && event->Play()) {
        return 1;
    }
    return 0;
}

void CMediaPlayer_Cocoa::FillBuffer(int index)
{
    memset(m_streamData[index], 0, GetBufferSize());

    pthread_mutex_lock(&m_soundEventsMutex);
    CLinkListNode* node = m_soundEvents.GetHead();
    while (node != NULL) {
        CSoundEventPCM* event = (CSoundEventPCM*)node;
        CLinkListNode* next = node->GetNext();
        event->AddSamplesToBuffer(m_streamData[index], GetBufferSize());
        node = next;
    }
    pthread_mutex_unlock(&m_soundEventsMutex);
}

unsigned char CMediaPlayer_Cocoa::SetDopplerFactor()
{
    alGetError();
    alDopplerFactor((float)m_dopplerFactor * kFixedToFloat);
    return alGetError() == AL_NO_ERROR;
}

unsigned char CMediaPlayer_Cocoa::SetSpeedOfSound()
{
    alGetError();
    alSpeedOfSound((float)m_speedOfSound * kFixedToFloat);
    return alGetError() == AL_NO_ERROR;
}

unsigned char CMediaPlayer_Cocoa::SetDistanceModel()
{
    alGetError();

    ALenum model;
    switch (m_distanceModel) {
    case 1:
        model = AL_INVERSE_DISTANCE;
        break;
    case 2:
        model = AL_INVERSE_DISTANCE_CLAMPED;
        break;
    case 3:
        model = AL_LINEAR_DISTANCE;
        break;
    case 4:
        model = AL_LINEAR_DISTANCE_CLAMPED;
        break;
    case 5:
        model = AL_EXPONENT_DISTANCE;
        break;
    case 6:
        model = AL_EXPONENT_DISTANCE_CLAMPED;
        break;
    default:
        model = AL_NONE;
        break;
    }
    alDistanceModel(model);
    return alGetError() == AL_NO_ERROR;
}

unsigned char CMediaPlayer_Cocoa::SetListenerPosition()
{
    alGetError();
    alListener3f(AL_POSITION, (float)m_listenerPosition[0] * kFixedToFloat,
                 (float)m_listenerPosition[1] * kFixedToFloat, (float)m_listenerPosition[2] * kFixedToFloat);
    return alGetError() == AL_NO_ERROR;
}

unsigned char CMediaPlayer_Cocoa::SetListenerVelocity()
{
    alGetError();
    alListener3f(AL_VELOCITY, (float)m_listenerVelocity[0] * kFixedToFloat,
                 (float)m_listenerVelocity[1] * kFixedToFloat, (float)m_listenerVelocity[2] * kFixedToFloat);
    return alGetError() == AL_NO_ERROR;
}

unsigned char CMediaPlayer_Cocoa::SetListenerOrientation()
{
    alGetError();
    // "at" vector then "up" vector
    ALfloat orientation[6];
    orientation[0] = (float)m_listenerOrientation[0] * kFixedToFloat;
    orientation[1] = (float)m_listenerOrientation[1] * kFixedToFloat;
    orientation[2] = (float)m_listenerOrientation[2] * kFixedToFloat;
    orientation[3] = (float)m_listenerOrientation[3] * kFixedToFloat;
    orientation[4] = (float)m_listenerOrientation[4] * kFixedToFloat;
    orientation[5] = (float)m_listenerOrientation[5] * kFixedToFloat;
    alListenerfv(AL_ORIENTATION, orientation);
    return alGetError() == AL_NO_ERROR;
}
