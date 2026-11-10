// SDL owns the context. The existing window surface draws into its own FBO.
#include "Host.h"
#include "Config.h"
#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>
#include "CGraphics_OGLES_EAGL.h"
#include "CRenderSurface_OGLES_Window_Cocoa.h"
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <filesystem>
#include <vector>

extern "C" void GL_APIENTRY ckzBlitFramebuffer(GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint,
                                                GLbitfield, GLenum);

using namespace com::glu::platform::graphics;

namespace ckz {
namespace {

bool frameDumpRequested;
int frameDumpCount;

// Frame rate log (FpsLog in zombsniper.cfg): frames, average and longest frame time of each second,
// and the work time of a frame: from the end of one swap to the start of the next, i.e. without the
// wait for vertical sync. With vsync on the frame rate is capped by the display; the work time shows
// the headroom (165 fps needs under ~6 ms). The driver may block for vsync inside a later GL call,
// so the work time can still contain the wait; "cpu" is the main thread's CPU time per frame
// (GetThreadTimes, timer-tick resolution, fine over a second), which leaves sleeping out.
// The frame count of the last full second is also what DrawFPS shows.
int shownFps;
Uint64 fpsWindowStart;
Uint64 lastPresent;
int fpsFrames;
double fpsLongestMs;
double workTotalMs;
double workLongestMs;
double cpuWindowStartMs;

/** CPU time (kernel + user) of the calling thread in milliseconds. */
double ThreadCpuMs()
{
    FILETIME creation;
    FILETIME exit;
    FILETIME kernel;
    FILETIME user;
    GetThreadTimes(GetCurrentThread(), &creation, &exit, &kernel, &user);
    ULARGE_INTEGER kernelTime;
    kernelTime.LowPart = kernel.dwLowDateTime;
    kernelTime.HighPart = kernel.dwHighDateTime;
    ULARGE_INTEGER userTime;
    userTime.LowPart = user.dwLowDateTime;
    userTime.HighPart = user.dwHighDateTime;
    // FILETIME counts 100 ns units.
    return static_cast<double>(kernelTime.QuadPart + userTime.QuadPart) / 10000.0;
}

/**
 * Count a presented frame; once per second update shownFps and, with FpsLog, log the second.
 * `swapStart` is the time just before SDL_GL_SwapWindow.
 */
void CountFrame(Uint64 swapStart)
{
    const Uint64 now = SDL_GetTicksNS();
    if (lastPresent != 0) {
        const double frameMs = static_cast<double>(now - lastPresent) / 1e6;
        if (frameMs > fpsLongestMs) {
            fpsLongestMs = frameMs;
        }
        const double workMs = static_cast<double>(swapStart - lastPresent) / 1e6;
        workTotalMs += workMs;
        if (workMs > workLongestMs) {
            workLongestMs = workMs;
        }
    }
    lastPresent = now;
    if (fpsWindowStart == 0) {
        fpsWindowStart = now;
        cpuWindowStartMs = ThreadCpuMs();
    }
    fpsFrames++;
    const double elapsedMs = static_cast<double>(now - fpsWindowStart) / 1e6;
    if (elapsedMs >= 1000.0) {
        const double cpuMs = ThreadCpuMs();
        shownFps = fpsFrames;
        if (config.fpsLog) {
            fprintf(stderr, "FPS %d (avg %.2f ms, longest %.2f ms; work avg %.2f ms, longest %.2f ms; cpu %.2f ms)\n",
                    fpsFrames, elapsedMs / fpsFrames, fpsLongestMs, workTotalMs / fpsFrames, workLongestMs,
                    (cpuMs - cpuWindowStartMs) / fpsFrames);
        }
        cpuWindowStartMs = cpuMs;
        fpsWindowStart = now;
        fpsFrames = 0;
        fpsLongestMs = 0.0;
        workTotalMs = 0.0;
        workLongestMs = 0.0;
    }
}

// DrawFPS: digits of a 3 x 5 block font, top row first ('1' = block lit). Each block is a
// scissored glClear, so the overlay needs no textures or shaders and leaves no GL state behind.
constexpr int GlyphWidth = 3;
constexpr int GlyphHeight = 5;
const char* const DigitGlyphs[10][GlyphHeight] = {
    { "111", "101", "101", "101", "111" }, // 0
    { "010", "110", "010", "010", "111" }, // 1
    { "111", "001", "111", "100", "111" }, // 2
    { "111", "001", "111", "001", "111" }, // 3
    { "101", "101", "111", "001", "001" }, // 4
    { "111", "100", "111", "001", "111" }, // 5
    { "111", "100", "111", "101", "111" }, // 6
    { "111", "001", "001", "001", "001" }, // 7
    { "111", "101", "111", "101", "111" }, // 8
    { "111", "101", "111", "001", "111" }, // 9
};
// Size of one font block: the game picture height divided by this.
constexpr int FpsBlocksPerPicture = 192;
// Left edge of the counter in game pixels (1024 wide): just right of the HUD pause button,
// which ends at about x = 55.
constexpr int FpsLeftGamePixels = 72;
constexpr float FpsTextColor[4] = { 1.0f, 0.9f, 0.2f, 1.0f };
constexpr float FpsBackColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

/** Fill a rectangle of the draw framebuffer (GL origin bottom-left); scissor test must be on. */
void FillRect(int x, int y, int width, int height, const float* color)
{
    glScissor(x, y, width, height);
    glClearColor(color[0], color[1], color[2], color[3]);
    glClear(GL_COLOR_BUFFER_BIT);
}

/** DrawFPS: the frame rate of the last second in the top-left corner of `picture` (GL origin). */
void DrawFps(const SDL_Rect& picture)
{
    char digits[16];
    snprintf(digits, sizeof(digits), "%d", shownFps);
    const int digitCount = static_cast<int>(strlen(digits));
    int block = picture.h / FpsBlocksPerPicture;
    if (block < 2) {
        block = 2;
    }
    // A one-block gap between digits and a one-block border, two blocks below the picture top.
    const int textWidth = digitCount * (GlyphWidth + 1) - 1;
    const int left = picture.x + picture.w * FpsLeftGamePixels / WindowWidth;
    const int top = picture.y + picture.h - 2 * block;

    GLfloat savedColor[4];
    GLint savedBox[4];
    glGetFloatv(GL_COLOR_CLEAR_VALUE, savedColor);
    glGetIntegerv(GL_SCISSOR_BOX, savedBox);
    glEnable(GL_SCISSOR_TEST);
    FillRect(left, top - (GlyphHeight + 2) * block, (textWidth + 2) * block, (GlyphHeight + 2) * block, FpsBackColor);
    for (int index = 0; index < digitCount; index++) {
        const char* const* glyph = DigitGlyphs[digits[index] - '0'];
        const int glyphLeft = left + (1 + index * (GlyphWidth + 1)) * block;
        for (int row = 0; row < GlyphHeight; row++) {
            for (int column = 0; column < GlyphWidth; column++) {
                if (glyph[row][column] != '1') {
                    continue;
                }
                const int y = top - (row + 2) * block;
                FillRect(glyphLeft + column * block, y, block, block, FpsTextColor);
            }
        }
    }
    glDisable(GL_SCISSOR_TEST);
    glScissor(savedBox[0], savedBox[1], savedBox[2], savedBox[3]);
    glClearColor(savedColor[0], savedColor[1], savedColor[2], savedColor[3]);
}

/** Black out the whole window around the game picture; scissor test must be off. */
void ClearWindow()
{
    GLfloat savedColor[4];
    glGetFloatv(GL_COLOR_CLEAR_VALUE, savedColor);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(savedColor[0], savedColor[1], savedColor[2], savedColor[3]);
}

/** Write the game framebuffer (bound for reading) as raw RGBA rows, bottom row first. */
void DumpFrame()
{
    std::vector<unsigned char> pixels(static_cast<size_t>(ckz::WindowWidth) * ckz::WindowHeight * 4);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, ckz::WindowWidth, ckz::WindowHeight, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    char name[64];
    snprintf(name, sizeof(name), "frame_%d_%dx%d.rgba", frameDumpCount, ckz::WindowWidth, ckz::WindowHeight);
    frameDumpCount++;
    const auto path = std::filesystem::u8path(SDL_GetBasePath()) / name;
    FILE* file = _wfopen(path.c_str(), L"wb");
    if (file == nullptr) {
        fprintf(stderr, "Frame dump: could not write %s\n", name);
        return;
    }
    fwrite(pixels.data(), 1, pixels.size(), file);
    fclose(file);
    fprintf(stderr, "Frame dump: %s\n", name);
}

} // namespace

void RequestFrameDump()
{
    frameDumpRequested = true;
}

SDL_Rect GamePictureRect(int windowWidth, int windowHeight)
{
    SDL_Rect picture;
    // Widest picture of the game's shape that fits; if it is too tall, fit the height instead.
    picture.w = windowWidth;
    picture.h = windowWidth * WindowHeight / WindowWidth;
    if (picture.h > windowHeight) {
        picture.h = windowHeight;
        picture.w = windowHeight * WindowWidth / WindowHeight;
    }
    picture.x = (windowWidth - picture.w) / 2;
    picture.y = (windowHeight - picture.h) / 2;
    return picture;
}

} // namespace ckz

