// SplashWindow.cpp - start screens: the Glu logo movie, the game intro movie, then the game.
#include "SplashWindow.h"
#include "App.h"
#include "MovieScreen.h"
#include "CStrWChar.h"
#include "CFileUtil.h"
#include "CDH_SoundManager.h"
#include "CDH_RootNode.h"

using namespace com::glu::platform::components;

/** Mime key passed to the movie player. */
static const unsigned int MOVIE_MIME_KEY = 0xf23d9ad4;
/** Easing command that closes a start screen. */
static const int COMMAND_CLOSE = 0x200;
/** Soft key "back": leave the application. */
static const int COMMAND_BACK = (int)0x97204784;
/** Soft key "select": skip the screen. */
static const int COMMAND_SELECT = 0x2074d009;
/** Updates before a screen may be skipped. */
static const int MIN_FRAMES = 6;
/** Seconds before a screen closes by itself / may be skipped. */
static const float AUTO_CLOSE_TIME = 4.0f;
static const float SKIP_TIME = 2.7f;
static const float SKIP_TIME_GLU_LOGO = 2.0f;

void SplashWindow::GluLogo::Init()
{
    CStrWChar path;
    if (App::IsVGA() == true) {
        CFileUtil::GetApplicationPathForFile(path, m_vgaFileName);
    } else if (App::IsXGA() == true) {
        CFileUtil::GetApplicationPathForFile(path, m_xgaFileName);
    } else {
        CFileUtil::GetApplicationPathForFile(path, m_fileName);
    }
    m_movie = new CMovieScreen();
    m_movie->SetFileName(path.m_string);
    if (m_audioFileName != NULL) {
        CStrWChar audioPath;
        CFileUtil::GetApplicationPathForFile(audioPath, m_audioFileName);
        m_movie->SetAudioFileName(audioPath.m_string);
    }
    m_movie->SetMimeKey(MOVIE_MIME_KEY);
    m_movie->Start();
}

void SplashWindow::GluLogo::Shutdown()
{
    m_movie->Stop();
    if (m_movie != NULL) {
        delete m_movie;
    }
    m_movie = NULL;
}

void SplashWindow::GluLogo::Update()
{
    Window::Update();
    m_movie->HandleUpdate(WindowApp::m_instance->m_updateFrameTime);
    if (m_movie->IsFinished() == true) {
        m_parent->ReverseEasy(COMMAND_CLOSE);
    }
}

void SplashWindow::GluLogo::Paint(com::glu::platform::graphics::ICGraphics2d& graphics)
{
}

SplashWindow::SplashWindow(EType type)
    : WindowColor(0)
{
    ClearFlags(FLAG_EVENTS_WHILE_SHOWING | FLAG_EVENTS_WHILE_CLOSING);
    m_type = type;
    m_soundActivated = true;
    m_startTime = WindowApp::m_instance->m_timeSeconds;
    m_frameCount = 0;
    m_autoClose = false;
    m_fieldBE = false;
    GluLogo* logo;
    switch (type) {
    case TYPE_GLU_LOGO:
        logo = new GluLogo(L"glu_logo_landscape.m4v", L"glu_logo_landscape.m4v", L"glu_logo_landscape.m4v",
                           L"glu_logo_audio.wav");
        break;
    case TYPE_INTRO:
        logo = new GluLogo(L"ckz_intro_scene_1024.m4v", L"ckz_intro_scene_1024.m4v", L"ckz_intro_scene_1024.m4v",
                           NULL);
        break;
    default:
        return;
    }
    ((App*)WindowApp::m_instance)->m_soundManager->SetActivated(true);
    AddToFront(logo);
}

void SplashWindow::Shutdown()
{
    Window* parent = m_parent;
    if ((parent->m_flags & FLAG_CLOSING) != 0) {
        return;
    }
    switch (m_type) {
    case TYPE_GLU_LOGO:
        parent->AddModal(new SplashWindow(TYPE_INTRO));
        break;
    case TYPE_INTRO:
    case TYPE_NONE:
        RunApp();
        break;
    }
}

void SplashWindow::RunApp()
{
    App::StartNGServer((App*)WindowApp::m_instance);
    ((App*)WindowApp::m_instance)->m_mainLayer->AddModal(new CDH_RootNode());
}

void SplashWindow::TryClose(float minTime)
{
    if ((m_flags & (FLAG_CLOSE_AFTER_EASY | FLAG_CLOSING)) != 0) {
        return;
    }
    if (m_frameCount < MIN_FRAMES) {
        return;
    }
    if (WindowApp::m_instance->m_timeSeconds - m_startTime > minTime) {
        ReverseEasy(COMMAND_CLOSE);
    }
}

void SplashWindow::Update()
{
    Window::Update();
    if (m_type == TYPE_NONE) {
        Close();
        return;
    }
    m_frameCount++;
    if (m_autoClose) {
        TryClose(AUTO_CLOSE_TIME);
    }
}

void SplashWindow::OnPointerEvent(Event& event)
{
    if (event.m_type == Event::POINTER_PRESSED) {
        if (m_type == TYPE_GLU_LOGO) {
            TryClose(SKIP_TIME_GLU_LOGO);
        } else {
            TryClose(SKIP_TIME);
        }
    }
    event.Clear();
}

void SplashWindow::OnCommand(Event& event)
{
    switch (event.m_id) {
    case COMMAND_SELECT:
        TryClose(SKIP_TIME);
        event.Clear();
        break;
    case COMMAND_BACK:
        WindowApp::HandleTunnelCommand(DisplayWindow::COMMAND_EXIT_CONSUMED, 0, 0, 0);
        break;
    }
}
