// CProxy_OGLES.h - platform glue of the OpenGL ES renderer: context creation, current context and
// presentation (EGL / EAGL). Header-only interface, created by b002 (implemented by cocoa's
// CGraphics_OGLES_EAGL; no unit owns it).
#ifndef CPROXY_OGLES_H
#define CPROXY_OGLES_H

#include <stddef.h>
#include "CClass.h"

namespace com { namespace glu { namespace platform { namespace graphics {

class ICRenderSurface;

class CProxy_OGLES : public core::CClass {
public:
    CProxy_OGLES()
    {
        m_nativeWindow = NULL;
        m_context = NULL;
        m_versionMajor = 0;
        m_versionMinor = 0;
    }

    virtual ~CProxy_OGLES() {}

    /** Show the finished frame of the window surface. */
    virtual void Present(ICRenderSurface& surface) = 0;
    /** New rendering context for `colorFormat` (a components::ColorFormat); NULL on failure. */
    virtual void* CreateContext(void* colorFormat, const int* attributes) = 0;
    virtual void ReleaseContext(void* context) = 0;
    virtual void SetCurrentContext(void* context, void* drawSurface, void* readSurface) = 0;

    void* m_nativeWindow;           // 0x08 drawable (CAEAGLLayer on iOS)
    void* m_context;                // 0x0c current rendering context
    unsigned int m_versionMajor;    // 0x10 platform GL API version
    unsigned int m_versionMinor;    // 0x14
};

} } } }

#endif
