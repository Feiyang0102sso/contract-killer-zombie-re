/*
 * Minimal EGL 1.4 declarations for the m3gcore build (not the Khronos header).
 *
 * m3gcore manages GL contexts through EGL. On the desktop there is exactly one GL context,
 * the one SDL created and keeps current, so CkzEgl.cpp implements these calls as a fake
 * EGL in which that context is the only context and every surface is an off-screen size.
 * Only the names m3gcore uses are declared; values follow the EGL 1.4 specification.
 */
#ifndef CKZ_EGL_EGL_H
#define CKZ_EGL_EGL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t EGLint;
typedef unsigned int EGLBoolean;
typedef void* EGLDisplay;
typedef void* EGLConfig;
typedef void* EGLContext;
typedef void* EGLSurface;
typedef void* EGLNativeDisplayType;
typedef void* EGLNativeWindowType;
typedef void* EGLNativePixmapType;
// EGL 1.0 names that m3gcore still uses; the Khronos header keeps them as aliases too.
typedef EGLNativeWindowType NativeWindowType;
typedef EGLNativePixmapType NativePixmapType;

#define EGL_FALSE 0
#define EGL_TRUE 1
#define EGL_DEFAULT_DISPLAY ((EGLNativeDisplayType)0)
#define EGL_NO_DISPLAY ((EGLDisplay)0)
#define EGL_NO_CONTEXT ((EGLContext)0)
#define EGL_NO_SURFACE ((EGLSurface)0)

/* Errors */
#define EGL_SUCCESS 0x3000
#define EGL_NOT_INITIALIZED 0x3001
#define EGL_BAD_ALLOC 0x3003
#define EGL_BAD_SURFACE 0x300D

/* Config attributes */
#define EGL_ALPHA_SIZE 0x3021
#define EGL_BLUE_SIZE 0x3022
#define EGL_GREEN_SIZE 0x3023
#define EGL_RED_SIZE 0x3024
#define EGL_DEPTH_SIZE 0x3025
#define EGL_CONFIG_CAVEAT 0x3027
#define EGL_CONFIG_ID 0x3028
#define EGL_SAMPLES 0x3031
#define EGL_SAMPLE_BUFFERS 0x3032
#define EGL_SURFACE_TYPE 0x3033
#define EGL_NONE 0x3038
#define EGL_MATCH_NATIVE_PIXMAP 0x3041

/* Surface attributes and query names */
#define EGL_VERSION 0x3054
#define EGL_HEIGHT 0x3056
#define EGL_WIDTH 0x3057

/* EGL_SURFACE_TYPE bits */
#define EGL_PBUFFER_BIT 0x0001
#define EGL_PIXMAP_BIT 0x0002
#define EGL_WINDOW_BIT 0x0004

EGLDisplay eglGetDisplay(EGLNativeDisplayType display);
EGLDisplay eglGetCurrentDisplay(void);
EGLBoolean eglInitialize(EGLDisplay display, EGLint* major, EGLint* minor);
EGLBoolean eglTerminate(EGLDisplay display);
EGLint eglGetError(void);
const char* eglQueryString(EGLDisplay display, EGLint name);

EGLBoolean eglChooseConfig(EGLDisplay display, const EGLint* attributes, EGLConfig* configs,
                           EGLint size, EGLint* count);
EGLBoolean eglGetConfigs(EGLDisplay display, EGLConfig* configs, EGLint size, EGLint* count);
EGLBoolean eglGetConfigAttrib(EGLDisplay display, EGLConfig config, EGLint attribute, EGLint* value);

EGLContext eglCreateContext(EGLDisplay display, EGLConfig config, EGLContext share, const EGLint* attributes);
EGLBoolean eglDestroyContext(EGLDisplay display, EGLContext context);
EGLBoolean eglQueryContext(EGLDisplay display, EGLContext context, EGLint attribute, EGLint* value);
EGLBoolean eglMakeCurrent(EGLDisplay display, EGLSurface draw, EGLSurface read, EGLContext context);

EGLSurface eglCreateWindowSurface(EGLDisplay display, EGLConfig config, EGLNativeWindowType window,
                                  const EGLint* attributes);
EGLSurface eglCreatePixmapSurface(EGLDisplay display, EGLConfig config, EGLNativePixmapType pixmap,
                                  const EGLint* attributes);
EGLSurface eglCreatePbufferSurface(EGLDisplay display, EGLConfig config, const EGLint* attributes);
EGLBoolean eglDestroySurface(EGLDisplay display, EGLSurface surface);
EGLBoolean eglQuerySurface(EGLDisplay display, EGLSurface surface, EGLint attribute, EGLint* value);
EGLBoolean eglSwapBuffers(EGLDisplay display, EGLSurface surface);
EGLBoolean eglCopyBuffers(EGLDisplay display, EGLSurface surface, EGLNativePixmapType target);

#ifdef __cplusplus
}
#endif

#endif
