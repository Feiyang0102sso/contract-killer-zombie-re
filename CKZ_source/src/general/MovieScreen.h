// MovieScreen.h - plays a full-screen movie (intro / logo) through the platform movie player.
#ifndef MOVIESCREEN_H
#define MOVIESCREEN_H

#include "CStrWChar.h"

class CMovieScreen {
public:
    CMovieScreen();
    virtual ~CMovieScreen();

    /** Start playing; always returns 0. */
    virtual int Start();
    virtual void Stop();
    /** Drive the player while the movie runs; always returns 1. */
    virtual int HandleUpdate(int elapsed);

    void SetFileName(const wchar_t* fileName);
    void SetAudioFileName(const wchar_t* fileName);
    void SetMimeKey(unsigned int mimeKey);
    bool IsFinished() const;

    com::glu::platform::components::CStrWChar m_fileName;          // 0x04
    com::glu::platform::components::CStrWChar m_audioFileName;     // 0x14
    unsigned int m_mimeKey;     // 0x24 passed to the player as play flags
    unsigned int m_movie;       // 0x28 player handle of the movie (never assigned in this file)
};

#endif
