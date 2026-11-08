// CRenderSurface_OGLES_Targetable.cpp - OpenGL ES texture surface that can be a render target.
#include "CRenderSurface_OGLES_Targetable.h"
#include "CGraphics_OGLES.h"
#include "CBlit.h"
#include "CBlitUtil.h"
#include "CMathFixed.h"
#include "CRectangle.h"

namespace com { namespace glu { namespace platform { namespace graphics {

// fixed point alpha reference of the copy (0.1)
static const GLfixed Targetable_AlphaReference = 0x1998;
// texture coordinates are 4.12 fixed point shorts (0x1000 = 1.0)
static const short Targetable_TexCoordOne = 0x1000;

/** Power of two texture size that holds `size` texels (at most 0x800). */
static inline int CRenderSurface_OGLES_Targetable_TextureSize(short size, int previous)
{
    int textureSize = previous;
    if (size < 9) {
        if (size < 2) {
            textureSize = 1;
        } else if (size < 3) {
            textureSize = 2;
        } else if (size < 5) {
            textureSize = 4;
        } else if (size <= 8) {
            textureSize = 8;
        }
    } else {
        if (size < 0x11) {
            textureSize = 0x10;
        } else if (size < 0x21) {
            textureSize = 0x20;
        } else if (size < 0x41) {
            textureSize = 0x40;
        } else if (size < 0x81) {
            textureSize = 0x80;
        } else if (size <= 0x100) {
            textureSize = 0x100;
        } else if (size <= 0x200) {
            textureSize = 0x200;
        } else if (size <= 0x400) {
            textureSize = 0x400;
        } else {
            textureSize = 0x800;
        }
    }
    return textureSize;
}

CRenderSurface_OGLES_Targetable::CRenderSurface_OGLES_Targetable()
{
    m_targetType = 0;
    m_lockCount = 0;
}

CRenderSurface_OGLES_Targetable::~CRenderSurface_OGLES_Targetable()
{
    Destroy();
}

void CRenderSurface_OGLES_Targetable::Destroy()
{
    CRenderSurface_OGLES_Texture::Destroy();
    m_lockCount = 0;
}

void CRenderSurface_OGLES_Targetable::Lock(unsigned char** data, int& pitch, LockType lockType,
                                           const components::CRectangle<short>* area)
{
    unsigned int flags = m_flags;
    CRenderSurface_OGLES_Texture* source = GetSourceTexture();
    // b117-va: the original lays out the first-lock path first and the re-lock path last
    if ((flags & Flag_LockMask) == 0) {
        CRenderSurfaceBuffer* buffer = source->m_buffers[Buffer_Color];
        if (!m_keepBufferData) {
            // the pixels were dropped after the upload: allocate them again
            buffer->Initialize(buffer->m_format, buffer->m_imageWidth, buffer->m_imageHeight, 1, NULL, NULL,
                               CRenderSurfaceBuffer::PitchCondition_Positive, buffer->m_pitch, NULL);
        }
        unsigned char convert = buffer->m_format != RSBFormat_A8B8G8R8;
        void* pixels;
        if (convert) {
            pixels = malloc(buffer->m_imageWidth * buffer->m_imageHeight * 4);
        } else {
            pixels = buffer->GetLevel(0);
        }

        if ((buffer->m_format & 0x4000) == 0) {
            if (this != ICGraphics::GetInstance()->GetTargetSurface()) {
                ICGraphics::GetInstance()->SetTargetSurface(*this);
            }
            glReadPixels(0, 0, buffer->m_imageWidth, buffer->m_imageHeight, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
            if (!convert) {
                buffer->MakePitchNegative();
            } else {
                // OpenGL reads A8B8G8R8: convert into the buffer
                CBlitUtil::BlitOpDesc blitOp;
                blitOp.m_source = RSBFormat_A8B8G8R8;
                blitOp.m_destination = buffer->m_format;
                blitOp.m_colorKeyTest = CBlitUtil::ColorKeyTest_None;
                blitOp.m_alphaTest = CBlitUtil::AlphaTest_None;
                blitOp.m_blend = CBlitUtil::Blend_None;
                blitOp.m_opId = CBlitUtil::BlitOpId_Buffer;
                CBlitUtil::GetBlitOp(blitOp);
                CBlit::BufferOpDesc copy;
                copy.m_scaleX = 0x10000;
                copy.m_scaleY = 0x10000;
                copy.m_flipX = 0;
                copy.m_flipY = 0;
                copy.m_width = buffer->m_imageWidth;
                copy.m_height = buffer->m_imageHeight;
                copy.m_reserved08 = 0;
                copy.m_sourcePitch = (short)(buffer->m_imageWidth << 2);
                copy.m_source = pixels;
                copy.m_destination = buffer->GetLevel(0);
                copy.m_destinationPitch = buffer->m_pitch;
                ((void (*)(const CBlit::BufferOpDesc&))blitOp.m_function)(copy);
                buffer->MakePitchNegative();
                free(pixels);
            }
        } else {
            buffer->MakePitchNegative();
            if (convert) {
                free(pixels);
            }
        }

        // lock the source buffer through this surface
        CRenderSurfaceBuffer* ownBuffer = m_buffers[Buffer_Color];
        m_buffers[Buffer_Color] = buffer;
        unsigned char* locked;
        CRenderSurface_SW::Lock(&locked, pitch, lockType, area);
        m_buffers[Buffer_Color] = ownBuffer;
        if (locked == NULL) {
            if (data != NULL) {
                *data = NULL;
            }
            pitch = 0;
        } else {
            if (data != NULL) {
                *data = locked;
                *data = locked + (buffer->m_height - buffer->m_imageHeight) * pitch;
            }
            m_lockCount++;
            m_flags |= Flag_LockMask;
        }
    } else {
        // locked again: the pixels were read back by the first Lock
        if (!source->FetchFromBuffer(Buffer_Color, data, pitch, area)) {
            return;
        }
        m_lockCount++;
        if (data == NULL) {
            return;
        }
        *data = *data + (source->m_buffers[Buffer_Color]->m_height - source->m_buffers[Buffer_Color]->m_imageHeight) * pitch;
    }
}

void CRenderSurface_OGLES_Targetable::Unlock()
{
    if (m_flags & Flag_LockMask) {
        CRenderSurfaceBuffer* buffer = GetSourceTexture()->m_buffers[Buffer_Color];
        m_lockCount--;
        if (m_lockCount != 0) {
            return;
        }
        // upload the changed pixels
        buffer->MakePitchPositive();
        InitializeTextureObjects();
        m_textureUpdated = 1;
        m_flags &= ~Flag_LockMask;
    }
}

void CRenderSurface_OGLES_Targetable::UpdateTargetWithTexture()
{
    CRenderSurface_OGLES_Texture* source = GetSourceTexture();
    glDisable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(0);
    glDisable(GL_CULL_FACE);
    glShadeModel(GL_FLAT);
    glAlphaFuncx(GL_GREATER, Targetable_AlphaReference);
    glDepthRangex(0, 0x10000);
    unsigned int width;
    unsigned int height;
    GetWidthAndHeight(width, height);
    glDisable(GL_SCISSOR_TEST);
    glActiveTexture(GL_TEXTURE0);
    glTexEnvx(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glMatrixMode(GL_TEXTURE);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_TEXTURE_2D);
    glLoadIdentity();
    // the texture coordinates are 4.12 fixed point
    glScalex(0x10, 0x10, math::CMathFixed::One);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_BLEND);
    CGraphics_OGLES* graphics = (CGraphics_OGLES*)ICGraphics::GetInstance();
    const ICGraphics::Description& description = graphics->GetDescription();
    glDisable(GL_DITHER);
    glColorMask(1, 1, 1, 1);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glDepthMask(0);
    glDisable(GL_NORMALIZE);
    glDisable(GL_MULTISAMPLE);
    glDisable(GL_FOG);
    glDisable(GL_COLOR_MATERIAL);
    glDisable(GL_LIGHTING);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
    for (unsigned int unit = 1; unit < description.m_maxTextureUnits; unit++) {
        glActiveTexture(GL_TEXTURE0 + unit);
        glDisable(GL_TEXTURE_2D);
        glClientActiveTexture(GL_TEXTURE0 + unit);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    }
    glDisableClientState(GL_NORMAL_ARRAY);
    glColor4x(components::Color_ARGB_fixed::White.r, components::Color_ARGB_fixed::White.g,
              components::Color_ARGB_fixed::White.b, components::Color_ARGB_fixed::White.a);
    glActiveTexture(GL_TEXTURE0);
    glDisable(GL_TEXTURE_2D);
    glClientActiveTexture(GL_TEXTURE0);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);

