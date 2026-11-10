// Fake EGL for m3gcore on the desktop.
//
// The game owns exactly one GL context (created by SDL) and keeps it current. m3gcore still
// asks EGL for configs, contexts and surfaces, so this file answers every query as if that
// context were the only context and the only config, and surfaces were plain sizes.
// Nothing here touches GL; making a context current is always a no-op that succeeds.
#include <EGL/egl.h>
#include "CkzEgl.h"
#include <stdio.h>

namespace {

// Every handle only has to be non-null and unique; their contents are never read by m3gcore.
int displayTag;
int configTag;
int contextTag;
constexpr EGLint ConfigId = 1;
constexpr EGLint ColorBits = 8;
constexpr EGLint DepthBits = 24;
const char* const VersionString = "1.4 CKZ desktop shim";

/** A surface is just the size m3gcore reads back with eglQuerySurface. */
struct Surface {
    EGLint width;
    EGLint height;
};

EGLint lastError = EGL_SUCCESS;

Surface* AsSurface(EGLSurface surface)
{
    return static_cast<Surface*>(surface);
}

/** Read one attribute from an EGL_NONE terminated attribute list. */
EGLint FindAttribute(const EGLint* attributes, EGLint name, EGLint fallback)
{
    if (attributes == nullptr) {
        return fallback;
    }
    for (const EGLint* entry = attributes; entry[0] != EGL_NONE; entry += 2) {
        if (entry[0] == name) {
            return entry[1];
        }
    }
    return fallback;
}

} // namespace

namespace ckz::egl {

EGLSurface CreateSurface(EGLint width, EGLint height)
{
    Surface* surface = new Surface{ width, height };
    return surface;
}

void ResizeSurface(EGLSurface surface, EGLint width, EGLint height)
{
    AsSurface(surface)->width = width;
    AsSurface(surface)->height = height;
}

void DestroySurface(EGLSurface surface)
{
    delete AsSurface(surface);
}

} // namespace ckz::egl

