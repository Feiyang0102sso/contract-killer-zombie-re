// CRenderSurface_OGLES_Texture_FBO.h - targetable texture surface rendered into through a framebuffer
// object (OES_framebuffer_object).
// (first drafted by b002 as PROVISIONAL; completed by b039x, the owner)
#ifndef CRENDERSURFACE_OGLES_TEXTURE_FBO_H
#define CRENDERSURFACE_OGLES_TEXTURE_FBO_H

#include "CRenderSurface_OGLES_Targetable.h"

namespace com { namespace glu { namespace platform { namespace graphics {

class CRenderSurface_OGLES_Texture_FBO : public CRenderSurface_OGLES_Targetable {
public:
    CRenderSurface_OGLES_Texture_FBO();
    virtual ~CRenderSurface_OGLES_Texture_FBO();

    virtual unsigned char Initialize(const InitParam* params);
    virtual void Destroy();
    virtual void Bind();
    virtual unsigned char SaveStateForFutureRecreation();
    virtual unsigned char Recreate();
    virtual void UpdateTargetWithTexture()
    {
    }
    virtual void RemoveSelfAsTarget();
    virtual unsigned char InitializeFrameBuffer();
    virtual void DestroyFrameBuffer();
    virtual void BindFrameBuffer();

    unsigned int m_framebuffer;         // 0x4c
    unsigned int m_depthRenderbuffer;   // 0x50
    unsigned int m_depthFormat;         // 0x54 GL format of the depth renderbuffer (0: none)
};

} } } }

#endif
