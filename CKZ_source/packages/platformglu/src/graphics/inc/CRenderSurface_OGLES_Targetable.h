// CRenderSurface_OGLES_Targetable.h - OpenGL ES texture surface that can be a render target; the
// target can be read back into the color buffer (Lock) and the texture drawn into it.
// (created by b039x, the owner)
#ifndef CRENDERSURFACE_OGLES_TARGETABLE_H
#define CRENDERSURFACE_OGLES_TARGETABLE_H

#include "CRenderSurface_OGLES_Texture.h"

namespace com { namespace glu { namespace platform { namespace graphics {

class CRenderSurface_OGLES_Targetable : public CRenderSurface_OGLES_Texture {
public:
    CRenderSurface_OGLES_Targetable();
    virtual ~CRenderSurface_OGLES_Targetable();

    virtual void Destroy();
    virtual void Lock(unsigned char** data, int& pitch, LockType lockType, const components::CRectangle<short>* area);
    virtual void Unlock();
    /** Draw the textures of the surface into the current target. */
    virtual void UpdateTargetWithTexture();
    /** Stop being the target of the device (unbind the framebuffer). */
    virtual void RemoveSelfAsTarget() = 0;

    unsigned int m_targetType;          // 0x40 kind of target; targets of one kind share the binding
    int m_lockCount;                    // 0x44
    unsigned char m_textureUpdated;     // 0x48 the textures were rebuilt by Unlock
};

} } } }

#endif
