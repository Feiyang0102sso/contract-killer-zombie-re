// ICRasterizerState.h - fixed function rasterizer state of a device.
// (first drafted by b002; completed by b037x, owner of CRasterizerState_v1)
// Slot order from the vtable of CRasterizerState_v1_OGLES.
#ifndef ICRASTERIZERSTATE_H
#define ICRASTERIZERSTATE_H

namespace com { namespace glu { namespace platform { namespace graphics {

class ICRasterizerState {
public:
    /** Back face culling. */
    enum CullState {
        CullState_None = 0,
        CullState_Clockwise = 1,            // counter clockwise faces are in front
        CullState_CounterClockwise = 2      // clockwise faces are in front
    };

    /** Blend factors (OpenGL equivalents in the comments). */
    enum BlendFactor {
        BlendFactor_DestAlpha = 0,          // GL_DST_ALPHA
        BlendFactor_DestColor = 1,          // GL_DST_COLOR
        BlendFactor_One = 2,                // GL_ONE
        BlendFactor_OneMinusDestAlpha = 3,  // GL_ONE_MINUS_DST_ALPHA
        BlendFactor_OneMinusDestColor = 4,  // GL_ONE_MINUS_DST_COLOR
        BlendFactor_OneMinusSrcAlpha = 5,   // GL_ONE_MINUS_SRC_ALPHA
        BlendFactor_OneMinusSrcColor = 6,   // GL_ONE_MINUS_SRC_COLOR
        BlendFactor_SrcAlpha = 7,           // GL_SRC_ALPHA
        BlendFactor_SrcColor = 8,           // GL_SRC_COLOR
        BlendFactor_Zero = 9                // GL_ZERO
    };

    /** Blend equations. */
    enum BlendOp {
        BlendOp_Add = 0
    };

    /** Depth comparison (0 disables the depth test). */
    enum DepthTestFunc {
        DepthTestFunc_None = 0,
        DepthTestFunc_Equal = 1,            // GL_EQUAL
        DepthTestFunc_GreaterEqual = 2,     // GL_GEQUAL
        DepthTestFunc_Less = 3,             // GL_LESS
        DepthTestFunc_LessEqual = 4         // GL_LEQUAL
    };

    /** State groups of PushState / PopState. */
    enum State {
        State_BackfaceCulling = 0,
        State_Blending = 1,
        State_ColorMask = 2,
        State_Depth = 3,
        State_Scissor = 4
    };

    /** Platform factory (CGraphicsAbstractionManager_Cocoa.cpp). */
    static ICRasterizerState* CreateInstance();

    virtual ~ICRasterizerState() {}

    virtual void EnableBlending(unsigned char enable) = 0;
    virtual void EnableDepthWrite(unsigned char enable) = 0;
    virtual void EnableScissorTest(unsigned char enable) = 0;
    virtual void SetBackfaceCulling(CullState state) = 0;
    virtual void SetBlendFactors(BlendFactor source, BlendFactor destination) = 0;
    virtual void SetBlendFactors(BlendFactor source, BlendFactor destination, BlendFactor sourceAlpha,
                                 BlendFactor destinationAlpha) = 0;
    virtual void SetBlendOp(BlendOp op) = 0;
    virtual void SetBlendOps(BlendOp op, BlendOp opAlpha) = 0;
    virtual void SetColorMask(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha) = 0;
    virtual void SetDepthBias(float bias) = 0;
    virtual void SetDepthRange(float nearValue, float farValue) = 0;
    virtual void SetDepthTestFunc(DepthTestFunc func) = 0;
    /** Scissor rectangle. */
    virtual void SetScissor(short x, short y, short width, short height) = 0;
    virtual void GetColorMaskState(unsigned char& red, unsigned char& green, unsigned char& blue,
                                   unsigned char& alpha) = 0;
    virtual void GetCullState(CullState& state) = 0;
    virtual void GetBlendState(unsigned char& enabled, BlendFactor& source, BlendFactor& destination,
                               BlendFactor& sourceAlpha, BlendFactor& destinationAlpha, BlendOp& op,
                               BlendOp& opAlpha) = 0;
    virtual void GetDepthState(DepthTestFunc& func, unsigned char& write, float& bias, float& nearValue,
                               float& farValue) = 0;
    virtual void GetScissorState(unsigned char& enabled, short& x, short& y, short& width, short& height) = 0;
    /** Save the current values of `state`. */
    virtual void PushState(State state) = 0;
    /** Restore the values saved by the last PushState(state). */
    virtual void PopState(State state) = 0;
};

} } } }

#endif