    // one quad per texture tile
    unsigned int tileWidth = description.m_maxRenderTargetWidth;
    unsigned int tileHeight = description.m_maxRenderTargetHeight;
    short texCoords[8];
    texCoords[0] = 0;
    texCoords[2] = 0;
    texCoords[3] = 0;
    texCoords[7] = 0;
    glEnable(GL_TEXTURE_2D);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);
    short vertices[8];
    short x = 0;
    short y = 0;
    int textureWidth;
    int textureHeight;
    for (int index = 0; index < source->m_textureCount; index++) {
        GLuint texture = source->GetTextureName(index);
        short tileRight = x + (short)tileWidth;
        short quadWidth;
        if (tileRight > (short)width) {
            // the last column: scale the coordinates to the part of the texture that is used
            quadWidth = (short)width - x;
            textureWidth = CRenderSurface_OGLES_Targetable_TextureSize(quadWidth, textureWidth);
            texCoords[4] = (short)((unsigned int)math::CMathFixed::Div(quadWidth << 16, textureWidth << 16) >> 4);
        } else {
            texCoords[4] = Targetable_TexCoordOne;
            quadWidth = tileWidth;
        }
        texCoords[6] = texCoords[4];
        short tileTop = y + (short)tileHeight;
        short quadHeight;
        if (tileTop > (short)height) {
            quadHeight = (short)height - y;
            textureHeight = CRenderSurface_OGLES_Targetable_TextureSize(quadHeight, textureHeight);
            texCoords[1] = (short)((unsigned int)math::CMathFixed::Div(quadHeight << 16, textureHeight << 16) >> 4);
        } else {
            texCoords[1] = Targetable_TexCoordOne;
            quadHeight = tileHeight;
        }
        texCoords[5] = texCoords[1];
        vertices[0] = x;
        vertices[1] = (short)height - (quadHeight + y);
        vertices[2] = x;
        vertices[3] = (short)height - y;
        vertices[4] = quadWidth + x;
        vertices[5] = vertices[1];
        vertices[6] = vertices[4];
        vertices[7] = vertices[3];
        graphics->m_textureUnits[0].m_surface = source;
        graphics->m_textureUnits[0].m_level = index;
        graphics->m_textureUnits[0].m_dirty = 1;
        glBindTexture(GL_TEXTURE_2D, texture);
        glVertexPointer(2, GL_SHORT, 4, vertices);
        glTexCoordPointer(2, GL_SHORT, 4, texCoords);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        if (tileRight >= (short)width) {
            x = 0;
            y = tileTop;
        } else {
            x = tileRight;
        }
    }
}

} } } }