extern "C" {

EGLDisplay eglGetDisplay(EGLNativeDisplayType)
{
    return &displayTag;
}

EGLDisplay eglGetCurrentDisplay(void)
{
    return &displayTag;
}

EGLBoolean eglInitialize(EGLDisplay, EGLint* major, EGLint* minor)
{
    if (major != nullptr) {
        *major = 1;
    }
    if (minor != nullptr) {
        *minor = 4;
    }
    return EGL_TRUE;
}

EGLBoolean eglTerminate(EGLDisplay)
{
    return EGL_TRUE;
}

EGLint eglGetError(void)
{
    // Like real EGL, reading the error resets it.
    const EGLint error = lastError;
    lastError = EGL_SUCCESS;
    return error;
}

const char* eglQueryString(EGLDisplay, EGLint name)
{
    if (name == EGL_VERSION) {
        return VersionString;
    }
    return "";
}

EGLBoolean eglChooseConfig(EGLDisplay, const EGLint*, EGLConfig* configs, EGLint size, EGLint* count)
{
    // The single config matches every request; m3gcore only needs one candidate.
    if (configs != nullptr && size > 0) {
        configs[0] = &configTag;
    }
    if (count != nullptr) {
        *count = 1;
    }
    return EGL_TRUE;
}

EGLBoolean eglGetConfigs(EGLDisplay, EGLConfig* configs, EGLint size, EGLint* count)
{
    if (configs != nullptr && size > 0) {
        configs[0] = &configTag;
    }
    if (count != nullptr) {
        *count = 1;
    }
    return EGL_TRUE;
}

EGLBoolean eglGetConfigAttrib(EGLDisplay, EGLConfig, EGLint attribute, EGLint* value)
{
    switch (attribute) {
    case EGL_CONFIG_ID:
        *value = ConfigId;
        return EGL_TRUE;
    case EGL_CONFIG_CAVEAT:
        *value = EGL_NONE; // hardware accelerated
        return EGL_TRUE;
    case EGL_SURFACE_TYPE:
        *value = EGL_PBUFFER_BIT | EGL_PIXMAP_BIT | EGL_WINDOW_BIT;
        return EGL_TRUE;
    case EGL_RED_SIZE:
    case EGL_GREEN_SIZE:
    case EGL_BLUE_SIZE:
    case EGL_ALPHA_SIZE:
        *value = ColorBits;
        return EGL_TRUE;
    case EGL_DEPTH_SIZE:
        *value = DepthBits;
        return EGL_TRUE;
    case EGL_SAMPLES:
    case EGL_SAMPLE_BUFFERS:
        *value = 0;
        return EGL_TRUE;
    }
    fprintf(stderr, "M3G EGL shim: unknown config attribute 0x%04x\n", attribute);
    return EGL_FALSE;
}

EGLContext eglCreateContext(EGLDisplay, EGLConfig, EGLContext, const EGLint*)
{
    return &contextTag;
}

EGLBoolean eglDestroyContext(EGLDisplay, EGLContext)
{
    return EGL_TRUE;
}

EGLBoolean eglQueryContext(EGLDisplay, EGLContext, EGLint attribute, EGLint* value)
{
    if (attribute == EGL_CONFIG_ID) {
        *value = ConfigId;
        return EGL_TRUE;
    }
    return EGL_FALSE;
}

EGLBoolean eglMakeCurrent(EGLDisplay, EGLSurface, EGLSurface, EGLContext)
{
    // The SDL context is always current; there is nothing to switch.
    return EGL_TRUE;
}

EGLSurface eglCreateWindowSurface(EGLDisplay, EGLConfig, EGLNativeWindowType, const EGLint*)
{
    fprintf(stderr, "M3G EGL shim: window surfaces are not supported\n");
    lastError = EGL_BAD_ALLOC;
    return EGL_NO_SURFACE;
}

EGLSurface eglCreatePixmapSurface(EGLDisplay, EGLConfig, EGLNativePixmapType, const EGLint*)
{
    fprintf(stderr, "M3G EGL shim: pixmap surfaces are not supported\n");
    lastError = EGL_BAD_ALLOC;
    return EGL_NO_SURFACE;
}

EGLSurface eglCreatePbufferSurface(EGLDisplay, EGLConfig, const EGLint* attributes)
{
    // m3gcore uses a pbuffer as a back buffer for buffered targets; it renders into the current
    // framebuffer like every other surface here.
    const EGLint width = FindAttribute(attributes, EGL_WIDTH, 0);
    const EGLint height = FindAttribute(attributes, EGL_HEIGHT, 0);
    fprintf(stderr, "M3G EGL shim: pbuffer %dx%d\n", width, height);
    return ckz::egl::CreateSurface(width, height);
}

EGLBoolean eglDestroySurface(EGLDisplay, EGLSurface surface)
{
    ckz::egl::DestroySurface(surface);
    return EGL_TRUE;
}

EGLBoolean eglQuerySurface(EGLDisplay, EGLSurface surface, EGLint attribute, EGLint* value)
{
    if (surface == EGL_NO_SURFACE) {
        lastError = EGL_BAD_SURFACE;
        return EGL_FALSE;
    }
    switch (attribute) {
    case EGL_WIDTH:
        *value = AsSurface(surface)->width;
        return EGL_TRUE;
    case EGL_HEIGHT:
        *value = AsSurface(surface)->height;
        return EGL_TRUE;
    case EGL_CONFIG_ID:
        *value = ConfigId;
        return EGL_TRUE;
    }
    return EGL_FALSE;
}

EGLBoolean eglSwapBuffers(EGLDisplay, EGLSurface)
{
    // Presenting stays with the game (SDL_GL_SwapWindow).
    return EGL_TRUE;
}

EGLBoolean eglCopyBuffers(EGLDisplay, EGLSurface, EGLNativePixmapType)
{
    return EGL_TRUE;
}

} // extern "C"
