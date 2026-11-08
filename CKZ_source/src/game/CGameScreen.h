// CGameScreen.h - the game screen: switches between the GPS map, the intermediate screen and the 3D game.
#ifndef CGAMESCREEN_H
#define CGAMESCREEN_H

#include "WindowBase.h"

/** Game screen (0xac bytes, no own fields). States: 1 map, 4/5 intermediate, 6 3D game. */
class CGameScreen : public Window {
public:
    /** Child states of the game screen. */
    enum EState {
        STATE_MAP = 1,
        STATE_INTERMEDIATE_IN = 4,
        STATE_INTERMEDIATE_OUT = 5,
        STATE_GAME = 6
    };

    CGameScreen();
    virtual ~CGameScreen();

    virtual void Init();
    virtual void Shutdown();
    virtual void OnCommand(Event& event);
    virtual void OnCreateState(Window** window, int state);
    virtual void OnStateInterrupt(int state, Window* window);
    virtual void Activate();
    virtual void Deactivate();
};

#endif
