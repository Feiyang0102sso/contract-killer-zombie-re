// ICMediaPlayer.h - platform sound/vibration player interface; one instance per application (singleton).
// (drafted by b005, confirmed by b003 against the CMediaPlayer_Cocoa vtable; implemented by CMediaPlayer_Cocoa)
// Slot order follows the vtables of CMediaPlayer / CMediaPlayer_Cocoa (ICMediaPlayer ends at GetBufferSize).
#ifndef ICMEDIAPLAYER_H
#define ICMEDIAPLAYER_H

#include "CSingleton.h"

namespace com { namespace glu { namespace platform { namespace components {

class CBinary;
class CMedia;

class ICMediaPlayer : public CSingleton {
public:
    enum { SingletonId = 0xf4f71410 };

    /** Player wide properties (Set/GetProperty*, F = 16.16 fixed point, FV = fixed point vector). */
    enum GlobalProperty {
        GlobalProperty_DopplerFactor = 0,           // F
        GlobalProperty_SpeedOfSound = 1,            // F
        GlobalProperty_DistanceModel = 2,           // int, 0..6
        GlobalProperty_ListenerPosition = 3,        // FV[3]
        GlobalProperty_ListenerVelocity = 4,        // FV[3]
        GlobalProperty_ListenerOrientation = 5      // FV[6] "at" + "up"
    };

    /** Per sound event properties. */
    enum SoundProperty {
        SoundProperty_Volume = 0,                   // int, 0..100
        SoundProperty_Pitch = 1,                    // int, -100..100
        SoundProperty_Pan = 2,                      // int, -100..100
        SoundProperty_OffsetBytes = 3,              // int
        SoundProperty_OffsetMS = 4,                 // int
        SoundProperty_RelativeToListener = 5,       // int, 0..1
        SoundProperty_MinGain = 6,                  // F
        SoundProperty_MaxGain = 7,                  // F
        SoundProperty_MaxDistance = 8,              // F
        SoundProperty_ReferenceDistance = 9,        // F
        SoundProperty_RolloffFactor = 10,           // F
        SoundProperty_ConeOuterGain = 11,           // F
        SoundProperty_ConeInnerAngle = 12,          // F, degrees
        SoundProperty_ConeOuterAngle = 13,          // F, degrees
        SoundProperty_Position = 14,                // FV[3]
        SoundProperty_Velocity = 15,                // FV[3]
        SoundProperty_Direction = 16                // FV[3]
    };

    /** Encoding of the data handed to PlayStream(). */
    enum SoundStreamType {
        SoundStreamType_PCM = 0,
        SoundStreamType_ADPCM = 1,
        SoundStreamType_OGG = 2
    };

    ICMediaPlayer() : CSingleton(SingletonId) {}

    virtual ~ICMediaPlayer();

    /** Platform factory (CMediaPlayer_Cocoa). */
    static ICMediaPlayer* CreateInstance();

    /** The application's media player: cached in CApp, created on first use. */
    static inline ICMediaPlayer* GetInstance()
    {
        framework::CApp* app = CApplet::m_pApp;
        ICMediaPlayer* instance;
        if (app != NULL) {
            instance = app->m_mediaPlayer;
            if (instance == NULL) {
                ICMediaPlayer* found = NULL;
                app->m_singletons->Find(SingletonId, (void**)&found);
                instance = found;
                if (instance == NULL) {
                    instance = CreateInstance();
                }
                app->m_mediaPlayer = instance;
            }
        } else {
            instance = NULL;
        }
        return instance;
    }

