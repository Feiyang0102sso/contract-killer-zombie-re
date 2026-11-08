// CGPSMapScreen.cpp - screen hosting the map game.
#include "CGPSMapScreen.h"
#include "App.h"
#include "DGHelper.h"
#include "CDH_SoundManager.h"
#include "CBH_Player.h"
#include "CGPSMapGame.h"
#include "CLoadingScreen.h"
#include "CTutorialManager.h"
#include "CUpdateManager.h"
#include "CHuntingInfo.h"
#include "CMissionManager.h"
#include "CMission.h"
#include "CAfterGameStatsWindow.h"

/** Id given to the loading screen (hashed name). */
static const unsigned int LOADING_SCREEN_MUSIC_ID = 0x451967d0;
// b074i: the historical name above is kept; original 11d4cc corresponds to the tunnel command for load completion.

CGPSMapScreen::CGPSMapScreen()
    : Window(false)
{
    ((App*)WindowApp::m_instance)->m_helper->initGPSScreen(this);
}

CGPSMapScreen::~CGPSMapScreen()
{
    ((App*)WindowApp::m_instance)->m_soundManager->StopMusic();
    CBH_Player::GetInstance()->Save(true);
    ((App*)WindowApp::m_instance)->m_soundManager->StopAllSoundsAndRelease();
}

void CGPSMapScreen::Init()
{
    CGPSMapGame* game = new CGPSMapGame();
    m_fieldb0 = -1;
    m_mapGame = game;
    if (!((App*)WindowApp::m_instance)->m_tutorialManager->IsAutoStartMission()) {
        AddNewState(STATE_LOADING);
        return;
    }
    m_mapGame->ClearFlags(1);
    AddToFront(m_mapGame);
}

void CGPSMapScreen::OnPointerEvent(Event& event)
{
    switch (event.m_type) {
    case Event::POINTER_DRAGGED:
        m_pointer.TouchMove(event.m_param1, event.m_param2);
        break;
    case Event::POINTER_PRESSED:
        m_pointer.TouchDown(event.m_param1, event.m_param2);
        break;
    case Event::POINTER_RELEASED:
        m_pointer.TouchUp();
        return;
    default:
        return;
    }
    event.Clear();
}

void CGPSMapScreen::Update()
{
    if (m_currentState == STATE_LOADING) {
        return;
    }
    if (!((App*)WindowApp::m_instance)->m_tutorialManager->IsAutoStartMission() &&
        !((App*)WindowApp::m_instance)->m_soundManager->IsPlaying(CDH_SoundManager::SOUND_MUSIC_ALL)) {
        ((App*)WindowApp::m_instance)->m_soundManager->Play(CDH_SoundManager::SOUND_MUSIC_ALL, false, false);
    }
    m_pointer.TouchUpdate(WindowApp::m_instance->m_updateFrameTime);
    PointerEventController::Event event;  // filled field by field (store order of the original)
    event.m_position.x = 0;
    event.m_position.y = 0;
    event.m_deltaX = 0; event.m_deltaY = 0; event.m_type = PointerEventController::EVENT_NONE;
    while (m_pointer.GetEvent(event)) {
        if (m_firstChild == (Window*)m_mapGame) {
            m_mapGame->OnPointerEvent(event);
        }
    }
    CBH_Player::GetInstance()->Update();
}

void CGPSMapScreen::OnCreateState(Window** window, int state)
{
    CLoadingScreen* screen = NULL;
    if (state == STATE_LOADING) {
        CUpdateManager* updates = ((App*)WindowApp::m_instance)->m_updateManager;
        if (updates->GetState() == 1) {
            ((App*)WindowApp::m_instance)->m_updateManager->SetState(0);
        }
        screen = new CLoadingScreen(false);
        FillMapLoadingTasks(screen, m_mapGame);
        screen->SetFinishCmd(LOADING_SCREEN_MUSIC_ID);
        if (CBH_Player::GetInstance()->GetGameMode() == 2) {
            XString missionName = ((App*)WindowApp::m_instance)->m_huntingInfo->m_assignedData.m_missionName;
            CMission* mission = ((App*)WindowApp::m_instance)->m_missionManager->GetMissionByName(missionName);
            if (mission != NULL && mission->m_type == 4) {
                screen->SetInfoWindow(new CAfterGameStatsWindow());
                screen->m_field102 = false;
                screen->SetBackground("IDB_LOADING_SCREEN_05");
            }
        }
    }
    *window = screen;
}

void CGPSMapScreen::OnStateInterrupt(int state, Window* window)
{
    if (state != STATE_LOADING) {
        return;
    }
    PopState();
    if (!((App*)WindowApp::m_instance)->m_tutorialManager->IsAutoStartMission() &&
        !((App*)WindowApp::m_instance)->m_soundManager->IsPlaying(CDH_SoundManager::SOUND_MUSIC_ALL)) {
        ((App*)WindowApp::m_instance)->m_soundManager->Play(CDH_SoundManager::SOUND_MUSIC_ALL, false, false);
    }
    AddToFront((Window*)m_mapGame);
}
