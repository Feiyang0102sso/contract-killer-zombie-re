// CRasterizerState.h - device independent rasterizer state (created by b037x, owner).
// The state is kept in a typed variable table whose packed storage doubles as the
// "RasterizerState" display program instruction (one instruction per entry).
#ifndef CRASTERIZERSTATE_H
#define CRASTERIZERSTATE_H

#include "ICRasterizerState.h"
#include "CClass.h"
#include "CTypedVariableTable.h"

namespace com { namespace glu { namespace platform { namespace graphics {

class ICGraphics;

class CRasterizerState_v1 : public ICRasterizerState, public core::CClass {
public:
    /** Runtime class id. */
    static const unsigned int ClassId = 0xa6100536;

    CRasterizerState_v1();

    virtual ~CRasterizerState_v1() {}

    virtual void GetColorMaskState(unsigned char& red, unsigned char& green, unsigned char& blue,
                                   unsigned char& alpha);
    virtual void GetCullState(CullState& state);
    virtual void GetBlendState(unsigned char& enabled, BlendFactor& source, BlendFactor& destination,
                               BlendFactor& sourceAlpha, BlendFactor& destinationAlpha, BlendOp& op,
                               BlendOp& opAlpha);
    virtual void GetDepthState(DepthTestFunc& func, unsigned char& write, float& bias, float& nearValue,
                               float& farValue);
    virtual void GetScissorState(unsigned char& enabled, short& x, short& y, short& width, short& height);
    virtual void PushState(State state);

    ICGraphics* m_graphics;                         // 0x0c device the state belongs to
    components::CTypedVariableTable m_table;        // 0x10 entries below; the table is an instruction
    int m_backfaceCullingIndex;                     // 0x88 entry "BackfaceCulling": CullState
    int m_blendingIndex;                            // 0x8c entry "Blending": enable<<31 | op<<8 | opAlpha,
                                                    //      src<<24 | dst<<16 | srcAlpha<<8 | dstAlpha
    int m_colorMaskIndex;                           // 0x90 entry "ColorMask": r<<24 | g<<16 | b<<8 | a
    int m_depthIndex;                               // 0x94 entry "Depth": func, write, bias, -, near, far
    int m_scissorIndex;                             // 0x98 entry "Scissor": enable, x, y, width, height
    unsigned char** m_stateLevels[5];               // 0x9c per State: its slot in the display program being
                                                    //      recorded (set by the device's Begin)
};

} } } }

#endif
