// CGraphics_OGLES_Cocoa.h - the OpenGL ES graphics device on iOS: EAGL context, window surface,
// buffer presentation and multisample resolve.
#ifndef CGRAPHICS_OGLES_COCOA_H
#define CGRAPHICS_OGLES_COCOA_H

#include "CGraphics_OGLES.h"

/** EAGL framework version (CGraphics_OGLES_Cocoa_mm.mm). */
void CGraphics_OGLES_Cocoa_GetEAGLVersion(unsigned int* major, unsigned int* minor);

class CGraphics_OGLES_Cocoa : public com::glu::platform::graphics::CGraphics_OGLES {
public:
    explicit CGraphics_OGLES_Cocoa(EventHandler eventHandler) : CGraphics_OGLES(eventHandler)
    {
    }

    virtual unsigned char Initialize(const InitParam* params);
    virtual void SwapBuffers();
    virtual unsigned char InitializeWindowSurface(com::glu::platform::graphics::ICRenderSurface& surface);
    virtual void ReleaseDevice();
};

#endif
