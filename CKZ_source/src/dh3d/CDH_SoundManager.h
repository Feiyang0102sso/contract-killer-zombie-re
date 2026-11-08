// CDH_SoundManager.h - sounds and music of the game (media player events per sound id) and streamed tracks.
#ifndef CDH_SOUNDMANAGER_H
#define CDH_SOUNDMANAGER_H

#include <stddef.h>
#include "CClass.h"
#include "CSharedPtr.h"
#include "Vector_ss.h"

namespace com { namespace glu { namespace platform { namespace components {
class ICMediaPlayer;
} } } }

/** A streamed music track; fades the other music while it plays. */
class TrackHandler {
public:
    /** m_state values. */
    enum {
        STATE_STOPPED = 0,
        STATE_PLAYING = 1,
        STATE_FADING_OUT = 2
    };

    TrackHandler(com::glu::platform::components::ICMediaPlayer* player, const char* name, bool fadeMusic);

    /** The original owning control block stops the track before freeing it. */
    ~TrackHandler() { stop(); }

    /** Fade-out step. */
    void update(int elapsedMs);
    void stop();
    /** Fade out within `durationMs` (only while playing). */
    void fadeOut(unsigned long durationMs);

    int m_state;                                                // 0x00
    com::glu::platform::components::ICMediaPlayer* m_player;    // 0x04
    unsigned int m_resourceHandle;                              // 0x08 0: released
    unsigned int m_eventId;                                     // 0x0c media player event
    unsigned long m_fadeDuration;                               // 0x10
    unsigned long m_fadeLeft;                                   // 0x14
};

class CDH_SoundManager : public com::glu::platform::core::CClass {
public:
    enum { ClassId = 0x7ba2290e };

    /** Sound ids: index of SOUND_TO_RESOURCE_REFLECTION; the last four are music. */
    enum eSound {
        SOUND_MUSIC_FIRST = 0x9c,
        SOUND_MUSIC_ALL = 0x9c,     // menu music (paused / resumed with the application)
        SOUND_COUNT = 0xa0,
        MUSIC_COUNT = SOUND_COUNT - SOUND_MUSIC_FIRST
    };

    /** A sound played after a delay. */
    struct SDeferredPlayback {
        eSound m_sound;     // 0x00
        float m_delay;      // 0x04 seconds left
        bool m_exclusive;   // 0x08 Play() argument
    };

    /** Resource id of every sound ("" for unused ids). */
    static const char* SOUND_TO_RESOURCE_REFLECTION[SOUND_COUNT];

    CDH_SoundManager();
    virtual ~CDH_SoundManager();

    /** Sound id of resource `resourceId` (SOUND_COUNT when unknown). */
    static int GetSoundFromResourceID(const char* resourceId);
    void initResource(eSound sound);
    void StopAllSoundsAndRelease();
    void releaseResources();
    /** Stream track `name` (NULL handler when sound is off). */
    com::glu::platform::core::CSharedPtr<TrackHandler> PlayTrack(const char* name, bool fadeMusic);
    void Update(unsigned long elapsedMs);
    /** Apply the sound / vibration / volume settings to the media player. */
    void UpdateMediaPlayerSettings();
    /** Play `sound` (not again while playing when `exclusive`); music and `loop` sounds loop. */
    void Play(eSound sound, bool exclusive, bool loop);
    void SetMusicLevel(unsigned long level);
    unsigned char IsPlaying(eSound sound);
    /** Duration of `sound` for the given sample size and rate. */
    float GetSoundDuration(eSound sound, unsigned int bytesPerSample, unsigned int sampleRate);
    void StopMusic();
    void StopSounds();
    void Stop(eSound sound);
    void Pause(eSound sound);
    void Resume(eSound sound);
    /** Random sound of key set `keySet`. */
    int GetRandomSound(const char* keySet);
    /** Play a random sound of `keySet` after `delay` seconds. */
    /** Returns the first random id; the second random draw is the one queued (0x162c46/96). */
    int PlayRandomSoundWithDelay(const char* keySet, float delay, bool exclusive);
    void PrecacheSound(eSound sound);
    void PrecacheKeysetSounds(const char* keySet);
    /** Pause / resume everything. */
    void Pause();
    void Resume();
    void SetActivated(bool activated);

    void** m_sounds;                                                        // 0x08 CMedia* per sound id (loaded on use)
    unsigned int m_eventIds[SOUND_COUNT];                                   // 0x0c last media player event per sound
    Vector<com::glu::platform::core::CSharedPtr<TrackHandler> > m_tracks;   // 0x28c
    Vector<SDeferredPlayback> m_deferred;                                   // 0x29c sorted by delay
    bool m_activated;                                                       // 0x2ac
    unsigned long m_musicLevel;                                             // 0x2b0 0..100
    unsigned long m_targetMusicLevel;                                       // 0x2b4
    unsigned long m_musicFadeSpeed;                                         // 0x2b8 level units per second (0: no fade)
};

#endif
