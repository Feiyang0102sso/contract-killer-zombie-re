// CRenderSurface_SW.cpp - render surface whose pixels live in system memory.
#include "CRenderSurface_SW.h"
#include "CRectangle.h"

namespace com { namespace glu { namespace platform { namespace graphics {

CRenderSurface_SW::CRenderSurface_SW()
{
    m_flags |= Flag_Software | Flag_Lockable;
}

CRenderSurface_SW::~CRenderSurface_SW()
{
}

void CRenderSurface_SW::Lock(unsigned char** data, int& pitch, LockType lockType,
                             const components::CRectangle<short>* area)
{
    if (m_flags & Flag_Reference) {
        if (m_referenceSurface != NULL) {
            m_referenceSurface->Lock(data, pitch, lockType, area);
            return;
        }
    } else {
        if (m_buffers[Buffer_Color] != NULL) {
            pitch = m_buffers[Buffer_Color]->m_pitch;
            m_flags |= Flag_LockMask;
            if (data != NULL) {
                *data = (unsigned char*)m_buffers[Buffer_Color]->GetLevel(0);
            }
            if (area == NULL) {
                return;
            }
            if (area->m_x >= 0 && area->m_x + area->m_width >= 0 && area->m_y >= 0 && area->m_y + area->m_height >= 0) {
                unsigned int width;
                unsigned int height;
                if (GetWidthAndHeight(width, height) && area->m_width + area->m_x <= (int)width &&
                    area->m_height + area->m_y <= (int)height) {
                    if (data != NULL) {
                        *data = (m_buffers[Buffer_Color]->m_format & RSBFormat_BytesPerPixelMask) * area->m_x +
                                pitch * area->m_y + *data;
                    }
                    return;
                }
            }
            // the area is outside of the surface
            m_flags &= ~Flag_LockMask;
        }
    }
    if (data != NULL) {
        *data = NULL;
    }
    pitch = 0;
}

void CRenderSurface_SW::Unlock()
{
    if ((m_flags & Flag_Reference) != 0 && m_referenceSurface != NULL) {
        m_referenceSurface->Unlock();
        return;
    }
    m_flags &= ~Flag_LockMask;
}

components::ColorFormat CRenderSurface_SW::GetColorFormat()
{
    if (m_flags & Flag_Reference) {
        if (m_referenceSurface == NULL) {
            return (components::ColorFormat)0;
        }
        return m_referenceSurface->GetColorFormat();
    }
    components::ColorFormat format = (components::ColorFormat)0;
    if (m_buffers[Buffer_Color] != NULL) {
        format = (components::ColorFormat)m_buffers[Buffer_Color]->m_format;
    }
    return format;
}

unsigned char CRenderSurface_SW::GetWidthAndHeight(unsigned int& width, unsigned int& height)
{
    if (m_flags & Flag_Reference) {
        unsigned char result = 0;
        if (m_referenceSurface != NULL) {
            result = m_referenceSurface->GetWidthAndHeight(width, height);
        }
        return result;
    }
    // the size of the first buffer there is
    int index = Buffer_Color;
    CRenderSurfaceBuffer* buffer = m_buffers[Buffer_Color];
    if (buffer == NULL) {
        index = Buffer_Depth;
        buffer = m_buffers[Buffer_Depth];
        if (buffer == NULL) {
            index = Buffer_Stencil;
            buffer = m_buffers[Buffer_Stencil];
            if (buffer == NULL) {
                return 0;
            }
        }
    }
    width = buffer->m_imageWidth;
    height = m_buffers[index]->m_imageHeight;
    return 1;
}

unsigned char CRenderSurface_SW::GetDescription(Description& description)
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
        unsigned char found = 0;
        if (m_buffers[Buffer_Depth] != NULL) {
            description.m_depthFormat = (components::ColorFormat)m_buffers[Buffer_Depth]->m_format;
            found = valid;
        }
        description.m_hasDepthBuffer = 1;
        valid = found;
    } else {
        description.m_hasDepthBuffer = 0;
    }

    description.m_stencilFormat = (components::ColorFormat)0;
    if (m_flags & Flag_HasStencilBuffer) {
        unsigned char found = 0;
        if (m_buffers[Buffer_Stencil] != NULL) {
            description.m_stencilFormat = (components::ColorFormat)m_buffers[Buffer_Stencil]->m_format;
            found = valid;
        }
        description.m_hasStencilBuffer = 1;
        valid = found;
    } else {
        description.m_hasStencilBuffer = 0;
    }

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
    description.m_magFilter = 3;
    description.m_minFilter = 3;
    description.m_wrapS = 0;
    description.m_wrapT = 0;
    description.m_ownsPalette = (m_flags & Flag_OwnsPalette) != 0;
    return valid;
}

unsigned char CRenderSurface_SW::FetchFromBuffer(unsigned int buffer, unsigned char** data, int& pitch,
                                                 const components::CRectangle<short>* area)
{
    int x = 0;
    int y = 0;
    if (area != NULL) {
        unsigned int width;
        unsigned int height;
        if (!(area->m_x >= 0 && area->m_x + area->m_width >= 0 && area->m_y >= 0 &&
              area->m_y + area->m_height >= 0 && GetWidthAndHeight(width, height) &&
              area->m_width + area->m_x <= (int)width && area->m_height + area->m_y <= (int)height)) {
            if (data != NULL) {
                *data = NULL;
            }
            pitch = 0;
            return 0;
        }
        x = area->m_x;
        y = area->m_y;
    }
    int rowPitch = m_buffers[buffer]->m_pitch;
    pitch = rowPitch;
    if (data != NULL) {
        CRenderSurfaceBuffer* source = m_buffers[buffer];
        RSBFormat format = source->m_format;
        *data = (unsigned char*)source->GetLevel(0) + (format & RSBFormat_BytesPerPixelMask) * x + rowPitch * y;
    }
    return 1;
}

} } } }
