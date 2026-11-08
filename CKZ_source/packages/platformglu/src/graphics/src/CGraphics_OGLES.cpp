// CGraphics_OGLES.cpp - the OpenGL ES (1.1 fixed function) graphics device: display program
// instruction handlers, cached GL state, draws and the built-in shader program emulation.
#include <string.h>
#include "CGraphics_OGLES.h"
#include "CDisplayProgram_OGLES.h"
#include "CRasterizerState_OGLES.h"
#include "CRenderSurface_OGLES_Texture.h"
#include "CShaderProgram_OGLES.h"
#include "CRectangle.h"
#include "CEvent.h"
#include "CStdUtil_Cocoa.h"
#include "ICResourceManager.h"
#include "CResource.h"
#include "CMatrix4dh.h"
#include "CMathFixed.h"
#include "CBlitUtil.h"
#include "IntrinsicShaders.h"

namespace com { namespace glu { namespace platform { namespace graphics {

// events the device posts
static const unsigned int OGLES_Event_Destroy = 0x4979d834;
static const unsigned int OGLES_Event_BeforePresent = 0xc2b455cb;
static const unsigned int OGLES_Event_AfterPresent = 0x4b2237ac;
static const unsigned int OGLES_Event_TargetChanged = 0x34b4e700;
static const unsigned int OGLES_Event_IntrinsicShadersLoaded = 0x1386c162;

// level value of opcodes the recording mode ignores, size of the empty level nodes
static const unsigned int OGLES_IgnoredLevel = 0x7fff;
static const unsigned short OGLES_LevelNodeSize = 0xc;

// mime types of the built-in programs
static const unsigned int OGLES_ParameterDeclarationType = 0x2c435e5a;
static const unsigned int OGLES_ShaderProgramType = 0xb52802fb;

// alpha test of the fixed function pipeline
static const GLfloat OGLES_AlphaReference = 0.09999f;

/** Index of an intrinsic opcode in the opcode -> level table. */
#define OGLES_OPCODE_INDEX(opcode) ((opcode) & 0xff)

/** OpenGL type of a ComponentType (0 for types OpenGL ES 1 has no arrays of). */
static inline GLenum GetGLComponentType(unsigned int type)
{
    switch (type) {
    case ComponentType_Byte:
        return GL_BYTE;
    case ComponentType_UnsignedByte:
        return GL_UNSIGNED_BYTE;
    case ComponentType_Short:
        return GL_SHORT;
    case ComponentType_UnsignedShort:
        return GL_UNSIGNED_SHORT;
    case ComponentType_Float:
        return GL_FLOAT;
    case ComponentType_Fixed:
        return GL_FIXED;
    }
    return 0;
}

/** OpenGL mode of a primitive. */
static inline GLenum GetGLPrimitive(ICGraphics::Primitive primitive)
{
    switch (primitive) {
    case ICGraphics::Primitive_Lines:
        return GL_LINES;
    case ICGraphics::Primitive_LineStrip:
        return GL_LINE_STRIP;
    case ICGraphics::Primitive_Triangles:
        return GL_TRIANGLES;
    case ICGraphics::Primitive_TriangleStrip:
        return GL_TRIANGLE_STRIP;
    }
    return GL_POINTS;
}

/** OpenGL blend factor of an ICRasterizerState::BlendFactor. */
static inline GLenum GetGLBlendFactor(unsigned int factor)
{
    switch (factor) {
    case ICRasterizerState::BlendFactor_DestAlpha:
        return GL_DST_ALPHA;
    case ICRasterizerState::BlendFactor_DestColor:
        return GL_DST_COLOR;
    case ICRasterizerState::BlendFactor_One:
        return GL_ONE;
    case ICRasterizerState::BlendFactor_OneMinusDestAlpha:
        return GL_ONE_MINUS_DST_ALPHA;
    case ICRasterizerState::BlendFactor_OneMinusDestColor:
        return GL_ONE_MINUS_DST_COLOR;
    case ICRasterizerState::BlendFactor_OneMinusSrcAlpha:
        return GL_ONE_MINUS_SRC_ALPHA;
    case ICRasterizerState::BlendFactor_OneMinusSrcColor:
        return GL_ONE_MINUS_SRC_COLOR;
    case ICRasterizerState::BlendFactor_SrcAlpha:
        return GL_SRC_ALPHA;
    case ICRasterizerState::BlendFactor_SrcColor:
        return GL_SRC_COLOR;
    case ICRasterizerState::BlendFactor_Zero:
        return GL_ZERO;
    }
    return GL_ZERO;
}

// resource name of the (only) built-in program
static const char* const OGLES_IntrinsicShaderProgramName = "common_sdrpgm_mvp_conditional_constcolor_mod_tex0";

/** Name of built-in program `index` in the resources. */
static inline const char* GetIntrinsicShaderProgramName(int index)
{
    const char* name = NULL;
    if (index == 0) {
        name = "common_sdrpgm_mvp_conditional_constcolor_mod_tex0";
    }
    return name;
}

unsigned char CGraphics_OGLES_Handler_NameToInputStream(void* context, const char* name,
                                                        components::CInputStream& stream, unsigned int& type)
{
    if (strcmp(name, "common_mvp_conditional_constcolor_mod_tex0") != 0) {
        return 0;
    }
    CGraphics_OGLES* graphics = (CGraphics_OGLES*)context;
    graphics->m_shaderSource.Open(common_mvp_conditional_constcolor_mod_tex0_rawData,
                                  sizeof(common_mvp_conditional_constcolor_mod_tex0_rawData));
    stream.Open(&graphics->m_shaderSource, graphics->m_shaderSource.Available());
    type = OGLES_ParameterDeclarationType;
    return 1;
}

void foo(void* context, unsigned char* instruction)
{
}

void InstrBackfaceCulling(void* context, unsigned char* instruction)
{
    CGraphics_OGLES* graphics = (CGraphics_OGLES*)context;
    CGraphics_OGLES_GLState& state = graphics->m_glState;
    switch (*(unsigned int*)(instruction + 0x10)) {
    case ICRasterizerState::CullState_None:
        state.EnableCullFace(0);
        break;
    case ICRasterizerState::CullState_Clockwise:
        state.EnableCullFace(1);
        state.CullFace(GL_BACK);
        state.FrontFace(GL_CW);
        break;
    case ICRasterizerState::CullState_CounterClockwise:
        state.EnableCullFace(1);
        state.CullFace(GL_BACK);
        state.FrontFace(GL_CCW);
        break;
    }
}

void InstrBending(void* context, unsigned char* instruction)
{
    CGraphics_OGLES* graphics = (CGraphics_OGLES*)context;
    CGraphics_OGLES_GLState& state = graphics->m_glState;
    unsigned int* blending = (unsigned int*)(instruction + 0x10);
    if ((blending[0] & 0x80000000) != 0) {
        GLenum source = GetGLBlendFactor((blending[1] & 0xffffff) >> 16);
        GLenum destination = GetGLBlendFactor(blending[1] & 0xff);
        state.EnableBlend(1);
        state.BlendFunc(source, destination);
    } else {
        state.EnableBlend(0);
    }
}

void InstrBlendingInternalReset(void* context, unsigned char* instruction)
{
    CGraphics_OGLES* graphics = (CGraphics_OGLES*)context;
    unsigned int blending = *(unsigned int*)(instruction + 0x10);
    unsigned int factors = *(unsigned int*)(instruction + 0x14);
    graphics->m_rasterizerState->EnableBlending((unsigned char)(blending >> 31));
    graphics->m_rasterizerState->SetBlendFactors(
        (ICRasterizerState::BlendFactor)(factors >> 24), (ICRasterizerState::BlendFactor)((factors & 0xffffff) >> 16),
        (ICRasterizerState::BlendFactor)((factors & 0xffff) >> 8), (ICRasterizerState::BlendFactor)(factors & 0xff));
    graphics->m_rasterizerState->SetBlendOps((ICRasterizerState::BlendOp)((blending & 0xffff) >> 8),
                                             (ICRasterizerState::BlendOp)(blending & 0xff));
}

void InstrClearBuffers(void* context, unsigned char* instruction)
{
    CGraphics_OGLES* graphics = (CGraphics_OGLES*)context;
    CGraphics_OGLES_GLState& state = graphics->m_glState;
    components::CTypedVariableTable& table = graphics->m_clearTable;

    // the instruction is a copy of the clear table: read it through the table
    unsigned char* savedTable = NULL;
    if (instruction != NULL) {
        savedTable = table.m_table;
        table.m_table = instruction;
    }

    // the rasterizer scissor and color mask are restored afterwards
    unsigned char scissorEnabled;
    short scissorX;
    short scissorY;
    short scissorWidth;
    short scissorHeight;
    graphics->m_rasterizerState->GetScissorState(scissorEnabled, scissorX, scissorY, scissorWidth, scissorHeight);
    unsigned char maskRed;
    unsigned char maskGreen;
    unsigned char maskBlue;
    unsigned char maskAlpha;
    graphics->m_rasterizerState->GetColorMaskState(maskRed, maskGreen, maskBlue, maskAlpha);

    if (*table.GetEntryData(table.GetEntryIndex(L"ScissorTest")) == 0) {
        state.EnableScissorTest(0);
    } else {
        int* scissor = (int*)table.GetEntryData(table.GetEntryIndex(L"Scissor"));
        int x = scissor[0];
        int y = scissor[1];
        int width = scissor[2];
        int height = scissor[3];
        unsigned int targetWidth;
        unsigned int targetHeight;
        graphics->m_targetSurface->GetWidthAndHeight(targetWidth, targetHeight);
        if (((x | y) & 0xffff) == 0 && (short)width == (int)targetWidth && (short)height == (int)targetHeight) {
            // the whole target: no scissor needed
            state.EnableScissorTest(0);
        } else {
            components::CRectangle<short> rectangle;
            rectangle.m_x = (short)x;
            rectangle.m_y = (short)y;
            rectangle.m_width = (short)width;
            rectangle.m_height = (short)height;
            components::CRectangle<short> transformed;
            transformed.m_x = 0;
            transformed.m_y = 0;
            transformed.m_width = 0;
            transformed.m_height = 0;
            graphics->OrientationReinterpretationYFlipTransform(rectangle, transformed);
            state.EnableScissorTest(1);
            if (transformed.m_width != 0 && transformed.m_height != 0) {
                state.Scissor(transformed.m_x, transformed.m_y, transformed.m_width, transformed.m_height);
            } else {
                state.Scissor(0, 0, 0, 0);
            }
        }
    }

    GLbitfield clearMask = 0;
    unsigned int* flags = (unsigned int*)table.GetEntryData(table.GetEntryIndex(L"Flags"));
    if ((*flags & ICGraphics::ClearFlag_Color) != 0) {
        GLfloat* color = (GLfloat*)table.GetEntryData(table.GetEntryIndex(L"Color"));
        state.ClearColor(color[0], color[1], color[2], color[3]);
        clearMask = GL_COLOR_BUFFER_BIT;
    }
    unsigned int colorMask = *(unsigned int*)table.GetEntryData(table.GetEntryIndex(L"ColorMask"));
    state.ColorMask((GLboolean)(colorMask >> 24), (GLboolean)((colorMask >> 16) & 0xff),
                    (GLboolean)((colorMask >> 8) & 0xff), (GLboolean)(colorMask & 0xff));
    if ((*flags & ICGraphics::ClearFlag_Depth) != 0) {
        GLfloat* depth = (GLfloat*)table.GetEntryData(table.GetEntryIndex(L"Depth"));
        clearMask |= GL_DEPTH_BUFFER_BIT;
        state.ClearDepth(*depth);
    }
    if ((*flags & ICGraphics::ClearFlag_Stencil) != 0) {
        GLint* stencil = (GLint*)table.GetEntryData(table.GetEntryIndex(L"Stencil"));
        clearMask |= GL_STENCIL_BUFFER_BIT;
        state.ClearStencil(*stencil);
    }
    if (*flags != 0) {
        state.DepthMask(1);
        state.StencilMask(1);
        glClear(clearMask);
    }

    state.EnableScissorTest(scissorEnabled);
    state.Scissor(scissorX, scissorY, scissorWidth, scissorHeight);
    state.ColorMask(maskRed, maskGreen, maskBlue, maskAlpha);

    if (savedTable != NULL) {
        table.m_table = savedTable;
    }
}

void InstrColorMask(void* context, unsigned char* instruction)
{
    CGraphics_OGLES* graphics = (CGraphics_OGLES*)context;
    unsigned int mask = *(unsigned int*)(instruction + 0x10);
    graphics->m_glState.ColorMask((GLboolean)(mask >> 24), (GLboolean)((mask >> 16) & 0xff),
                                  (GLboolean)((mask >> 8) & 0xff), (GLboolean)(mask & 0xff));
}

void InstrDepth(void* context, unsigned char* instruction)
{
    CGraphics_OGLES* graphics = (CGraphics_OGLES*)context;
    CGraphics_OGLES_GLState& state = graphics->m_glState;
    float* depth = (float*)(instruction + 0x10);
    unsigned int depthTestFunc = (unsigned int)depth[0];
    if (depthTestFunc != ICRasterizerState::DepthTestFunc_None) {
        switch (depthTestFunc) {
        case ICRasterizerState::DepthTestFunc_LessEqual:
            state.DepthFunc(GL_LEQUAL);
            break;
        case ICRasterizerState::DepthTestFunc_GreaterEqual:
            state.DepthFunc(GL_GEQUAL);
            break;
        case ICRasterizerState::DepthTestFunc_Less:
            state.DepthFunc(GL_LESS);
            break;
        case ICRasterizerState::DepthTestFunc_Equal:
            state.DepthFunc(GL_EQUAL);
            break;
        }
        state.EnableDepthTest(1);
        state.DepthMask(depth[1]);
        state.PolygonOffset(depth[2], 0.0f);
        state.DepthRange(depth[4], depth[5]);
    } else {
        state.EnableDepthTest(0);
    }
}

void InstrDepthInternalReset(void* context, unsigned char* instruction)
{
    CGraphics_OGLES* graphics = (CGraphics_OGLES*)context;
    float* depth = (float*)(instruction + 0x10);
    graphics->m_rasterizerState->SetDepthTestFunc((ICRasterizerState::DepthTestFunc)(unsigned int)depth[0]);
    graphics->m_rasterizerState->EnableDepthWrite(depth[1] != 0.0f);
    graphics->m_rasterizerState->SetDepthBias(depth[2]);
    graphics->m_rasterizerState->SetDepthRange(depth[4], depth[5]);
}

/** Run a recorded DrawArraysInstruction. */
static inline void RunDrawArraysInstruction(CGraphics_OGLES* graphics, unsigned char* instruction,
                                            unsigned int opcode, ICGraphics::Primitive primitive)
{
    CGraphics_OGLES::DrawArraysInstruction* draw = (CGraphics_OGLES::DrawArraysInstruction*)instruction;
    unsigned int count = draw->m_count;
    unsigned int first = 0;
    if ((opcode & CDisplayProgram::Instruction::Opcode::Flag_Batch) == 0) {
        first = draw->m_first;
    }
    graphics->SetVertexStream(0, *draw->m_vertexStream);
    graphics->Draw(primitive, first, count);
}

/** Run a recorded DrawElementsInstruction. */
static inline void RunDrawElementsInstruction(CGraphics_OGLES* graphics, unsigned char* instruction,
                                              unsigned int opcode, ICGraphics::Primitive primitive)
{
    CGraphics_OGLES::DrawElementsInstruction* draw = (CGraphics_OGLES::DrawElementsInstruction*)instruction;
    ICBuffer::Stream* indices = draw->m_indexStream;
    unsigned int count = draw->m_count;
    unsigned int first = 0;
    if ((opcode & CDisplayProgram::Instruction::Opcode::Flag_Batch) == 0) {
        first = draw->m_first;
    }
    graphics->SetVertexStream(0, *draw->m_vertexStream);
    graphics->Draw(primitive, *indices, first, count);
}

void InstrDraw(void* context, unsigned char* instruction)
{
    CGraphics_OGLES* graphics = (CGraphics_OGLES*)context;
    unsigned int opcode = ((CDisplayProgram::Instruction::Header*)instruction)->m_opcode;
    switch (opcode) {
    case CDisplayProgram::Instruction::Opcode::VtxStreamLines:
    case CDisplayProgram::Instruction::Opcode::VtxStreamLinesBatch:
        RunDrawArraysInstruction(graphics, instruction, opcode, ICGraphics::Primitive_Lines);
        break;
    case CDisplayProgram::Instruction::Opcode::IdxVtxStreamLines:
    case CDisplayProgram::Instruction::Opcode::IdxVtxStreamLinesBatch:
        RunDrawElementsInstruction(graphics, instruction, opcode, ICGraphics::Primitive_Lines);
        break;
    case CDisplayProgram::Instruction::Opcode::VtxStreamLineStrip:
    case CDisplayProgram::Instruction::Opcode::VtxStreamLineStripBatch:
        RunDrawArraysInstruction(graphics, instruction, opcode, ICGraphics::Primitive_LineStrip);
        break;
    case CDisplayProgram::Instruction::Opcode::IdxVtxStreamLineStrip:
    case CDisplayProgram::Instruction::Opcode::IdxVtxStreamLineStripBatch:
        RunDrawElementsInstruction(graphics, instruction, opcode, ICGraphics::Primitive_LineStrip);
        break;
    case CDisplayProgram::Instruction::Opcode::VtxStreamTris:
    case CDisplayProgram::Instruction::Opcode::VtxStreamTrisBatch:
        RunDrawArraysInstruction(graphics, instruction, opcode, ICGraphics::Primitive_Triangles);
        break;
    case CDisplayProgram::Instruction::Opcode::IdxVtxStreamTris:
    case CDisplayProgram::Instruction::Opcode::IdxVtxStreamTrisBatch:
        RunDrawElementsInstruction(graphics, instruction, opcode, ICGraphics::Primitive_Triangles);
        break;
    case CDisplayProgram::Instruction::Opcode::VtxStreamTriStrip:
    case CDisplayProgram::Instruction::Opcode::VtxStreamTriStripBatch:
        RunDrawArraysInstruction(graphics, instruction, opcode, ICGraphics::Primitive_TriangleStrip);
        break;
    case CDisplayProgram::Instruction::Opcode::IdxVtxStreamTriStrip:
    case CDisplayProgram::Instruction::Opcode::IdxVtxStreamTriStripBatch:
        RunDrawElementsInstruction(graphics, instruction, opcode, ICGraphics::Primitive_TriangleStrip);
        break;
    }
}

void InstrScissor(void* context, unsigned char* instruction)
{
    int* scissor = (int*)(instruction + 0x10);
    CRasterizerState_v1_OGLES::SyncScissorConsiderOrientationReinterpretation(
        (unsigned char)scissor[0], (short)scissor[1], (short)scissor[2], (short)scissor[3], (short)scissor[4]);
}

void InstrScissorInternalReset(void* context, unsigned char* instruction)
{
    CGraphics_OGLES* graphics = (CGraphics_OGLES*)context;
    int* scissor = (int*)(instruction + 0x10);
    ICRasterizerState* rasterizer = graphics->m_rasterizerState;
    rasterizer->EnableScissorTest((unsigned char)scissor[0]);
    rasterizer->SetScissor((short)scissor[1], (short)scissor[2], (short)scissor[3], (short)scissor[4]);
}

void InstrShaders(void* context, unsigned char* instruction)
{
    CGraphics_OGLES* graphics = (CGraphics_OGLES*)context;
    ICShaderProgram* program = ((CGraphics::ShadersInstruction*)instruction)->m_program;
    if (program != NULL) {
        graphics->m_shaderProgram = program;
    }
}

void InstrShaderParams(void* context, unsigned char* instruction)
{
    CGraphics_OGLES* graphics = (CGraphics_OGLES*)context;
    graphics->m_shaderParamsInstruction = instruction;
}

void InstrTarget(void* context, unsigned char* instruction)
{
    ((CGraphics::TargetInstruction*)instruction)->m_surface->Bind();
}

void InstrTexure(void* context, unsigned char* instruction)
{
    CGraphics_OGLES* graphics = (CGraphics_OGLES*)context;
    CGraphics::TexturesInstruction* textures = (CGraphics::TexturesInstruction*)instruction;
    unsigned int count = textures->m_count;
    for (unsigned int index = 0; index < count; index++) {
        unsigned int unit = textures->m_units[index].m_unit;
        CRenderSurface_OGLES_Texture* surface = (CRenderSurface_OGLES_Texture*)textures->m_units[index].m_surface;
        if (surface->m_textureCount > 1) {
            // one texture per mip level
            unsigned int level = textures->m_units[index].m_level;
            graphics->m_glState.ActiveTexture(GL_TEXTURE0 + unit);
            graphics->m_glState.BindTexture(GL_TEXTURE_2D, surface->m_textures, level);
        } else {
            graphics->m_glState.ActiveTexture(GL_TEXTURE0 + unit);
            graphics->m_glState.BindTexture(GL_TEXTURE_2D, surface->m_texture);
        }
        graphics->m_textureUnits[unit].m_dirty = 0;
    }
}

void InstrTexureInternalReset(void* context, unsigned char* instruction)
{
    CGraphics_OGLES* graphics = (CGraphics_OGLES*)context;
    CGraphics::TexturesInstruction* textures = (CGraphics::TexturesInstruction*)instruction;
    unsigned int count = textures->m_count;
    for (unsigned int index = 0; index < count; index++) {
        graphics->SetTexture2d(textures->m_units[index].m_unit, *textures->m_units[index].m_surface);
    }
}

void InstrViewport(void* context, unsigned char* instruction)
{
    CGraphics_OGLES* graphics = (CGraphics_OGLES*)context;
    CGraphics::ViewportInstruction* viewport = (CGraphics::ViewportInstruction*)instruction;
    int x = viewport->m_x;
    int y = viewport->m_y;
    int width = viewport->m_width;
    int height = viewport->m_height;
    components::CRectangle<short> rectangle;
    rectangle.m_x = (short)x;
    rectangle.m_y = (short)y;
    rectangle.m_width = (short)width;
    rectangle.m_height = (short)height;
    components::CRectangle<short> transformed;
    transformed.m_x = 0;
    transformed.m_y = 0;
    transformed.m_width = 0;
    transformed.m_height = 0;
    graphics->OrientationReinterpretationYFlipTransform(rectangle, transformed);
    graphics->m_glState.Viewport(transformed.m_x, transformed.m_y, transformed.m_width, transformed.m_height);
}

void CGraphics_OGLES::DisplayProgramRecoveryCache::Config(CGraphics_OGLES* graphics)
{
    CRasterizerState_v1* rasterizer = (CRasterizerState_v1*)graphics->m_rasterizerState;
    m_viewportLevel = graphics->m_viewportLevel;
    m_shadersLevel = graphics->m_shadersLevel;
    m_texturesLevel = graphics->m_texturesLevel;
    m_rasterizerLevels[0] = rasterizer->m_stateLevels[0];
    m_rasterizerLevels[1] = rasterizer->m_stateLevels[1];
    m_rasterizerLevels[2] = rasterizer->m_stateLevels[2];
    m_rasterizerLevels[3] = rasterizer->m_stateLevels[3];
    m_rasterizerLevels[4] = rasterizer->m_stateLevels[4];
    m_vertexStream = graphics->m_vertexStreams[0];
    m_vertexStreamEnabled = graphics->m_vertexStreamEnabled[0];
    m_vertexStreamEnable = graphics->m_vertexStreamEnable[0];

    graphics->m_displayProgram = NULL;
    graphics->m_targetLevel = NULL;
    graphics->m_clearLevel = NULL;
    graphics->m_viewportLevel = NULL;
    graphics->m_shadersLevel = NULL;
    graphics->m_texturesLevel = NULL;
    graphics->m_shaderParamsLevel = NULL;
    graphics->m_shaderParamsInstruction = NULL;
    rasterizer->m_stateLevels[0] = NULL;
    rasterizer->m_stateLevels[1] = NULL;
    rasterizer->m_stateLevels[2] = NULL;
    rasterizer->m_stateLevels[3] = NULL;
    rasterizer->m_stateLevels[4] = NULL;
}

void CGraphics_OGLES::DisplayProgramRecoveryCache::Flush(CGraphics_OGLES* graphics)
{
    graphics->m_shaderParamsInstruction = NULL;

    if (m_viewportLevel != NULL) {
        CGraphics::ViewportInstruction* viewport = (CGraphics::ViewportInstruction*)*m_viewportLevel;
        if (viewport->m_header.m_size != OGLES_LevelNodeSize) {
            graphics->SetViewport((short)viewport->m_x, (short)viewport->m_y, (short)viewport->m_width,
                                  (short)viewport->m_height);
        }
    }
    if (m_shadersLevel != NULL) {
        CGraphics::ShadersInstruction* shaders = (CGraphics::ShadersInstruction*)*m_shadersLevel;
        if (shaders->m_header.m_size != OGLES_LevelNodeSize && shaders->m_program != NULL) {
            graphics->SetShaderProgram(*shaders->m_program);
        }
    }
    if (m_texturesLevel != NULL) {
        unsigned char* textures = *m_texturesLevel;
        if (((CDisplayProgram::Instruction::Header*)textures)->m_size != OGLES_LevelNodeSize) {
            InstrTexureInternalReset(graphics, textures);
        }
    }
    if (m_rasterizerLevels[ICRasterizerState::State_BackfaceCulling] != NULL) {
        unsigned char* culling = *m_rasterizerLevels[ICRasterizerState::State_BackfaceCulling];
        if (((CDisplayProgram::Instruction::Header*)culling)->m_size != OGLES_LevelNodeSize) {
            graphics->m_rasterizerState->SetBackfaceCulling(
                (ICRasterizerState::CullState)*(unsigned int*)(culling + 0x10));
        }
    }
    if (m_rasterizerLevels[ICRasterizerState::State_Blending] != NULL) {
        unsigned char* blending = *m_rasterizerLevels[ICRasterizerState::State_Blending];
        if (((CDisplayProgram::Instruction::Header*)blending)->m_size != OGLES_LevelNodeSize) {
            InstrBlendingInternalReset(graphics, blending);
        }
    }
    if (m_rasterizerLevels[ICRasterizerState::State_ColorMask] != NULL) {
        unsigned char* colorMask = *m_rasterizerLevels[ICRasterizerState::State_ColorMask];
        if (((CDisplayProgram::Instruction::Header*)colorMask)->m_size != OGLES_LevelNodeSize) {
            unsigned int mask = *(unsigned int*)(colorMask + 0x10);
            graphics->m_rasterizerState->SetColorMask((unsigned char)(mask >> 24),
                                                      (unsigned char)((mask & 0xffffff) >> 16),
                                                      (unsigned char)((mask & 0xffff) >> 8),
                                                      (unsigned char)(mask & 0xff));
        }
    }
    if (m_rasterizerLevels[ICRasterizerState::State_Depth] != NULL) {
        unsigned char* depth = *m_rasterizerLevels[ICRasterizerState::State_Depth];
        if (((CDisplayProgram::Instruction::Header*)depth)->m_size != OGLES_LevelNodeSize) {
            InstrDepthInternalReset(graphics, depth);
        }
    }
    if (m_rasterizerLevels[ICRasterizerState::State_Scissor] != NULL) {
        unsigned char* scissor = *m_rasterizerLevels[ICRasterizerState::State_Scissor];
        if (((CDisplayProgram::Instruction::Header*)scissor)->m_size != OGLES_LevelNodeSize) {
            InstrScissorInternalReset(graphics, scissor);
        }
    }
    if (m_vertexStream != NULL) {
        graphics->SetVertexStream(0, *m_vertexStream);
    }
    // the stream stays enabled when it was (or was about to be)
    graphics->EnableVertexStream(0, m_vertexStreamEnable || m_vertexStreamEnabled);
    if (m_vertexStream == NULL) {
        graphics->m_vertexStreams[0] = NULL;
    }
}

CGraphics_OGLES::CGraphics_OGLES(EventHandler eventHandler) : CGraphics(eventHandler)
{
    m_versionString = "";
    m_rendererString = "";
    m_versionMajor = 0;
    m_proxy = NULL;
    m_rasterizerState = NULL;
    m_resourceRouter = NULL;
    m_intrinsicAttributes = NULL;
    m_vertexStreams[0] = NULL;
    m_vertexStreamDirty[0] = 1;
    m_vertexStreamEnabled[0] = 0;
    m_vertexStreamEnable[0] = 0;
    m_streamArrays[0].Clear();
    m_arrayRequested = NULL;
    m_arrayEnabled = NULL;
    m_state304 = 0;
    m_defaultColorFormat = components::ColorFormat_Invalid;
    ResetFixedFunctionState();
    m_fixedFunction = 1;
    m_drawInstruction = NULL;
}

unsigned char CGraphics_OGLES::Initialize(const InitParam* params)
{
    m_rasterizerState = ICRasterizerState::CreateInstance();
    m_vertexStreams[0] = NULL;
    m_vertexStreamDirty[0] = 1;
    m_vertexStreamEnabled[0] = 0;
    m_arrayRequested = new int[ClientArray_Count];
    for (int array = 0; array < ClientArray_Count; array++) {
        m_arrayRequested[array] = 0;
    }
    m_arrayEnabled = new unsigned char[ClientArray_Count];
    for (int array = 0; array < ClientArray_Count; array++) {
        m_arrayEnabled[array] = 0;
    }
    m_vertexStreamEnable[0] = 0;
    m_vertexStreamFlag = 1;
    m_streamArrays[0].EnsureCapacity(16);
    m_streamArrays[0].Clear();

    // the vertex layouts the fixed function pipeline can draw
    m_intrinsicAttributes = new IntrinsicAttribute[43];
    for (int index = 0; index < 43; index++) {
        if (index < 3) {
            m_intrinsicAttributes[index].m_id = CVertex::Attribute::Id::Color;
            m_intrinsicAttributes[index].m_name = CVertex::Attribute::Id::GetInstrinsicName(CVertex::Attribute::Id::Color);
            m_intrinsicAttributes[index].m_count = 4;
        } else if (index < 7) {
            m_intrinsicAttributes[index].m_id = CVertex::Attribute::Id::Normal;
            m_intrinsicAttributes[index].m_name = CVertex::Attribute::Id::GetInstrinsicName(CVertex::Attribute::Id::Normal);
            m_intrinsicAttributes[index].m_count = 3;
        }
        if (index < 7) {
            switch (index) {
            case 0:
            case 3:
                m_intrinsicAttributes[index].m_type = ComponentType_Byte;
                break;
            case 4:
                m_intrinsicAttributes[index].m_type = ComponentType_Short;
                break;
            case 1:
            case 5:
                m_intrinsicAttributes[index].m_type = ComponentType_Int;
                break;
            case 2:
            case 6:
                m_intrinsicAttributes[index].m_type = ComponentType_Float;
                break;
            }
            continue;
        }

        int variant;
        if (index < 19) {
            m_intrinsicAttributes[index].m_id = CVertex::Attribute::Id::Position;
            m_intrinsicAttributes[index].m_name =
                CVertex::Attribute::Id::GetInstrinsicName(CVertex::Attribute::Id::Position);
            variant = index - 7;
        } else if (index < 31) {
            m_intrinsicAttributes[index].m_id = CVertex::Attribute::Id::TexCoord;
            m_intrinsicAttributes[index].m_name =
                CVertex::Attribute::Id::GetInstrinsicName(CVertex::Attribute::Id::TexCoord);
            variant = index - 19;
        } else {
            m_intrinsicAttributes[index].m_id = CVertex::Attribute::Id::TexCoord1;
            m_intrinsicAttributes[index].m_name =
                CVertex::Attribute::Id::GetInstrinsicName(CVertex::Attribute::Id::TexCoord1);
            variant = index - 31;
        }
        if (variant < 3) {
            m_intrinsicAttributes[index].m_type = ComponentType_Byte;
        } else if (variant < 6) {
            variant -= 3;
            m_intrinsicAttributes[index].m_type = ComponentType_Short;
        } else if (variant < 9) {
            variant -= 6;
            m_intrinsicAttributes[index].m_type = ComponentType_Int;
        } else {
            variant -= 9;
            m_intrinsicAttributes[index].m_type = ComponentType_Float;
        }
        if (variant == 2) {
            m_intrinsicAttributes[index].m_count = 4;
        } else if (variant == 1) {
            m_intrinsicAttributes[index].m_count = 3;
        } else if (variant == 0) {
            m_intrinsicAttributes[index].m_count = 2;
        }
    }

    m_resourceRouter = new components::CIdToObjectRouter;
    SetTargetSurface(*m_windowSurface);
    SetDefaultColorFormat(m_windowSurface->GetColorFormat());
    m_shaderProgram = NULL;
    m_displayProgram = NULL;
    InitializeIntrinsicHardwareShaderManagement(0);

    unsigned int width;
    unsigned int height;
    m_windowSurface->GetWidthAndHeight(width, height);
    SetClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    SetClearColorMask(1, 1, 1, 1);
    SetClearDepth(1.0f);
    SetClearStencil(0);
    SetClearScissor(0, 0, (short)width, (short)height);
    EnableClearScissor(0);
    SetViewport(0, 0, (short)width, (short)height);
    m_rasterizerState->EnableBlending(0);
    m_rasterizerState->EnableDepthWrite(1);
    m_rasterizerState->EnableScissorTest(0);
    m_rasterizerState->SetBackfaceCulling(ICRasterizerState::CullState_None);
    m_rasterizerState->SetBlendFactors(ICRasterizerState::BlendFactor_SrcAlpha,
                                       ICRasterizerState::BlendFactor_OneMinusSrcAlpha);
    m_rasterizerState->SetBlendOp(ICRasterizerState::BlendOp_Add);
    m_rasterizerState->SetColorMask(1, 1, 1, 1);
    m_rasterizerState->SetDepthBias(0.0f);
    m_rasterizerState->SetDepthRange(0.0f, 1.0f);
    m_rasterizerState->SetDepthTestFunc(ICRasterizerState::DepthTestFunc_None);
    m_rasterizerState->SetScissor(0, 0, (short)width, (short)height);
    m_shaderParamsInstruction = NULL;
    m_deviceInitialized = 1;

    m_drawInstruction = new unsigned char[100];
    m_defaultDisplayProgram = ICDisplayProgram::CreateInstance();
    ICDisplayProgram::InitParam programParams[1];
    programParams[0].m_key = ICDisplayProgram::InitParam::Key_End;
    programParams[0].m_value = 0;
    m_defaultDisplayProgram->Initialize(programParams);
    Sync();
    return m_deviceInitialized;
}

void CGraphics_OGLES::Destroy()
{
    systems::CEvent* event = new systems::CEvent(this, OGLES_Event_Destroy, 0, 0, components::CExecutable::OneShot);
    event->Run();

    if (m_defaultDisplayProgram != NULL) {
        delete m_defaultDisplayProgram;
        m_defaultDisplayProgram = NULL;
    }
    if (m_drawInstruction != NULL) {
        delete[] m_drawInstruction;
        m_drawInstruction = NULL;
    }
    DestroyIntrinsicSoftwareShaderManagement();
    m_shaderParameters.Destroy();
    if (m_resourceRouter != NULL) {
        delete m_resourceRouter;
        m_resourceRouter = NULL;
    }
    m_vertexStreams[0] = NULL;
    m_vertexStreamDirty[0] = 1;
    m_vertexStreamEnabled[0] = 0;
    m_vertexStreamEnable[0] = 0;
    m_streamArrays[0].Clear();
    if (m_arrayEnabled != NULL) {
        delete[] m_arrayEnabled;
        m_arrayEnabled = NULL;
    }
    if (m_arrayRequested != NULL) {
        delete[] m_arrayRequested;
        m_arrayRequested = NULL;
    }
    if (m_intrinsicAttributes != NULL) {
        delete[] m_intrinsicAttributes;
        m_intrinsicAttributes = NULL;
    }
    if (m_rasterizerState != NULL) {
        delete m_rasterizerState;
        m_rasterizerState = NULL;
    }
    m_state304 = 0;
    m_defaultColorFormat = components::ColorFormat_Invalid;
    if (m_windowSurface != NULL) {
        delete m_windowSurface;
        m_windowSurface = NULL;
    }
    m_targetSurface = NULL;
    ReleaseDevice();
    m_displayProgram = NULL;
    m_shaderParamsInstruction = NULL;
    ResetFixedFunctionState();
    DestroyCore();
    m_deviceInitialized = 0;
}

ICDisplayProgram* CGraphics_OGLES::Begin(ICDisplayProgram::Mode& mode, unsigned char keepState, unsigned char runOnEnd)
{
    if (m_defaultDisplayProgram == NULL) {
        return NULL;
    }
    Begin(*m_defaultDisplayProgram, mode, keepState, runOnEnd);
    return m_defaultDisplayProgram;
}

void CGraphics_OGLES::Begin(ICDisplayProgram& displayProgram, ICDisplayProgram::Mode& mode, unsigned char keepState,
                            unsigned char runOnEnd)
{
    CDisplayProgram_OGLES& program = static_cast<CDisplayProgram_OGLES&>(displayProgram);
    m_displayProgram = &displayProgram;
    program.m_deviceVersion = m_versionMajor;
    mode.m_configure(mode.m_context, *m_displayProgram);

    // target
    if (program.m_opcodeLevels[OGLES_OPCODE_INDEX(CDisplayProgram::Instruction::Opcode::Target)] == OGLES_IgnoredLevel) {
        m_targetLevel = NULL;
    } else {
        program.SetCallBackInstructionExecution(
            program.m_opcodeLevels[OGLES_OPCODE_INDEX(CDisplayProgram::Instruction::Opcode::Target)], this, InstrTarget);
        unsigned char** level =
            &program.m_levelInstructions[program.m_opcodeLevels[OGLES_OPCODE_INDEX(CDisplayProgram::Instruction::Opcode::Target)]];
        m_targetLevel = level;
        if (!keepState) {
            *level = (unsigned char*)m_targetInstruction;
        }
        // (the original stores the target level into the viewport instruction)
        m_viewportInstruction->m_header.m_level =
            (unsigned short)program.m_opcodeLevels[OGLES_OPCODE_INDEX(CDisplayProgram::Instruction::Opcode::Target)];
    }

    // clear buffers
    if (program.m_opcodeLevels[OGLES_OPCODE_INDEX(CDisplayProgram::Instruction::Opcode::ClearBuffers)] ==
        OGLES_IgnoredLevel) {
        m_clearLevel = NULL;
    } else {
        program.SetCallBackInstructionExecution(
            program.m_opcodeLevels[OGLES_OPCODE_INDEX(CDisplayProgram::Instruction::Opcode::ClearBuffers)], this,
            InstrClearBuffers);
        m_clearLevel = &program.m_levelInstructions[
            program.m_opcodeLevels[OGLES_OPCODE_INDEX(CDisplayProgram::Instruction::Opcode::ClearBuffers)]];
        ((CDisplayProgram::Instruction::Header*)m_clearTable.GetTableHeader())->m_level =
            (unsigned short)program.m_opcodeLevels[OGLES_OPCODE_INDEX(CDisplayProgram::Instruction::Opcode::ClearBuffers)];
    }

    // viewport
    if (program.m_opcodeLevels[OGLES_OPCODE_INDEX(CDisplayProgram::Instruction::Opcode::Viewport)] ==
        OGLES_IgnoredLevel) {
        m_viewportLevel = NULL;
    } else {
        program.SetCallBackInstructionExecution(
            program.m_opcodeLevels[OGLES_OPCODE_INDEX(CDisplayProgram::Instruction::Opcode::Viewport)], this,
            InstrViewport);
        unsigned char** level = &program.m_levelInstructions[
            program.m_opcodeLevels[OGLES_OPCODE_INDEX(CDisplayProgram::Instruction::Opcode::Viewport)]];
        m_viewportLevel = level;
        if (!keepState) {
            *level = (unsigned char*)m_viewportInstruction;
        }
        m_viewportInstruction->m_header.m_level =
            (unsigned short)program.m_opcodeLevels[OGLES_OPCODE_INDEX(CDisplayProgram::Instruction::Opcode::Viewport)];
    }

    // layer instructions do nothing when executed
    program.SetCallBackInstructionExecution(
        program.m_opcodeLevels[OGLES_OPCODE_INDEX(CDisplayProgram::Instruction::Opcode::Layer)], this, foo);

    if (program.m_opcodeLevels[OGLES_OPCODE_INDEX(CDisplayProgram::Instruction::Opcode::Blending)] !=
        OGLES_IgnoredLevel) {
        program.SetCallBackInstructionExecution(
            program.m_opcodeLevels[OGLES_OPCODE_INDEX(CDisplayProgram::Instruction::Opcode::Blending)], this,
            InstrBending);
    }

    // shaders
    if (program.m_opcodeLevels[OGLES_OPCODE_INDEX(CDisplayProgram::Instruction::Opcode::Shaders)] ==
        OGLES_IgnoredLevel) {
        m_shadersLevel = NULL;
    } else {
        SetCallBackInstructionExecutionShaders();
        unsigned char** level = &program.m_levelInstructions[
            program.m_opcodeLevels[OGLES_OPCODE_INDEX(CDisplayProgram::Instruction::Opcode::Shaders)]];
        m_shadersLevel = level;
        if (!keepState) {
            *level = (unsigned char*)m_shadersInstruction;
        }
        m_shadersInstruction->m_header.m_level =
            (unsigned short)program.m_opcodeLevels[OGLES_OPCODE_INDEX(CDisplayProgram::Instruction::Opcode::Shaders)];
    }

    // textures
    if (program.m_opcodeLevels[OGLES_OPCODE_INDEX(CDisplayProgram::Instruction::Opcode::Textures)] ==
        OGLES_IgnoredLevel) {
        m_texturesLevel = NULL;
    } else {
        program.SetCallBackInstructionExecution(
            program.m_opcodeLevels[OGLES_OPCODE_INDEX(CDisplayProgram::Instruction::Opcode::Textures)], this,
            InstrTexure);
        m_texturesLevel = &program.m_levelInstructions[
            program.m_opcodeLevels[OGLES_OPCODE_INDEX(CDisplayProgram::Instruction::Opcode::Textures)]];
        m_texturesInstruction->m_count = 0;
        if (!keepState) {
            *m_texturesLevel = (unsigned char*)m_texturesInstruction;
        }
        TexturesInstruction* textures = m_texturesInstruction;
        textures->m_header.m_size = sizeof(CDisplayProgram::Instruction::Header) + sizeof(unsigned int);
        textures->m_header.m_level =
            (unsigned short)program.m_opcodeLevels[OGLES_OPCODE_INDEX(CDisplayProgram::Instruction::Opcode::Textures)];
    }

    // rasterizer states
    if (program.m_opcodeLevels[OGLES_OPCODE_INDEX(CDisplayProgram::Instruction::Opcode::BackfaceCulling)] !=
        OGLES_IgnoredLevel) {
        program.SetCallBackInstructionExecution(
            program.m_opcodeLevels[OGLES_OPCODE_INDEX(CDisplayProgram::Instruction::Opcode::BackfaceCulling)], this,
            InstrBackfaceCulling);
    }
    if (program.m_opcodeLevels[OGLES_OPCODE_INDEX(CDisplayProgram::Instruction::Opcode::ColorMask)] !=
        OGLES_IgnoredLevel) {
        program.SetCallBackInstructionExecution(
            program.m_opcodeLevels[OGLES_OPCODE_INDEX(CDisplayProgram::Instruction::Opcode::ColorMask)], this,
            InstrColorMask);
    }
    if (program.m_opcodeLevels[OGLES_OPCODE_INDEX(CDisplayProgram::Instruction::Opcode::Depth)] != OGLES_IgnoredLevel) {
        program.SetCallBackInstructionExecution(
            program.m_opcodeLevels[OGLES_OPCODE_INDEX(CDisplayProgram::Instruction::Opcode::Depth)], this, InstrDepth);
    }
    if (program.m_opcodeLevels[OGLES_OPCODE_INDEX(CDisplayProgram::Instruction::Opcode::Scissor)] !=
        OGLES_IgnoredLevel) {
        program.SetCallBackInstructionExecution(
            program.m_opcodeLevels[OGLES_OPCODE_INDEX(CDisplayProgram::Instruction::Opcode::Scissor)], this,
            InstrScissor);
    }

    // shader parameters
    if (program.m_opcodeLevels[OGLES_OPCODE_INDEX(CDisplayProgram::Instruction::Opcode::ShaderParams)] ==
        OGLES_IgnoredLevel) {
        m_shaderParamsLevel = NULL;
    } else {
        program.SetCallBackInstructionExecution(
            program.m_opcodeLevels[OGLES_OPCODE_INDEX(CDisplayProgram::Instruction::Opcode::ShaderParams)], this,
            InstrShaderParams);
        m_shaderParamsLevel = &program.m_levelInstructions[
            program.m_opcodeLevels[OGLES_OPCODE_INDEX(CDisplayProgram::Instruction::Opcode::ShaderParams)]];
        ICShaderProgram::ParameterTable* parameters = NULL;
        if (m_shaderProgram != NULL) {
            parameters = ((CShaderProgram_OGLES*)m_shaderProgram)->m_parameterTable;
        }
        if (m_shaderProgram != NULL && parameters != NULL) {
            ((CDisplayProgram::Instruction::Header*)parameters->GetTableHeader())->m_level = (unsigned short)
                program.m_opcodeLevels[OGLES_OPCODE_INDEX(CDisplayProgram::Instruction::Opcode::ShaderParams)];
        }
    }

    // draws are the last level
    program.SetCallBackInstructionExecution(program.m_levelCount - 1, this, InstrDraw);
    program.CDisplayProgram::Reset(0);

    // the rasterizer table instructions get their levels
    CRasterizerState_v1* rasterizer = (CRasterizerState_v1*)m_rasterizerState;
    CDisplayProgram::Instruction::Header* header =
        (CDisplayProgram::Instruction::Header*)rasterizer->m_table.GetTableHeader();
    unsigned int entry = 0;
    while (true) {
        header->m_level = (unsigned short)program.m_opcodeLevels[OGLES_OPCODE_INDEX(header->m_opcode)];
        if (entry >= rasterizer->m_table.GetNumberOfEntries()) {
            break;
        }
        header = (CDisplayProgram::Instruction::Header*)rasterizer->m_table.GetEntryHeader(entry);
        entry++;
    }

    // rasterizer state slots
    if (program.m_opcodeLevels[OGLES_OPCODE_INDEX(CDisplayProgram::Instruction::Opcode::BackfaceCulling)] ==
        OGLES_IgnoredLevel) {
        rasterizer->m_stateLevels[ICRasterizerState::State_BackfaceCulling] = NULL;
    } else {
        unsigned char** level = &program.m_levelInstructions[
            program.m_opcodeLevels[OGLES_OPCODE_INDEX(CDisplayProgram::Instruction::Opcode::BackfaceCulling)]];
        rasterizer->m_stateLevels[ICRasterizerState::State_BackfaceCulling] = level;
        if (!keepState && ((CDisplayProgram::Instruction::Header*)*level)->m_size == OGLES_LevelNodeSize) {
            *level = rasterizer->m_table.GetEntryHeader(rasterizer->m_backfaceCullingIndex);
        }
    }
    if (program.m_opcodeLevels[OGLES_OPCODE_INDEX(CDisplayProgram::Instruction::Opcode::Blending)] ==
        OGLES_IgnoredLevel) {
        rasterizer->m_stateLevels[ICRasterizerState::State_Blending] = NULL;
    } else {
        unsigned char** level = &program.m_levelInstructions[
            program.m_opcodeLevels[OGLES_OPCODE_INDEX(CDisplayProgram::Instruction::Opcode::Blending)]];
        rasterizer->m_stateLevels[ICRasterizerState::State_Blending] = level;
        if (!keepState && ((CDisplayProgram::Instruction::Header*)*level)->m_size == OGLES_LevelNodeSize) {
            *level = rasterizer->m_table.GetEntryHeader(rasterizer->m_blendingIndex);
        }
    }
    if (program.m_opcodeLevels[OGLES_OPCODE_INDEX(CDisplayProgram::Instruction::Opcode::ColorMask)] ==
        OGLES_IgnoredLevel) {
        rasterizer->m_stateLevels[ICRasterizerState::State_ColorMask] = NULL;
    } else {
        unsigned char** level = &program.m_levelInstructions[
            program.m_opcodeLevels[OGLES_OPCODE_INDEX(CDisplayProgram::Instruction::Opcode::ColorMask)]];
        rasterizer->m_stateLevels[ICRasterizerState::State_ColorMask] = level;
        if (!keepState && ((CDisplayProgram::Instruction::Header*)*level)->m_size == OGLES_LevelNodeSize) {
            *level = rasterizer->m_table.GetEntryHeader(rasterizer->m_colorMaskIndex);
        }
    }
    if (program.m_opcodeLevels[OGLES_OPCODE_INDEX(CDisplayProgram::Instruction::Opcode::Depth)] == OGLES_IgnoredLevel) {
        rasterizer->m_stateLevels[ICRasterizerState::State_Depth] = NULL;
    } else {
        unsigned char** level = &program.m_levelInstructions[
            program.m_opcodeLevels[OGLES_OPCODE_INDEX(CDisplayProgram::Instruction::Opcode::Depth)]];
        rasterizer->m_stateLevels[ICRasterizerState::State_Depth] = level;
        if (!keepState && ((CDisplayProgram::Instruction::Header*)*level)->m_size == OGLES_LevelNodeSize) {
            *level = rasterizer->m_table.GetEntryHeader(rasterizer->m_depthIndex);
        }
    }
    if (program.m_opcodeLevels[OGLES_OPCODE_INDEX(CDisplayProgram::Instruction::Opcode::Scissor)] ==
        OGLES_IgnoredLevel) {
        rasterizer->m_stateLevels[ICRasterizerState::State_Scissor] = NULL;
    } else {
        unsigned char** level = &program.m_levelInstructions[
            program.m_opcodeLevels[OGLES_OPCODE_INDEX(CDisplayProgram::Instruction::Opcode::Scissor)]];
        rasterizer->m_stateLevels[ICRasterizerState::State_Scissor] = level;
        if (!keepState && ((CDisplayProgram::Instruction::Header*)*level)->m_size == OGLES_LevelNodeSize) {
            *level = rasterizer->m_table.GetEntryHeader(rasterizer->m_scissorIndex);
        }
    }

    program.m_nodeChunks.m_current = program.m_nodeChunks.m_head;
    for (unsigned int level = 0; level < program.m_levelCount; level++) {
        program.m_levelLastAdded[level] = NULL;
    }
    m_runOnEnd = runOnEnd;
    program.m_batchDepth = 0;
}

void CGraphics_OGLES::End(ICDisplayProgram& program)
{
    unsigned char runProgram = m_runOnEnd;
    if (!runProgram) {
        m_recoveryCache.Config(this);
    }
    if (m_runOnEnd) {
        m_runOnEnd = 0;
        Run(program);
    }
    if (!runProgram) {
        m_recoveryCache.Flush(this);
    }
}

void CGraphics_OGLES::Run(ICDisplayProgram& program)
{
    m_recoveryCache.Config(this);
    EnableVertexStream(0, 1);
    program.Run();
    m_recoveryCache.Flush(this);
}

void CGraphics_OGLES::ClearBuffers(unsigned int flags)
{
    unsigned int* stamp = (unsigned int*)m_clearTable.GetEntryData(ClearEntry_Stamp);
    *stamp = CStdUtil_Cocoa::GetUpTimeMS();
    *(unsigned int*)m_clearTable.GetEntryData(ClearEntry_Flags) = flags;
    unsigned char** clearLevel = m_clearLevel;
    if (clearLevel != NULL) {
        if (((CDisplayProgram::Instruction::Header*)*clearLevel)->m_size == OGLES_LevelNodeSize) {
            *clearLevel = m_clearTable.GetTableHeader();
        }
    } else {
        // not recording: clear right away
        InstrClearBuffers(this, NULL);
    }
}

void CGraphics_OGLES::SwapBuffers()
{
    systems::CEvent* event =
        new systems::CEvent(this, OGLES_Event_BeforePresent, 0, 0, components::CExecutable::OneShot);
    event->Run();
    m_proxy->Present(*m_targetSurface);
    event = new systems::CEvent(this, OGLES_Event_AfterPresent, 0, 0, components::CExecutable::OneShot);
    event->Run();
}

void CGraphics_OGLES::EnableClearScissor(unsigned char enable)
{
    *m_clearTable.GetEntryData(ClearEntry_ScissorTest) = enable;
}

void CGraphics_OGLES::SetClearScissor(short x, short y, short width, short height)
{
    int* scissor = (int*)m_clearTable.GetEntryData(ClearEntry_Scissor);
    scissor[0] = x;
    scissor[1] = y;
    scissor[2] = width;
    scissor[3] = height;
}

void CGraphics_OGLES::SetClearColor(float red, float green, float blue, float alpha)
{
    float* color = (float*)m_clearTable.GetEntryData(ClearEntry_Color);
    color[0] = red;
    color[1] = green;
    color[2] = blue;
    color[3] = alpha;
}

void CGraphics_OGLES::SetClearColor(const components::Color_RGBA& color)
{
    float* clearColor = (float*)m_clearTable.GetEntryData(ClearEntry_Color);
    clearColor[0] = color.r;
    clearColor[1] = color.g;
    clearColor[2] = color.b;
    clearColor[3] = color.a;
}

void CGraphics_OGLES::SetClearColorMask(unsigned char red, unsigned char green, unsigned char blue,
                                        unsigned char alpha)
{
    unsigned int* mask = (unsigned int*)m_clearTable.GetEntryData(ClearEntry_ColorMask);
    // ?: kept on purpose: the original materialises red as "mov 0; movne 0x1000000"
    unsigned int value = red ? 0x1000000 : 0;
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
}

void CGraphics_OGLES::SetClearDepth(float depth)
{
    *(float*)m_clearTable.GetEntryData(ClearEntry_Depth) = depth;
}

void CGraphics_OGLES::SetClearStencil(int stencil)
{
    *(int*)m_clearTable.GetEntryData(ClearEntry_Stencil) = stencil;
}

void CGraphics_OGLES::SetShaderProgram(ICShaderProgram& program)
{
    m_shaderProgram = &program;
    m_shadersInstruction->m_program = &program;
    if (m_shadersLevel != NULL) {
        if (((CDisplayProgram::Instruction::Header*)*m_shadersLevel)->m_size == OGLES_LevelNodeSize) {
            *m_shadersLevel = (unsigned char*)m_shadersInstruction;
        }
        NotifyOfChangeInShaderProgramTableData(*m_shaderProgram);
    }
    if (m_vertexStreams[0] != NULL) {
        SetVertexStream(0, *m_vertexStreams[0]);
    }
}

void CGraphics_OGLES::NotifyOfChangeInShaderProgramTableData(ICShaderProgram& program)
{
    CDisplayProgram* displayProgram = static_cast<CDisplayProgram*>(m_displayProgram);
    if (displayProgram == NULL || m_shaderProgram != &program) {
        return;
    }
    unsigned int level =
        displayProgram->m_opcodeLevels[OGLES_OPCODE_INDEX(CDisplayProgram::Instruction::Opcode::ShaderParams)];
    if (level == OGLES_IgnoredLevel) {
        return;
    }
    CShaderProgram_OGLES* programView = (CShaderProgram_OGLES*)m_shaderProgram;
    displayProgram->m_levelInstructions[level] = programView->m_parameterTable->m_table;
    CDisplayProgram::Instruction::Header* header =
        (CDisplayProgram::Instruction::Header*)programView->m_parameterTable->GetTableHeader();
    header->m_child = NULL;
    header->m_sibling = NULL;
    header->m_size = (unsigned short)programView->m_parameterTable->GetTableSize();
    header->m_level = (unsigned short)level;
    header->m_opcode = CDisplayProgram::Instruction::Opcode::ShaderParams;
}

ICShaderProgram* CGraphics_OGLES::GetShaderProgram()
{
    return m_shaderProgram;
}

void CGraphics_OGLES::SetTextureSamplerHint(unsigned int unit, TextureSampleHint hint)
{
    m_textureSampleHints[unit] = hint;
}

void CGraphics_OGLES::SetTexture2d(unsigned int unit, ICRenderSurface& surface)
{
    SetTexture2d(unit, surface, 0);
}

/** A "Textures" instruction with one unit. */
struct OGLES_SingleTextureInstruction {
    CDisplayProgram::Instruction::Header m_header;  // 0x00
    unsigned int m_count;                           // 0x10
    CGraphics::TexturesInstruction::Unit m_unit;    // 0x14
};

void CGraphics_OGLES::SetTexture2d(unsigned int unit, ICRenderSurface& surface, unsigned int level)
{
    static const OGLES_SingleTextureInstruction texture = {
        { NULL, NULL, sizeof(OGLES_SingleTextureInstruction), 0, CDisplayProgram::Instruction::Opcode::Textures },
        1,
        { 0, NULL, 0 }
    };

    m_textureUnits[unit].m_surface = &surface;
    m_textureUnits[unit].m_level = level;
    m_textureUnits[unit].m_dirty = 1;
    if (m_texturesLevel != NULL) {
        // recording: add / update the unit in the "Textures" instruction
        TexturesInstruction* textures = m_texturesInstruction;
        if (((CDisplayProgram::Instruction::Header*)*m_texturesLevel)->m_size == OGLES_LevelNodeSize) {
            *m_texturesLevel = (unsigned char*)textures;
        }
        TexturesInstruction::Unit* entry = textures->m_units;
        unsigned int count = textures->m_count;
        unsigned int index;
        for (index = 0; index < count; index++) {
            if (entry->m_unit == unit) {
                if (entry->m_surface != &surface) {
                    entry->m_surface = &surface;
                }
                if (entry->m_level != level) {
                    entry->m_level = level;
                    count = textures->m_count;
                }
                break;
            }
            entry++;
        }
        if (count == index) {
            textures->m_count = count + 1;
            entry->m_unit = unit;
            entry->m_surface = &surface;
            entry->m_level = level;
            m_texturesInstruction->m_header.m_size += sizeof(TexturesInstruction::Unit);
        }
    } else {
        // not recording: bind right away
        OGLES_SingleTextureInstruction instruction = texture;
        instruction.m_unit.m_unit = unit;
        instruction.m_unit.m_surface = &surface;
        instruction.m_unit.m_level = level;
        InstrTexure(this, (unsigned char*)&instruction);
    }
}

void CGraphics_OGLES::SetViewport(short x, short y, short width, short height)
{
    ViewportInstruction* viewport = m_viewportInstruction;
    viewport->m_x = x;
    viewport->m_y = y;
    viewport->m_width = width;
    viewport->m_height = height;
    if (m_viewportLevel != NULL) {
        if (((CDisplayProgram::Instruction::Header*)*m_viewportLevel)->m_size == OGLES_LevelNodeSize) {
            *m_viewportLevel = (unsigned char*)m_viewportInstruction;
        }
    } else {
        // not recording: set the viewport right away
        InstrViewport(this, (unsigned char*)m_viewportInstruction);
    }
}

void CGraphics_OGLES::SetTargetSurface(ICRenderSurface& surface)
{
    surface.Bind();
    m_targetInstruction->m_surface = m_targetSurface;
    if (m_targetLevel != NULL) {
        if (((CDisplayProgram::Instruction::Header*)*m_targetLevel)->m_size == OGLES_LevelNodeSize) {
            *m_targetLevel = (unsigned char*)m_targetInstruction;
        }
        if (m_clearLevel != NULL) {
            // a new target starts without a pending clear
            CDisplayProgram* program = static_cast<CDisplayProgram*>(m_displayProgram);
            unsigned int level =
                program->m_opcodeLevels[OGLES_OPCODE_INDEX(CDisplayProgram::Instruction::Opcode::ClearBuffers)];
            program->m_levelInstructions[level] = (unsigned char*)program->m_levelNodes[level];
        }
    }
    systems::CEvent* event =
        new systems::CEvent(this, OGLES_Event_TargetChanged, 0, 0, components::CExecutable::OneShot);
    event->Run();
}

ICRenderSurface* CGraphics_OGLES::GetTargetSurface() const
{
    return m_targetSurface;
}

void CGraphics_OGLES::Draw(Primitive primitive)
{
    Draw(primitive, 0, 0x7fffffff);
}

void CGraphics_OGLES::Draw(Primitive primitive, unsigned int first, unsigned int count)
{
    if (m_displayProgram != NULL) {
#include "CGraphics_OGLES_DrawArraysAddToDisplayProgram.i"
    }
    ICShaderProgram* pShaderProgram = m_shaderProgram;
#include "CGraphics_OGLES_UpdateAttributes.i"
#include "CGraphics_OGLES_UpdateShaderParameters.i"
#include "CGraphics_OGLES_DrawArrays.i"
}

void CGraphics_OGLES::Draw(Primitive primitive, ICBuffer::Stream& indices)
{
    Draw(primitive, indices, 0, 0x7fffffff);
}

void CGraphics_OGLES::Draw(Primitive primitive, ICBuffer::Stream& indices, unsigned int first, unsigned int count)
{
    if (m_displayProgram != NULL) {
#include "CGraphics_OGLES_DrawElementsAddToDisplayProgram.i"
    }
    ICShaderProgram* pShaderProgram = m_shaderProgram;
#include "CGraphics_OGLES_UpdateAttributes.i"
#include "CGraphics_OGLES_UpdateShaderParameters.i"
#include "CGraphics_OGLES_DrawElements.i"
}

void CGraphics_OGLES::SetVertexStream(unsigned int index, ICBuffer::Stream& stream)
{
    if (m_vertexStreams[index] != NULL) {
        // switch streams: the new one is enabled when the old one was
        unsigned char enabled = m_vertexStreamEnabled[index];
        unsigned char enable = m_vertexStreamEnable[index];
        EnableVertexStream(index, 0);
        m_vertexStreams[index] = &stream;
        m_vertexStreamDirty[index] = 1;
        if (enabled) {
            m_vertexStreamEnable[index] = 1;
        } else {
            m_vertexStreamEnable[index] = enable;
        }
    } else {
        m_vertexStreams[index] = &stream;
        m_vertexStreamDirty[index] = 1;
    }
}

void CGraphics_OGLES::EnableVertexStream(unsigned int index, unsigned char enable)
{
    if (!enable && m_vertexStreamEnabled[index]) {
        int count = m_streamArrays[index].m_size;
        for (int array = 0; array < count; array++) {
            m_arrayRequested[m_streamArrays[index].m_data[array]] = 0;
        }
        m_vertexStreamEnabled[index] = 0;
    }
    m_vertexStreamEnable[index] = enable;
}

void CGraphics_OGLES::SyncCore()
{
    m_glState.Invalidate();
    glDisable(GL_DITHER);
    for (int unit = 0; unit < TextureUnitCount; unit++) {
        if (m_textureUnits[unit].m_surface != NULL) {
            CRenderSurface_OGLES_Texture* surface = (CRenderSurface_OGLES_Texture*)m_textureUnits[unit].m_surface;
            if (surface->m_textureCount > 1) {
                // one texture per mip level
                m_glState.ActiveTexture(GL_TEXTURE0 + unit);
                m_glState.BindTexture(GL_TEXTURE_2D, surface->m_textures, m_textureUnits[unit].m_level);
            } else {
                m_glState.ActiveTexture(GL_TEXTURE0 + unit);
                m_glState.BindTexture(GL_TEXTURE_2D, surface->m_texture);
            }
        }
        m_textureUnits[unit].m_dirty = 0;
    }
    glActiveTexture(GL_TEXTURE0);
    InstrViewport(this, (unsigned char*)m_viewportInstruction);
    ((CRasterizerState_v1_OGLES*)m_rasterizerState)->Sync();
    if (m_shaderProgram != NULL) {
        SetShaderProgram(*m_shaderProgram);
    }
}

void CGraphics_OGLES::Sync()
{
    SyncCore();

    unsigned int multisample = m_targetSurface->m_flags & ICRenderSurface::Flag_MultisampleMask;
    if (multisample != 0x100000 && multisample != 0x200000 && multisample != 0x300000) {
        glDisable(GL_MULTISAMPLE);
    } else {
        glEnable(GL_MULTISAMPLE);
    }
    glDisable(GL_NORMALIZE);
    glDisable(GL_FOG);
    glDisable(GL_COLOR_MATERIAL);
    glDisable(GL_LIGHTING);
    glShadeModel(GL_SMOOTH);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    if (m_alphaTest) {
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(GL_GREATER, OGLES_AlphaReference);
    } else {
        glDisable(GL_ALPHA_TEST);
    }
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    m_textureMatrixType = 0;
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_POINT_SIZE_ARRAY_OES);
    for (unsigned int unit = 0; unit < (unsigned int)m_maxTextureUnits; unit++) {
        glActiveTexture(GL_TEXTURE0 + unit);
        glClientActiveTexture(GL_TEXTURE0 + unit);
        glDisable(GL_TEXTURE_2D);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    }

