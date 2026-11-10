// Play the intro movie like the iOS player did: full screen, aspect fit, one click skips it.
// Media Foundation decodes, OpenAL plays the sound track, Present() shows the current frame.
#include "Host.h"
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <wrl/client.h>
#include <stdio.h>
#include <string.h>
#include <filesystem>
#include <string>
#include <vector>
#include <AL/al.h>
#include <AL/alc.h>
#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>
#include "CApplet.h"
#include "CApp.h"
#include "CMoviePlayer.h"

extern "C" void GL_APIENTRY ckzBlitFramebuffer(GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint,
                                                GLbitfield, GLenum);

using namespace com::glu::platform::components;
using Microsoft::WRL::ComPtr;

namespace {

// CMovieEvent::m_state values, the same ones the iOS player used.
constexpr int StatePlaying = 1;
constexpr int StatePaused = 2;

// Media Foundation counts time in 100 ns units, SDL in 1 ns units.
constexpr Uint64 NanosecondsPerMovieTick = 100;
constexpr int BytesPerPixel = 4;

class MovieEventPC;

// The movie Present() draws and a click skips; nullptr while none is playing.
MovieEventPC* activeMovie;

class MovieEventPC final : public CMovieEvent {
public:
    ~MovieEventPC() override { Destroy(); }

    unsigned char Initialize() override
    {
        Destroy();
        m_lastError = -1;
        if (FAILED(MFStartup(MF_VERSION, MFSTARTUP_LITE))) {
            return Fail("Media Foundation is unavailable");
        }
        m_mediaFoundationStarted = true;
        if (!OpenVideo()) {
            return Fail("no decoder for the video track");
        }
        CreateCanvasTexture();
        // A movie without sound still plays.
        LoadSoundTrack();
        m_lastError = 0;
        return 1;
    }

    void Destroy() override
    {
        if (activeMovie == this) {
            activeMovie = nullptr;
        }
        if (m_source != 0) {
            alSourceStop(m_source);
            alDeleteSources(1, &m_source);
            m_source = 0;
        }
        if (m_soundBuffer != 0) {
            alDeleteBuffers(1, &m_soundBuffer);
            m_soundBuffer = 0;
        }
        if (m_framebuffer != 0) {
            glDeleteFramebuffersOES(1, &m_framebuffer);
            m_framebuffer = 0;
        }
        if (m_texture != 0) {
            glDeleteTextures(1, &m_texture);
            m_texture = 0;
        }
        m_videoReader.Reset();
        if (m_mediaFoundationStarted) {
            MFShutdown();
            m_mediaFoundationStarted = false;
        }
    }

    unsigned char Play() override
    {
        m_startTime = SDL_GetTicksNS();
        m_frameTime = -1;
        if (m_source != 0) {
            alSourcePlay(m_source);
        }
        m_state = StatePlaying;
        activeMovie = this;
        fprintf(stderr, "Movie started: %dx%d, sound %s\n", m_width, m_height, m_source != 0 ? "on" : "off");
        return 1;
    }

    unsigned char Stop() override
    {
        if (m_source != 0) {
            alSourceStop(m_source);
        }
        if (activeMovie == this) {
            activeMovie = nullptr;
        }
        m_state = StateFinished;
        return m_lastError == 0;
    }

    unsigned char Pause() override
    {
        if (m_state == StatePlaying) {
            if (m_source != 0) {
                alSourcePause(m_source);
            }
            m_state = StatePaused;
        }
        return m_lastError == 0;
    }

    // As on iOS: a paused movie is not resumed, it ends.
    unsigned char Resume() override { return Stop(); }

    unsigned char HandleUpdate(int) override
    {
        if (m_state != StatePlaying) {
            return 1;
        }
        // The wall clock drives the picture; the sound track started at the same moment.
        const LONGLONG movieTime = static_cast<LONGLONG>((SDL_GetTicksNS() - m_startTime) / NanosecondsPerMovieTick);
        bool frameChanged = false;
        while (m_frameTime < movieTime) {
            if (!ReadNextFrame()) {
                fprintf(stderr, "Movie finished\n");
                Stop();
                return 1;
            }
            frameChanged = true;
        }
        if (frameChanged) {
            UploadFrame();
        }
        return 1;
    }

    int GetLastErr() override { return m_lastError; }

