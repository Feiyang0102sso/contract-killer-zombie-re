// CDH_RootNode.h - root window of the game screens (owns the resource bank of the game data).
#ifndef CDH_ROOTNODE_H
#define CDH_ROOTNODE_H

#include "WindowBase.h"
#include "CResBank.h"

/** Root node (0xd0 bytes). States: 3 game scene, 4 exit. */
class CDH_RootNode : public Window {
public:
    enum EState {
        STATE_GAME_SCENE = 3,
        STATE_EXIT = 4
    };

    CDH_RootNode();
    virtual ~CDH_RootNode();

    virtual void Init();
    virtual void Shutdown();
    virtual void OnCreateState(Window** window, int state);
    virtual void OnStateInterrupt(int state, Window* window);

    CResBank m_resBank;         // 0xac
};

#endif