    // set the client arrays to the cached state
    for (unsigned int array = 0; array < 4; array++) {
        if (m_arrayEnabled[array]) {
            if (array == ClientArray_Color) {
                glEnableClientState(GL_COLOR_ARRAY);
            } else if (array >= ClientArray_TexCoord0 && array < ClientArray_TexCoord0 + 4) {
                glActiveTexture(GL_TEXTURE0 + array - ClientArray_TexCoord0);
                glClientActiveTexture(GL_TEXTURE0 + array - ClientArray_TexCoord0);
                glEnable(GL_TEXTURE_2D);
                glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            } else if (array == ClientArray_Vertex) {
                glEnableClientState(GL_VERTEX_ARRAY);
            }
        } else {
            if (array == ClientArray_Color) {
                glDisableClientState(GL_COLOR_ARRAY);
            } else if (array >= ClientArray_TexCoord0 && array < ClientArray_TexCoord0 + 4) {
                glActiveTexture(GL_TEXTURE0 + array - ClientArray_TexCoord0);
                glClientActiveTexture(GL_TEXTURE0 + array - ClientArray_TexCoord0);
                glDisable(GL_TEXTURE_2D);
                glDisableClientState(GL_TEXTURE_COORD_ARRAY);
            } else if (array == ClientArray_Vertex) {
                glDisableClientState(GL_VERTEX_ARRAY);
            }
        }
    }
    if (m_fixedFunction) {
        glActiveTexture(GL_TEXTURE0);
        if (m_fixedFunction) {
            glClientActiveTexture(GL_TEXTURE0);
        }
    }
}

unsigned char CGraphics_OGLES::InitializeIntrinsicHardwareShaderManagement(unsigned char reload)
{
    if (m_intrinsicShadersInitialized) {
        return 1;
    }
    CBlitUtil_AddBlitOps_OGLES();

    ICShaderProgram::ParameterTable& parameters = m_shaderParameters;
    parameters.Initialize(4);
    parameters.Add(L"mvp", components::CTypedVariableTable::Entry::Matrix4d, 1, 8);
    parameters.Add(L"constColor", components::CTypedVariableTable::Entry::Vector4d, 1, 4);
    parameters.Add(L"texEnv0", components::CTypedVariableTable::Entry::Integer, 1, 4);
    parameters.Add(L"texEnv1", components::CTypedVariableTable::Entry::Integer, 1, 4);
    math::CMatrix4dh* mvp = (math::CMatrix4dh*)parameters.GetEntryData(parameters.GetEntryIndex(L"mvp"));
    mvp->m_orthonormalizeInterval = 0;
    mvp->m_autoOrthonormalize = 0;
    mvp->LoadIdentity();

    systems::ICResourceManager* resources = systems::ICResourceManager::GetInstance();
    unsigned int majorVersion;
    unsigned int minorVersion;
    resources->GetVersion(majorVersion, minorVersion);
    m_resourceRouter->m_nameToInputStreamContext = this;
    m_resourceRouter->m_nameToInputStream = CGraphics_OGLES_Handler_NameToInputStream;

    for (int index = 0; hcommon_sdrpgm[index] != 0; index++) {
        // drop an older copy of the program
        for (int existing = m_intrinsicShaderPrograms.Size(); existing >= 1; existing--) {
            IntrinsicShaderProgramInfo& info = m_intrinsicShaderPrograms.m_data[existing - 1];
            if (info.m_id == hcommon_sdrpgm[index]) {
                if (!info.m_fromResources && info.m_program != NULL) {
                    delete info.m_program;
                }
                m_intrinsicShaderPrograms.RemoveRange(existing - 1, existing);
                break;
            }
        }

        ICShaderProgram* program = NULL;
        unsigned char fromResources = 0;
        unsigned char create = 1;
        unsigned char builtIn = 0;
        if (majorVersion == 0) {
            builtIn = (index == 0);
        } else {
            const char* name = GetIntrinsicShaderProgramName(index);
            resources->ReleaseResource(name);
            builtIn = (index == 0);
            if (!reload) {
                systems::CResource* resource;
                if (resources->CreateResource(name, &resource) != 0) {
                    fromResources = 1;
                    program = (ICShaderProgram*)resource->GetData();
                    if (program != NULL) {
                        create = 0;
                    }
                }
            }
        }
        if (create) {
            program = ICShaderProgram::CreateInstance();
            components::CArrayInputStream stream;
            if (builtIn) {
                stream.Open(common_sdrpgm_mvp_conditional_constcolor_mod_tex0_rawData, 0x37);
            }
            program->Load(stream, OGLES_ShaderProgramType, m_resourceRouter);
        }
        program->SetParameterTable(m_shaderParameters);
        IntrinsicShaderProgramInfo info;
        info.m_id = hcommon_sdrpgm[index];
        info.m_program = program;
        info.m_fromResources = fromResources;
        m_intrinsicShaderPrograms.Add(info);
    }
    m_intrinsicShadersInitialized = 1;

    systems::CEvent* event =
        new systems::CEvent(this, OGLES_Event_IntrinsicShadersLoaded, 0, 0, components::CExecutable::OneShot);
    event->Run();

    if (m_fixedFunction) {
        glMatrixMode(GL_MODELVIEW);
    }
    if (m_alphaTest) {
        if (m_fixedFunction) {
            glEnable(GL_ALPHA_TEST);
        }
        if (m_fixedFunction) {
            glAlphaFunc(GL_GREATER, OGLES_AlphaReference);
        }
    } else {
        if (m_fixedFunction) {
            glDisable(GL_ALPHA_TEST);
        }
    }
    return 1;
}

void CGraphics_OGLES::DestroyIntrinsicSoftwareShaderManagement()
{
    systems::ICResourceManager* resources = systems::ICResourceManager::GetInstance();
    for (unsigned int index = 0; index < sizeof(hcommon_sdrpgm) / sizeof(hcommon_sdrpgm[0]); index++) {
        if (hcommon_sdrpgm[index] != 0) {
            for (int existing = m_intrinsicShaderPrograms.Size(); existing >= 1; existing--) {
                IntrinsicShaderProgramInfo& info = m_intrinsicShaderPrograms.m_data[existing - 1];
                if (hcommon_sdrpgm[index] == info.m_id) {
                    if (info.m_fromResources) {
                        resources->ReleaseResource(OGLES_IntrinsicShaderProgramName);
                    } else if (info.m_program != NULL) {
                        delete info.m_program;
                    }
                    m_intrinsicShaderPrograms.RemoveRange(existing - 1, existing);
                    break;
                }
            }
        }
    }
    m_intrinsicShadersInitialized = 0;
}

void CGraphics_OGLES::SetCallBackInstructionExecutionShaders()
{
    CDisplayProgram* program = static_cast<CDisplayProgram*>(m_displayProgram);
    program->SetCallBackInstructionExecution(
        program->m_opcodeLevels[OGLES_OPCODE_INDEX(CDisplayProgram::Instruction::Opcode::Shaders)], this, InstrShaders);
}

} } } }