    // The movie always fills the window with its aspect kept, so the rectangles are not settable (as on iOS).
    unsigned char SetSrcRectX() override { return 0; }
    unsigned char SetSrcRectY() override { return 0; }
    unsigned char SetSrcRectDX() override { return 0; }
    unsigned char SetSrcRectDY() override { return 0; }
    unsigned char SetDestRectX() override { return 0; }
    unsigned char SetDestRectY() override { return 0; }
    unsigned char SetDestRectDX() override { return 0; }
    unsigned char SetDestRectDY() override { return 0; }
    unsigned char SetTouchSkippable() override { return 1; }
    unsigned char SetAudioDelay() override { return 1; }
    unsigned char SetScaling() override { return 1; }
    unsigned char SetRotation() override { return 1; }

    /** Stop on a click, like a touch on the iOS overlay view. Returns true when the click was used. */
    bool Skip()
    {
        if (m_state != StatePlaying || !m_touchSkippable) {
            return false;
        }
        fprintf(stderr, "Movie skipped\n");
        Stop();
        return true;
    }

    /** Stretch the canvas over the game picture. The read framebuffer binding is left on the canvas. */
    void BlitToWindow(const SDL_Rect& picture)
    {
        if (m_framebuffer == 0) {
            glGenFramebuffersOES(1, &m_framebuffer);
            glBindFramebufferOES(GL_READ_FRAMEBUFFER_APPLE, m_framebuffer);
            glFramebufferTexture2DOES(GL_READ_FRAMEBUFFER_APPLE, GL_COLOR_ATTACHMENT0_OES, GL_TEXTURE_2D, m_texture, 0);
        }
        glBindFramebufferOES(GL_READ_FRAMEBUFFER_APPLE, m_framebuffer);
        // Decoded rows run top to bottom while GL rows run bottom to top: flip while blitting.
        ckzBlitFramebuffer(0, 0, m_canvasWidth, m_canvasHeight, picture.x, picture.y + picture.h,
                           picture.x + picture.w, picture.y, GL_COLOR_BUFFER_BIT, GL_LINEAR);
    }

private:
    unsigned char Fail(const char* reason)
    {
        fwprintf(stderr, L"Movie skipped, %hs: %ls\n", reason, m_fileName.m_string);
        // CMoviePlayer keeps a movie that failed to start; mark it finished so the game moves on.
        m_state = StateFinished;
        return 0;
    }

