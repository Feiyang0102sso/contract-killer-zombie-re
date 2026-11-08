// CDH_LoadingBarWidget.h - loading bar at the bottom of the loading screen (two stripe images and a text).
#ifndef CDH_LOADINGBARWIDGET_H
#define CDH_LOADINGBARWIDGET_H

#include "WindowBase.h"
#include "CStrWChar.h"

namespace com { namespace glu { namespace platform { namespace graphics {
class CFont;
} } } }

/** Loading bar (0xe4 bytes). */
class CDH_LoadingBarWidget : public Window {
public:
    enum eState {
        STATE_LOADING = 0,      // "IDS_LOADING"
        STATE_CONNECTING = 1,   // "Connecting", blinking
        STATE_TAP = 2           // "IDS_TAP_TO_CONTINUE", blinking
    };

    CDH_LoadingBarWidget();
    virtual ~CDH_LoadingBarWidget()
    {
    }

    virtual void Update();
    virtual void Paint(com::glu::platform::graphics::ICGraphics2d& graphics);

    void setState(eState state);
    /** Progress in percent. */
    void setProgress(int percent)
    {
        m_progress = percent;
    }

    ImageRes m_stripes[2];                              // 0xac background stripe, progress stripe
    com::glu::platform::components::CStrWChar m_text;   // 0xbc
    com::glu::platform::graphics::CFont* m_font;        // 0xcc
    int m_textLength;                                   // 0xd0 characters of m_text
    int m_blinkTime;                                    // 0xd4 ms; the text shows while it is negative
    int m_progress;                                     // 0xd8 percent
    eState m_state;                                     // 0xdc
    bool m_blinking;                                    // 0xe0
};

#endif
