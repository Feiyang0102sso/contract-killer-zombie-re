// CMoviePlayer.h - platform-independent movie player and its per-movie event (created by b006).
// CMoviePlayer owns at most one CMovieEvent; the platform subclass of CMovieEvent
// (CMovieEvent::CreateInstance) does the actual playback.
#ifndef CMOVIEPLAYER_H
#define CMOVIEPLAYER_H

#include "ICMoviePlayer.h"
#include "CStrWChar.h"

namespace com { namespace glu { namespace platform {

namespace systems {
class CEvent;
class CEventListener;
}

namespace components {

class CMoviePlayer;

/** One movie being played. Property setters forward to the platform hooks. */
class CMovieEvent {
public:
    /** Playback state value of a movie that has finished. */
    static const int StateFinished = 4;

    /** Platform implementation (defined by the platform layer). */
    static CMovieEvent* CreateInstance();

    CMovieEvent();
    virtual ~CMovieEvent();

    void SetFileName(const wchar_t* fileName);
    void SetAudioFileName(const wchar_t* fileName);

    /** Volume 0..10. */
    virtual void SetVolume(unsigned char volume);
    virtual unsigned char SetProperty(ICMoviePlayer::MovieProperty property, int value);
    virtual unsigned char GetProperty(ICMoviePlayer::MovieProperty property, int& value);

    // platform hooks
    virtual unsigned char Initialize() = 0;
    virtual void Destroy() = 0;
    virtual unsigned char Play() = 0;
    virtual unsigned char Stop() = 0;  // return type corrected by b003 (CMovieEvent_Cocoa returns a status)
    virtual unsigned char Pause() = 0;  // return type corrected by b003
    virtual unsigned char Resume() = 0;  // return type corrected by b003
    virtual unsigned char HandleUpdate(int elapsed) = 0;  // return type corrected by b003
    virtual int GetLastErr() = 0;
    /** Clamp `value` before it is stored by SetProperty (default: no limit). */
    virtual void LimitPropertyValue(ICMoviePlayer::MovieProperty property, int& value);
    virtual unsigned char SetSrcRectX() = 0;
    virtual unsigned char SetSrcRectY() = 0;
    virtual unsigned char SetSrcRectDX() = 0;
    virtual unsigned char SetSrcRectDY() = 0;
    virtual unsigned char SetDestRectX() = 0;
    virtual unsigned char SetDestRectY() = 0;
    virtual unsigned char SetDestRectDX() = 0;
    virtual unsigned char SetDestRectDY() = 0;
    virtual unsigned char SetTouchSkippable() = 0;
    virtual unsigned char SetAudioDelay() = 0;
    virtual unsigned char SetScaling() = 0;
    virtual unsigned char SetRotation() = 0;

    CMoviePlayer* m_player;         // 0x04 owner
    unsigned int m_handle;          // 0x08 value returned by CMoviePlayer::Play
    CStrWChar m_fileName;           // 0x0c
    CStrWChar m_audioFileName;      // 0x1c
    unsigned int m_flags;           // 0x2c Play() flags
    unsigned char m_volume;         // 0x30 0..10
    int m_state;                    // 0x34 platform playback state (StateFinished = done)
    int m_width;                    // 0x38 MovieWidth
    int m_height;                   // 0x3c MovieHeight
    int m_srcRectX;                 // 0x40
    int m_srcRectY;                 // 0x44
    int m_srcRectDX;                // 0x48
    int m_srcRectDY;                // 0x4c
    int m_destRectX;                // 0x50
    int m_destRectY;                // 0x54
    int m_destRectDX;               // 0x58
    int m_destRectDY;               // 0x5c
    unsigned char m_touchSkippable; // 0x60
    int m_audioDelay;               // 0x64
    int m_scaling;                  // 0x68
    int m_rotation;                 // 0x6c degrees, 0..359
};

/** ICMoviePlayer implementation; listens to the system pause/resume/stop events. */
class CMoviePlayer : public ICMoviePlayer {
public:
    /** Maximum volume. */
    static const unsigned char MaxVolume = 10;

    CMoviePlayer();
    virtual ~CMoviePlayer();

    /** CEventListener callback: forwards to HandleEvent of the listener's player. */
    static unsigned char EventCB(systems::CEvent& event, void* listener);

    virtual void SetSoundEnabled(unsigned char enabled);
    virtual void SetVolume(unsigned char volume);
    virtual unsigned char GetSoundEnabled();
    virtual unsigned char GetVolume();
    virtual unsigned char IsPlaying(unsigned int handle);
    virtual unsigned int Play(const wchar_t* fileName, unsigned int flags, const wchar_t* audioFileName);
    virtual void Stop(unsigned int handle);
    virtual unsigned char CanSetProperty(MovieProperty property);
    virtual unsigned char CanGetProperty(MovieProperty property);
    virtual unsigned char SetProperty(unsigned int handle, MovieProperty property, int value);
    virtual unsigned char GetProperty(unsigned int handle, MovieProperty property, int& value);
    virtual unsigned char HandleUpdate(int elapsed);

    unsigned char HandleEvent(unsigned int eventId, unsigned int param, unsigned int context);

    systems::CEventListener* m_listener;    // 0x08
    unsigned char m_soundEnabled;           // 0x0c
    unsigned char m_volume;                 // 0x0d 0..10
    unsigned int m_lastHandle;              // 0x10 handle given to the last movie
    unsigned char m_paused;                 // 0x14 paused by a system event
    CMovieEvent* m_event;                   // 0x18 current movie
};

} } } }

#endif