void CGraphics_OGLES_Cocoa_GetEAGLVersion(unsigned int* major, unsigned int* minor)
{
    *major = 1;
    *minor = 1;
}

void* CGraphics_OGLES_EAGL::CreateContext(void*, const int*) { return ckz::context; }
void CGraphics_OGLES_EAGL::ReleaseContext(void*) {}
void CGraphics_OGLES_EAGL::SetCurrentContext(void* context, void*, void*)
{
    SDL_GL_MakeCurrent(ckz::window, static_cast<SDL_GLContext>(context));
}

unsigned char CGraphics_OGLES_EAGL::RenderBufferStorage(void*)
{
    glRenderbufferStorageOES(GL_RENDERBUFFER_OES, 0x8058, ckz::WindowWidth, ckz::WindowHeight);
    return glGetError() == GL_NO_ERROR;
}

void CGraphics_OGLES_EAGL::Present(ICRenderSurface& surface)
{
    auto& target = static_cast<CRenderSurface_OGLES_Window_Cocoa&>(surface);
    const bool scissor = glIsEnabled(GL_SCISSOR_TEST) != 0;
    int width = 0;
    int height = 0;
    SDL_GetWindowSizeInPixels(ckz::window, &width, &height);
    glDisable(GL_SCISSOR_TEST);
    glBindFramebufferOES(GL_READ_FRAMEBUFFER_APPLE, target.m_framebuffer);
    glBindFramebufferOES(GL_DRAW_FRAMEBUFFER_APPLE, 0);
    if (ckz::frameDumpRequested) {
        ckz::frameDumpRequested = false;
        ckz::DumpFrame();
    }
    // The picture keeps the game's shape; GL counts rows from the bottom of the window.
    SDL_Rect picture = ckz::GamePictureRect(width, height);
    picture.y = height - picture.y - picture.h;
    if (picture.w != width || picture.h != height) {
        ckz::ClearWindow();
    }
    // A playing movie covers the game picture, as the movie view did on iOS.
    if (!ckz::BlitMovieFrame(picture)) {
        // Smooth the picture when the window is not exactly the game resolution.
        GLenum filter = GL_NEAREST;
        if (picture.w != ckz::WindowWidth || picture.h != ckz::WindowHeight) {
            filter = GL_LINEAR;
        }
        ckzBlitFramebuffer(0, 0, ckz::WindowWidth, ckz::WindowHeight, picture.x, picture.y,
                           picture.x + picture.w, picture.y + picture.h, GL_COLOR_BUFFER_BIT, filter);
    }
    if (ckz::config.drawFps) {
        ckz::DrawFps(picture);
    }
    // Leave the window's own framebuffer bound for reading too: recorders such as Bandicam read the
    // bound read framebuffer at swap time, and the 1024 x 768 game framebuffer ended up in a corner
    // of their video. BindFrameBuffer below binds the game framebuffer again.
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, 0);
    const Uint64 swapStart = SDL_GetTicksNS();
    SDL_GL_SwapWindow(ckz::window);
    ckz::CountFrame(swapStart);
    target.BindFrameBuffer();
    if (scissor) {
        glEnable(GL_SCISSOR_TEST);
    }
}
