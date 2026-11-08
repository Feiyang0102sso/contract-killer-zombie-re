// SplashWindow.h - start screens: the Glu logo movie, the game intro movie, then the game.
// Owner: src/general/SplashWindow.cpp (b058; first draft by b057x).
// Slot order from the vtables of SplashWindow / SplashWindow::GluLogo, fields from their constructors.
#ifndef SPLASHWINDOW_H
#define SPLASHWINDOW_H

#include <stddef.h>
#include <wchar.h>
#include "WindowBase.h"

class CMovieScreen;

/** A black window that shows one start movie and closes itself when it is skipped or finished. */
class SplashWindow : public WindowColor {
public:
    /** Kinds of start screen; each one opens the next when it shuts down. */
    enum EType {
        TYPE_GLU_LOGO = 0,      // Glu logo movie, then TYPE_INTRO
        TYPE_INTRO = 1,         // game intro movie, then the game
        TYPE_NONE = 2,          // closes at once and starts the game
        // first names of b057x, still used by App.h
        TYPE_1 = TYPE_INTRO,
        TYPE_2 = TYPE_NONE
    };

    /** Plays a movie file (chosen by the screen resolution) with an optional separate sound file. */
    class GluLogo : public Window {
    public:
        GluLogo(const wchar_t* xgaFileName, const wchar_t* vgaFileName, const wchar_t* fileName,
                const wchar_t* audioFileName)
            : Window(false)
        {
            m_movie = NULL;
            m_xgaFileName = xgaFileName;
            m_vgaFileName = vgaFileName;
            m_fileName = fileName;
            m_audioFileName = audioFileName;
        }

        virtual ~GluLogo()
        {
        }

        virtual void Init();
        virtual void Shutdown();
        virtual void Update();
        virtual void Paint(com::glu::platform::graphics::ICGraphics2d& graphics);

        CMovieScreen* m_movie;              // 0xac
        const wchar_t* m_xgaFileName;       // 0xb0 movie on 1024x768 screens
        const wchar_t* m_vgaFileName;       // 0xb4 movie on 960x640 screens
        const wchar_t* m_fileName;          // 0xb8 movie on the other screens
        const wchar_t* m_audioFileName;     // 0xbc NULL: the movie has its own sound
    };

    SplashWindow(EType type);

    virtual ~SplashWindow()
    {
    }

    virtual void Shutdown();
    virtual void OnCommand(Event& event);
    virtual void OnPointerEvent(Event& event);
    virtual void Update();

    /** Start the network server and show the game (root node). */
    void RunApp();
    /** Close (ease out) once the screen was shown for more than `minTime` seconds. */
    void TryClose(float minTime);

    EType m_type;               // 0xb0
    float m_startTime;          // 0xb4 WindowApp time when the screen was created
    int m_frameCount;           // 0xb8 updates since then
    bool m_soundActivated;      // 0xbc set, never read in this file
    bool m_autoClose;           // 0xbd close by itself after 4 seconds
    bool m_fieldBE;             // 0xbe cleared, never read in this file
};

#endif
