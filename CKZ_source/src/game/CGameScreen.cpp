// CGameScreen.cpp - the game screen (map / intermediate screen / 3D game states).
#include "CGameScreen.h"
#include "App.h"
#include "CTutorialManager.h"
#include "CMissionManager.h"
#include "CMission.h"
#include "CBH_Player.h"
#include "CHuntingInfo.h"
#include "CSaveManager.h"
#include "CDH_SoundManager.h"
#include "DGHelper.h"
#include "CGPSMapScreen.h"
#include "CIntermediateState.h"
#include "CSwerveGame.h"

/** Command ids (hashed command names). */
static const int CMD_GAME_PAUSE = (int)0x9247fd7b;
static const int CMD_GAME_BACK = (int)0x97204784;
/** Sound played when the 3D game starts (CDH_SoundManager::eSound 0x1e; name not known). */
static const int SOUND_GAME_START = 0x1e;

CGameScreen::CGameScreen()
    : Window(false)
{
}

CGameScreen::~CGameScreen()
{
}

void CGameScreen::Init()
{
    if (((App*)WindowApp::m_instance)->m_tutorialManager->FirstMissionCompleted() == true) {
        ChangeState(STATE_MAP, STATE_REPLACE);
        return;
    }

    // First start: run the first tutorial mission directly.
    const char* missionName = ((App*)WindowApp::m_instance)->m_tutorialManager->GetFirstTutorialMission();
    CMission* mission = ((App*)WindowApp::m_instance)->m_missionManager->GetMissionByName(XString(missionName));
    CBH_Player::GetInstance()->SetTutorialState(1);

    CHuntingInfo::SAssignedData data;
    data.m_missionName = mission->GetName();
    data.m_questId = ((App*)WindowApp::m_instance)->m_tutorialManager->GetActiveQuestId();
    CBH_Player* player = CBH_Player::GetInstance();
    player->FieldAt(0x5c) = 0;      // game mode
    player->Save(true);
    CBH_Player::GetInstance()->SetFieldAt(0x60, -1);
    CBH_Player::GetInstance()->SetFieldAt(0x64, data.m_questId);   // active quest
    ((App*)WindowApp::m_instance)->m_huntingInfo->SetHuntingData(data);
    mission->OnMissionStarted();
    CBH_Player::GetInstance()->FieldAt(0xe0)++;
    ChangeState(STATE_GAME, STATE_REPLACE);
}

void CGameScreen::Shutdown()
{
}

void CGameScreen::Activate()
{
}

void CGameScreen::Deactivate()
{
}

void CGameScreen::OnCommand(Event& event)
{
    int id = event.m_id;
    if (id != CMD_GAME_PAUSE && id != CMD_GAME_BACK) {
        return;
    }
    if (m_currentState == STATE_GAME) {
        // Pause the running 3D game (not while a dialog is up or the game is already paused).
        App* app = (App*)WindowApp::m_instance;
        CSwerveGame* game = app->m_helper->m_swerveGame;
        if ((game == NULL || game->m_currentState == 2) && app->m_timePauseCount == 0 && app->m_timeScale != 0.0f) {
            SetInterrupt(3);
        }
    } else if (id != CMD_GAME_BACK) {
        SetInterrupt(2);
    }
    event.Clear();
}

void CGameScreen::OnCreateState(Window** window, int state)
{
    Window* created = NULL;
    switch (state) {
    case STATE_MAP:
        created = new CGPSMapScreen();
        break;
    case STATE_INTERMEDIATE_IN:
    case STATE_INTERMEDIATE_OUT:
        created = new CIntermediateState();
        break;
    case STATE_GAME:
        ((App*)WindowApp::m_instance)->m_saveManager->PauseSaving();
        if (CBH_Player::GetInstance()->FieldAt(0x1e0) != 0) {
            ((App*)WindowApp::m_instance)->m_soundManager->Play((CDH_SoundManager::eSound)SOUND_GAME_START, false, false);
        }
        created = new CSwerveGame();
        break;
    }
    *window = created;
}

void CGameScreen::OnStateInterrupt(int state, Window* window)
{
    switch (state) {
    case STATE_MAP:
        if (window->GetInterrupt() == 1) {
            ChangeState(STATE_INTERMEDIATE_IN, STATE_REPLACE);
        }
        break;
    case STATE_INTERMEDIATE_IN:
        ChangeState(STATE_GAME, STATE_REPLACE);
        break;
    case STATE_INTERMEDIATE_OUT:
        ChangeState(STATE_MAP, STATE_REPLACE);
        break;
    case STATE_GAME:
        ((App*)WindowApp::m_instance)->m_saveManager->ResumeSaving();
        ((App*)WindowApp::m_instance)->m_huntingInfo->AfterSceneActivity();
        ChangeState(STATE_INTERMEDIATE_OUT, STATE_REPLACE);
        break;
    }
}
