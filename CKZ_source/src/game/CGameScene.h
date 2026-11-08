// CGameScene.h - the game scene: loading screen, game screen and the pause screens.
#ifndef CGAMESCENE_H
#define CGAMESCENE_H

#include "WindowBase.h"
#include "CResBank.h"

class CLoadingScreen;

/** Game scene (0xd4 bytes). States: 7 loading, 8 game screen, 9 pause, 10 pause menu (sound off). */
class CGameScene : public Window {
public:
    enum EState {
        STATE_DEMO = 1,
        STATE_LOADING = 7,
        STATE_GAME = 8,
        STATE_PAUSE = 9,
        STATE_PAUSE_MENU = 10
    };

    CGameScene();
    virtual ~CGameScene();

    virtual void Init();
    virtual void Shutdown();
    virtual void OnCommand(Event& event);
    virtual void Update();
    virtual void OnPause();
    virtual void OnCreateState(Window** window, int state);
    virtual void OnStateInterrupt(int state, Window* window);
    /** Platform event hook (nothing handled here). */
    virtual bool HandleEvent(unsigned int type, unsigned int param1, unsigned int param2);

    /** Queue the game data loading steps. */
    void FillGameLoadingTasks(CLoadingScreen* loadingScreen);

    CResBank m_resBank;         // 0xac
    bool m_gserveStarted;       // 0xd0 the light GServe login was started by the loading step
};

#endif
