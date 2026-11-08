// MovieScreen.cpp - plays a full-screen movie (intro / logo) through the platform movie player.
#include "MovieScreen.h"
#include "ICMoviePlayer.h"
#include "ICGraphics.h"
#include "ICRenderSurface.h"
#include "ICCore.h"
#include "CApplet.h"

using namespace com::glu::platform;
using namespace com::glu::platform::components;
using namespace com::glu::platform::graphics;

/** Player volume used for movies. */
static const unsigned char MOVIE_VOLUME = 3;
/** Audio delay (ms) needed before iOS 3.2. */
static const int OLD_IOS_AUDIO_DELAY = 600;

CMovieScreen::CMovieScreen()
{
    m_mimeKey = 0;
    m_movie = 0;
}

CMovieScreen::~CMovieScreen()
{
}

void CMovieScreen::SetFileName(const wchar_t* fileName)
{
    m_fileName = fileName;
}

void CMovieScreen::SetAudioFileName(const wchar_t* fileName)
{
    m_audioFileName = fileName;
}

void CMovieScreen::SetMimeKey(unsigned int mimeKey)
{
    m_mimeKey = mimeKey;
}

int CMovieScreen::Start()
{
    ICMoviePlayer* player = ICMoviePlayer::GetInstance();
    player->SetVolume(MOVIE_VOLUME);
    unsigned int movie = player->Play(m_fileName.m_string, m_mimeKey, m_audioFileName.m_string);
    if (movie != 0) {
        // stretch the movie over the whole render target
        unsigned int width;
        unsigned int height;
        ICGraphics::GetInstance()->GetTargetSurface()->GetWidthAndHeight(width, height);
        player->SetProperty(movie, ICMoviePlayer::DestRectX, 0);
        player->SetProperty(movie, ICMoviePlayer::DestRectY, 0);
        player->SetProperty(movie, ICMoviePlayer::DestRectDX, width);
        player->SetProperty(movie, ICMoviePlayer::DestRectDY, height);
        player->SetProperty(movie, ICMoviePlayer::Scaling, 1);
        player->SetProperty(movie, ICMoviePlayer::TouchSkippable, 1);
        CStrWChar version = CApplet::m_pCore->GetPlatformVersion();
        if (wcscmp(version.m_string, L"3.2") < 0) {
            player->SetProperty(movie, ICMoviePlayer::AudioDelay, OLD_IOS_AUDIO_DELAY);
        }
    }
    return 0;
}

void CMovieScreen::Stop()
{
    ICMoviePlayer::GetInstance()->Stop(0);
}

bool CMovieScreen::IsFinished() const
{
    return ICMoviePlayer::GetInstance()->IsPlaying(m_movie) == 0;
}

int CMovieScreen::HandleUpdate(int elapsed)
{
    if (!IsFinished()) {
        ICMoviePlayer::GetInstance()->HandleUpdate(elapsed);
    }
    return 1;
}