    /** Open the video track with frames converted to 32-bit BGR. */
    bool OpenVideo()
    {
        const std::wstring path = std::filesystem::path(m_fileName.m_string).make_preferred().wstring();
        ComPtr<IMFAttributes> attributes;
        ComPtr<IMFMediaType> wantedType;
        ComPtr<IMFMediaType> actualType;
        HRESULT result = MFCreateAttributes(&attributes, 1);
        if (SUCCEEDED(result)) {
            // Lets the reader convert the decoder's YUV output to RGB for us.
            result = attributes->SetUINT32(MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING, TRUE);
        }
        if (SUCCEEDED(result)) {
            result = MFCreateSourceReaderFromURL(path.c_str(), attributes.Get(), &m_videoReader);
        }
        if (SUCCEEDED(result)) {
            result = m_videoReader->SetStreamSelection(static_cast<DWORD>(MF_SOURCE_READER_ALL_STREAMS), FALSE);
        }
        if (SUCCEEDED(result)) {
            result = m_videoReader->SetStreamSelection(static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM), TRUE);
        }
        if (SUCCEEDED(result)) {
            result = MFCreateMediaType(&wantedType);
        }
        if (SUCCEEDED(result)) {
            wantedType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
            wantedType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
            result = m_videoReader->SetCurrentMediaType(static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM),
                                                        nullptr, wantedType.Get());
        }
        if (SUCCEEDED(result)) {
            result = m_videoReader->GetCurrentMediaType(static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM),
                                                        &actualType);
        }
        UINT32 width = 0;
        UINT32 height = 0;
        if (SUCCEEDED(result)) {
            result = MFGetAttributeSize(actualType.Get(), MF_MT_FRAME_SIZE, &width, &height);
        }
        if (FAILED(result) || width == 0 || height == 0) {
            fprintf(stderr, "Movie video track failed to open (HRESULT 0x%08lx)\n", static_cast<unsigned long>(result));
            m_videoReader.Reset();
            return false;
        }
        m_width = static_cast<int>(width);
        m_height = static_cast<int>(height);
        m_pixels.assign(static_cast<size_t>(m_width) * m_height * BytesPerPixel, 0);
        return true;
    }

    /**
     * Create the black canvas the frames are copied into. It has the window's aspect ratio,
     * so stretching it over the window keeps the movie's aspect and leaves black bars.
     */
    void CreateCanvasTexture()
    {
        m_canvasWidth = m_width;
        m_canvasHeight = m_height;
        if (m_width * ckz::WindowHeight >= m_height * ckz::WindowWidth) {
            // The movie is wider than the window: bars above and below.
            m_canvasHeight = m_width * ckz::WindowHeight / ckz::WindowWidth;
        } else {
            m_canvasWidth = m_height * ckz::WindowWidth / ckz::WindowHeight;
        }
        m_frameX = (m_canvasWidth - m_width) / 2;
        m_frameY = (m_canvasHeight - m_height) / 2;

        const std::vector<unsigned char> black(static_cast<size_t>(m_canvasWidth) * m_canvasHeight * BytesPerPixel, 0);
        // The game caches its texture binding, so put it back afterwards.
        GLint previousTexture = 0;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture);
        glGenTextures(1, &m_texture);
        glBindTexture(GL_TEXTURE_2D, m_texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_canvasWidth, m_canvasHeight, 0,
                     GL_BGRA_EXT, GL_UNSIGNED_BYTE, black.data());
        glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previousTexture));
    }

    /** Decode the whole sound track into one OpenAL buffer; the movie is short. */
    void LoadSoundTrack()
    {
        if (alcGetCurrentContext() == nullptr) {
            return;
        }
        const std::wstring path = std::filesystem::path(m_fileName.m_string).make_preferred().wstring();
        const DWORD audioStream = static_cast<DWORD>(MF_SOURCE_READER_FIRST_AUDIO_STREAM);
        ComPtr<IMFSourceReader> reader;
        ComPtr<IMFMediaType> wantedType;
        ComPtr<IMFMediaType> actualType;
        HRESULT result = MFCreateSourceReaderFromURL(path.c_str(), nullptr, &reader);
        if (SUCCEEDED(result)) {
            result = reader->SetStreamSelection(static_cast<DWORD>(MF_SOURCE_READER_ALL_STREAMS), FALSE);
        }
        if (SUCCEEDED(result)) {
            result = reader->SetStreamSelection(audioStream, TRUE);
        }
        if (SUCCEEDED(result)) {
            result = MFCreateMediaType(&wantedType);
        }
        if (SUCCEEDED(result)) {
            wantedType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
            wantedType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
            wantedType->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16);
            result = reader->SetCurrentMediaType(audioStream, nullptr, wantedType.Get());
        }
        if (SUCCEEDED(result)) {
            result = reader->GetCurrentMediaType(audioStream, &actualType);
        }
        if (FAILED(result)) {
            fprintf(stderr, "Movie sound track failed to open (HRESULT 0x%08lx)\n", static_cast<unsigned long>(result));
            return;
        }
        const UINT32 channels = MFGetAttributeUINT32(actualType.Get(), MF_MT_AUDIO_NUM_CHANNELS, 0);
        const UINT32 sampleRate = MFGetAttributeUINT32(actualType.Get(), MF_MT_AUDIO_SAMPLES_PER_SECOND, 0);
        const UINT32 bitsPerSample = MFGetAttributeUINT32(actualType.Get(), MF_MT_AUDIO_BITS_PER_SAMPLE, 0);
        if (bitsPerSample != 16 || sampleRate == 0 || (channels != 1 && channels != 2)) {
            fprintf(stderr, "Movie sound format unsupported: %u channels, %u bits\n", channels, bitsPerSample);
            return;
        }

        std::vector<unsigned char> samples;
        while (true) {
            DWORD flags = 0;
            ComPtr<IMFSample> sample;
            result = reader->ReadSample(audioStream, 0, nullptr, &flags, nullptr, &sample);
            if (FAILED(result) || (flags & MF_SOURCE_READERF_ENDOFSTREAM) != 0) {
                break;
            }
            if (sample == nullptr) {
                continue;
            }
            ComPtr<IMFMediaBuffer> buffer;
            BYTE* data = nullptr;
            DWORD size = 0;
            if (SUCCEEDED(sample->ConvertToContiguousBuffer(&buffer)) && SUCCEEDED(buffer->Lock(&data, nullptr, &size))) {
                samples.insert(samples.end(), data, data + size);
                buffer->Unlock();
            }
        }
        if (samples.empty()) {
            return;
        }

        ALenum format = AL_FORMAT_MONO16;
        if (channels == 2) {
            format = AL_FORMAT_STEREO16;
        }
        alGenBuffers(1, &m_soundBuffer);
        alBufferData(m_soundBuffer, format, samples.data(), static_cast<ALsizei>(samples.size()),
                     static_cast<ALsizei>(sampleRate));
        alGenSources(1, &m_source);
        alSourcei(m_source, AL_BUFFER, static_cast<ALint>(m_soundBuffer));
        // Plain stereo playback: keep the source on the listener.
        alSourcei(m_source, AL_SOURCE_RELATIVE, AL_TRUE);
    }

    /** Decode the next frame into m_pixels (rows top to bottom). Returns false at the end of the movie. */
    bool ReadNextFrame()
    {
        DWORD flags = 0;
        LONGLONG timestamp = 0;
        ComPtr<IMFSample> sample;
        const HRESULT result = m_videoReader->ReadSample(static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM),
                                                         0, nullptr, &flags, &timestamp, &sample);
        if (FAILED(result) || (flags & MF_SOURCE_READERF_ENDOFSTREAM) != 0) {
            return false;
        }
        if (sample == nullptr) {
            // A gap in the stream: nothing to show, ask again.
            return true;
        }
        m_frameTime = timestamp;

        ComPtr<IMFMediaBuffer> buffer;
        if (FAILED(sample->ConvertToContiguousBuffer(&buffer))) {
            return false;
        }
        const size_t rowBytes = static_cast<size_t>(m_width) * BytesPerPixel;
        ComPtr<IMF2DBuffer> buffer2d;
        BYTE* firstRow = nullptr;
        LONG pitch = 0;
        // The 2D lock gives the top row and a signed pitch, whatever order the decoder stores rows in.
        if (SUCCEEDED(buffer.As(&buffer2d)) && SUCCEEDED(buffer2d->Lock2D(&firstRow, &pitch))) {
            for (int row = 0; row < m_height; row++) {
                memcpy(m_pixels.data() + row * rowBytes, firstRow + static_cast<ptrdiff_t>(row) * pitch, rowBytes);
            }
            buffer2d->Unlock2D();
            return true;
        }
        BYTE* data = nullptr;
        DWORD size = 0;
        if (FAILED(buffer->Lock(&data, nullptr, &size))) {
            return false;
        }
        if (size >= m_pixels.size()) {
            memcpy(m_pixels.data(), data, m_pixels.size());
        }
        buffer->Unlock();
        return true;
    }

    /** Copy m_pixels into the middle of the canvas texture. */
    void UploadFrame()
    {
        GLint previousTexture = 0;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture);
        glBindTexture(GL_TEXTURE_2D, m_texture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, m_frameX, m_frameY, m_width, m_height,
                        GL_BGRA_EXT, GL_UNSIGNED_BYTE, m_pixels.data());
        glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previousTexture));
    }

    ComPtr<IMFSourceReader> m_videoReader;
    bool m_mediaFoundationStarted = false;
    int m_lastError = 0;         // 0 or -1, as on iOS
    Uint64 m_startTime = 0;      // SDL nanosecond clock at Play()
    LONGLONG m_frameTime = -1;   // timestamp of the frame in m_pixels, 100 ns units
    std::vector<unsigned char> m_pixels; // current frame, BGRA, rows top to bottom
    int m_canvasWidth = 0;
    int m_canvasHeight = 0;
    int m_frameX = 0;            // where the frame sits inside the canvas
    int m_frameY = 0;
    GLuint m_texture = 0;        // the canvas
    GLuint m_framebuffer = 0;    // wraps the canvas so it can be blitted
    ALuint m_soundBuffer = 0;
    ALuint m_source = 0;
};

} // namespace

ICMoviePlayer* ICMoviePlayer::CreateInstance() { return new CMoviePlayer; }
CMovieEvent* CMovieEvent::CreateInstance() { return new MovieEventPC; }

namespace ckz {

bool BlitMovieFrame(const SDL_Rect& picture)
{
    if (activeMovie == nullptr) {
        return false;
    }
    activeMovie->BlitToWindow(picture);
    return true;
}

bool SkipMovie()
{
    if (activeMovie == nullptr) {
        return false;
    }
    return activeMovie->Skip();
}

} // namespace ckz
