// CGameScene.cpp - the game scene: loads the game data, then runs the game screen and the pause screens.
#include "CGameScene.h"
#include "AdBannerSizes.h"
#include "ICInAppPurchase.h"
#include "App.h"
#include "CDemoMgr.h"
#include "CLoadingScreen.h"
#include "CGameScreen.h"
#include "CDH_PauseScreen.h"
#include "CSaveManager.h"
#include "CTutorialManager.h"
#include "CSwerveGame.h"
#include "DGHelper.h"
#include "CNGSLoginFlow.h"
#include "CNetworkAvailability.h"
#include "ICMediaPlayer.h"

using namespace com::glu::platform::components;

CGameScene::CGameScene()
    : Window(false)
{
    m_gserveStarted = false;
}

CGameScene::~CGameScene()
{
}

void CGameScene::Init()
{
    if (CDemoMgr::GetInstance()->IsDemo()) {
        ChangeState(STATE_DEMO, STATE_PUSH);
    } else {
        ChangeState(STATE_LOADING, STATE_PUSH);
    }
}

void CGameScene::Shutdown()
{
    m_resBank.removeAllKeysets();
}

bool CGameScene::HandleEvent(unsigned int type, unsigned int param1, unsigned int param2)
{
    return false;
}

void CGameScene::Update()
{
    Window::Update();
}

void CGameScene::FillGameLoadingTasks(CLoadingScreen* loadingScreen)
{
    /** Game data that does not depend on the player profile. */
    class AppInitPlayerIndependentGameData : public CLoadingScreen::IStep {
    public:
        virtual bool Load(CLoadingScreen* screen)
        {
            ((App*)WindowApp::m_instance)->InitPlayerIndependentGameData();
            return true;
        }
    };

    loadingScreen->AddLoadingStep(new AppInitPlayerIndependentGameData(), "AppInitPlayerIndependentGameData");
    if (WindowApp::m_instance->m_restartDelay != 2) {
        /** Game data after the light GServe login (waits while the login flow runs). */
        class AppInitGameDataGS : public CLoadingScreen::IStep {
        public:
            AppInitGameDataGS(bool* gserveStarted)
            {
                m_gserveStarted = gserveStarted;
            }

            virtual bool Load(CLoadingScreen* screen)
            {
                if (!*m_gserveStarted) {
                    if (((App*)WindowApp::m_instance)->GetNetworkAvailability()->IsAvailable()) {
                        if (!CNGSLoginFlow::GetInstance()->IsRunning()) {
                            ((App*)WindowApp::m_instance)->StartGServerLight();
                            *m_gserveStarted = true;
                        }
                    }
                }
                if (*m_gserveStarted) {
                    if (!((App*)WindowApp::m_instance)->GetNetworkAvailability()->IsAvailable() ||
                        !CNGSLoginFlow::GetInstance()->IsRunning()) {
                        *m_gserveStarted = false;
                    }
                }
                if (*m_gserveStarted) {
                    return false;
                }
                ((App*)WindowApp::m_instance)->m_saveManager->PauseByGServe(0);
                ((App*)WindowApp::m_instance)->InitGameData();
                return true;
            }

            bool* m_gserveStarted;      // 0x04 CGameScene::m_gserveStarted
        };

        loadingScreen->AddLoadingStep(new AppInitGameDataGS(&m_gserveStarted), "AppInitGameDataGS");
    } else {
        /** Game data without GServe (application restart). */
        class AppInitGameData : public CLoadingScreen::IStep {
        public:
            AppInitGameData(bool* gserveStarted)
            {
                m_gserveStarted = gserveStarted;
            }

            virtual bool Load(CLoadingScreen* screen)
            {
                ((App*)WindowApp::m_instance)->m_saveManager->PauseByGServe(0);
                ((App*)WindowApp::m_instance)->InitGameData();
                return true;
            }

            bool* m_gserveStarted;      // 0x04 not used
        };

        loadingScreen->AddLoadingStep(new AppInitGameData(&m_gserveStarted), "AppInitGameData");
    }
    loadingScreen->SetRange(0, 20, false);
}

void CGameScene::OnCreateState(Window** window, int state)
{
    // The scene always runs the game type 1 (GetGameType() below is not inlined).
    ((App*)WindowApp::m_instance)->m_helper->m_gamePlayManager.m_gameType = (game_type_t)1;
    Window* created = NULL;
    switch (state) {
    case STATE_LOADING:
        created = new CLoadingScreen(false);
        FillGameLoadingTasks((CLoadingScreen*)created);
        break;
    case STATE_GAME:
        created = new CGameScreen();
        break;
    case STATE_PAUSE:
        ((App*)WindowApp::m_instance)->m_timeScale = 0.0f;
        created = new CDH_PauseScreen(false, false);
        break;
    case STATE_PAUSE_MENU: {
        ICMediaPlayer::GetInstance()->Stop(0);
        ICMediaPlayer::GetInstance()->StopVibrate(0);
        ((App*)WindowApp::m_instance)->m_timeScale = 0.0f;
        bool survival = ((App*)WindowApp::m_instance)->m_helper->GetGameType() == 1;
        bool inTutorial = ((App*)WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial();
        bool flag = survival & !inTutorial & !((App*)WindowApp::m_instance)->m_helper->m_swerveGame->GetFlagAc();
        created = new CDH_PauseScreen(true, flag);
        break;
    }
    }
    *window = created;
}

void CGameScene::OnCommand(Event& event)
{
}

void CGameScene::OnStateInterrupt(int state, Window* window)
{
    switch (state) {
    case STATE_LOADING:
        if (window->GetInterrupt() == 1) {
            ChangeState(STATE_GAME, STATE_REPLACE);
        } else {
            ChangeState(STATE_PAUSE_MENU, STATE_PUSH);
        }
        break;
    case STATE_GAME:
        switch (window->GetInterrupt()) {
        case 1:
            SetInterrupt(1);
            break;
        case 2:
            ChangeState(STATE_PAUSE, STATE_PUSH);
            break;
        case 3:
            ChangeState(STATE_PAUSE_MENU, STATE_PUSH);
            break;
        }
        break;
    case STATE_PAUSE: {
        int interrupt = window->GetInterrupt();
        // Written 2 first: the original compares 1 first (if-else chains come out reversed).
        if (interrupt == 2) {
            PopState();
        } else if (interrupt == 1) {
            SetInterrupt(2);
        }
        ((App*)WindowApp::m_instance)->m_timeScale = 1.0f;
        break;
    }
    case STATE_PAUSE_MENU:
        if (window->GetInterrupt() == 2) {
            ((App*)WindowApp::m_instance)->m_timeScale = 1.0f;
            PopState();
        }
        break;
    }
}

void CGameScene::OnPause()
{
}