    virtual unsigned char Initialize(unsigned int channels, unsigned int bitsPerSample, unsigned int sampleRate,
                                     unsigned int maxSounds, unsigned int bufferLength,
                                     unsigned char useSoftwareMixer) = 0;
    virtual void Destroy() = 0;
    virtual unsigned char GetDescription(unsigned int& channels, unsigned int& bitsPerSample, unsigned int& sampleRate,
                                         unsigned int& maxSounds, unsigned int& bufferLength,
                                         unsigned char& useSoftwareMixer) = 0;
    virtual void SetSoundEnabled(unsigned char enabled) = 0;
    virtual void SetVibrationEnabled(unsigned char enabled) = 0;
    virtual void SetVolume(unsigned char volume) = 0;
    virtual unsigned char GetSoundEnabled() const = 0;
    virtual unsigned char GetVibrationEnabled() const = 0;
    virtual unsigned char GetVolume() const = 0;

    virtual unsigned char CanSetProperty(GlobalProperty property) = 0;
    virtual unsigned char CanGetProperty(GlobalProperty property) = 0;
    virtual unsigned char SetProperty(GlobalProperty property, int value) = 0;
    virtual unsigned char SetPropertyF(GlobalProperty property, int value) = 0;
    virtual unsigned char SetPropertyFV(GlobalProperty property, int* values, int count) = 0;
    virtual unsigned char GetProperty(GlobalProperty property, int& value) = 0;
    virtual unsigned char GetPropertyF(GlobalProperty property, int& value) = 0;
    virtual unsigned char GetPropertyFV(GlobalProperty property, int* values, int count) = 0;

    /** eventId 0 means "any event". */
    virtual unsigned char IsPlaying(unsigned int eventId) const = 0;
    virtual unsigned char IsPaused(unsigned int eventId) const = 0;
    virtual unsigned char IsVibrating(unsigned int eventId) const = 0;
    virtual unsigned char IsVibrationPaused(unsigned int eventId) const = 0;

    /** Start playing `media`; returns the event id, 0 on failure. */
    virtual unsigned int Play(CMedia* media, unsigned char loop, unsigned char priority) = 0;
    virtual unsigned int PlayStream(CBinary* binary, SoundStreamType type, unsigned char loop,
                                    unsigned char priority) = 0;
    virtual void Stop(unsigned int eventId) = 0;
    virtual void Pause(unsigned int eventId) = 0;
    virtual void Resume(unsigned int eventId) = 0;

    virtual unsigned char CanSetProperty(SoundProperty property) = 0;
    virtual unsigned char CanGetProperty(SoundProperty property) = 0;
    virtual unsigned char SetProperty(unsigned int eventId, SoundProperty property, int value) = 0;
    virtual unsigned char SetPropertyF(unsigned int eventId, SoundProperty property, int value) = 0;
    virtual unsigned char SetPropertyFV(unsigned int eventId, SoundProperty property, int* values, int count) = 0;
    virtual unsigned char GetProperty(unsigned int eventId, SoundProperty property, int& value) = 0;
    virtual unsigned char GetPropertyF(unsigned int eventId, SoundProperty property, int& value) = 0;
    virtual unsigned char GetPropertyFV(unsigned int eventId, SoundProperty property, int* values, int count) = 0;

    /** Vibrate for `duration`; returns the event id, 0 on failure. */
    virtual unsigned int Vibrate(unsigned int duration, unsigned char priority) = 0;
    virtual void StopVibrate(unsigned int eventId) = 0;
    virtual void PauseVibrate(unsigned int eventId) = 0;
    virtual void ResumeVibrate(unsigned int eventId) = 0;

    virtual void HandleSuspend() = 0;
    virtual void HandleResume() = 0;
    virtual void HandleUpdate(int deltaTime) = 0;
    /** Size in bytes of one mixing buffer. */
    // b118: the original ICMediaPlayer ends at HandleUpdate; GetBufferSize is a virtual function added by CMediaPlayer.
};

/** Forget the cached instance in CApp. */
inline ICMediaPlayer::~ICMediaPlayer()
{
    if (CApplet::m_pApp != NULL) {
        CApplet::m_pApp->m_mediaPlayer = NULL;
    }
}

} } } }

#endif
