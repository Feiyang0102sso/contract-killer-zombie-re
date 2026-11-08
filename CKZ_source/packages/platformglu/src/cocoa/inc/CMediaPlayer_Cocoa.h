// CMediaPlayer_Cocoa.h - iOS media player: OpenAL sources for hardware sounds (or one OpenAL stream
// fed by the software mixer), system sound vibration, and an AudioQueue background music track.
#ifndef CMEDIAPLAYER_COCOA_H
#define CMEDIAPLAYER_COCOA_H

#include <pthread.h>
#include "CMediaPlayer.h"
#include "CLinkList.h"

class CMediaPlayer_Cocoa : public com::glu::platform::components::CMediaPlayer3d {
public:
    /** Audio session category used while the game runs. */
    enum eAudioMode {
        AudioMode_Solo = 0,         // kAudioSessionCategory_SoloAmbientSound
        AudioMode_Ambient = 1       // kAudioSessionCategory_AmbientSound (user music keeps playing)
    };

    /** Number of buffers queued on the software mixer stream. */
    enum { StreamBufferCount = 3 };

    CMediaPlayer_Cocoa();
    virtual ~CMediaPlayer_Cocoa();

    /** The movie player borrows the audio hardware: these give it back / take it. */
    void PublicDestroySoundEngine();
    void PublicInitializeSoundEngine();

    virtual unsigned char Initialize(unsigned int channels, unsigned int bitsPerSample, unsigned int sampleRate,
                                     unsigned int maxSounds, unsigned int bufferLength,
                                     unsigned char useSoftwareMixer);
    virtual void Destroy();
    virtual void SetVolume(unsigned char volume);
    virtual unsigned char CanSetProperty(GlobalProperty property);
    virtual unsigned char CanGetProperty(GlobalProperty property);
    virtual unsigned char CanSetProperty(SoundProperty property);
    virtual unsigned char CanGetProperty(SoundProperty property);
    virtual void HandleSuspend();
    virtual void HandleUpdate(int deltaTime);

    virtual void InitializeSoundEngine();
    virtual void DestroySoundEngine();
    virtual unsigned char InitializeVibrationEngine();
    virtual void DestroyVibrationEngine();
    virtual unsigned char PlayInternal(com::glu::platform::components::CSoundEvent* event);
    virtual unsigned char VibrateInternal(com::glu::platform::components::CVibrationEvent* event);

    virtual unsigned char SetDopplerFactor();
    virtual unsigned char SetSpeedOfSound();
    virtual unsigned char SetDistanceModel();
    virtual unsigned char SetListenerPosition();
    virtual unsigned char SetListenerVelocity();
    virtual unsigned char SetListenerOrientation();

    /** Another application (iPod) is playing music. */
    virtual unsigned char IsUserMusicPlaying();
    virtual void SetAudioMode(eAudioMode mode);
    /** Play a compressed music file through an AudioQueue; `loop` 0 stops at the end. */
    virtual unsigned char PlayMusic(const wchar_t* fileName, unsigned char loop);
    virtual void StopMusic();
    /** Music volume 0..10. */
    virtual void SetMusicVolume(unsigned char volume);
    virtual unsigned char GetMusicVolume() const;
    virtual unsigned char IsMusicPlaying() const;

    /** Take an OpenAL buffer / source pair from the pool; 0 when the pool is empty. */
    unsigned char AquireBufferAndSource(unsigned int& buffer, unsigned int& source);
    /** Give the pair owning `buffer` back to the pool. */
    unsigned char ReleaseBufferAndSource(unsigned int buffer, unsigned int source);

    unsigned char StreamStart();
    void StreamStop();
    unsigned char SetStreamVolume();
    /** pthread entry of the stream refill thread (argument: the player). */
    static void* ThreadProc(void* player);
    void OnThreadProc();
    /** Mix every software sound event into stream buffer `index`. */
    void FillBuffer(int index);

    unsigned char m_soundEngineInitialized;                 // 0xb8
    int m_streamFormat;                                     // 0xbc AL_FORMAT_* of the mixer stream
    pthread_t m_streamThread;                               // 0xc0
    unsigned int m_streamBuffers[StreamBufferCount];        // 0xc4 OpenAL buffers
    unsigned char** m_streamData;                           // 0xd0 StreamBufferCount mixing buffers
    unsigned int m_streamSource;                            // 0xd4 OpenAL source of the stream
    unsigned char m_streaming;                              // 0xd8
    unsigned char m_streamThreadStopped;                    // 0xd9 refill thread gave up (meaning guessed)
    com::glu::platform::core::CLinkList m_freePool;         // 0xdc CALPoolObject nodes ready to use
    com::glu::platform::core::CLinkList m_usedPool;         // 0xec CALPoolObject nodes owned by sounds
    eAudioMode m_audioMode;                                 // 0xfc
    unsigned char m_musicVolume;                            // 0x100 0..10
};

#endif
