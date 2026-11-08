// CDH_RootNode.cpp - root window: starts the game scene, exits the application.
#include "CDH_RootNode.h"
#include "CGameData.h"
#include "CGameScene.h"
#include "ICMediaPlayer.h"
#include "ICCore.h"

using namespace com::glu::platform::components;

CDH_RootNode::CDH_RootNode()
    : Window(false)
{
}

CDH_RootNode::~CDH_RootNode()
{
}

void CDH_RootNode::Init()
{
    CGameData::GetInstance()->m_resBank = &m_resBank;
    ChangeState(STATE_GAME_SCENE, STATE_PUSH);
}

void CDH_RootNode::Shutdown()
{
    m_resBank.removeAllKeysets();
}

void CDH_RootNode::OnCreateState(Window** window, int state)
{
    Window* created = NULL;
    switch (state) {
    case STATE_GAME_SCENE:
        ICMediaPlayer::GetInstance()->Stop(0);
        ICMediaPlayer::GetInstance()->StopVibrate(0);
        created = new CGameScene();
        break;
    case STATE_EXIT:
        CApplet::m_pCore->ExitApp((com::glu::platform::framework::ICCore::eExitStatus)0);
        break;
    }
    *window = created;
}

void CDH_RootNode::OnStateInterrupt(int state, Window* window)
{
    if (state == STATE_GAME_SCENE) {
        ChangeState(2, STATE_REPLACE);
    }
}
