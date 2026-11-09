// CRasterizerState_OGLES.cpp - rasterizer state of the OpenGL ES device.
// Every setter writes its table entry, then either sends the state to OpenGL at once or, while a
// display program is recorded, links the entry into the program slot of that state.
#include "CRasterizerState_OGLES.h"
#include "CGraphics_OGLES.h"
#include "CDisplayProgram.h"
#include "CRectangle.h"

namespace com { namespace glu { namespace platform { namespace graphics {

CRasterizerState_v1_OGLES::CRasterizerState_v1_OGLES()
{
}

void CRasterizerState_v1_OGLES::EnableBlending(unsigned char enable)
{
    unsigned int* blending = (unsigned int*)m_table.GetEntryData(m_blendingIndex);
    unsigned int flag = enable ? 0x80000000u : 0;
    blending[0] = (blending[0] & 0xffff) | flag;
    // the original links the entry into the *scissor* slot here (bug, kept)
    unsigned char** level = m_stateLevels[State_Scissor];
    if (level != NULL) {
        if (((CDisplayProgram::Instruction::Header*)*level)->m_size == CDisplayProgram::Instruction::LevelNodeSize) {
            *level = m_table.GetEntryHeader(m_scissorIndex);
        }
    } else {
        SyncBlending();
    }
}

void CRasterizerState_v1_OGLES::EnableDepthWrite(unsigned char enable)
{
    float* depth = (float*)m_table.GetEntryData(m_depthIndex);
    // ?: keeps the original select of the two float constants
    depth[1] = enable ? 1.0f : 0.0f;
    unsigned char** level = m_stateLevels[State_Depth];
    if (level != NULL) {
        if (((CDisplayProgram::Instruction::Header*)*level)->m_size == CDisplayProgram::Instruction::LevelNodeSize) {
            *level = m_table.GetEntryHeader(m_depthIndex);
        }
    } else {
        SyncDepth();
    }
}

void CRasterizerState_v1_OGLES::EnableScissorTest(unsigned char enable)
{
    int* scissor = (int*)m_table.GetEntryData(m_scissorIndex);
    scissor[0] = enable;
    unsigned char** level = m_stateLevels[State_Scissor];
    if (level != NULL) {
        if (((CDisplayProgram::Instruction::Header*)*level)->m_size == CDisplayProgram::Instruction::LevelNodeSize) {
            *level = m_table.GetEntryHeader(m_scissorIndex);
        }
    } else {
        SyncScissor();
    }
}

void CRasterizerState_v1_OGLES::SetBackfaceCulling(CullState state)
{
    CullState* culling = (CullState*)m_table.GetEntryData(m_backfaceCullingIndex);
    *culling = state;
    unsigned char** level = m_stateLevels[State_BackfaceCulling];
    if (level != NULL) {
        if (((CDisplayProgram::Instruction::Header*)*level)->m_size == CDisplayProgram::Instruction::LevelNodeSize) {
            *level = m_table.GetEntryHeader(m_backfaceCullingIndex);
        }
    } else {
        SyncBackfaceCulling();
    }
}

void CRasterizerState_v1_OGLES::SetBlendFactors(BlendFactor source, BlendFactor destination)
{
    unsigned int* blending = (unsigned int*)m_table.GetEntryData(m_blendingIndex);
    blending[1] = source << 24 | (source & 0xff) << 16 | (destination & 0xff) << 8 | (destination & 0xff);
    unsigned char** level = m_stateLevels[State_Blending];
    if (level != NULL) {
        if (((CDisplayProgram::Instruction::Header*)*level)->m_size == CDisplayProgram::Instruction::LevelNodeSize) {
            *level = m_table.GetEntryHeader(m_blendingIndex);
        }
    } else {
        SyncBlending();
    }
}

void CRasterizerState_v1_OGLES::SetBlendFactors(BlendFactor source, BlendFactor destination, BlendFactor sourceAlpha,
                                                BlendFactor destinationAlpha)
{
    unsigned int* blending = (unsigned int*)m_table.GetEntryData(m_blendingIndex);
    blending[1] = source << 24 | (destination & 0xff) << 16 | (sourceAlpha & 0xff) << 8 | (destinationAlpha & 0xff);
    unsigned char** level = m_stateLevels[State_Blending];
    if (level != NULL) {
        if (((CDisplayProgram::Instruction::Header*)*level)->m_size == CDisplayProgram::Instruction::LevelNodeSize) {
            *level = m_table.GetEntryHeader(m_blendingIndex);
        }
    } else {
        SyncBlending();
    }
}

void CRasterizerState_v1_OGLES::SetBlendOp(BlendOp op)
{
    unsigned short* blending = (unsigned short*)m_table.GetEntryData(m_blendingIndex);
    blending[0] = (unsigned short)((op & 0xff) << 8 | (op & 0xff));
    unsigned char** level = m_stateLevels[State_Blending];
    if (level != NULL) {
        if (((CDisplayProgram::Instruction::Header*)*level)->m_size == CDisplayProgram::Instruction::LevelNodeSize) {
            *level = m_table.GetEntryHeader(m_blendingIndex);
        }
    } else {
        SyncBlending();
    }
}

void CRasterizerState_v1_OGLES::SetBlendOps(BlendOp op, BlendOp opAlpha)
{
    unsigned short* blending = (unsigned short*)m_table.GetEntryData(m_blendingIndex);
    blending[0] = (unsigned short)(op << 8 | (opAlpha & 0xff));
    unsigned char** level = m_stateLevels[State_Blending];
    if (level != NULL) {
        if (((CDisplayProgram::Instruction::Header*)*level)->m_size == CDisplayProgram::Instruction::LevelNodeSize) {
            *level = m_table.GetEntryHeader(m_blendingIndex);
        }
    } else {
        SyncBlending();
    }
}

void CRasterizerState_v1_OGLES::SetColorMask(unsigned char red, unsigned char green, unsigned char blue,
                                             unsigned char alpha)
{
    unsigned int* mask = (unsigned int*)m_table.GetEntryData(m_colorMaskIndex);
    unsigned int value = 0;
    if (red) {
        value = 0x1000000;
    }
    if (green) {
        value |= 0x10000;
    }
    if (blue) {
        value |= 0x100;
    }
    if (alpha) {
        value |= 1;
    }
    *mask = value;
    unsigned char** level = m_stateLevels[State_ColorMask];
    if (level != NULL) {
        if (((CDisplayProgram::Instruction::Header*)*level)->m_size == CDisplayProgram::Instruction::LevelNodeSize) {
            *level = m_table.GetEntryHeader(m_colorMaskIndex);
        }
    } else {
        SyncColorMask();
    }
}

void CRasterizerState_v1_OGLES::SetDepthBias(float bias)
{
    float* depth = (float*)m_table.GetEntryData(m_depthIndex);
    depth[2] = bias;
    unsigned char** level = m_stateLevels[State_Depth];
    if (level != NULL) {
        if (((CDisplayProgram::Instruction::Header*)*level)->m_size == CDisplayProgram::Instruction::LevelNodeSize) {
            *level = m_table.GetEntryHeader(m_depthIndex);
        }
    } else {
        SyncDepth();
    }
}

void CRasterizerState_v1_OGLES::SetDepthRange(float nearValue, float farValue)
{
    float* depth = (float*)m_table.GetEntryData(m_depthIndex);
    depth[4] = nearValue;
    depth[5] = farValue;
    unsigned char** level = m_stateLevels[State_Depth];
    if (level != NULL) {
        if (((CDisplayProgram::Instruction::Header*)*level)->m_size == CDisplayProgram::Instruction::LevelNodeSize) {
            *level = m_table.GetEntryHeader(m_depthIndex);
        }
    } else {
        SyncDepth();
    }
}

void CRasterizerState_v1_OGLES::SetDepthTestFunc(DepthTestFunc func)
{
    float* depth = (float*)m_table.GetEntryData(m_depthIndex);
    depth[0] = (float)(unsigned int)func;
    unsigned char** level = m_stateLevels[State_Depth];
    if (level != NULL) {
        if (((CDisplayProgram::Instruction::Header*)*level)->m_size == CDisplayProgram::Instruction::LevelNodeSize) {
            *level = m_table.GetEntryHeader(m_depthIndex);
        }
    } else {
        SyncDepth();
    }
}

void CRasterizerState_v1_OGLES::SetScissor(short x, short y, short width, short height)
{
    int* scissor = (int*)m_table.GetEntryData(m_scissorIndex);
    scissor[1] = x;
    scissor[2] = y;
    scissor[3] = width;
    scissor[4] = height;
    unsigned char** level = m_stateLevels[State_Scissor];
    if (level != NULL) {
        if (((CDisplayProgram::Instruction::Header*)*level)->m_size == CDisplayProgram::Instruction::LevelNodeSize) {
            *level = m_table.GetEntryHeader(m_scissorIndex);
        }
    } else {
        SyncScissor();
    }
}

void CRasterizerState_v1_OGLES::PopState(State state)
{
    switch (state) {
    case State_BackfaceCulling:
        m_table.PopEntry(m_backfaceCullingIndex);
        if (m_stateLevels[State_BackfaceCulling] == NULL) {
            SyncBackfaceCulling();
        }
        break;
    case State_Blending:
        m_table.PopEntry(m_blendingIndex);
        if (m_stateLevels[State_Blending] == NULL) {
            SyncBlending();
        }
        break;
    case State_ColorMask:
        m_table.PopEntry(m_colorMaskIndex);
        if (m_stateLevels[State_ColorMask] == NULL) {
            SyncColorMask();
        }
        break;
    case State_Depth:
        m_table.PopEntry(m_depthIndex);
        if (m_stateLevels[State_Depth] == NULL) {
            SyncDepth();
        }
        break;
    case State_Scissor:
        m_table.PopEntry(m_scissorIndex);
        if (m_stateLevels[State_Scissor] == NULL) {
            SyncScissor();
        }
        break;
    }
}

void CRasterizerState_v1_OGLES::Sync()
{
    SyncBackfaceCulling();
    SyncBlending();
    SyncColorMask();
    SyncDepth();
    SyncScissor();
}

void CRasterizerState_v1_OGLES::SyncBackfaceCulling()
{
    CGraphics_OGLES* graphics = (CGraphics_OGLES*)m_graphics;
    CullState state = *(CullState*)m_table.GetEntryData(m_backfaceCullingIndex);
    // b147: case order as in the original (compare chain 2, 1, 0; blocks None, Clockwise, CounterClockwise)
    switch (state) {
    case CullState_None:
        graphics->m_glState.EnableCullFace(0);
        break;
    case CullState_Clockwise:
        graphics->m_glState.EnableCullFace(1);
        graphics->m_glState.CullFace(GL_BACK);
        graphics->m_glState.FrontFace(GL_CW);
        break;
    case CullState_CounterClockwise:
        graphics->m_glState.EnableCullFace(1);
        graphics->m_glState.CullFace(GL_BACK);
        graphics->m_glState.FrontFace(GL_CCW);
        break;
    }
}

void CRasterizerState_v1_OGLES::SyncColorMask()
{
    CGraphics_OGLES* graphics = (CGraphics_OGLES*)m_graphics;
    unsigned int mask = *(unsigned int*)m_table.GetEntryData(m_colorMaskIndex);
    graphics->m_glState.ColorMask((GLboolean)(mask >> 24), (GLboolean)(mask >> 16), (GLboolean)(mask >> 8),
                                  (GLboolean)mask);
}

/** OpenGL blend factor of a BlendFactor (0 for an unknown one). */
static inline GLenum CRasterizerState_v1_OGLES_GetGLBlendFactor(unsigned int factor)
{
    GLenum glFactor = GL_DST_ALPHA;
    switch (factor) {
    case ICRasterizerState::BlendFactor_DestAlpha:
        break;
    case ICRasterizerState::BlendFactor_DestColor:
        glFactor = GL_DST_COLOR;
        break;
    case ICRasterizerState::BlendFactor_One:
        glFactor = GL_ONE;
        break;
    case ICRasterizerState::BlendFactor_OneMinusDestAlpha:
        glFactor = GL_ONE_MINUS_DST_ALPHA;
        break;
    case ICRasterizerState::BlendFactor_OneMinusDestColor:
        glFactor = GL_ONE_MINUS_DST_COLOR;
        break;
    case ICRasterizerState::BlendFactor_OneMinusSrcAlpha:
        glFactor = GL_ONE_MINUS_SRC_ALPHA;
        break;
    case ICRasterizerState::BlendFactor_OneMinusSrcColor:
        glFactor = GL_ONE_MINUS_SRC_COLOR;
        break;
    case ICRasterizerState::BlendFactor_SrcAlpha:
        glFactor = GL_SRC_ALPHA;
        break;
    case ICRasterizerState::BlendFactor_SrcColor:
        glFactor = GL_SRC_COLOR;
        break;
    case ICRasterizerState::BlendFactor_Zero:  // b147: listed in the original's jump table (10 entries)
        glFactor = GL_ZERO;
        break;
    default:
        glFactor = GL_ZERO;
        break;
    }
    return glFactor;
}

void CRasterizerState_v1_OGLES::SyncBlending()
{
    CGraphics_OGLES* graphics = (CGraphics_OGLES*)m_graphics;
    unsigned int* blending = (unsigned int*)m_table.GetEntryData(m_blendingIndex);
    // b147: the original tests the enable bit with the blending branch first
    if (blending[0] & 0x80000000) {
        unsigned int factors = blending[1];
        GLenum source = CRasterizerState_v1_OGLES_GetGLBlendFactor((factors >> 16) & 0xff);
        GLenum destination = CRasterizerState_v1_OGLES_GetGLBlendFactor(factors & 0xff);
        graphics->m_glState.EnableBlend(1);
        graphics->m_glState.BlendFunc(source, destination);
    } else {
        graphics->m_glState.EnableBlend(0);
    }
}

void CRasterizerState_v1_OGLES::SyncDepth()
{
    CGraphics_OGLES* graphics = (CGraphics_OGLES*)m_graphics;
    float* depth = (float*)m_table.GetEntryData(m_depthIndex);
    switch ((DepthTestFunc)(unsigned int)depth[0]) {
    case DepthTestFunc_None:
        graphics->m_glState.EnableDepthTest(0);
        return;
    case DepthTestFunc_Equal:
        graphics->m_glState.DepthFunc(GL_EQUAL);
        break;
    case DepthTestFunc_GreaterEqual:
        graphics->m_glState.DepthFunc(GL_GEQUAL);
        break;
    case DepthTestFunc_Less:
        graphics->m_glState.DepthFunc(GL_LESS);
        break;
    case DepthTestFunc_LessEqual:
        graphics->m_glState.DepthFunc(GL_LEQUAL);
        break;
    }
    graphics->m_glState.EnableDepthTest(1);
    graphics->m_glState.DepthMask(depth[1]);
    graphics->m_glState.PolygonOffset(depth[2], 0.0f);
    graphics->m_glState.DepthRange(depth[4], depth[5]);
}

void CRasterizerState_v1_OGLES::SyncScissor()
{
    int* scissor = (int*)m_table.GetEntryData(m_scissorIndex);
    SyncScissorConsiderOrientationReinterpretation((unsigned char)scissor[0], (short)scissor[1], (short)scissor[2],
                                                   (short)scissor[3], (short)scissor[4]);
}

void CRasterizerState_v1_OGLES::SyncScissorConsiderOrientationReinterpretation(unsigned char enabled, short x, short y,
                                                                               short width, short height)
{
    CGraphics_OGLES* graphics = (CGraphics_OGLES*)ICGraphics::GetInstance();
    unsigned int targetWidth;
    unsigned int targetHeight;
    graphics->m_targetSurface->GetWidthAndHeight(targetWidth, targetHeight);
    if (enabled) {
        // a scissor covering the whole target is the same as no scissor
        if (x != 0 || y != 0 || width != (int)targetWidth || height != (int)targetHeight) {
            components::CRectangle<short> rectangle;
            rectangle.m_x = x;
            rectangle.m_y = y;
            rectangle.m_width = width;
            rectangle.m_height = height;
            components::CRectangle<short> transformed;
            transformed.m_x = 0;
            transformed.m_y = 0;
            transformed.m_width = 0;
            transformed.m_height = 0;
            graphics->OrientationReinterpretationYFlipTransform(rectangle, transformed);
            graphics->m_glState.EnableScissorTest(1);
            if (transformed.m_width != 0 && transformed.m_height != 0) {
                graphics->m_glState.Scissor(transformed.m_x, transformed.m_y, transformed.m_width,
                                            transformed.m_height);
            } else {
                graphics->m_glState.Scissor(0, 0, 0, 0);
            }
            return;
        }
    }
    graphics->m_glState.EnableScissorTest(0);
}

} } } }
