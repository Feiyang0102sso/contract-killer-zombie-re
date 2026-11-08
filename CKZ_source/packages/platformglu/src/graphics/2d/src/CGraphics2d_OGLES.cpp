// CGraphics2d_OGLES.cpp - OpenGL ES 2d renderer: surfaces (textures, possibly split into tiles),
// lines, rectangles and bitmap font text, drawn through the device either immediately or as batch
// instructions of the display program being recorded. (b035)
#include <string.h>
#include "CGraphics2d_OGLES.h"
#include "CGraphics_OGLES.h"
#include "CRasterizerState.h"
#include "ICVertexBuffer.h"
#include "ICIndexBuffer.h"
#include "ICRenderSurface.h"
#include "ICRenderText2d.h"
#include "CBitmapFont.h"
#include "CRenderText2d.h"
#include "CRenderSurface_OGLES_Texture.h"
#include "CBlitUtil.h"
#include "CRectangle.h"
#include "CLineSegment2d.h"
#include "CMatrix4dh.h"
#include "CEvent.h"
#include "CEventListener.h"
#include "CHandle.h"
#include "ICCore.h"

namespace com { namespace glu { namespace platform { namespace graphics {

// events the renderer listens to
static const unsigned int Graphics2d_Event_DeviceOrientationChanged = 0x747ab255;
static const unsigned int Graphics2d_Event_TargetChanged = 0x34b4e700;
static const unsigned int Graphics2d_Event_SurfaceChanged = 0x97b4dec3;
static const unsigned int Graphics2d_Event_IntrinsicShadersLoaded = 0x1386c162;

// intrinsic program the fixed function draws use (IntrinsicShaders id)
static const unsigned int Graphics2d_FixedFunctionProgramId = 0x3b01104c;

// "texEnv0" values of the fixed function program
static const int Graphics2d_TexEnv_Color = 0;
static const int Graphics2d_TexEnv_ModulateConstColor = 3;
static const int Graphics2d_TexEnv_Replace = 4;

// far plane of the orthographic projection
static const float Graphics2d_OrthoFar = 32767.0f;

// depth of the 3d vertices: a counter, offset into the negative half of the short range
static const short Graphics2d_DepthMax = 0x7fff;

/** Surface buffer format bit (RSBFormat): the format has an alpha channel. */
static const unsigned int Graphics2d_FormatFlag_Alpha = 0x4000;

// vertex stream the last recorded 3d batch wrote to, and the depth counter of immediate draws
static ICBuffer::Stream* tptr;
static short tdepth;

/** Exponent of the smallest power of two >= size (sizes up to 1024, 11 above). */
static inline int Graphics2d_PowerOfTwoExponent(int size)
{
    int exponent;
    if (size <= 1) {
        exponent = 0;
    } else if (size <= 2) {
        exponent = 1;
    } else if (size <= 4) {
        exponent = 2;
    } else if (size <= 8) {
        exponent = 3;
    } else if (size <= 16) {
        exponent = 4;
    } else if (size <= 32) {
        exponent = 5;
    } else if (size <= 64) {
        exponent = 6;
    } else if (size <= 128) {
        exponent = 7;
    } else if (size <= 256) {
        exponent = 8;
    } else if (size <= 512) {
        exponent = 9;
    } else if (size <= 1024) {
        exponent = 10;
    } else {
        exponent = 11;
    }
    return exponent;
}

unsigned char Graphics2d_OGLES_HandleEvent(systems::CEvent& event, void* listener)
{
    CGraphics2d_OGLES* graphics2d = (CGraphics2d_OGLES*)((systems::CEventListener*)listener)->m_object;
    switch (event.m_eventId) {
    case Graphics2d_Event_IntrinsicShadersLoaded:
        graphics2d->BindIntrinsicShaderPrograms();
        return 0;
    case Graphics2d_Event_SurfaceChanged:
    case Graphics2d_Event_TargetChanged: {
        int index = graphics2d->m_graphics->m_shaderParameters.GetEntryIndex(L"mvp");
        if (index != 0x7fffffff) {
            graphics2d->SetWidthAndHeightMappedOrthoProjection(
                *(math::CMatrix4d*)graphics2d->m_graphics->m_shaderParameters.GetEntryData(index));
        }
        return 1;
    }
    }
    return 0;
}

void CGraphics2d_OGLES::BindIntrinsicShaderPrograms()
{
    CGraphics_OGLES* graphics = m_graphics;
    for (int index = 0; index < graphics->m_intrinsicShaderPrograms.m_size; index++) {
        if (graphics->m_intrinsicShaderPrograms.m_data[index].m_id == Graphics2d_FixedFunctionProgramId) {
            m_fixedFunctionProgram = graphics->m_intrinsicShaderPrograms.m_data[index].m_program;
        }
    }
}

void CGraphics2d_OGLES::SetWidthAndHeightMappedOrthoProjection(math::CMatrix4d& matrix)
{
    ICGraphics* graphics = ICGraphics::GetInstance();
    CGraphics* device = (CGraphics*)graphics;
    if (device->m_targetSurface == NULL) {
        return;
    }
    unsigned int width;
    unsigned int height;
    device->m_targetSurface->GetWidthAndHeight(width, height);
    if ((device->m_targetSurface->m_flags & ICRenderSurface::Flag_Targetable) != 0 &&
        CApplet::m_pCore->m_deviceOrientationSet) {
        switch (CApplet::m_pCore->m_deviceOrientation) {
        case 2:
            CGraphics2d_OGLES_Ortho(matrix, 0.0f, -(float)height, 0.0f, (float)width, 0.0f, Graphics2d_OrthoFar);
            ((math::CMatrix4dh&)matrix).RotateZ(90.0f);
            return;
        case 3:
            CGraphics2d_OGLES_Ortho(matrix, (float)width, 0.0f, 0.0f, (float)height, 0.0f, Graphics2d_OrthoFar);
            return;
        case 4:
            CGraphics2d_OGLES_Ortho(matrix, (float)height, 0.0f, -(float)width, 0.0f, 0.0f, Graphics2d_OrthoFar);
            ((math::CMatrix4dh&)matrix).RotateZ(-90.0f);
            return;
        }
    }
    CGraphics2d_OGLES_Ortho(matrix, 0.0f, (float)width, (float)height, 0.0f, 0.0f, Graphics2d_OrthoFar);
}

CGraphics2d_OGLES::CGraphics2d_OGLES()
{
    m_drawInstruction = NULL;
    m_fixedFunctionProgram = NULL;
    m_colorProgram = NULL;
    m_textureColorProgram = NULL;
    m_textureProgram = NULL;
    m_unusedProgram = NULL;
    m_vertexBuffer2d = NULL;
    m_vertexBuffer2dTextured = NULL;
    m_vertexBuffer3d = NULL;
    m_vertexBuffer3dTextured = NULL;
    m_indexBuffer = NULL;
    m_eventListener = NULL;
    m_graphics = NULL;
    m_rasterizerState = NULL;
}

CGraphics2d_OGLES::~CGraphics2d_OGLES()
{
}

unsigned char CGraphics2d_OGLES::Initialize()
{
    systems::CEventListener* listener = new systems::CEventListener();
    m_eventListener = listener;
    unsigned int handle = core::CHandle::FromName("Graphics2d");
    listener->m_object = this;
    listener->m_handle = handle;
    listener->m_callback = Graphics2d_OGLES_HandleEvent;
    m_eventListener->Register(Graphics2d_Event_DeviceOrientationChanged, core::CPriorityClass::DefaultPriority);
    m_eventListener->Register(Graphics2d_Event_TargetChanged, core::CPriorityClass::DefaultPriority);
    m_eventListener->Register(Graphics2d_Event_SurfaceChanged, core::CPriorityClass::DefaultPriority);
    m_eventListener->Register(Graphics2d_Event_IntrinsicShadersLoaded, core::CPriorityClass::DefaultPriority);

    ICGraphicsAbstractionManager::SetAbstractionLayer(*ICGraphicsAbstractionManager::GetInstance(),
                                                      ICGraphicsAbstract::Abstraction_Hardware);
    m_graphics = (CGraphics_OGLES*)ICGraphics::GetInstance();
    m_rasterizerState = (CRasterizerState_v1*)m_graphics->GetRasterizerState();
    BindIntrinsicShaderPrograms();

    int index = m_graphics->m_shaderParameters.GetEntryIndex(L"mvp");
    if (index != 0x7fffffff) {
        SetWidthAndHeightMappedOrthoProjection(*(math::CMatrix4d*)m_graphics->m_shaderParameters.GetEntryData(index));
    }

    m_vertexBuffer2d = ICVertexBuffer::CreateInstance();
    m_vertexBuffer2d->Initialize(Vtx2d_AttrDecl, 10, 4);
    m_vertexBuffer2dTextured = ICVertexBuffer::CreateInstance();
    m_vertexBuffer2dTextured->Initialize(Vtx2d_t_AttrDecl, 10, 8);
    m_vertexBuffer3d = ICVertexBuffer::CreateInstance();
    m_vertexBuffer3d->Initialize(Vtx3d_AttrDecl, 10, 8);
    m_vertexBuffer3dTextured = ICVertexBuffer::CreateInstance();
    m_vertexBuffer3dTextured->Initialize(Vtx3d_t_AttrDecl, 10, 12);
    m_indexBuffer = ICIndexBuffer::CreateInstance();
    m_indexBuffer->Initialize(ComponentType_UnsignedShort, 30, 0);

    m_drawInstruction = (CDisplayProgram::Instruction::Draw*)new unsigned char[DrawInstructionBufferSize];
    m_transformFlags.Initialize(0x20);
    m_matrices.Initialize(0x20);
    m_matrices.Top().identity();
    m_transformEnabled = 1;
    LoadIdentity();
    return 1;
}

void CGraphics2d_OGLES::Destroy()
{
    m_fixedFunctionProgram = NULL;
    m_colorProgram = NULL;
    m_textureColorProgram = NULL;
    m_textureProgram = NULL;
    m_unusedProgram = NULL;
    if (m_drawInstruction != NULL) {
        delete[] (unsigned char*)m_drawInstruction;
        m_drawInstruction = NULL;
    }
    if (m_vertexBuffer2d != NULL) {
        delete m_vertexBuffer2d;
        m_vertexBuffer2d = NULL;
    }
    if (m_vertexBuffer2dTextured != NULL) {
        delete m_vertexBuffer2dTextured;
        m_vertexBuffer2dTextured = NULL;
    }
    if (m_vertexBuffer3d != NULL) {
        delete m_vertexBuffer3d;
        m_vertexBuffer3d = NULL;
    }
    if (m_vertexBuffer3dTextured != NULL) {
        delete m_vertexBuffer3dTextured;
        m_vertexBuffer3dTextured = NULL;
    }
    if (m_indexBuffer != NULL) {
        delete m_indexBuffer;
        m_indexBuffer = NULL;
    }
    m_matrices.Destroy();
    m_transformFlags.Destroy();
    if (m_eventListener != NULL) {
        m_eventListener->UnregisterAll();
        delete m_eventListener;
        m_eventListener = NULL;
    }
}

void CGraphics2d_OGLES::LoadIdentity()
{
    m_transformFlags.Top() = 0;
    if (m_transformEnabled) {
        m_matrices.Top().identity();
    }
}

void CGraphics2d_OGLES::LoadTransform(const math::CMatrix2d* matrix, const math::CVector2d* translation)
{
    float m00 = matrix->m[0];
    float m01 = matrix->m[1];
    float m10 = matrix->m[2];
    float m11 = matrix->m[3];
    m_transformFlags.Top() |= TransformFlag_Translate;
    m_transformFlags.Top() |= TransformFlag_Rotate;
    m_transformFlags.Top() |= TransformFlag_Scale;
    if (m_transformEnabled) {
        int fixedTx = (int)(translation->m_x * 65536.0f);
        int fixedTy = (int)(translation->m_y * 65536.0f);
        Matrix& top = m_matrices.Top();
        top.m[0][0] = (int)(m00 * 65536.0f);
        top.m[0][1] = (int)(m10 * 65536.0f);
        top.m[1][0] = (int)(m01 * 65536.0f);
        top.m[1][1] = (int)(m11 * 65536.0f);
        top.m_tx = fixedTx;
        top.m_ty = fixedTy;
    }
}

void CGraphics2d_OGLES::MultiplyTransform(const math::CMatrix2d* matrix, const math::CVector2d* translation)
{
    float m00 = matrix->m[0];
    float m01 = matrix->m[1];
    float m10 = matrix->m[2];
    float m11 = matrix->m[3];
    m_transformFlags.Top() |= TransformFlag_Translate;
    m_transformFlags.Top() |= TransformFlag_Rotate;
    m_transformFlags.Top() |= TransformFlag_Scale;
    if (m_transformEnabled) {
        m_matrices.Top().multiply((int)(m00 * 65536.0f), (int)(m10 * 65536.0f), (int)(m01 * 65536.0f),
                                  (int)(m11 * 65536.0f), (int)(translation->m_x * 65536.0f),
                                  (int)(translation->m_y * 65536.0f));
    }
}

void CGraphics2d_OGLES::Translate(float x, float y)
{
    m_transformFlags.Top() |= TransformFlag_Translate;
    if (m_transformEnabled) {
        int fixedX = (int)(x * 65536.0f);
        int fixedY = (int)(y * 65536.0f);
        Matrix& top = m_matrices.Top();
        int translateX = arm::smult16(fixedX, top.m[0][0]) + arm::smult16(fixedY, top.m[0][1]);
        int translateY = arm::smult16(fixedX, top.m[1][0]) + arm::smult16(fixedY, top.m[1][1]);
        top.m_tx += translateX;
        top.m_ty += translateY;
    }
}

void CGraphics2d_OGLES::Rotate(float angle)
{
    m_transformFlags.Top() |= TransformFlag_Rotate;
    if (m_transformEnabled) {
        m_matrices.Top().rotate((int)(angle * 65536.0f));
    }
}

void CGraphics2d_OGLES::Scale(float x, float y)
{
    m_transformFlags.Top() |= TransformFlag_Scale;
    if (m_transformEnabled) {
        int fixedX = (int)(x * 65536.0f);
        int fixedY = (int)(y * 65536.0f);
        Matrix& top = m_matrices.Top();
        for (int row = 0; row < 2; row++) {
            top.m[row][0] = arm::smult16(top.m[row][0], fixedX);
            top.m[row][1] = arm::smult16(top.m[row][1], fixedY);
        }
    }
}

void CGraphics2d_OGLES::PushTransform()
{
    m_transformFlags.Push();
    if (m_transformEnabled) {
        m_matrices.Push();
    }
}

void CGraphics2d_OGLES::PopTransform()
{
    m_transformFlags.Pop();
    if (m_transformEnabled) {
        m_matrices.Pop();
    }
}

void CGraphics2d_OGLES::GetTransform(const math::CMatrix2d** matrix, const math::CVector2d** translation)
{
    Matrix* matrices = m_matrices.m_data;
    unsigned int top = m_matrices.m_count - 1;
    if (matrix != NULL) {
        m_matrix.m[0] = (float)matrices[top].m[0][0] * (1.0f / 65536.0f);
        m_matrix.m[2] = (float)matrices[top].m[0][1] * (1.0f / 65536.0f);
        m_matrix.m[1] = (float)matrices[top].m[1][0] * (1.0f / 65536.0f);
        m_matrix.m[3] = (float)matrices[top].m[1][1] * (1.0f / 65536.0f);
        *matrix = &m_matrix;
    }
    if (translation != NULL) {
        m_translation.m_x = (float)matrices[top].m_tx * (1.0f / 65536.0f);
        m_translation.m_y = (float)matrices[top].m_ty * (1.0f / 65536.0f);
        *translation = &m_translation;
    }
}

void CGraphics2d_OGLES::SetVertexPositionCoords(int count, unsigned int type, int stride, const void* coords)
{
}

void CGraphics2d_OGLES::Draw(ICRenderSurface& surface, const components::CRectangle<short>* source,
                             const components::Color_RGBA* color, Flip flip)
{
    CRenderSurface_OGLES_Texture& surfaceView = (CRenderSurface_OGLES_Texture&)surface;
    components::Color_RGBA clampedColor;
    const components::Color_RGBA* pColor = NULL;
    if (color != NULL) {
        clampedColor = *color;
        if (clampedColor.r < 0.0f) {
            clampedColor.r = 0.0f;
        } else if (clampedColor.r > 1.0f) {
            clampedColor.r = 1.0f;
        }
        if (clampedColor.g < 0.0f) {
            clampedColor.g = 0.0f;
        } else if (clampedColor.g > 1.0f) {
            clampedColor.g = 1.0f;
        }
        if (clampedColor.b < 0.0f) {
            clampedColor.b = 0.0f;
        } else if (clampedColor.b > 1.0f) {
            clampedColor.b = 1.0f;
        }
        if (clampedColor.a < 0.0f) {
            clampedColor.a = 0.0f;
        } else if (clampedColor.a > 1.0f) {
            clampedColor.a = 1.0f;
        }
        pColor = &clampedColor;
    }

    short surfaceWidth = surfaceView.m_buffers[CRenderSurface::Buffer_Color]->m_imageWidth;
    short surfaceHeight = surfaceView.m_buffers[CRenderSurface::Buffer_Color]->m_imageHeight;

    unsigned char flipHorizontal = 0;
    unsigned char flipVertical = 0;
    if (flip == Flip_Horizontal) {
        flipHorizontal = 1;
    } else if (flip == Flip_Both) {
        flipHorizontal = 1;
        flipVertical = 1;
    } else if (flip == Flip_Vertical) {
        flipVertical = 1;
    }

    // the part of the surface to draw
    short sourceX;
    short sourceY;
    short sourceWidth;
    short sourceHeight;
    if (source == NULL) {
        sourceHeight = surfaceHeight;
        sourceWidth = surfaceWidth;
    } else {
        sourceX = source->m_x;
        sourceY = source->m_y;
        sourceWidth = source->m_width;
        sourceHeight = source->m_height;
        if (surfaceView.m_textureCount > 1 && (flipHorizontal || flipVertical)) {
            int clipX = sourceX;
            int clipY = sourceY;
            CBlitUtil::ClipSubRectangle(clipX, clipY, 0, 0, surfaceWidth, surfaceHeight, sourceX, sourceY,
                                        sourceWidth, sourceHeight, 0, 0);
        }
    }

    // the surface is split into tiles of at most tileWidth x tileHeight (one texture each)
    unsigned int tileWidth = m_graphics->m_description.m_maxRenderTargetWidth;
    unsigned int tileHeight = m_graphics->m_description.m_maxRenderTargetHeight;
    short* vertices = NULL;
    unsigned short* indices = NULL;
    if (surfaceView.m_textureCount <= 0) {
        return;
    }

    short offsetY = 0;          // y of the current draw relative to rowY
    short offsetX = 0;          // x of the current draw relative to originX
    if (flipVertical) {
        offsetY = sourceHeight;
    }
    if (flipHorizontal) {
        offsetX = sourceWidth;
    }
    unsigned char firstDraw = 1;
    unsigned char rowStart = 0;
    unsigned char rowStartFlipped = flipVertical;
    short originX = 0;
    short rowY = 0;
    short lastHeight = 0;
    short tileX = 0;
    short tileY = 0;
    unsigned int clippedTileHeight = tileHeight;
    unsigned char blendNeeded = 0;

    for (int tile = 0; tile < surfaceView.m_textureCount; tile++) {
        short currentTileX = tileX;
        short currentTileY = tileY;
        tileX = currentTileX + (short)tileWidth;
        unsigned int clippedTileWidth = tileWidth;
        if (surfaceWidth < currentTileX + (short)tileWidth) {
            clippedTileWidth = surfaceWidth - currentTileX;
        }
        if (surfaceHeight < currentTileY + (short)clippedTileHeight) {
            clippedTileHeight = surfaceHeight - currentTileY;
        }
        short tileDrawWidth = (short)clippedTileWidth;
        short tileDrawHeight = (short)clippedTileHeight;

        // the part of the tile to draw
        short drawX = currentTileX;
        short drawY = currentTileY;
        short drawWidth;
        short drawHeight;
        unsigned char skip = 0;
        if (source == NULL) {
            if (firstDraw) {
                short startY = 0;
                if (!flipVertical) {
                    startY = sourceHeight;
                }
                rowStart = 1;
                rowY = startY - offsetY;
            }
            drawWidth = tileDrawWidth;
            drawHeight = tileDrawHeight;
        } else {
            // textures are stored bottom up
            short sourceBottom = (surfaceHeight - sourceY) - sourceHeight;
            if (sourceX < currentTileX) {
                drawWidth = sourceWidth - (currentTileX - sourceX);
                drawX = currentTileX;
                if (drawWidth <= 0) {
                    skip = 1;
                }
            } else {
                drawWidth = sourceWidth;
                drawX = sourceX;
            }
            if (!skip && currentTileX + tileDrawWidth < drawX + drawWidth) {
                drawWidth = (currentTileX + tileDrawWidth + drawWidth) - (drawX + drawWidth);
                if (drawWidth <= 0) {
                    skip = 1;
                }
            }
            if (!skip) {
                if (sourceBottom < currentTileY) {
                    drawHeight = sourceHeight - (currentTileY - sourceBottom);
                    drawY = currentTileY;
                    if (drawHeight <= 0) {
                        skip = 1;
                    }
                } else {
                    drawHeight = sourceHeight;
                    drawY = sourceBottom;
                }
                if (!skip && currentTileY + tileDrawHeight < drawY + drawHeight) {
                    drawHeight = (currentTileY + tileDrawHeight + drawHeight) - (drawY + drawHeight);
                    if (drawHeight <= 0) {
                        skip = 1;
                    }
                }
            }
            if (!skip && firstDraw) {
                short startY = sourceHeight;
                if (flipVertical) {
                    startY = 0;
                }
                rowY = startY - offsetY;
                short startX = sourceWidth;
                if (!flipHorizontal) {
                    startX = 0;
                }
                rowStart = 1;
                originX = startX - offsetX;
            }
        }

        if (!skip) {
            // ---- shader program and texture ----
            CGraphics_OGLES* graphics = m_graphics;
            ICShaderProgram* program;
            if (graphics->m_versionMajor == 1) {
                if (pColor == NULL) {
                    int texEnvIndex = graphics->m_shaderParameters.GetEntryIndex(L"texEnv0");
                    *(int*)graphics->m_shaderParameters.GetEntryData(texEnvIndex) = Graphics2d_TexEnv_Replace;
                } else {
                    int colorIndex = graphics->m_shaderParameters.GetEntryIndex(L"constColor");
                    *(components::Color_RGBA*)graphics->m_shaderParameters.GetEntryData(colorIndex) = *pColor;
                    int texEnvIndex = graphics->m_shaderParameters.GetEntryIndex(L"texEnv0");
                    *(int*)graphics->m_shaderParameters.GetEntryData(texEnvIndex) =
                        Graphics2d_TexEnv_ModulateConstColor;
                }
                program = m_fixedFunctionProgram;
            } else {
                program = m_textureProgram;
                if (pColor != NULL) {
                    int colorIndex = graphics->m_shaderParameters.GetEntryIndex(L"constColor");
                    *(components::Color_RGBA*)graphics->m_shaderParameters.GetEntryData(colorIndex) = *pColor;
                    program = m_textureColorProgram;
                }
            }
            graphics = m_graphics;
            if (graphics->m_shaderProgram != program ||
                (graphics->m_shadersLevel != NULL &&
                 ((CDisplayProgram::Instruction::Header*)*graphics->m_shadersLevel)->m_size ==
                     CDisplayProgram::Instruction::LevelNodeSize)) {
                graphics->SetShaderProgram(*program);
                graphics = m_graphics;
            }
            CGraphics::TextureUnit& unit = graphics->m_textureUnits[0];
            unsigned char textureBound = 0;
            if (unit.m_dirty == 0 && unit.m_surface != NULL) {
                CRenderSurface_OGLES_Texture* bound = (CRenderSurface_OGLES_Texture*)unit.m_surface;
                if (bound->m_textureCount == surfaceView.m_textureCount) {
                    if (bound->m_textureCount < 2) {
                        textureBound = bound->m_texture == surfaceView.m_texture;
                    } else {
                        textureBound = bound->m_textures[unit.m_level] == surfaceView.m_textures[tile];
                    }
                    if (textureBound && graphics->m_texturesLevel != NULL &&
                        ((CDisplayProgram::Instruction::Header*)*graphics->m_texturesLevel)->m_size ==
                            CDisplayProgram::Instruction::LevelNodeSize) {
                        textureBound = 0;
                    }
                }
            }
            if (!textureBound) {
                graphics->SetTexture2d(0, surface, tile);
            }

            // ---- blending, vertex layout and depth ----
            unsigned int surfaceFormat = surfaceView.m_buffers[CRenderSurface::Buffer_Color]->m_format;
            const components::Color_RGBA* pBlendColor = pColor;
            unsigned char blendPushed;
            unsigned char blendEnabled;
#define CONFIG_BLENDING_TEXTURE_NEEDS_BLEND \
    ((surfaceFormat & Graphics2d_FormatFlag_Alpha) != 0 && \
     (m_graphics->m_alphaTest == 0 || (surfaceFormat | 0x10000) != RSBFormat_R5G5B5A1))
#include "CGraphics2d_OGLES_ConfigBlending.i"
#undef CONFIG_BLENDING_TEXTURE_NEEDS_BLEND
#define CONFIG_VERTEX_FORMAT_2D VertexFormat_Vtx2d_t
#define CONFIG_VERTEX_FORMAT_3D VertexFormat_Vtx3d_t
#include "CGraphics2d_OGLES_ConfigVertexInfoAndDepthUsage.i"
#undef CONFIG_VERTEX_FORMAT_2D
#undef CONFIG_VERTEX_FORMAT_3D

            // ---- vertex storage ----
            CDisplayProgram* pDisplayProgram = (CDisplayProgram*)m_graphics->m_displayProgram;
            if (pDisplayProgram != NULL) {
                // record a batch draw of one quad
                pDisplayProgram->m_sortedByLayer = blendEnabled;
                m_drawInstruction->m_header.m_level = (unsigned short)pDisplayProgram->m_opcodeLevels[
                    CDisplayProgram::Instruction::Opcode::IdxVtxStreamTrisBatch & 0xff];
                CDisplayProgram::Instruction::Draw* pDraw = m_drawInstruction;
                pDraw->m_header.m_opcode = CDisplayProgram::Instruction::Opcode::IdxVtxStreamTrisBatch;
                pDraw->m_vertexStream = NULL;
                pDraw->m_indexCount = 6;
                pDraw->m_vertexCount = 4;
                pDraw->m_indexStream = NULL;
                unsigned char* pInstruction = (unsigned char*)m_drawInstruction;
#include "../../src/CDisplayProgram_Add.i"
                // the quad is the last one of the (possibly merged) batch
                ICBuffer::Stream* pVertexStream = pDraw->m_vertexStream;
                unsigned int vertexCount = pDraw->m_vertexCount;
                unsigned int indexCount = pDraw->m_indexCount;
                ICBuffer::Stream* pIndexStream = pDraw->m_indexStream;
                int lockedSize;
                pVertexStream->m_buffer->Lock((unsigned char**)&vertices, lockedSize,
                                              (vertexCount + pVertexStream->m_first - 4) * m_vertexSize,
                                              m_vertexSize * 4, ICBuffer::LockType_ReadWrite);
                pIndexStream->m_buffer->Lock((unsigned char**)&indices, lockedSize,
                                             (indexCount + pIndexStream->m_first - 6) * 2, 0xc,
                                             ICBuffer::LockType_ReadWrite);
                unsigned short firstIndex = (unsigned short)(vertexCount - 4);
                indices[0] = firstIndex;
                indices[1] = firstIndex + 1;
                indices[2] = firstIndex + 2;
                indices[3] = firstIndex + 1;
                indices[4] = firstIndex + 2;
                indices[5] = firstIndex + 3;
                if (m_positionComponents == 3) {
                    short depth;
                    short* pProgramDepth = &pDisplayProgram->m_batchDepth;
                    if (tptr == pVertexStream) {
                        depth = *pProgramDepth;
                    } else {
                        depth = *pProgramDepth + 1;
                        tptr = pVertexStream;
                        *pProgramDepth = depth;
                    }
                    depth = depth - Graphics2d_DepthMax;
                    vertices[2] = depth;
                    vertices[m_vertexShorts + 2] = depth;
                    vertices[m_vertexShorts * 2 + 2] = depth;
                    vertices[m_vertexShorts * 3 + 2] = depth;
                }
            } else {
                int lockedSize;
                if (m_positionComponents == 3) {
                    m_vertexBuffer3dTextured->Lock((unsigned char**)&vertices, lockedSize, 0, 0x30,
                                                   ICBuffer::LockType_ReadWrite);
                    short depth = tdepth + 1;
                    if (depth == Graphics2d_DepthMax) {
                        depth = 0;
                    }
                    tdepth = depth;
                    depth = depth - Graphics2d_DepthMax;
                    vertices[2] = depth;
                    vertices[m_vertexShorts + 2] = depth;
                    vertices[m_vertexShorts * 2 + 2] = depth;
                    vertices[m_vertexShorts * 3 + 2] = depth;
                } else {
                    m_vertexBuffer2dTextured->Lock((unsigned char**)&vertices, lockedSize, 0, 0x20,
                                                   ICBuffer::LockType_ReadWrite);
                }
            }

            // ---- positions ----
            int texCoordOffset = m_texCoordOffset;
            if (rowStart) {
                if (flipVertical) {
                    rowY = rowY + drawHeight;
                } else {
                    rowY = rowY - drawHeight;
                }
            }
            if (flipHorizontal) {
                offsetX = offsetX - drawWidth;
            }
            if (rowStartFlipped) {
                offsetY = offsetY - drawHeight;
            }
            short x0 = originX + offsetX;
            short y0 = rowY + offsetY;
            short x1 = x0 + drawWidth;
            short y1 = y0 + drawHeight;
            short vertex0X = x0;
            short vertex0Y = y0;
            short vertex1X = x0;
            short vertex1Y = y1;
            short vertex2X = x1;
            short vertex2Y = y0;
            short vertex3X = x1;
            short vertex3Y = y1;
            unsigned int transformFlags = m_transformFlags.Top();
            Matrix& transform = m_matrices.m_data[m_matrices.m_count - 1];
            if ((transformFlags & (TransformFlag_Scale | TransformFlag_Rotate)) == 0) {
                if (transformFlags == TransformFlag_Translate) {
                    short translateX = (short)(transform.m_tx >> 16);
                    short translateY = (short)(transform.m_ty >> 16);
                    vertex0X += translateX;
                    vertex0Y += translateY;
                    vertex1X += translateX;
                    vertex1Y += translateY;
                    vertex2X += translateX;
                    vertex2Y += translateY;
                    vertex3X += translateX;
                    vertex3Y += translateY;
                }
            } else {
                int fixedX0 = x0 << 16;
                int fixedY0 = y0 << 16;
                int fixedX1 = x1 << 16;
                int fixedY1 = y1 << 16;
                vertex0X = (short)((arm::smult16(fixedX0, transform.m[0][0]) + arm::smult16(fixedY0, transform.m[0][1]) +
                                    transform.m_tx) >> 16);
                vertex0Y = (short)((arm::smult16(fixedX0, transform.m[1][0]) + arm::smult16(fixedY0, transform.m[1][1]) +
                                    transform.m_ty) >> 16);
                vertex1X = (short)((arm::smult16(fixedX0, transform.m[0][0]) + arm::smult16(fixedY1, transform.m[0][1]) +
                                    transform.m_tx) >> 16);
                vertex1Y = (short)((arm::smult16(fixedX0, transform.m[1][0]) + arm::smult16(fixedY1, transform.m[1][1]) +
                                    transform.m_ty) >> 16);
                vertex2X = (short)((arm::smult16(fixedX1, transform.m[0][0]) + arm::smult16(fixedY0, transform.m[0][1]) +
                                    transform.m_tx) >> 16);
                vertex2Y = (short)((arm::smult16(fixedX1, transform.m[1][0]) + arm::smult16(fixedY0, transform.m[1][1]) +
                                    transform.m_ty) >> 16);
                vertex3X = (short)((arm::smult16(fixedX1, transform.m[0][0]) + arm::smult16(fixedY1, transform.m[0][1]) +
                                    transform.m_tx) >> 16);
                vertex3Y = (short)((arm::smult16(fixedX1, transform.m[1][0]) + arm::smult16(fixedY1, transform.m[1][1]) +
                                    transform.m_ty) >> 16);
            }
            vertices[0] = vertex0X;
            vertices[1] = vertex0Y;
            vertices[m_vertexShorts] = vertex1X;
            vertices[m_vertexShorts + 1] = vertex1Y;
            vertices[m_vertexShorts * 2] = vertex2X;
            vertices[m_vertexShorts * 2 + 1] = vertex2Y;
            vertices[m_vertexShorts * 3] = vertex3X;
            vertices[m_vertexShorts * 3 + 1] = vertex3Y;

            // ---- texture coordinates (4.12 fixed point of the tile) ----
            unsigned int scaleU;
            unsigned int scaleV;
            if ((surfaceView.m_flags & ICRenderSurface::Flag_PowerOfTwo) == 0) {
                scaleU = 0x1000000 / tileDrawWidth;
                scaleV = 0x1000000 / tileDrawHeight;
            } else {
                scaleU = 0x1000000 >> Graphics2d_PowerOfTwoExponent(tileDrawWidth);
                scaleV = 0x1000000 >> Graphics2d_PowerOfTwoExponent(tileDrawHeight);
            }
            short left;
            short right;
            if (flipHorizontal) {
                right = (short)((scaleU * (drawX - currentTileX)) >> 12) + 1;
                left = (short)((scaleU * (drawX + (drawWidth - currentTileX))) >> 12) - 1;
            } else {
                left = (short)((scaleU * (drawX - currentTileX)) >> 12) + 1;
                right = (short)((scaleU * (drawX + (drawWidth - currentTileX))) >> 12) - 1;
            }
            vertices[texCoordOffset] = left;
            vertices[m_vertexShorts + texCoordOffset] = left;
            vertices[texCoordOffset + m_vertexShorts * 2] = right;
            vertices[m_vertexShorts * 3 + texCoordOffset] = right;
            short bottom;
            short top;
            if (flipVertical) {
                bottom = (short)((scaleV * (drawY - currentTileY)) >> 12) + 1;
                top = (short)(((drawY + (drawHeight - currentTileY)) * scaleV) >> 12) - 1;
            } else {
                top = (short)((scaleV * (drawY - currentTileY)) >> 12) + 1;
                bottom = (short)((scaleV * (drawY + (drawHeight - currentTileY))) >> 12) - 1;
            }
            vertices[texCoordOffset + 1] = bottom;
            vertices[m_vertexShorts + texCoordOffset + 1] = top;
            vertices[m_vertexShorts * 2 + texCoordOffset + 1] = bottom;
            vertices[m_vertexShorts * 3 + texCoordOffset + 1] = top;

            // ---- immediate draw ----
            if (pDisplayProgram == NULL) {
                ICBuffer::Stream vertexStream;
                if (m_positionComponents == 3) {
                    vertexStream.m_buffer = m_vertexBuffer3dTextured;
                } else {
                    vertexStream.m_buffer = m_vertexBuffer2dTextured;
                }
                vertexStream.m_count = 4;
                vertexStream.m_first = 0;
                vertexStream.m_dirty = 1;
                m_graphics->SetVertexStream(0, vertexStream);
                m_graphics->EnableVertexStream(0, 1);
                int lockedSize;
                m_indexBuffer->Lock((unsigned char**)&indices, lockedSize, 0, 0xc, ICBuffer::LockType_ReadWrite);
                indices[0] = 0;
                indices[1] = 1;
                indices[2] = 2;
                indices[3] = 1;
                indices[4] = 2;
                indices[5] = 3;
                ICBuffer::Stream indexStream;
                indexStream.m_buffer = m_indexBuffer;
                indexStream.m_count = 6;
                indexStream.m_first = 0;
                indexStream.m_dirty = 1;
                // the original draws the 6 indexed triangle vertices as a strip (GL_TRIANGLE_STRIP)
                m_graphics->Draw(ICGraphics::Primitive_TriangleStrip, indexStream);
            }
            if (blendPushed) {
                m_rasterizerState->PopState(ICRasterizerState::State_Blending);
            }
            m_drawCount++;
            if (!flipHorizontal) {
                offsetX = offsetX + drawWidth;
            }
            firstDraw = 0;
            rowStartFlipped = 0;
            rowStart = 0;
            lastHeight = drawHeight;
        }

        // end of a row of tiles
        if (surfaceWidth <= currentTileX + (int)tileWidth) {
            offsetX = sourceWidth;
            if (!flipHorizontal) {
                offsetX = 0;
            }
            if (flipVertical) {
                rowStartFlipped = 1;
            } else {
                offsetY = offsetY - lastHeight;
            }
            rowStart = 1;
            tileY = currentTileY + (short)tileHeight;
            tileX = 0;
            rowY = lastHeight + rowY;
        }
    }
}

void CGraphics2d_OGLES::ConfigureVertexInfo(unsigned int vertexFormat)
{
    CDisplayProgram::Instruction::Header* pHeader = &m_drawInstruction->m_header;
    pHeader->m_child = NULL;
    pHeader->m_sibling = NULL;
    pHeader->m_size = sizeof(CDisplayProgram::Instruction::Draw);
    CDisplayProgram::Instruction::Draw* pDraw = m_drawInstruction;
    pDraw->m_vertexStream = NULL;
    pDraw->m_vertexCount = 4;
    pDraw->m_indexStream = NULL;
    pDraw->m_indexType = ComponentType_UnsignedShort;
    pDraw->m_indexCount = 6;
    switch (vertexFormat) {
    case VertexFormat_Vtx3d_t:
        pDraw->m_vertexFormat = VertexFormat_Vtx3d_t;
        m_vertexFormat = VertexFormat_Vtx3d_t;
        m_vertexDecl = Vtx3d_t_AttrDecl;
        m_positionComponents = 3;
        m_texCoordComponents = 2;
        m_vertexSize = 12;
        m_vertexShorts = 6;
        m_texCoordOffset = 4;
        break;
    case VertexFormat_Vtx2d_t:
        pDraw->m_vertexFormat = VertexFormat_Vtx2d_t;
        m_vertexFormat = VertexFormat_Vtx2d_t;
        m_vertexDecl = Vtx2d_t_AttrDecl;
        m_positionComponents = 2;
        m_texCoordComponents = 2;
        m_vertexSize = 8;
        m_vertexShorts = 4;
        m_texCoordOffset = 2;
        break;
    case VertexFormat_Vtx3d:
        pDraw->m_vertexFormat = VertexFormat_Vtx3d;
        m_vertexFormat = VertexFormat_Vtx3d;
        m_vertexDecl = Vtx3d_AttrDecl;
        m_positionComponents = 3;
        m_texCoordComponents = 0;
        m_vertexSize = 8;
        m_vertexShorts = 4;
        break;
    case VertexFormat_Vtx2d:
        pDraw->m_vertexFormat = VertexFormat_Vtx2d;
        m_vertexFormat = VertexFormat_Vtx2d;
        m_vertexDecl = Vtx2d_AttrDecl;
        m_positionComponents = 2;
        m_texCoordComponents = 0;
        m_vertexSize = 4;
        m_vertexShorts = 2;
        break;
    }
}

void CGraphics2d_OGLES::Draw(ICRenderSurface& surface, float x, float y, const components::CRectangle<short>* source,
                             const components::Color_RGBA* color, Flip flip)
{
    PushTransform();
    Translate(x, y);
    Draw(surface, source, color, flip);
    PopTransform();
}

void CGraphics2d_OGLES::Draw(ICRenderSurface& surface, unsigned int width, unsigned int height,
                             const components::CRectangle<short>* source, const components::Color_RGBA* color,
                             Flip flip)
{
    CRenderSurface_OGLES_Texture& surfaceView = (CRenderSurface_OGLES_Texture&)surface;
    components::CRectangle<short> clipped = components::CRectangle<short>();
    const components::CRectangle<short>* region = NULL;
    if (source != NULL) {
        short surfaceWidth = surfaceView.m_buffers[CRenderSurface::Buffer_Color]->m_imageWidth;
        short surfaceHeight = surfaceView.m_buffers[CRenderSurface::Buffer_Color]->m_imageHeight;
        int clipX = source->m_x;
        int clipY = source->m_y;
        clipped.m_x = source->m_x;
        clipped.m_y = source->m_y;
        clipped.m_width = source->m_width;
        clipped.m_height = source->m_height;
        CBlitUtil::ClipSubRectangle(clipX, clipY, 0, 0, surfaceWidth, surfaceHeight, clipped.m_x, clipped.m_y,
                                    clipped.m_width, clipped.m_height, 0, 0);
        region = &clipped;
    }
    // repeat the surface `width` x `height` times
    PushTransform();
    for (unsigned int row = 0; row < height; row++) {
        Translate(0.0f, (float)row);
        PushTransform();
        for (unsigned int column = 0; column < width; column++) {
            Translate((float)column, 0.0f);
            Draw(surface, region, color, flip);
        }
        PopTransform();
    }
    PopTransform();
}

void CGraphics2d_OGLES::Draw(ICRenderSurface& surface, float x, float y, unsigned int width, unsigned int height,
                             const components::CRectangle<short>* source, const components::Color_RGBA* color,
                             Flip flip)
{
    PushTransform();
    Translate(x, y);
    Draw(surface, width, height, source, color, flip);
    PopTransform();
}

void CGraphics2d_OGLES::Draw(const components::CRectangle<short>& rectangle, const components::Color_RGBA& color)
{
    short coords[4];
    coords[0] = rectangle.m_x;
    coords[1] = rectangle.m_y;
    coords[2] = rectangle.m_x + rectangle.m_width;
    coords[3] = rectangle.m_y + rectangle.m_height;
    m_positionCoords = coords;
    m_positionCount = 2;
    m_positionType = ComponentType_Short;
    m_positionStride = 4;
    Draw(Primitive_Rectangles, 0, 1, color);
}

void CGraphics2d_OGLES::Draw(const components::CRectangle<short>& rectangle, const components::Color_RGBA& color,
                             float x, float y)
{
    PushTransform();
    Translate(x, y);
    Draw(rectangle, color);
    PopTransform();
}

void CGraphics2d_OGLES::Draw(const components::CRectangle<int>& rectangle, const components::Color_RGBA& color)
{
    short coords[4];
    coords[0] = rectangle.m_x;
    coords[1] = rectangle.m_y;
    coords[2] = rectangle.m_x + rectangle.m_width;
    coords[3] = rectangle.m_y + rectangle.m_height;
    m_positionCoords = coords;
    m_positionCount = 2;
    m_positionType = ComponentType_Short;
    m_positionStride = 4;
    Draw(Primitive_Rectangles, 0, 1, color);
}

void CGraphics2d_OGLES::Draw(const components::CRectangle<int>& rectangle, const components::Color_RGBA& color,
                             float x, float y)
{
    PushTransform();
    Translate(x, y);
    Draw(rectangle, color);
    PopTransform();
}

void CGraphics2d_OGLES::Draw(const components::CRectangle<float>& rectangle, const components::Color_RGBA& color)
{
    short coords[4];
    coords[0] = (short)rectangle.m_x;
    coords[1] = (short)rectangle.m_y;
    coords[2] = (short)(rectangle.m_x + rectangle.m_width);
    coords[3] = (short)(rectangle.m_y + rectangle.m_height);
    m_positionCoords = coords;
    m_positionCount = 2;
    m_positionType = ComponentType_Short;
    m_positionStride = 4;
    Draw(Primitive_Rectangles, 0, 1, color);
}

void CGraphics2d_OGLES::Draw(const components::CRectangle<float>& rectangle, const components::Color_RGBA& color,
                             float x, float y)
{
    PushTransform();
    Translate(x, y);
    Draw(rectangle, color);
    PopTransform();
}

void CGraphics2d_OGLES::Draw(const math::CLineSegment2d& line, const components::Color_RGBA& color)
{
    short coords[4];
    coords[0] = (short)line.m_start.m_x;
    coords[1] = (short)line.m_start.m_y;
    coords[2] = (short)line.m_end.m_x;
    coords[3] = (short)line.m_end.m_y;
    m_positionCoords = coords;
    m_positionCount = 2;
    m_positionType = ComponentType_Short;
    m_positionStride = 4;
    Draw(Primitive_Lines, 0, 1, color);
}

void CGraphics2d_OGLES::Draw(const math::CLineSegment2d& line, const components::Color_RGBA& color, float x, float y)
{
    PushTransform();
    Translate(x, y);
    Draw(line, color);
    PopTransform();
}

void CGraphics2d_OGLES::Draw(Primitive primitive, int first, int count, const components::Color_RGBA& color)
{
    components::Color_RGBA clampedColor = color;
    if (clampedColor.r < 0.0f) {
        clampedColor.r = 0.0f;
    } else if (clampedColor.r > 1.0f) {
        clampedColor.r = 1.0f;
    }
    if (clampedColor.g < 0.0f) {
        clampedColor.g = 0.0f;
    } else if (clampedColor.g > 1.0f) {
        clampedColor.g = 1.0f;
    }
    if (clampedColor.b < 0.0f) {
        clampedColor.b = 0.0f;
    } else if (clampedColor.b > 1.0f) {
        clampedColor.b = 1.0f;
    }
    if (clampedColor.a < 0.0f) {
        clampedColor.a = 0.0f;
    } else if (clampedColor.a > 1.0f) {
        clampedColor.a = 1.0f;
    }
    switch (primitive) {
    case Primitive_Lines:
        DrawLines(first, count, clampedColor);
        break;
    case Primitive_Rectangles:
        DrawRectangles(first, count, clampedColor);
        break;
    }
}

void CGraphics2d_OGLES::DrawLines(int first, int count, const components::Color_RGBA& color)
{
    short* vertices = NULL;
    unsigned short* indices = NULL;

    // ---- shader program ----
    CGraphics_OGLES* graphics = m_graphics;
    unsigned int versionMajor = graphics->m_versionMajor;
    int colorIndex = graphics->m_shaderParameters.GetEntryIndex(L"constColor");
    ICShaderProgram* program;
    if (versionMajor == 1) {
        program = m_fixedFunctionProgram;
        *(components::Color_RGBA*)graphics->m_shaderParameters.GetEntryData(colorIndex) = color;
        int texEnvIndex = graphics->m_shaderParameters.GetEntryIndex(L"texEnv0");
        *(int*)graphics->m_shaderParameters.GetEntryData(texEnvIndex) = Graphics2d_TexEnv_Color;
    } else {
        program = m_colorProgram;
        *(components::Color_RGBA*)graphics->m_shaderParameters.GetEntryData(colorIndex) = color;
    }
    graphics = m_graphics;
    if (graphics->m_shaderProgram != program ||
        (graphics->m_shadersLevel != NULL &&
         ((CDisplayProgram::Instruction::Header*)*graphics->m_shadersLevel)->m_size ==
             CDisplayProgram::Instruction::LevelNodeSize)) {
        graphics->SetShaderProgram(*program);
    }

    // ---- blending, vertex layout and depth ----
    const components::Color_RGBA* pBlendColor = &color;
    unsigned char blendPushed;
    unsigned char blendEnabled;
    unsigned char blendNeeded;
#define CONFIG_BLENDING_TEXTURE_NEEDS_BLEND 1
#include "CGraphics2d_OGLES_ConfigBlending.i"
#undef CONFIG_BLENDING_TEXTURE_NEEDS_BLEND
#define CONFIG_VERTEX_FORMAT_2D VertexFormat_Vtx2d
#define CONFIG_VERTEX_FORMAT_3D VertexFormat_Vtx3d
#include "CGraphics2d_OGLES_ConfigVertexInfoAndDepthUsage.i"
#undef CONFIG_VERTEX_FORMAT_2D
#undef CONFIG_VERTEX_FORMAT_3D

    if (m_positionType == ComponentType_Short && count > 0) {
        CDisplayProgram* pDisplayProgram = (CDisplayProgram*)m_graphics->m_displayProgram;
        for (int line = first; line < first + count; line += 2) {
            if (pDisplayProgram != NULL) {
                // record a batch draw of one line
                m_drawInstruction->m_header.m_level = (unsigned short)pDisplayProgram->m_opcodeLevels[
                    CDisplayProgram::Instruction::Opcode::IdxVtxStreamLinesBatch & 0xff];
                CDisplayProgram::Instruction::Draw* pDraw = m_drawInstruction;
                pDraw->m_header.m_opcode = CDisplayProgram::Instruction::Opcode::IdxVtxStreamLinesBatch;
                pDraw->m_vertexStream = NULL;
                pDraw->m_indexCount = 2;
                pDraw->m_vertexCount = 2;
                pDraw->m_indexStream = NULL;
                unsigned char* pInstruction = (unsigned char*)m_drawInstruction;
#include "../../src/CDisplayProgram_Add.i"
                // the line is the last one of the (possibly merged) batch
                ICBuffer::Stream* pVertexStream = pDraw->m_vertexStream;
                unsigned int vertexCount = pDraw->m_vertexCount;
                unsigned int indexCount = pDraw->m_indexCount;
                ICBuffer::Stream* pIndexStream = pDraw->m_indexStream;
                int lockedSize;
                pVertexStream->m_buffer->Lock((unsigned char**)&vertices, lockedSize,
                                              (vertexCount + pVertexStream->m_first - 2) * m_vertexSize,
                                              m_vertexSize * 2, ICBuffer::LockType_ReadWrite);
                pIndexStream->m_buffer->Lock((unsigned char**)&indices, lockedSize,
                                             (indexCount + pIndexStream->m_first - 2) * 2, 4,
                                             ICBuffer::LockType_ReadWrite);
                unsigned short firstIndex = (unsigned short)(vertexCount - 2);
                indices[0] = firstIndex;
                indices[1] = firstIndex + 1;
                if (m_positionComponents == 3) {
                    short depth;
                    short* pProgramDepth = &pDisplayProgram->m_batchDepth;
                    if (tptr == pVertexStream) {
                        depth = *pProgramDepth;
                    } else {
                        depth = *pProgramDepth + 1;
                        tptr = pVertexStream;
                        *pProgramDepth = depth;
                    }
                    depth = depth - Graphics2d_DepthMax;
                    vertices[2] = depth;
                    vertices[m_vertexShorts + 2] = depth;
                }
            } else {
                int lockedSize;
                if (m_positionComponents == 3) {
                    m_vertexBuffer3d->Lock((unsigned char**)&vertices, lockedSize, 0, 0x10,
                                           ICBuffer::LockType_ReadWrite);
                    short depth = tdepth + 1;
                    if (depth == Graphics2d_DepthMax) {
                        depth = 0;
                    }
                    tdepth = depth;
                    depth = depth - Graphics2d_DepthMax;
                    vertices[2] = depth;
                    vertices[m_vertexShorts + 2] = depth;
                } else {
                    m_vertexBuffer2d->Lock((unsigned char**)&vertices, lockedSize, 0, 8,
                                           ICBuffer::LockType_ReadWrite);
                }
            }

            // ---- positions ----
            const unsigned char* coords = (const unsigned char*)m_positionCoords;
            short x0 = *(const short*)(coords + m_positionStride * line);
            short y0 = *(const short*)(coords + m_positionStride * line + 2);
            short x1 = *(const short*)(coords + m_positionStride * (line + 1));
            short y1 = *(const short*)(coords + m_positionStride * (line + 1) + 2);
            unsigned int transformFlags = m_transformFlags.Top();
            Matrix& transform = m_matrices.m_data[m_matrices.m_count - 1];
            if ((transformFlags & (TransformFlag_Scale | TransformFlag_Rotate)) == 0) {
                if (transformFlags == TransformFlag_Translate) {
                    short translateX = (short)(transform.m_tx >> 16);
                    short translateY = (short)(transform.m_ty >> 16);
                    x0 += translateX;
                    y0 += translateY;
                    x1 += translateX;
                    y1 += translateY;
                }
            } else {
                short transformedX0 = (short)((arm::smult16(x0 << 16, transform.m[0][0]) +
                                               arm::smult16(y0 << 16, transform.m[0][1]) + transform.m_tx) >> 16);
                short transformedY0 = (short)((arm::smult16(x0 << 16, transform.m[1][0]) +
                                               arm::smult16(y0 << 16, transform.m[1][1]) + transform.m_ty) >> 16);
                short transformedX1 = (short)((arm::smult16(x1 << 16, transform.m[0][0]) +
                                               arm::smult16(y1 << 16, transform.m[0][1]) + transform.m_tx) >> 16);
                short transformedY1 = (short)((arm::smult16(x1 << 16, transform.m[1][0]) +
                                               arm::smult16(y1 << 16, transform.m[1][1]) + transform.m_ty) >> 16);
                x0 = transformedX0;
                y0 = transformedY0;
                x1 = transformedX1;
                y1 = transformedY1;
            }
            vertices[0] = x0;
            vertices[1] = y0;
            vertices[m_vertexShorts] = x1;
            vertices[m_vertexShorts + 1] = y1;

            // ---- immediate draw ----
            if (pDisplayProgram == NULL) {
                ICBuffer::Stream vertexStream;
                if (m_positionComponents == 3) {
                    vertexStream.m_buffer = m_vertexBuffer3d;
                } else {
                    vertexStream.m_buffer = m_vertexBuffer2d;
                }
                vertexStream.m_count = 2;
                vertexStream.m_first = 0;
                vertexStream.m_dirty = 1;
                m_graphics->SetVertexStream(0, vertexStream);
                m_graphics->EnableVertexStream(0, 1);
                int lockedSize;
                m_indexBuffer->Lock((unsigned char**)&indices, lockedSize, 0, 4, ICBuffer::LockType_ReadWrite);
                indices[0] = 0;
                indices[1] = 1;
                ICBuffer::Stream indexStream;
                indexStream.m_buffer = m_indexBuffer;
                indexStream.m_first = 0;
                indexStream.m_count = 2;
                indexStream.m_dirty = 1;
                // the original draws the 2 indexed line vertices as a strip (GL_LINE_STRIP)
                m_graphics->Draw(ICGraphics::Primitive_LineStrip, indexStream);
            }
        }
    }
    if (blendPushed) {
        m_rasterizerState->PopState(ICRasterizerState::State_Blending);
    }
}

void CGraphics2d_OGLES::DrawRectangles(int first, int count, const components::Color_RGBA& color)
{
    short* vertices = NULL;
    unsigned short* indices = NULL;

    // ---- shader program ----
    CGraphics_OGLES* graphics = m_graphics;
    unsigned int versionMajor = graphics->m_versionMajor;
    int colorIndex = graphics->m_shaderParameters.GetEntryIndex(L"constColor");
    ICShaderProgram* program;
    if (versionMajor == 1) {
        program = m_fixedFunctionProgram;
        *(components::Color_RGBA*)graphics->m_shaderParameters.GetEntryData(colorIndex) = color;
        int texEnvIndex = graphics->m_shaderParameters.GetEntryIndex(L"texEnv0");
        *(int*)graphics->m_shaderParameters.GetEntryData(texEnvIndex) = Graphics2d_TexEnv_Color;
    } else {
        program = m_colorProgram;
        *(components::Color_RGBA*)graphics->m_shaderParameters.GetEntryData(colorIndex) = color;
    }
    graphics = m_graphics;
    if (graphics->m_shaderProgram != program ||
        (graphics->m_shadersLevel != NULL &&
         ((CDisplayProgram::Instruction::Header*)*graphics->m_shadersLevel)->m_size ==
             CDisplayProgram::Instruction::LevelNodeSize)) {
        graphics->SetShaderProgram(*program);
    }

    // ---- blending, vertex layout and depth ----
    const components::Color_RGBA* pBlendColor = &color;
    unsigned char blendPushed;
    unsigned char blendEnabled;
    unsigned char blendNeeded;
#define CONFIG_BLENDING_TEXTURE_NEEDS_BLEND 1
#include "CGraphics2d_OGLES_ConfigBlending.i"
#undef CONFIG_BLENDING_TEXTURE_NEEDS_BLEND
#define CONFIG_VERTEX_FORMAT_2D VertexFormat_Vtx2d
#define CONFIG_VERTEX_FORMAT_3D VertexFormat_Vtx3d
#include "CGraphics2d_OGLES_ConfigVertexInfoAndDepthUsage.i"
#undef CONFIG_VERTEX_FORMAT_2D
#undef CONFIG_VERTEX_FORMAT_3D

    if (m_positionType == ComponentType_Short && count > 0) {
        CDisplayProgram* pDisplayProgram = (CDisplayProgram*)m_graphics->m_displayProgram;
        for (int rectangle = first; rectangle < first + count; rectangle += 2) {
            if (pDisplayProgram != NULL) {
                // record a batch draw of one quad
                m_drawInstruction->m_header.m_level = (unsigned short)pDisplayProgram->m_opcodeLevels[
                    CDisplayProgram::Instruction::Opcode::IdxVtxStreamTrisBatch & 0xff];
                CDisplayProgram::Instruction::Draw* pDraw = m_drawInstruction;
                pDraw->m_header.m_opcode = CDisplayProgram::Instruction::Opcode::IdxVtxStreamTrisBatch;
                pDraw->m_vertexStream = NULL;
                pDraw->m_indexCount = 6;
                pDraw->m_vertexCount = 4;
                pDraw->m_indexStream = NULL;
                unsigned char* pInstruction = (unsigned char*)m_drawInstruction;
#include "../../src/CDisplayProgram_Add.i"
                // the quad is the last one of the (possibly merged) batch
                ICBuffer::Stream* pVertexStream = pDraw->m_vertexStream;
                unsigned int vertexCount = pDraw->m_vertexCount;
                unsigned int indexCount = pDraw->m_indexCount;
                ICBuffer::Stream* pIndexStream = pDraw->m_indexStream;
                int lockedSize;
                pVertexStream->m_buffer->Lock((unsigned char**)&vertices, lockedSize,
                                              (vertexCount + pVertexStream->m_first - 4) * m_vertexSize,
                                              m_vertexSize * 4, ICBuffer::LockType_ReadWrite);
                pIndexStream->m_buffer->Lock((unsigned char**)&indices, lockedSize,
                                             (indexCount + pIndexStream->m_first - 6) * 2, 0xc,
                                             ICBuffer::LockType_ReadWrite);
                unsigned short firstIndex = (unsigned short)(vertexCount - 4);
                indices[0] = firstIndex;
                indices[1] = firstIndex + 1;
                indices[2] = firstIndex + 2;
                indices[3] = firstIndex + 1;
                indices[4] = firstIndex + 2;
                indices[5] = firstIndex + 3;
                if (m_positionComponents == 3) {
                    short depth;
                    short* pProgramDepth = &pDisplayProgram->m_batchDepth;
                    if (tptr == pVertexStream) {
                        depth = *pProgramDepth;
                    } else {
                        depth = *pProgramDepth + 1;
                        tptr = pVertexStream;
                        *pProgramDepth = depth;
                    }
                    depth = depth - Graphics2d_DepthMax;
                    vertices[2] = depth;
                    vertices[m_vertexShorts + 2] = depth;
                    vertices[m_vertexShorts * 2 + 2] = depth;
                    vertices[m_vertexShorts * 3 + 2] = depth;
                }
            } else {
                int lockedSize;
                if (m_positionComponents == 3) {
                    m_vertexBuffer3d->Lock((unsigned char**)&vertices, lockedSize, 0, 0x20,
                                           ICBuffer::LockType_ReadWrite);
                    short depth = tdepth + 1;
                    if (depth == Graphics2d_DepthMax) {
                        depth = 0;
                    }
                    tdepth = depth;
                    depth = depth - Graphics2d_DepthMax;
                    vertices[2] = depth;
                    vertices[m_vertexShorts + 2] = depth;
                    vertices[m_vertexShorts * 2 + 2] = depth;
                    vertices[m_vertexShorts * 3 + 2] = depth;
                } else {
                    m_vertexBuffer2d->Lock((unsigned char**)&vertices, lockedSize, 0, 0x10,
                                           ICBuffer::LockType_ReadWrite);
                }
            }

            // ---- positions: the two coordinates are opposite corners ----
            const unsigned char* coords = (const unsigned char*)m_positionCoords;
            short x0 = *(const short*)(coords + m_positionStride * rectangle);
            short y0 = *(const short*)(coords + m_positionStride * rectangle + 2);
            short x1 = *(const short*)(coords + m_positionStride * (rectangle + 1));
            short y1 = *(const short*)(coords + m_positionStride * (rectangle + 1) + 2);
            short vertex0X = x0;
            short vertex0Y = y0;
            short vertex1X = x0;
            short vertex1Y = y1;
            short vertex2X = x1;
            short vertex2Y = y0;
            short vertex3X = x1;
            short vertex3Y = y1;
            unsigned int transformFlags = m_transformFlags.Top();
            Matrix& transform = m_matrices.m_data[m_matrices.m_count - 1];
            if ((transformFlags & (TransformFlag_Scale | TransformFlag_Rotate)) == 0) {
                if (transformFlags == TransformFlag_Translate) {
                    short translateX = (short)(transform.m_tx >> 16);
                    short translateY = (short)(transform.m_ty >> 16);
                    vertex0X += translateX;
                    vertex0Y += translateY;
                    vertex1X += translateX;
                    vertex1Y += translateY;
                    vertex2X += translateX;
                    vertex2Y += translateY;
                    vertex3X += translateX;
                    vertex3Y += translateY;
                }
            } else {
                int fixedX0 = x0 << 16;
                int fixedY0 = y0 << 16;
                int fixedX1 = x1 << 16;
                int fixedY1 = y1 << 16;
                vertex0X = (short)((arm::smult16(fixedX0, transform.m[0][0]) + arm::smult16(fixedY0, transform.m[0][1]) +
                                    transform.m_tx) >> 16);
                vertex0Y = (short)((arm::smult16(fixedX0, transform.m[1][0]) + arm::smult16(fixedY0, transform.m[1][1]) +
                                    transform.m_ty) >> 16);
                vertex1X = (short)((arm::smult16(fixedX0, transform.m[0][0]) + arm::smult16(fixedY1, transform.m[0][1]) +
                                    transform.m_tx) >> 16);
                vertex1Y = (short)((arm::smult16(fixedX0, transform.m[1][0]) + arm::smult16(fixedY1, transform.m[1][1]) +
                                    transform.m_ty) >> 16);
                vertex2X = (short)((arm::smult16(fixedX1, transform.m[0][0]) + arm::smult16(fixedY0, transform.m[0][1]) +
                                    transform.m_tx) >> 16);
                vertex2Y = (short)((arm::smult16(fixedX1, transform.m[1][0]) + arm::smult16(fixedY0, transform.m[1][1]) +
                                    transform.m_ty) >> 16);
                vertex3X = (short)((arm::smult16(fixedX1, transform.m[0][0]) + arm::smult16(fixedY1, transform.m[0][1]) +
                                    transform.m_tx) >> 16);
                vertex3Y = (short)((arm::smult16(fixedX1, transform.m[1][0]) + arm::smult16(fixedY1, transform.m[1][1]) +
                                    transform.m_ty) >> 16);
            }
            vertices[0] = vertex0X;
            vertices[1] = vertex0Y;
            vertices[m_vertexShorts] = vertex1X;
            vertices[m_vertexShorts + 1] = vertex1Y;
            vertices[m_vertexShorts * 2] = vertex2X;
            vertices[m_vertexShorts * 2 + 1] = vertex2Y;
            vertices[m_vertexShorts * 3] = vertex3X;
            vertices[m_vertexShorts * 3 + 1] = vertex3Y;

            // ---- immediate draw ----
            if (pDisplayProgram == NULL) {
                ICBuffer::Stream vertexStream;
                if (m_positionComponents == 3) {
                    vertexStream.m_buffer = m_vertexBuffer3d;
                } else {
                    vertexStream.m_buffer = m_vertexBuffer2d;
                }
                vertexStream.m_count = 4;
                vertexStream.m_first = 0;
                vertexStream.m_dirty = 1;
                m_graphics->SetVertexStream(0, vertexStream);
                m_graphics->EnableVertexStream(0, 1);
                int lockedSize;
                m_indexBuffer->Lock((unsigned char**)&indices, lockedSize, 0, 0xc, ICBuffer::LockType_ReadWrite);
                indices[0] = 0;
                indices[1] = 1;
                indices[2] = 2;
                indices[3] = 1;
                indices[4] = 2;
                indices[5] = 3;
                ICBuffer::Stream indexStream;
                indexStream.m_buffer = m_indexBuffer;
                indexStream.m_count = 6;
                indexStream.m_first = 0;
                indexStream.m_dirty = 1;
                // the original draws the 6 indexed triangle vertices as a strip (GL_TRIANGLE_STRIP)
                m_graphics->Draw(ICGraphics::Primitive_TriangleStrip, indexStream);
            }
        }
    }
    if (blendPushed) {
        m_rasterizerState->PopState(ICRasterizerState::State_Blending);
    }
}

void CGraphics2d_OGLES::Draw(Primitive primitive, int first, int count, const components::Color_RGBA& color, float x,
                             float y)
{
    PushTransform();
    Translate(x, y);
    Draw(primitive, first, count, color);
    PopTransform();
}

void CGraphics2d_OGLES::Draw(ICRenderText2d& text, const components::Color_RGBA* color)
{
    CRenderText2d::TextModel& textView = ((CRenderText2d&)text).m_model;
    if ((textView.m_flags & CRenderText2d::Flag_BitmapFont) != 0) {
        const wchar_t* characters = (const wchar_t*)textView.m_text;
        if (characters != NULL) {
            CBitmapFont* font = (CBitmapFont*)textView.m_font;
            CBitmapFont::FontHeader* header = font->m_header;
            ICRenderSurface* image = font->m_image;
            int x = 0;
            for (int index = 0; index < textView.m_length; index++) {
                const CBitmapFont::ControlCharDesc* control = font->GetControlCharDesc(characters[index]);
                if (control != NULL) {
                    x = control->m_advance + x + header->m_charSpacing;
                } else {
                    const CBitmapFont::CharDesc* glyph = font->GetCharDesc(characters[index]);
                    if (glyph != NULL) {
                        short glyphX = glyph->m_x;
                        short glyphY = glyph->m_y;
                        short glyphWidth = glyph->m_width;
                        short glyphHeight = glyph->m_height;
                        components::CRectangle<short> region;
                        region.m_x = glyphX;
                        region.m_y = glyphY;
                        region.m_width = glyphWidth;
                        region.m_height = glyphHeight;
                        PushTransform();
                        Translate((float)(glyph->m_offsetX + x), (float)glyph->m_offsetY);
                        Draw(*image, &region, color, Flip_None);
                        PopTransform();
                        x = glyph->m_advance + x + header->m_charSpacing;
                    }
                }
            }
        }
    } else if ((textView.m_flags & CRenderText2d::Flag_SystemFont) != 0) {
        if (textView.m_surface != NULL) {
            Draw(*textView.m_surface, NULL, color, Flip_None);
        }
    }
}

ICGraphicsAbstract::Abstraction CGraphics2d_OGLES::GetAbstraction() const
{
    return ICGraphicsAbstract::Abstraction_Hardware;
}

void CGraphics2d_OGLES_Ortho(math::CMatrix4d& matrix, float left, float right, float bottom, float top,
                             float nearValue, float farValue)
{
    matrix.m[0] = 2.0f / (right - left);
    matrix.m[1] = 0.0f;
    matrix.m[2] = 0.0f;
    matrix.m[3] = 0.0f;
    matrix.m[4] = 0.0f;
    matrix.m[5] = 2.0f / (top - bottom);
    matrix.m[6] = 0.0f;
    matrix.m[7] = 0.0f;
    matrix.m[8] = 0.0f;
    matrix.m[9] = 0.0f;
    matrix.m[10] = -2.0f / (farValue - nearValue);
    matrix.m[11] = 0.0f;
    matrix.m[12] = -(right + left) / (right - left);
    matrix.m[13] = -(top + bottom) / (top - bottom);
    matrix.m[14] = -(farValue + nearValue) / (farValue - nearValue);
    matrix.m[15] = 1.0f;
}

} } } }
