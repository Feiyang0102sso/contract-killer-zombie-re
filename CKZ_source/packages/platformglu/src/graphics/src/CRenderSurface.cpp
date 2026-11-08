// CRenderSurface.cpp - platform independent part of a render surface.
#include "CRenderSurface.h"
#include "CInputStream.h"
#include "CIdToObjectRouter.h"
#include "CSharedPtr.h"

namespace com { namespace glu { namespace platform { namespace graphics {

// image mime type keys a surface can be loaded from (the keys CResourceFactory maps to CResourceRenderSurface)
static const unsigned int SurfaceType_9f4bc472 = 0x9f4bc472;
static const unsigned int SurfaceType_e179efbe = 0xe179efbe;
static const unsigned int SurfaceType_e179f4be = 0xe179f4be;
static const unsigned int SurfaceType_fc31810d = 0xfc31810d;
static const unsigned int SurfaceType_593f8140 = 0x593f8140;
static const unsigned int SurfaceType_59efbc42 = 0x59efbc42;
static const unsigned int SurfaceType_59f4bc42 = 0x59f4bc42;
static const unsigned int SurfaceType_59f4bc59 = 0x59f4bc59;

// stream keys of Load that name their object instead of giving its key
static const int LoadKey_PaletteByName = 100;       // -> InitParam_Palette
static const int LoadKey_ReferenceByName = 0x65;    // -> InitParam_Reference
static const int LoadKey_SourceByName = 0x66;       // -> InitParam_SourceStream
// appended by Load when a color format of 0 was given
static const unsigned int InitParam_ZeroColorFormat = 10;
// most settings Load passes on
static const int LoadMaxParams = 20;

CRenderSurface::CRenderSurface()
{
    m_flags = 0;
    m_palette = NULL;
    memset(m_buffers, 0, sizeof(m_buffers));
}

CRenderSurface::~CRenderSurface()
{
    Destroy();
}

void CRenderSurface::Destroy()
{
    if (m_palette != NULL) {
        if (m_flags & Flag_OwnsPalette) {
            free(m_palette);
        }
        m_palette = NULL;
    }
    DestroyBuffers();
    m_flags &= Flag_Texture | Flag_Targetable | Flag_Format8 | Flag_Format10 | Flag_Format20 | Flag_Format40 |
               Flag_Hardware | Flag_Software | Flag_Lockable | Flag_Writable;
}

void CRenderSurface::DestroyBuffers()
{
    for (int index = 0; index < BufferCount; index++) {
        if (index == Buffer_Color && (m_flags & Flag_Reference)) {
            // the buffers of the referenced surface are not ours
            m_referenceSurface = NULL;
            continue;
        }
        if (m_buffers[index] != NULL) {
            delete m_buffers[index];
            m_buffers[index] = NULL;
        }
    }
}

void CRenderSurface::Bind()
{
    SetAsTargetSurface();
}

void CRenderSurface::DiscardContents()
{
    m_flags |= Flag_ContentsDiscarded;
}

unsigned char CRenderSurface::IsLocked()
{
    return (m_flags & Flag_LockMask) != 0;
}

unsigned char CRenderSurface::Load(components::CInputStream& stream, unsigned int type,
                                   components::CIdToObjectRouter* router)
{
    switch (type) {
    case SurfaceType_9f4bc472:
    case SurfaceType_e179efbe:
    case SurfaceType_e179f4be:
    case SurfaceType_fc31810d:
    case SurfaceType_593f8140:
    case SurfaceType_59efbc42:
    case SurfaceType_59f4bc42:
    case SurfaceType_59f4bc59:
        break;
    default:
        return 0;
    }

    unsigned int sourceType = 0;
    unsigned int referenceType = 0;
    components::CInputStream sourceStream;
    core::CSharedPtr<void*> palette;
    core::CSharedPtr<void*> reference;
    core::CSharedPtr<void*> namedPalette;
    core::CSharedPtr<void*> namedReference;
    char name[256];
    InitParam params[LoadMaxParams];
    memset(params, 0, sizeof(params));

    unsigned char result = 1;
    int count = 0;
    if (stream.Available() != 0) {
        unsigned char zeroColorFormat = 0;
        int index = 0;
        do {
            count = index + 1;
            int key = stream.ReadUInt32();
            if (key >= LoadKey_PaletteByName) {
                if (key == LoadKey_PaletteByName) {
                    char* character = name;
                    unsigned char value;
                    do {
                        value = stream.ReadUInt8();
                        *character = value;
                        character++;
                    } while (value != 0);
                    unsigned int paletteType;
                    result = router->NameToPointer(name, namedPalette, paletteType);
                    if (result) {
                        params[index].m_key = InitParam_Palette;
                        params[index].m_value = (unsigned int)((components::Color_Palette*)*namedPalette)->m_colors;
                    }
                } else if (key == LoadKey_ReferenceByName) {
                    char* character = name;
                    unsigned char value;
                    do {
                        value = stream.ReadUInt8();
                        *character = value;
                        character++;
                    } while (value != 0);
                    result = router->NameToPointer(name, namedReference, referenceType);
                    if (result) {
                        params[index].m_key = InitParam_Reference;
                        params[index].m_value = (unsigned int)*namedReference;
                    }
                } else if (key == LoadKey_SourceByName) {
                    char* character = name;
                    unsigned char value;
                    do {
                        value = stream.ReadUInt8();
                        *character = value;
                        character++;
                    } while (value != 0);
                    result = router->NameToInputStream(name, sourceStream, sourceType);
                    if (result) {
                        params[index].m_key = InitParam_SourceStream;
                        params[index].m_value = (unsigned int)&sourceStream;
                    }
                } else {
                    // unknown keys end the list
                    params[index].m_key = InitParam_End;
                    params[index].m_value = stream.ReadUInt32();
                }
            } else {
                switch (key) {
                case InitParam_ColorFormat: {
                    params[index].m_key = key;
                    unsigned int format = stream.ReadUInt32();
                    if (format == 0) {
                        params[index].m_value = 0;
                        zeroColorFormat = 1;
                    } else {
                        if (format == 0xffffffff) {
                            format = 0;
                        }
                        params[index].m_value = format;
                    }
                    break;
                }
                default:
                    // unknown keys end the list
                    key = InitParam_End;
                case 0:
                case InitParam_Width:
                case InitParam_Height:
                case InitParam_Window:
                case 4:
                case InitParam_Mipmaps:
                case InitParam_DepthFormat:
                case InitParam_StencilFormat:
                case InitParam_ColorKey:
                case InitParam_HighColor:
                case InitParam_Orientation:
                case InitParam_Multisample:
                case InitParam_MagFilter:
                case InitParam_MinFilter:
                case InitParam_WrapS:
                case InitParam_WrapT:
                    params[index].m_key = key;
                    params[index].m_value = stream.ReadUInt32();
                    break;
                case InitParam_Palette: {
                    unsigned int paletteType;
                    result = router->KeyToPointer(stream.ReadUInt32(), palette, paletteType);
                    if (result) {
                        params[index].m_key = key;
                        params[index].m_value = (unsigned int)((components::Color_Palette*)*palette)->m_colors;
                    }
                    break;
                }
                case InitParam_Reference:
                    result = router->KeyToPointer(stream.ReadUInt32(), reference, referenceType);
                    if (result) {
                        params[index].m_key = key;
                        params[index].m_value = (unsigned int)*reference;
                    }
                    break;
                case InitParam_SourceStream:
                    result = router->KeyToInputStream(stream.ReadUInt32(), sourceStream, sourceType);
                    if (result) {
                        params[index].m_key = key;
                        params[index].m_value = (unsigned int)&sourceStream;
                    }
                    break;
                }
            }
            index = count;
        } while (stream.Available() != 0);

        if (zeroColorFormat) {
            params[count].m_key = InitParam_ZeroColorFormat;
            params[count].m_value = zeroColorFormat;
            count++;
        }
    }

    unsigned char initialized = 0;
    if (sourceType != 0) {
        // a stream and a reference cannot be combined
        if (referenceType != 0) {
            return initialized;
        }
        params[count].m_key = InitParam_SourceType;
        params[count].m_value = sourceType;
    } else if (referenceType != 0) {
        params[count].m_key = InitParam_ReferenceType;
        params[count].m_value = referenceType;
    }
    if (result) {
        initialized = Initialize(params);
    }
    return initialized;
}

unsigned char CRenderSurface::SaveStateForFutureRecreation()
{
    return 1;
}

unsigned char CRenderSurface::Recreate()
{
    return 1;
}

unsigned char CRenderSurface::CreateBuffers(const RSBFormat* formats, unsigned short width, unsigned short height,
                                            void* palette, CRenderSurfaceBuffer::SourceStreamDesc* desc,
                                            CRenderSurfaceBuffer::PitchCondition pitchCondition, short pitch,
                                            unsigned char* pixels, unsigned char noMemorySecondary)
{
    unsigned char result = 0;
    if ((pitch == 0 && pixels == NULL) || desc == NULL) {
        void** paletteOut;
        if (palette != NULL) {
            m_palette = palette;
            paletteOut = &palette;
            m_flags &= ~Flag_OwnsPalette;
        } else {
            // a decoder may allocate the palette
            paletteOut = &m_palette;
            m_flags |= Flag_OwnsPalette;
        }
        CRenderSurfaceBuffer* buffer;
        if (m_flags & Flag_Mipmaps) {
            buffer = new CRenderSurfaceBufferMipmap();
        } else {
            buffer = new CRenderSurfaceBuffer();
        }
        m_buffers[Buffer_Color] = buffer;
        result = buffer->Initialize(formats[0], width, height, (m_flags & Flag_PowerOfTwo) != 0, paletteOut, desc,
                                    pitchCondition, pitch, pixels);
        if (result) {
            CreateAndInitializeSecondaryBuffers(formats, width, height, noMemorySecondary);
        }
    }
    return result;
}

void CRenderSurface::CreateAndInitializeSecondaryBuffers(const RSBFormat* formats, unsigned short width,
                                                         unsigned short height, unsigned char noMemory)
{
    // without memory the buffers only describe a render target (pixels pointer 1, pitch 1)
    if (formats[Buffer_Depth] != RSBFormat_None) {
        CRenderSurfaceBuffer* buffer = new CRenderSurfaceBuffer();
        m_buffers[Buffer_Depth] = buffer;
        if (noMemory) {
            buffer->Initialize(formats[Buffer_Depth], width, height, 0, NULL, NULL,
                               CRenderSurfaceBuffer::PitchCondition_Given, 1, (void*)1);
        } else {
            buffer->Initialize(formats[Buffer_Depth], width, height, 0, NULL, NULL,
                               CRenderSurfaceBuffer::PitchCondition_Positive, 0, NULL);
        }
    }
    if (formats[Buffer_Stencil] != RSBFormat_None) {
        CRenderSurfaceBuffer* buffer = new CRenderSurfaceBuffer();
        m_buffers[Buffer_Stencil] = buffer;
        if (noMemory) {
            buffer->Initialize(formats[Buffer_Stencil], width, height, 0, NULL, NULL,
                               CRenderSurfaceBuffer::PitchCondition_Given, 1, (void*)1);
        } else {
            buffer->Initialize(formats[Buffer_Stencil], width, height, 0, NULL, NULL,
                               CRenderSurfaceBuffer::PitchCondition_Positive, 0, NULL);
        }
    }
}

unsigned char CRenderSurface::CreateAndInitializeReferenceAndSecondaryBuffers(const RSBFormat* formats,
                                                                              void* reference,
                                                                              unsigned int resourceType,
                                                                              void* palette,
                                                                              unsigned char noMemory)
{
    Destroy();
    unsigned char result = 0;
    if (resourceType == ICRenderSurface::ResourceType && reference != NULL) {
        ICRenderSurface* surface = (ICRenderSurface*)reference;
        Description description;
        if (surface->GetDescription(description)) {
            // the referenced surface must provide what this one was asked for
            unsigned char compatible = 1;
            if (formats[0] != RSBFormat_None && formats[0] != (RSBFormat)description.m_colorFormat) {
                compatible = 0;
            }
            if (palette != NULL && (description.m_colorFormat & 0x8000) == 0) {
                compatible = 0;
            }
            unsigned int flags = m_flags;
            if ((flags & Flag_400) && !description.m_flag15) {
                compatible = 0;
            }
            if ((flags & Flag_Lockable) && (description.m_lockAccess | 2) != 3) {
                compatible = 0;
            }
            if ((flags & Flag_Writable) && description.m_lockAccess - 2 > 1) {
                compatible = 0;
            }
            if ((flags & Flag_Mipmaps) && !description.m_mipmaps) {
                compatible = 0;
            }
            if ((flags & Flag_HasColorFormat) && !description.m_hasColorFormat) {
                compatible = 0;
            }
            if (compatible) {
                m_flags = flags | Flag_Reference;
                m_palette = palette;
                m_referenceSurface = surface;
                CreateAndInitializeSecondaryBuffers(formats, description.m_width, description.m_height, noMemory);
                result = compatible;
            }
        }
    }
    return result;
}

} } } }
