// PROVISIONAL (created by b002, layout completed by b037x): owner packages/platformglu/src/graphics/src/CRasterizerState_OGLES.cpp not written yet
// CRasterizerState_OGLES.h - rasterizer state of the OpenGL ES device (CRasterizerState_v1 that sends its
// values to OpenGL). Slot order from the vtable of CRasterizerState_v1_OGLES; size 0xb0.
#ifndef CRASTERIZERSTATE_OGLES_H
#define CRASTERIZERSTATE_OGLES_H

#include "CRasterizerState.h"

namespace com { namespace glu { namespace platform { namespace graphics {

class CRasterizerState_v1_OGLES : public CRasterizerState_v1 {
public:
    CRasterizerState_v1_OGLES();

    virtual ~CRasterizerState_v1_OGLES() {}

    virtual void EnableBlending(unsigned char enable);
    virtual void EnableDepthWrite(unsigned char enable);
    virtual void EnableScissorTest(unsigned char enable);
    virtual void SetBackfaceCulling(CullState state);
    virtual void SetBlendFactors(BlendFactor source, BlendFactor destination);
    virtual void SetBlendFactors(BlendFactor source, BlendFactor destination, BlendFactor sourceAlpha,
                                 BlendFactor destinationAlpha);
    virtual void SetBlendOp(BlendOp op);
    virtual void SetBlendOps(BlendOp op, BlendOp opAlpha);
    virtual void SetColorMask(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha);
    virtual void SetDepthBias(float bias);
    virtual void SetDepthRange(float nearValue, float farValue);
    virtual void SetDepthTestFunc(DepthTestFunc func);
    virtual void SetScissor(short x, short y, short width, short height);
    virtual void PopState(State state);
    /** Send every state to OpenGL. */
    virtual void Sync();
    virtual void SyncBackfaceCulling();
    virtual void SyncBlending();
    virtual void SyncColorMask();
    virtual void SyncDepth();
    virtual void SyncScissor();

    /** Send a scissor state to OpenGL, mapped to the target's orientation (used by the "Scissor" instruction). */  // added by b037x
    static void SyncScissorConsiderOrientationReinterpretation(unsigned char enabled, short x, short y, short width,  // added by b037x
                                                               short height);  // added by b037x
};

} } } }

#endif
