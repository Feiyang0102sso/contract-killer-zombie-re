// CGPSMapScreen.h - screen hosting the map game: loading state, gestures and the menu music.
#ifndef CGPSMAPSCREEN_H
#define CGPSMAPSCREEN_H

#include "WindowBase.h"
#include "PointerEventController.h"

class CGPSMapGame;

class CGPSMapScreen : public Window {
public:
    /** Window state ids. */
    enum {
        STATE_LOADING = 1
    };

    CGPSMapScreen();
    virtual ~CGPSMapScreen();
    virtual void Init();
    virtual void OnPointerEvent(Event& event);
    virtual void Update();
    virtual void OnCreateState(Window** window, int state);
    virtual void OnStateInterrupt(int state, Window* window);

    int m_fieldac;                          // 0xac not touched here
    int m_fieldb0;                          // 0xb0 -1 after Init
    CGPSMapGame* m_mapGame;                 // 0xb4
    PointerEventController m_pointer;       // 0xb8
};

#endif
