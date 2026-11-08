// CRenderSurface_OGLES_Window_Cocoa.h - the window (CAEAGLLayer) render surface: an EAGL context with
// a color renderbuffer backed by the layer, a depth renderbuffer and an optional multisample framebuffer
// that is resolved into the window framebuffer before presenting.
// (first drafted by b002 as PROVISIONAL; completed by b039x, the owner of
// cocoa/src/CRenderSurface_OGLES_Window_Cocoa.cpp)
#ifndef CRENDERSURFACE_OGLES_WINDOW_COCOA_H
#define CRENDERSURFACE_OGLES_WINDOW_COCOA_H

#include "CRenderSurface_OGLES_Texture_FBO.h"

class CRenderSurface_OGLES_Window_Cocoa : public com::glu::platform::graphics::CRenderSurface_OGLES_Texture_FBO {
public:
    /** State of the window framebuffer (m_framebufferStatus). */
    enum FramebufferStatus {
        FramebufferStatus_None = 0,
        FramebufferStatus_Complete = 1,
        FramebufferStatus_Incomplete = 2,
        FramebufferStatus_StorageFailed = 3     // the layer could not back the color renderbuffer
    };

    CRenderSurface_OGLES_Window_Cocoa();
    virtual ~CRenderSurface_OGLES_Window_Cocoa();

    virtual unsigned char Initialize(const InitParam* params);
    virtual void Destroy();
    /** Only a locked surface can be locked again (its color buffer holds the read back pixels). */
    virtual void Lock(unsigned char** data, int& pitch, LockType lockType,
                      const com::glu::platform::components::CRectangle<short>* area);
    virtual void Unlock();
    /** Size of the color buffer, swapped when the device is in landscape orientation. */
    virtual unsigned char GetWidthAndHeight(unsigned int& width, unsigned int& height);
    virtual unsigned char SaveStateForFutureRecreation();
    virtual unsigned char Recreate();
    virtual void RemoveSelfAsTarget();
    virtual unsigned char InitializeFrameBuffer();
    virtual void DestroyFrameBuffer();
    virtual void BindFrameBuffer();

    void* m_context;                                // 0x58 EAGLContext the surface presents with
    unsigned int m_colorRenderbuffer;               // 0x5c backed by the layer
    unsigned int m_multisampleFramebuffer;          // 0x60 0: no multisampling
    unsigned int m_multisampleColorRenderbuffer;    // 0x64
    unsigned int m_multisampleDepthRenderbuffer;    // 0x68
    int m_framebufferStatus;                        // 0x6c FramebufferStatus
    unsigned char m_keepContext;                    // 0x70 Initialize found a context: Destroy keeps it
};

#endif
