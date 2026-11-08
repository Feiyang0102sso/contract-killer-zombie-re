// ICMoviePlayer.h - full-screen movie playback service (singleton interface).
// (drafted by b006, completed by b003 - owner CMoviePlayer_Cocoa_mm.mm)
#ifndef ICMOVIEPLAYER_H
#define ICMOVIEPLAYER_H

#include <stddef.h>
#include <wchar.h>
#include "CSingleton.h"

namespace com { namespace glu { namespace platform { namespace components {

class ICMoviePlayer : public CSingleton {
public:
    /** Singleton id: CStringToKey("ICMoviePlayer"). */
    static const unsigned int ClassId = 0xf0f714a2;

    /** Settable/readable movie properties (meaning of 0 and 1 inferred: read-only size). */
    enum MovieProperty {
        MovieWidth = 0,
        MovieHeight = 1,
        SrcRectX = 2,
        SrcRectY = 3,
        SrcRectDX = 4,
        SrcRectDY = 5,
        DestRectX = 6,
        DestRectY = 7,
        DestRectDX = 8,
        DestRectDY = 9,
        TouchSkippable = 10,
        AudioDelay = 11,
        Scaling = 12,
        Rotation = 13
    };

    ICMoviePlayer()
        : CSingleton(ClassId)
    {
    }

    /** Platform factory (CMoviePlayer_Cocoa). */
    static ICMoviePlayer* CreateInstance();

    /** The application's movie player: cached in CApp, created on first use. */  // added by b057x
    static inline ICMoviePlayer* GetInstance()  // added by b057x
    {  // added by b057x
        framework::CApp* app = CApplet::m_pApp;  // added by b057x
        ICMoviePlayer* instance;  // added by b057x
        if (app != NULL) {  // added by b057x
            instance = app->m_moviePlayer;  // changed by fx2 (CApp::m_moviePlayer is typed now, same form as ICMediaPlayer)
            if (instance == NULL) {  // added by b057x
                ICMoviePlayer* found = NULL;  // added by b057x
                app->m_singletons->Find(ClassId, (void**)&found);  // added by b057x
                instance = found;  // added by b057x
                if (instance == NULL) {  // added by b057x
                    instance = CreateInstance();  // added by b057x
                }  // added by b057x
                app->m_moviePlayer = instance;  // added by b057x
            }  // added by b057x
        } else {  // added by b057x
            instance = NULL;  // added by b057x
        }  // added by b057x
        return instance;  // added by b057x
    }  // added by b057x

    virtual ~ICMoviePlayer();

    virtual void SetSoundEnabled(unsigned char enabled) = 0;
    virtual void SetVolume(unsigned char volume) = 0;
    virtual unsigned char GetSoundEnabled() = 0;
    virtual unsigned char GetVolume() = 0;
    virtual unsigned char IsPlaying(unsigned int handle) = 0;
    /** Start a movie; returns its handle, 0 on failure. */
    virtual unsigned int Play(const wchar_t* fileName, unsigned int flags, const wchar_t* audioFileName) = 0;
    /** Stop the movie `handle` (0: whatever is playing). */
    virtual void Stop(unsigned int handle) = 0;
    virtual unsigned char CanSetProperty(MovieProperty property) = 0;
    virtual unsigned char CanGetProperty(MovieProperty property) = 0;
    virtual unsigned char SetProperty(unsigned int handle, MovieProperty property, int value) = 0;
    virtual unsigned char GetProperty(unsigned int handle, MovieProperty property, int& value) = 0;
    virtual unsigned char HandleUpdate(int elapsed) = 0;
};

/**
 * Forget the cached instance in CApp. Defined out of the class: the destructor is the key function,
 * so every user emits the (weak) vtable and destructors, as in the original.
 */
inline ICMoviePlayer::~ICMoviePlayer()
{
    if (CApplet::m_pApp != NULL) {
        CApplet::m_pApp->m_moviePlayer = NULL;
    }
}

} } } }

#endif
