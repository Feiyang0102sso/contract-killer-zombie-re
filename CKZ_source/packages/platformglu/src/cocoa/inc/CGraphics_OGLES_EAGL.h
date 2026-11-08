// CGraphics_OGLES_EAGL.h - OpenGL ES platform glue for iOS (EAGL contexts on a CAEAGLLayer).
// C++ part in CGraphics_OGLES_EAGL.cpp, EAGL calls in CGraphics_OGLES_EAGL_mm.mm.
#ifndef CGRAPHICS_OGLES_EAGL_H
#define CGRAPHICS_OGLES_EAGL_H

#include "CProxy_OGLES.h"

#ifdef __OBJC__
@class NSString;
#else
typedef void NSString;
#endif

// ---- EAGL wrappers (CGraphics_OGLES_EAGL_mm.mm) ----
/** Retained EAGLContext for `api` (1: ES 1.1, 2: ES 2.0) drawing to `layer` in `colorFormat`. */
void* CGraphics_OGLES_EAGL_CreateContext(void* layer, unsigned int api, NSString* colorFormat);
void* CGraphics_OGLES_EAGL_CreateContext_R5G6B5(void* layer, unsigned int api);
void* CGraphics_OGLES_EAGL_CreateContext_A8B8G8R8(void* layer, unsigned int api);
void CGraphics_OGLES_EAGL_ReleaseContext(void* context);
void CGraphics_OGLES_EAGL_SetCurrentContext(void* context);
/** Allocate the bound renderbuffer's storage from `layer`. */
unsigned char CGraphics_OGLES_EAGL_RenderBufferStorage(void* layer, void* context);
void CGraphics_OGLES_EAGL_PresentRenderBuffer(void* context);

class CGraphics_OGLES_EAGL : public com::glu::platform::graphics::CProxy_OGLES {
public:
    /** Runtime class id of the EAGL proxy. */
    static const unsigned int ClassId = 0xee8f0ec9;

    CGraphics_OGLES_EAGL()
    {
        m_classId = ClassId;
    }

    virtual ~CGraphics_OGLES_EAGL() {}

    virtual void Present(com::glu::platform::graphics::ICRenderSurface& surface);
    virtual void* CreateContext(void* colorFormat, const int* attributes);
    virtual void ReleaseContext(void* context);
    virtual void SetCurrentContext(void* context, void* drawSurface, void* readSurface);

    /** Storage of the bound renderbuffer from the layer, for `context`. */
    unsigned char RenderBufferStorage(void* context);
};

#endif
