// SDL owns the context. The existing window surface draws into its own FBO.
#include "Host.h"
#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>
#include "CGraphics_OGLES_EAGL.h"
#include "CRenderSurface_OGLES_Window_Cocoa.h"

extern "C" void GL_APIENTRY ckzBlitFramebuffer(GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint,
                                                GLbitfield, GLenum);

using namespace com::glu::platform::graphics;

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
    SDL_GL_SwapWindow(ckz::window);
    target.BindFrameBuffer();
    if (scissor) {
        glEnable(GL_SCISSOR_TEST);
    }
}
