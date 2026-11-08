// CMoviePlayer.cpp - platform-independent movie player.
#include "CMoviePlayer.h"
#include "CEventListener.h"
#include "CEvent.h"
#include "CHandle.h"

namespace com { namespace glu { namespace platform { namespace components {

/** Full turn, for the rotation property. */
static const unsigned int kDegreesPerTurn = 360;

// system events the player listens to
static const unsigned int kEventStopMovie = 0x36754280;
static const unsigned int kEventPause = 0x54246de3;
static const unsigned int kEventResume = 0x75424363;
static const unsigned int kEvent747ab255 = 0x747ab255;   // registered, not handled
static const unsigned int kEvent97b4dec3 = 0x97b4dec3;   // registered, not handled
static const unsigned int kEvent24314258 = 0x24314258;   // registered, not handled

CMovieEvent::CMovieEvent()
{
    m_player = NULL;
    m_handle = 0;
    m_flags = 0;
    m_volume = 0;
    m_state = 0;
    m_width = 0;
    m_height = 0;
    m_srcRectX = 0;
    m_srcRectY = 0;
    m_srcRectDX = 0;
    m_srcRectDY = 0;
    m_destRectX = 0;
    m_destRectY = 0;
    m_destRectDX = 0;
    m_destRectDY = 0;
    m_touchSkippable = 0;
    m_audioDelay = 0;
    m_scaling = 0;
    m_rotation = 0;
}

CMovieEvent::~CMovieEvent()
{
}

void CMovieEvent::SetFileName(const wchar_t* fileName)
{
    m_fileName = fileName;
}

void CMovieEvent::SetAudioFileName(const wchar_t* fileName)
{
    m_audioFileName = fileName;
}

void CMovieEvent::SetVolume(unsigned char volume)
{
    if (volume >= CMoviePlayer::MaxVolume) {
        volume = CMoviePlayer::MaxVolume;
    }
    m_volume = volume;
}

unsigned char CMovieEvent::SetProperty(ICMoviePlayer::MovieProperty property, int value)
{
    LimitPropertyValue(property, value);
    switch (property) {
    case ICMoviePlayer::SrcRectX:
        m_srcRectX = value;
        return SetSrcRectX();
    case ICMoviePlayer::SrcRectY:
        m_srcRectY = value;
        return SetSrcRectY();
    case ICMoviePlayer::SrcRectDX:
        m_srcRectDX = value;
        return SetSrcRectDX();
    case ICMoviePlayer::SrcRectDY:
        m_srcRectDY = value;
        return SetSrcRectDY();
    case ICMoviePlayer::DestRectX:
        m_destRectX = value;
        return SetDestRectX();
    case ICMoviePlayer::DestRectY:
        m_destRectY = value;
        return SetDestRectY();
    case ICMoviePlayer::DestRectDX:
        m_destRectDX = value;
        return SetDestRectDX();
    case ICMoviePlayer::DestRectDY:
        m_destRectDY = value;
        return SetDestRectDY();
    case ICMoviePlayer::TouchSkippable:
        m_touchSkippable = (value != 0);
        return SetTouchSkippable();
    case ICMoviePlayer::AudioDelay:
        m_audioDelay = value;
        return SetAudioDelay();
    case ICMoviePlayer::Scaling:
        m_scaling = value;
        return SetScaling();
    case ICMoviePlayer::Rotation:
        m_rotation = (unsigned int)value % kDegreesPerTurn;
        return SetRotation();
    default:
        return 0;
    }
}

unsigned char CMovieEvent::GetProperty(ICMoviePlayer::MovieProperty property, int& value)
{
    value = 0;
    switch (property) {
    case ICMoviePlayer::MovieWidth:
        value = m_width;
        break;
    case ICMoviePlayer::MovieHeight:
        value = m_height;
        break;
    case ICMoviePlayer::SrcRectX:
        value = m_srcRectX;
        break;
    case ICMoviePlayer::SrcRectY:
        value = m_srcRectY;
        break;
    case ICMoviePlayer::SrcRectDX:
        value = m_srcRectDX;
        break;
    case ICMoviePlayer::SrcRectDY:
        value = m_srcRectDY;
        break;
    case ICMoviePlayer::DestRectX:
        value = m_destRectX;
        break;
    case ICMoviePlayer::DestRectY:
        value = m_destRectY;
        break;
    case ICMoviePlayer::DestRectDX:
        value = m_destRectDX;
        break;
    case ICMoviePlayer::DestRectDY:
        // note: the original returns the source rectangle height here
        value = m_srcRectDY;
        break;
    case ICMoviePlayer::TouchSkippable:
        value = m_touchSkippable;
        break;
    case ICMoviePlayer::AudioDelay:
        value = m_audioDelay;
        break;
    case ICMoviePlayer::Scaling:
        value = m_scaling;
        break;
    case ICMoviePlayer::Rotation:
        value = m_rotation;
        break;
    default:
        return 0;
    }
    return 1;
}

void CMovieEvent::LimitPropertyValue(ICMoviePlayer::MovieProperty property, int& value)
{
}

CMoviePlayer::CMoviePlayer()
{
    m_listener = NULL;
    systems::CEventListener* listener = new systems::CEventListener();
    m_listener = listener;
    if (listener != NULL) {
        unsigned int handle = core::CHandle::FromName("CMoviePlayer");
        listener->m_object = this;
        listener->m_handle = handle;
        listener->m_callback = EventCB;
        m_listener->Register(kEventStopMovie, 0x3fffffff);
        m_listener->Register(kEventPause, 0x3fffffff);
        m_listener->Register(kEventResume, 0x3fffffff);
        m_listener->Register(kEvent747ab255, 0x3fffffff);
        m_listener->Register(kEvent97b4dec3, 0x3fffffff);
        m_listener->Register(kEvent24314258, 0x3fffffff);
    }
    m_soundEnabled = 0;
    m_volume = 0;
    m_lastHandle = 0;
    m_paused = 0;
    m_event = NULL;
}

unsigned char CMoviePlayer::EventCB(systems::CEvent& event, void* listener)
{
    systems::CEventListener* eventListener = (systems::CEventListener*)listener;
    CMoviePlayer* player = (CMoviePlayer*)eventListener->m_object;
    player->HandleEvent(event.m_eventId, event.m_param, (unsigned int)listener);
    return 0;
}

CMoviePlayer::~CMoviePlayer()
{
    if (m_event != NULL) {
        delete m_event;
        m_event = NULL;
    }
    if (m_listener != NULL) {
        m_listener->UnregisterAll();
        // the original re-checks the pointer (safe-delete idiom) and skips the store when NULL
        if (m_listener != NULL) {
            delete m_listener;
            m_listener = NULL;
        }
    }
}

void CMoviePlayer::SetSoundEnabled(unsigned char enabled)
{
    m_soundEnabled = enabled;
    if (m_event != NULL) {
        // ?: kept on purpose: the argument is evaluated after the virtual call target is loaded
        m_event->SetVolume(enabled ? m_volume : 0);
    }
}

void CMoviePlayer::SetVolume(unsigned char volume)
{
    if (volume >= MaxVolume) {
        volume = MaxVolume;
    }
    m_volume = volume;
    if (m_event != NULL) {
        if (!m_soundEnabled) {
            volume = 0;
        }
        m_event->SetVolume(volume);
    }
}

unsigned char CMoviePlayer::GetSoundEnabled()
{
    return m_soundEnabled;
}

unsigned char CMoviePlayer::GetVolume()
{
    return m_volume;
}

unsigned char CMoviePlayer::IsPlaying(unsigned int handle)
{
    if (m_event == NULL) {
        return 0;
    }
    return m_event->m_state != CMovieEvent::StateFinished;
}

unsigned int CMoviePlayer::Play(const wchar_t* fileName, unsigned int flags, const wchar_t* audioFileName)
{
    Stop(0);
    if (IsPlaying(0)) {
        return 0;
    }

    m_event = CMovieEvent::CreateInstance();
    if (m_event == NULL) {
        return 0;
    }
    m_event->m_player = this;
    // the original loads m_event before bumping the handle counter
    CMovieEvent* event = m_event;
    m_lastHandle++;
    event->m_handle = m_lastHandle;
    m_event->SetFileName(fileName);
    m_event->SetAudioFileName(audioFileName);
    m_event->m_flags = flags;
    m_event->SetVolume(m_volume);
    if (!m_event->Initialize()) {
        return 0;
    }
    if (!m_event->Play()) {
        return 0;
    }
    return m_event->m_handle;
}

void CMoviePlayer::Stop(unsigned int handle)
{
    if (m_event == NULL) {
        return;
    }
    if (handle != 0 && m_event->m_handle != handle) {
        return;
    }

    unsigned char stillPlaying = 0;
    if (m_event->m_state != CMovieEvent::StateFinished) {
        m_event->Stop();
        if (m_event->m_state != CMovieEvent::StateFinished) {
            stillPlaying = 1;
        }
    }
    if (!stillPlaying) {
        // the original only clears the pointer when there was an event to delete
        if (m_event != NULL) {
            delete m_event;
            m_event = NULL;
        }
    }
}

unsigned char CMoviePlayer::CanSetProperty(MovieProperty property)
{
    return 0;
}

unsigned char CMoviePlayer::CanGetProperty(MovieProperty property)
{
    return 0;
}

unsigned char CMoviePlayer::SetProperty(unsigned int handle, MovieProperty property, int value)
{
    if (m_event == NULL || m_event->m_handle != handle) {
        return 0;
    }
    return m_event->SetProperty(property, value);
}

unsigned char CMoviePlayer::GetProperty(unsigned int handle, MovieProperty property, int& value)
{
    if (m_event == NULL || m_event->m_handle != handle) {
        return 0;
    }
    return m_event->GetProperty(property, value);
}

unsigned char CMoviePlayer::HandleUpdate(int elapsed)
{
    if (m_event != NULL) {
        m_event->HandleUpdate(elapsed);
        if (m_event->m_state == CMovieEvent::StateFinished) {
            delete m_event;
            m_event = NULL;
        }
    }
    return 1;
}

unsigned char CMoviePlayer::HandleEvent(unsigned int eventId, unsigned int param, unsigned int context)
{
    // b117-va: the original compares Resume first, i.e. the chain is written in reverse
    if (eventId == kEventStopMovie) {
        if (m_event != NULL) {
            m_event->Stop();
        }
    } else if (eventId == kEventPause) {
        if (m_event != NULL) {
            m_event->Pause();
            m_paused = 1;
        }
    } else if (eventId == kEventResume) {
        if (m_event != NULL) {
            m_paused = 0;
            m_event->Resume();
        }
    }
    return 0;
}

} } } }
