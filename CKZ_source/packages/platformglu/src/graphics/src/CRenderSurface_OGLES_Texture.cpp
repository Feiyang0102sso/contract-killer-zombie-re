// CRenderSurface_OGLES_Texture.cpp - render surface kept in OpenGL ES textures.
#include "CRenderSurface_OGLES_Texture.h"
#include "CRenderSurface_OGLES_Targetable.h"
#include "CGraphics_OGLES.h"
#include <OpenGLES/ES1/glext.h>
#include "CBlit.h"
#include "CBlitUtil.h"
#include "CApplet.h"
#include "CApp.h"
#include "CHash.h"
#include "CRectangle.h"

namespace com { namespace glu { namespace platform { namespace graphics {

// render surface used to read a locked texture back (targetable texture)
static const ICRenderSurface::Targetability Texture_LockSurfaceTargetability = ICRenderSurface::Targetability_2;
// creation setting key 4 of the lock surface (not used by the texture classes themselves)
static const unsigned int Texture_LockSurfaceParam4 = 4;

/** Round `dimension` up to a power of two (0 above 0x1000); defined at the end of the file. */
static int CRenderSurface_OGLES_Texture_AdjustToPowOfTwoDimension(int dimension);

/**
 * Fill the format replacement tables of a decoder: image formats without alpha become `opaque`, the
 * ones with alpha `alpha`; with a color key (second table) `keyedOpaque` / `keyedAlpha`.
 */
static inline void CRenderSurface_OGLES_Texture_SetFormatMap(CRenderSurfaceBuffer::SourceStreamDesc& desc,
                                                             RSBFormat opaque, RSBFormat alpha,
                                                             RSBFormat keyedOpaque, RSBFormat keyedAlpha)
{
    CRenderSurfaceBuffer::FormatMapEntry* plain = desc.m_formatMap[0];
    CRenderSurfaceBuffer::FormatMapEntry* keyed = desc.m_formatMap[1];
    plain[0].m_from = RSBFormat_R5G6B5;
    plain[0].m_to = RSBFormat_R5G6B5;
    plain[1].m_from = RSBFormat_R8G8B8;
    plain[1].m_to = opaque;
    plain[2].m_from = RSBFormat_A8R8G8B8;
    plain[2].m_to = alpha;
    plain[3].m_from = RSBFormat_A8B8G8R8;
    plain[3].m_to = alpha;
    plain[4].m_from = RSBFormat_P16X8R8G8B8;
    plain[4].m_to = opaque;
    plain[5].m_from = RSBFormat_P16A8R8G8B8;
    plain[5].m_to = alpha;
    plain[6].m_from = RSBFormat_P256X8R8G8B8;
    plain[6].m_to = opaque;
    plain[7].m_from = RSBFormat_P256A8R8G8B8;
    plain[7].m_to = alpha;
    keyed[0].m_from = RSBFormat_R5G6B5;
    keyed[0].m_to = RSBFormat_R5G5B5A1;
    keyed[1].m_from = RSBFormat_R8G8B8;
    keyed[1].m_to = keyedOpaque;
    keyed[2].m_from = RSBFormat_A8R8G8B8;
    keyed[2].m_to = keyedAlpha;
    keyed[3].m_from = RSBFormat_A8B8G8R8;
    keyed[3].m_to = keyedAlpha;
    keyed[4].m_from = RSBFormat_P16X8R8G8B8;
    keyed[4].m_to = keyedOpaque;
    keyed[5].m_from = RSBFormat_P16A8R8G8B8;
    keyed[5].m_to = keyedAlpha;
    keyed[6].m_from = RSBFormat_P256X8R8G8B8;
    keyed[6].m_to = keyedOpaque;
    keyed[7].m_from = RSBFormat_P256A8R8G8B8;
    keyed[7].m_to = keyedAlpha;
}

CRenderSurface_OGLES_Texture::CRenderSurface_OGLES_Texture()
{
    m_flags = (m_flags & ~(Flag_Texture | Flag_Software | Flag_Hardware | Flag_400 | Flag_Lockable)) | Flag_Texture |
              Flag_Hardware;
    m_textureCount = 0;
    m_texture = 0;
    m_magFilter = Filter_Nearest;
    m_minFilter = Filter_Nearest;
    m_wrapS = Wrap_ClampToEdge;
    m_wrapT = Wrap_ClampToEdge;
    m_keepBufferData = 0;
    m_contentsUndefined = 1;
    m_sourceTexture = NULL;
    m_lockSurface = NULL;
}

CRenderSurface_OGLES_Texture::~CRenderSurface_OGLES_Texture()
{
    Destroy();
}

void CRenderSurface_OGLES_Texture::Destroy()
{
    if (m_lockSurface != NULL) {
        delete m_lockSurface;
        m_lockSurface = NULL;
    }
    if (m_sourceTexture != NULL) {
        // the textures belong to the source surface
        m_sourceTexture = NULL;
    } else {
        DestroyTextureObjects();
    }
    m_magFilter = Filter_Nearest;
    m_minFilter = Filter_Nearest;
    m_wrapS = Wrap_ClampToEdge;
    m_wrapT = Wrap_ClampToEdge;
    m_keepBufferData = 0;
    CRenderSurface::Destroy();
    m_contentsUndefined = 1;
}

unsigned char CRenderSurface_OGLES_Texture::Initialize(const InitParam* params)
{
    unsigned char result = InitializeBuffers(params);
    m_keepBufferData = 0;
    if (result) {
        m_contentsUndefined = 0;
        if ((m_flags & Flag_Reference) == 0 && m_sourceTexture == NULL) {
            InitializeTextureObjects();
        }
        m_flags |= Flag_Initialized;
    }
    return result;
}

void CRenderSurface_OGLES_Texture::InitializeTextureObjects()
{
    DestroyTextureObjects();
    CGraphics_OGLES* graphics = (CGraphics_OGLES*)ICGraphics::GetInstance();
    CRenderSurface_OGLES_Texture* source = GetSourceTexture();
    CRenderSurfaceBuffer* buffer = source->m_buffers[Buffer_Color];

    RSBFormat format = buffer->m_format;
    GLenum glFormat = GL_RGB + ((format >> 14) & 1);    // the alpha bit selects GL_RGBA
    GLenum glType;
    switch (format) {
    case RSBFormat_R5G6B5:
        glType = GL_UNSIGNED_SHORT_5_6_5;
        break;
    case RSBFormat_R5G5B5A1:
        glType = GL_UNSIGNED_SHORT_5_5_5_1;
        break;
    case RSBFormat_R4G4B4A4:
        glType = GL_UNSIGNED_SHORT_4_4_4_4;
        break;
    case RSBFormat_B8G8R8:
    case RSBFormat_A8B8G8R8:
        glType = GL_UNSIGNED_BYTE;
        break;
    case RSBFormat_PVRTC_RGB2:
        glType = GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG;
        break;
    case RSBFormat_PVRTC_RGBA2:
        glType = GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG;
        break;
    case RSBFormat_PVRTC_RGB4:
        glType = GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG;
        break;
    case RSBFormat_PVRTC_RGBA4:
        glType = GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG;
        break;
    default:
        glType = 0;
        break;
    }

    // images larger than the device allows are split into tiles
    const ICGraphics::Description& description = graphics->GetDescription();
    unsigned int tileWidth = description.m_maxRenderTargetWidth;
    unsigned int tileHeight = description.m_maxRenderTargetHeight;

    unsigned int imageWidth = buffer->m_imageWidth;
    // b146: the original computes the remainder before the quotient (__umodsi3 then __udivsi3)
    unsigned int lastColumnWidth = imageWidth % (unsigned short)tileWidth;
    unsigned short columns = imageWidth / (unsigned short)tileWidth;
    if (lastColumnWidth != 0) {
        columns++;
    } else if (imageWidth < (unsigned short)tileWidth) {
        lastColumnWidth = imageWidth;
    } else {
        lastColumnWidth = tileWidth;
    }
    unsigned int lastColumnTextureWidth = lastColumnWidth;
    if (m_flags & Flag_PowerOfTwo) {
        lastColumnTextureWidth = CRenderSurface_OGLES_Texture_AdjustToPowOfTwoDimension((unsigned short)lastColumnWidth);
    }

    unsigned int imageHeight = buffer->m_imageHeight;
    unsigned int lastRowHeight = imageHeight % (unsigned short)tileHeight;
    unsigned short rows = imageHeight / (unsigned short)tileHeight;
    if (lastRowHeight != 0) {
        rows++;
    } else if (imageHeight < (unsigned short)tileHeight) {
        lastRowHeight = imageHeight;
    } else {
        lastRowHeight = tileHeight;
    }
    unsigned int lastRowTextureHeight = lastRowHeight;
    if (m_flags & Flag_PowerOfTwo) {
        lastRowTextureHeight = CRenderSurface_OGLES_Texture_AdjustToPowOfTwoDimension((unsigned short)lastRowHeight);
    }

    source->m_textureCount = columns * rows;
    if (source->m_textureCount >= 2) {
        source->m_textures = new GLuint[source->m_textureCount];
        glGenTextures(source->m_textureCount, source->m_textures);
    } else if (source->m_texture == 0) {
        glGenTextures(1, &source->m_texture);
    }

    // a tile is copied out of the image before it is uploaded
    unsigned char* tileBuffer = NULL;
    if (source->m_textureCount >= 2) {
        tileBuffer = (unsigned char*)malloc(tileHeight * tileWidth * (buffer->m_format & RSBFormat_BytesPerPixelMask));
    }

    CBlitUtil::BlitOpDesc blitOp;
    blitOp.m_function = NULL;
    CBlit::BufferOpDesc copy;
    void* pixels;
    unsigned int uploadWidth;
    unsigned int uploadHeight;
    GLint magFilter;
    GLint minFilter;
    GLint wrapS;
    GLint wrapT;
    unsigned short column = 0;
    unsigned short row = 0;
    for (int index = 0; index < source->m_textureCount; index++) {
        if (source->m_textureCount == 1) {
            pixels = buffer->GetLevel(0);
            uploadWidth = (unsigned short)lastColumnTextureWidth;
            uploadHeight = (unsigned short)lastRowTextureHeight;
        } else {
            if (blitOp.m_function == NULL) {
                blitOp.m_source = buffer->m_format;
                blitOp.m_destination = buffer->m_format;
                blitOp.m_colorKeyTest = CBlitUtil::ColorKeyTest_None;
                blitOp.m_alphaTest = CBlitUtil::AlphaTest_None;
                blitOp.m_blend = CBlitUtil::Blend_None;
                blitOp.m_opId = CBlitUtil::BlitOpId_Buffer;
                CBlitUtil::GetBlitOp(blitOp);
                copy.m_scaleX = 0x10000;
                copy.m_scaleY = 0x10000;
                copy.m_flipX = 0;
                copy.m_flipY = 0;
                copy.m_reserved08 = 0;
                copy.m_sourcePitch = buffer->m_pitch;
                copy.m_destination = tileBuffer;
                pixels = tileBuffer;
            }
            if (column == columns - 1) {
                copy.m_width = (unsigned short)lastColumnWidth;
                uploadWidth = (unsigned short)lastColumnTextureWidth;
                copy.m_destinationPitch = (short)((blitOp.m_destination & RSBFormat_BytesPerPixelMask) * uploadWidth);
            } else {
                copy.m_width = tileWidth;
                uploadWidth = tileWidth;
                copy.m_destinationPitch = (short)((blitOp.m_destination & RSBFormat_BytesPerPixelMask) * (short)tileWidth);
            }
            if (row == rows - 1) {
                copy.m_height = (unsigned short)lastRowHeight;
            } else {
                copy.m_height = tileHeight;
            }
            copy.m_source = (unsigned char*)buffer->GetLevel(0) +
                            tileWidth * column * (blitOp.m_source & RSBFormat_BytesPerPixelMask) +
                            copy.m_sourcePitch * (row * tileHeight);
            ((void (*)(const CBlit::BufferOpDesc&))blitOp.m_function)(copy);
            if (row == rows - 1) {
                uploadHeight = (unsigned short)lastRowTextureHeight;
            } else {
                uploadHeight = tileHeight;
            }
            column++;
            if (column == columns) {
                row++;
                column = 0;
            }
        }

        if ((m_flags & Flag_PowerOfTwo) == 0) {
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        }
        graphics->m_textureUnits[0].m_surface = source;
        graphics->m_textureUnits[0].m_level = index;
        graphics->m_textureUnits[0].m_dirty = 1;
        graphics->m_glState.ActiveTexture(GL_TEXTURE0);
        graphics->m_glState.BindTexture(GL_TEXTURE_2D, source->GetTextureName(index));

        if (m_magFilter == Filter_Nearest) {
            magFilter = GL_NEAREST;
        } else if (m_magFilter == Filter_Linear) {
            magFilter = GL_LINEAR;
        }
        switch (m_minFilter) {
        case Filter_Linear:
            minFilter = GL_LINEAR;
            break;
        case Filter_LinearMipmapLinear:
            minFilter = GL_LINEAR_MIPMAP_LINEAR;
            break;
        case Filter_LinearMipmapNearest:
            minFilter = GL_LINEAR_MIPMAP_NEAREST;
            break;
        case Filter_Nearest:
            minFilter = GL_NEAREST;
            break;
        case Filter_NearestMipmapLinear:
            minFilter = GL_NEAREST_MIPMAP_LINEAR;
            break;
        case Filter_NearestMipmapNearest:
            minFilter = GL_NEAREST_MIPMAP_NEAREST;
            break;
        }
        if (m_wrapS == Wrap_Repeat) {
            wrapS = GL_REPEAT;
        } else if (m_wrapS == Wrap_ClampToEdge) {
            wrapS = GL_CLAMP_TO_EDGE;
        }
        if (m_wrapT == Wrap_Repeat) {
            wrapT = GL_REPEAT;
        } else if (m_wrapT == Wrap_ClampToEdge) {
            wrapT = GL_CLAMP_TO_EDGE;
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);

        if ((int)buffer->m_format < 0) {
            // compressed formats are uploaded as they are
            glCompressedTexImage2D(GL_TEXTURE_2D, 0, glType, uploadWidth, uploadHeight, 0,
                                   buffer->m_pitch * buffer->m_height, pixels);
        } else {
            glTexImage2D(GL_TEXTURE_2D, 0, glFormat, uploadWidth, uploadHeight, 0, glFormat, glType, pixels);
        }
        if ((m_flags & Flag_PowerOfTwo) == 0) {
            glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        }
    }

    if (tileBuffer != NULL) {
        free(tileBuffer);
    }
    if (!m_keepBufferData) {
        buffer->DestroyBufferData();
    }
}

void CRenderSurface_OGLES_Texture::DestroyTextureObjects()
{
    CRenderSurface_OGLES_Texture* source = GetSourceTexture();
    if (source != NULL && source->m_textureCount != 0) {
        if (source->m_textureCount == 1) {
            glDeleteTextures(1, &source->m_texture);
        } else {
            glDeleteTextures(source->m_textureCount, source->m_textures);
        }
        if (source->m_textureCount > 1 && source->m_textures != NULL) {
            delete[] source->m_textures;
        }
        source->m_textureCount = 0;
        source->m_texture = 0;
    }
}

void CRenderSurface_OGLES_Texture::Lock(unsigned char** data, int& pitch, LockType lockType,
                                        const components::CRectangle<short>* area)
{
    unsigned char* locked;
    if (m_sourceTexture != NULL) {
        m_sourceTexture->Bind();
        m_sourceTexture->Lock(&locked, pitch, lockType, area);
        Bind();
        if (locked != NULL) {
            if (data != NULL) {
                *data = locked;
            }
            m_flags |= Flag_LockMask;
            return;
        }
    } else {
        if ((m_flags & Flag_LockMask) == 0) {
            CRenderSurfaceBuffer* buffer = m_buffers[Buffer_Color];
            if (this == ICGraphics::GetInstance()->GetTargetSurface() && (buffer->m_flags & CRenderSurfaceBuffer::Flag_OwnsData)) {
                // read the target back through a temporary targetable texture that uses these textures
                ICGraphicsAbstractionManager::SetAbstractionLayer(*ICGraphicsAbstractionManager::GetInstance(),
                                                                  ICGraphicsAbstract::Abstraction_Hardware);
                m_lockSurface = (CRenderSurface_OGLES_Targetable*)ICRenderSurface::CreateInstance(
                    ICRenderSurface::Type_Default, Texture_LockSurfaceTargetability);
                if (m_lockSurface != NULL) {
                    InitParam params[6];
                    memset(params, 0, sizeof(params));
                    params[0].m_key = InitParam_ColorFormat;
                    params[1].m_key = InitParam_Width;
                    params[2].m_key = InitParam_Height;
                    params[3].m_key = Texture_LockSurfaceParam4;
                    params[3].m_value = 1;
                    params[4].m_key = InitParam_SourceTexture;
                    unsigned int width;
                    unsigned int height;
                    GetWidthAndHeight(width, height);
                    params[0].m_value = GetColorFormat();
                    params[1].m_value = width;
                    params[2].m_value = height;
                    params[4].m_value = (unsigned int)this;
                    unsigned char initialized = m_lockSurface->Initialize(params);
                    m_lockSurface->Bind();
                    unsigned char* lockedData;
                    m_lockSurface->Lock(&lockedData, pitch, lockType, area);
                    Bind();
                    locked = lockedData;
                    if (locked != NULL) {
                        if (data != NULL) {
                            *data = locked;
                        }
                        m_flags |= Flag_LockMask;
                        return;
                    }
                    if (initialized) {
                        return;
                    }
                }
            }
        } else {
            // locked again: the pixels are in the lock surface
            if (m_lockSurface != NULL) {
                m_lockSurface->Lock(data, pitch, lockType, area);
            }
            return;
        }
    }
    if (data != NULL) {
        *data = NULL;
    }
    pitch = 0;
}

void CRenderSurface_OGLES_Texture::Unlock()
{
    if (m_sourceTexture != NULL) {
        m_sourceTexture->Unlock();
        if (m_sourceTexture->IsLocked()) {
            return;
        }
        // b117-va: each path clears the lock bits itself (original block layout)
        m_flags &= ~Flag_LockMask;
    } else {
        if ((m_flags & Flag_LockMask) == 0) {
            return;
        }
        CRenderSurface_OGLES_Targetable* lockSurface = m_lockSurface;
        if (lockSurface->m_lockCount == 1) {
            // last unlock: the lock surface draws the pixels into the textures
            m_contentsUndefined = 0;
            lockSurface->Bind();
            m_lockSurface->Unlock();
            Bind();
            if (m_lockSurface != NULL) {
                delete m_lockSurface;
            }
            m_lockSurface = NULL;
            m_flags &= ~Flag_LockMask;
        } else {
            lockSurface->Unlock();
            return;
        }
    }
}

components::ColorFormat CRenderSurface_OGLES_Texture::GetColorFormat()
{
    if (m_sourceTexture != NULL) {
        return m_sourceTexture->GetColorFormat();
    }
    return CRenderSurface_SW::GetColorFormat();
}

unsigned char CRenderSurface_OGLES_Texture::GetWidthAndHeight(unsigned int& width, unsigned int& height)
{
    if (m_sourceTexture != NULL) {
        return m_sourceTexture->GetWidthAndHeight(width, height);
    }
    return CRenderSurface_SW::GetWidthAndHeight(width, height);
}

unsigned char CRenderSurface_OGLES_Texture::GetDescription(Description& description)
{
    unsigned char typeKnown = 0;
    if (m_flags & Flag_Software) {
        description.m_type = 2;
        typeKnown = 1;
    } else if (m_flags & Flag_Hardware) {
        typeKnown = 1;
        description.m_type = 1;
    }

    unsigned char valid = 0;
    if (m_flags & Flag_Texture) {
        description.m_usage = 0;
        valid = typeKnown;
    } else if (m_flags & Flag_Targetable) {
        description.m_usage = 1;
        valid = typeKnown;
    }

    unsigned int flags = m_flags;
    if (flags & Flag_Format8) {
        if (flags & Flag_Format20) {
            description.m_colorKind = 1;
        } else if (flags & Flag_Format40) {
            description.m_colorKind = 2;
        }
    } else if (flags & Flag_Format10) {
        if (flags & Flag_Format20) {
            description.m_colorKind = 3;
        } else if (flags & Flag_Format40) {
            description.m_colorKind = 4;
        }
    } else {
        description.m_colorKind = 0;
    }

    if (valid) {
        unsigned int width;
        unsigned int height;
        valid = 0;
        if (GetWidthAndHeight(width, height)) {
            description.m_width = (unsigned short)width;
            description.m_height = (unsigned short)height;
            valid = typeKnown;
        }
    } else {
        description.m_width = 0xffff;
        description.m_height = 0xffff;
    }

    description.m_window = (m_flags & Flag_Window) != 0;
    flags = m_flags;
    if ((flags & (Flag_Lockable | Flag_Writable)) == (Flag_Lockable | Flag_Writable)) {
        description.m_lockAccess = 3;
    } else if (flags & Flag_Lockable) {
        description.m_lockAccess = 1;
    } else if (flags & Flag_Writable) {
        description.m_lockAccess = 2;
    } else {
        description.m_lockAccess = 0;
    }
    description.m_flag15 = (m_flags & Flag_400) != 0;
    description.m_locked = IsLocked();
    description.m_mipmaps = (m_flags & Flag_Mipmaps) != 0;
    description.m_hasColorFormat = (m_flags & Flag_HasColorFormat) != 0;
    description.m_colorFormat = GetColorFormat();

    description.m_depthFormat = (components::ColorFormat)0;
    if (m_flags & Flag_HasDepthBuffer) {
        if (m_buffers[Buffer_Depth] != NULL) {
            description.m_depthFormat = (components::ColorFormat)m_buffers[Buffer_Depth]->m_format;
        }
        description.m_hasDepthBuffer = 1;
    } else {
        description.m_hasDepthBuffer = 0;
    }

    unsigned char hasStencilBuffer = 0;
    description.m_stencilFormat = (components::ColorFormat)0;
    if (m_flags & Flag_HasStencilBuffer) {
        if (m_buffers[Buffer_Stencil] != NULL) {
            description.m_stencilFormat = (components::ColorFormat)m_buffers[Buffer_Stencil]->m_format;
        }
        hasStencilBuffer = 1;
    }
    description.m_hasStencilBuffer = hasStencilBuffer;

    description.m_palette = m_palette;
    if (m_flags & Flag_Reference) {
        description.m_reference = m_referenceSurface;
    } else {
        description.m_reference = NULL;
    }

    switch (m_flags & Flag_MultisampleMask) {
    case Flag_Multisample1:
        description.m_multisample = 1;
        break;
    case Flag_Multisample2:
        description.m_multisample = 2;
        break;
    case Flag_Multisample3:
        description.m_multisample = 3;
        break;
    default:
        description.m_multisample = 0;
        break;
    }
    description.m_magFilter = m_magFilter;
    description.m_minFilter = m_minFilter;
    description.m_wrapS = m_wrapS;
    description.m_wrapT = m_wrapT;
    description.m_ownsPalette = (m_flags & Flag_OwnsPalette) != 0;
    return valid;
}

unsigned char CRenderSurface_OGLES_Texture::SaveStateForFutureRecreation()
{
    DestroyTextureObjects();
    return 0;
}

unsigned char CRenderSurface_OGLES_Texture::Recreate()
{
    InitializeTextureObjects();
    return 0;
}

unsigned char CRenderSurface_OGLES_Texture::InitializeBuffers(const InitParam* params)
{
    RSBFormat formats[BufferCount];
    formats[Buffer_Color] = RSBFormat_None;
    formats[Buffer_Depth] = RSBFormat_None;
    formats[Buffer_Stencil] = RSBFormat_None;
    Destroy();

    unsigned int colorKey = components::Color_X8R8G8B8_t::Magenta.value;
    ICRenderSurface* sourceTexture = NULL;
    unsigned int width = 0;
    unsigned int height = 0;
    void* palette = NULL;
    void* reference = NULL;
    unsigned int referenceType = 0;
    components::CInputStream* sourceStream = NULL;
    unsigned int sourceType = 0;
    unsigned char highColor = 0;
    unsigned char hasColorKey = 0;
    unsigned char flipYOff = 0;
    unsigned char transpose = 0;
    unsigned char flipX = 0;
    unsigned char failed = 0;
    for (const InitParam* param = params; param->m_key != InitParam_End && !failed; param++) {
        switch (param->m_key) {
        case InitParam_Width:
            width = param->m_value;
            break;
        case InitParam_Height:
            height = param->m_value;
            break;
        case InitParam_Window:
            // a texture cannot be the window
            failed = (unsigned char)param->m_value != 0;
            break;
        case InitParam_Mipmaps:
            if ((unsigned char)param->m_value) {
                m_flags |= Flag_Mipmaps;
            }
            break;
        case InitParam_ColorFormat:
            m_flags |= Flag_HasColorFormat;
            formats[Buffer_Color] = (RSBFormat)param->m_value;
            break;
        case InitParam_DepthFormat:
            m_flags |= Flag_HasDepthBuffer;
            formats[Buffer_Depth] = (RSBFormat)param->m_value;
            break;
        case InitParam_StencilFormat:
            m_flags |= Flag_HasStencilBuffer;
            formats[Buffer_Stencil] = (RSBFormat)param->m_value;
            break;
        case InitParam_ColorKey:
            if (reference != NULL || referenceType != 0) {
                failed = 1;
            } else {
                colorKey = param->m_value;
                hasColorKey = 1;
            }
            break;
        case InitParam_Palette:
            palette = (void*)param->m_value;
            break;
        case InitParam_Reference:
            reference = (void*)param->m_value;
            break;
        case InitParam_ReferenceType:
            if (referenceType != 0 || sourceStream != NULL || sourceType != 0) {
                failed = 1;
            } else {
                referenceType = param->m_value;
            }
            break;
        case InitParam_SourceStream:
            sourceStream = (components::CInputStream*)param->m_value;
            break;
        case InitParam_SourceType:
            if (sourceType != 0 || reference != NULL || referenceType != 0) {
                failed = 1;
            } else {
                sourceType = param->m_value;
            }
            break;
        case InitParam_HighColor:
            highColor = (unsigned char)param->m_value;
            break;
        case InitParam_Orientation:
            switch (param->m_value) {
            case 0:
                flipYOff = 0;
                transpose = 0;
                flipX = 1;
                break;
            case 1:
                flipYOff = 1;
                transpose = 0;
                flipX = 0;
                break;
            case 2:
                flipYOff = 1;
                transpose = 0;
                flipX = 1;
                break;
            case 3:
                flipYOff = 0;
                transpose = 1;
                flipX = 0;
                break;
            case 4:
                flipYOff = 0;
                transpose = 1;
                flipX = 1;
                break;
            case 5:
                flipYOff = 1;
                transpose = 1;
                flipX = 0;
                break;
            case 6:
                flipYOff = 1;
                transpose = 1;
                flipX = 1;
                break;
            default:
                failed = 1;
                break;
            }
            break;
        case InitParam_MagFilter:
            if (param->m_value != Filter_Linear && param->m_value != Filter_Nearest) {
                failed = 1;
            } else {
                m_magFilter = param->m_value;
            }
            break;
        case InitParam_MinFilter:
            m_minFilter = param->m_value;
            break;
        case InitParam_WrapS:
            m_wrapS = param->m_value;
            break;
        case InitParam_WrapT:
            m_wrapT = param->m_value;
            break;
        case InitParam_SourceTexture:
            sourceTexture = (ICRenderSurface*)param->m_value;
            break;
        }
    }
    if (sourceTexture != NULL && reference != NULL) {
        failed = 1;
    }

    if (!failed) {
        CGraphics_OGLES* graphics = (CGraphics_OGLES*)ICGraphics::GetInstance();
        if (!graphics->m_npotTextures) {
            // limited npot: clamped, unfiltered by mipmaps and without mipmaps only
            if (!graphics->m_npotRenderTargets || m_wrapS != Wrap_ClampToEdge || m_wrapT != Wrap_ClampToEdge ||
                (m_minFilter != Filter_Linear && m_minFilter != Filter_Nearest) || (m_flags & Flag_Mipmaps)) {
                m_flags |= Flag_PowerOfTwo;
            }
        }

        if (sourceTexture != NULL) {
            // use the textures of another surface of this device
            if (sourceTexture->GetAbstraction() == ICGraphicsAbstract::Abstraction_Hardware) {
                m_sourceTexture = (CRenderSurface_OGLES_Texture*)sourceTexture;
                return 1;
            }
        } else if (reference != NULL) {
            if (CreateAndInitializeReferenceAndSecondaryBuffers(formats, reference, referenceType, palette, 1)) {
                return 1;
            }
        } else {
            CRenderSurfaceBuffer::SourceStreamDesc desc;
            desc.m_stream = NULL;
            desc.m_type = 0;
            desc.m_colorKeyEnabled = 0;
            desc.m_colorKey = 0;
            desc.m_flag04 = 0;
            for (int index = 0; index < 10; index++) {
                desc.m_formatMap[0][index].m_from = RSBFormat_None;
                desc.m_formatMap[0][index].m_to = RSBFormat_None;
                desc.m_formatMap[1][index].m_from = RSBFormat_None;
                desc.m_formatMap[1][index].m_to = RSBFormat_None;
            }

            unsigned char unsupported = 0;
            if (sourceStream != NULL) {
                desc.m_stream = sourceStream;
                desc.m_type = sourceType;
                desc.m_colorKeyEnabled = hasColorKey;
                desc.m_colorKey = colorKey;
                desc.m_transpose = transpose;
                desc.m_flipX = flipX;
                desc.m_flipY = !flipYOff;
                if (formats[Buffer_Color] == RSBFormat_None) {
                    // the decoder picks the format: map every image format to one the device has
                    RSBFormat defaultFormat = (RSBFormat)ICGraphics::GetInstance()->GetDefaultColorFormat();
                    if (highColor) {
                        if (defaultFormat == RSBFormat_A8B8G8R8) {
                            CRenderSurface_OGLES_Texture_SetFormatMap(desc, RSBFormat_B8G8R8, RSBFormat_A8B8G8R8,
                                                                      RSBFormat_A8B8G8R8, RSBFormat_A8B8G8R8);
                            desc.m_formatMap[1][8].m_to = RSBFormat_A8B8G8R8;
                        } else if (defaultFormat == RSBFormat_R5G6B5) {
                            CRenderSurface_OGLES_Texture_SetFormatMap(desc, RSBFormat_R5G6B5, RSBFormat_A8B8G8R8,
                                                                      RSBFormat_A8B8G8R8, RSBFormat_A8B8G8R8);
                        } else {
                            unsupported = 1;
                        }
                    } else {
                        if (defaultFormat == RSBFormat_A8B8G8R8) {
                            CRenderSurface_OGLES_Texture_SetFormatMap(desc, RSBFormat_R5G6B5, RSBFormat_R4G4B4A4,
                                                                      RSBFormat_R5G5B5A1, RSBFormat_R4G4B4A4);
                        } else if (defaultFormat == RSBFormat_R5G6B5) {
                            CRenderSurface_OGLES_Texture_SetFormatMap(desc, RSBFormat_R5G6B5, RSBFormat_R4G4B4A4,
                                                                      RSBFormat_R5G5B5A1, RSBFormat_R4G4B4A4);
                        } else {
                            unsupported = 1;
                        }
                    }
                }
            }
            if (sourceStream == NULL && formats[Buffer_Color] == RSBFormat_None) {
                formats[Buffer_Color] = (RSBFormat)ICGraphics::GetInstance()->GetDefaultColorFormat();
            }
            if (!unsupported) {
                CRenderSurfaceBuffer::SourceStreamDesc* descToUse = &desc;
                if (sourceStream == NULL) {
                    descToUse = NULL;
                }
                if (CreateBuffers(formats, (unsigned short)width, (unsigned short)height, palette, descToUse,
                                  CRenderSurfaceBuffer::PitchCondition_Positive, 0, NULL, 1)) {
                    return 1;
                }
            }
        }
    }
    Destroy();
    return 0;
}

CRenderSurface_OGLES_Texture* CRenderSurface_OGLES_Texture::GetSourceTexture()
{
    CRenderSurface_OGLES_Texture* source = this;
    while (source->m_sourceTexture != NULL) {
        source = source->m_sourceTexture;
    }
    return source;
}

static int CRenderSurface_OGLES_Texture_AdjustToPowOfTwoDimension(int dimension)
{
    if (dimension == 0) {
        return 0;
    }
    if (dimension == 1) {
        return 1;
    }
    if (dimension <= 0x2) {
        return 0x2;
    }
    if (dimension <= 0x4) {
        return 0x4;
    }
    if (dimension <= 0x8) {
        return 0x8;
    }
    if (dimension <= 0x10) {
        return 0x10;
    }
    if (dimension <= 0x20) {
        return 0x20;
    }
    if (dimension <= 0x40) {
        return 0x40;
    }
    if (dimension <= 0x80) {
        return 0x80;
    }
    if (dimension <= 0x100) {
        return 0x100;
    }
    if (dimension <= 0x200) {
        return 0x200;
    }
    if (dimension <= 0x400) {
        return 0x400;
    }
    if (dimension <= 0x800) {
        return 0x800;
    }
    if (dimension <= 0x1000) {
        return 0x1000;
    }
    return 0;
}

} } } }
