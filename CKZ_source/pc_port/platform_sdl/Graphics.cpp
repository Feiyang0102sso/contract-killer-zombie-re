// SDL owns the context. The existing window surface draws into its own FBO.
#include "Host.h"
#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>
#include "CGraphics_OGLES_EAGL.h"
#include "CRenderSurface_OGLES_Window_Cocoa.h"
#include <windows.h>
#include <stdio.h>
#include <filesystem>
#include <vector>

extern "C" void GL_APIENTRY ckzBlitFramebuffer(GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint,
                                                GLbitfield, GLenum);

using namespace com::glu::platform::graphics;

namespace ckz {
namespace {

bool frameDumpRequested;
int frameDumpCount;

// Frame rate log (CKZ_FPS_LOG): frames, average and longest frame time of each second, and the
// work time of a frame: from the end of one swap to the start of the next, i.e. without the wait
// for vertical sync. With vsync on the frame rate is capped by the display; the work time shows
// the headroom (165 fps needs under ~6 ms). The driver may block for vsync inside a later GL call,
// so the work time can still contain the wait; "cpu" is the main thread's CPU time per frame
// (GetThreadTimes, timer-tick resolution, fine over a second), which leaves sleeping out.
constexpr const char* FpsLogVariable = "CKZ_FPS_LOG";
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

/** Log once per second; `swapStart` is the time just before SDL_GL_SwapWindow. */
void LogFrameRate(Uint64 swapStart)
{
    static const bool enabled = SDL_getenv(FpsLogVariable) != nullptr;
    if (!enabled) {
        return;
    }
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
        fprintf(stderr, "FPS %d (avg %.2f ms, longest %.2f ms; work avg %.2f ms, longest %.2f ms; cpu %.2f ms)\n",
                fpsFrames, elapsedMs / fpsFrames, fpsLongestMs, workTotalMs / fpsFrames, workLongestMs,
                (cpuMs - cpuWindowStartMs) / fpsFrames);
        cpuWindowStartMs = cpuMs;
        fpsWindowStart = now;
        fpsFrames = 0;
        fpsLongestMs = 0.0;
        workTotalMs = 0.0;
        workLongestMs = 0.0;
    }
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
    // A playing movie covers the game picture, as the movie view did on iOS.
    if (!ckz::BlitMovieFrame(width, height)) {
        // Smooth the picture when the window is not exactly the game resolution.
        GLenum filter = GL_NEAREST;
        if (width != ckz::WindowWidth || height != ckz::WindowHeight) {
            filter = GL_LINEAR;
        }
        ckzBlitFramebuffer(0, 0, ckz::WindowWidth, ckz::WindowHeight,
                           0, 0, width, height, GL_COLOR_BUFFER_BIT, filter);
    }
    const Uint64 swapStart = SDL_GetTicksNS();
    SDL_GL_SwapWindow(ckz::window);
    ckz::LogFrameRate(swapStart);
    target.BindFrameBuffer();
    if (scissor) {
        glEnable(GL_SCISSOR_TEST);
    }
}
