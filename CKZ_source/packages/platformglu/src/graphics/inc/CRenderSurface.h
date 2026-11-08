// CRenderSurface.h - platform independent part of a render surface: up to three pixel buffers
// (color, depth, stencil) or a reference to another surface whose buffers are used.
// (created by b039x, the owner)
#ifndef CRENDERSURFACE_H
#define CRENDERSURFACE_H

#include "ICRenderSurface.h"
#include "CRenderSurfaceBuffer.h"
#include "CGraphics.h"
#include "ICRasterizerState.h"

namespace com { namespace glu { namespace platform { namespace graphics {

class CRenderSurface : public ICRenderSurface {
public:
    /** m_buffers index. */
    enum Buffer {
        Buffer_Color = 0,
        Buffer_Depth = 1,
        Buffer_Stencil = 2,
        BufferCount = 3
    };

    CRenderSurface();
    virtual ~CRenderSurface();

    virtual unsigned char Load(components::CInputStream& stream, unsigned int type,
                               components::CIdToObjectRouter* router);
    virtual void Destroy();
    virtual void DiscardContents();
    virtual unsigned char IsLocked();
    virtual void Bind();
    virtual unsigned char SaveStateForFutureRecreation();
    virtual unsigned char Recreate();

    /** Delete the buffers the surface owns. */
    void DestroyBuffers();
    /** Create the color buffer (and the depth / stencil buffers of `formats`). */
    unsigned char CreateBuffers(const RSBFormat* formats, unsigned short width, unsigned short height, void* palette,
                                CRenderSurfaceBuffer::SourceStreamDesc* desc,
                                CRenderSurfaceBuffer::PitchCondition pitchCondition, short pitch,
                                unsigned char* pixels, unsigned char powerOfTwoSecondary);
    /** Create the depth (formats[1]) and stencil (formats[2]) buffers. */
    void CreateAndInitializeSecondaryBuffers(const RSBFormat* formats, unsigned short width, unsigned short height,
                                             unsigned char powerOfTwo);
    /** Use the buffers of `reference` (a surface when `resourceType` says so) and create the secondary ones. */
    unsigned char CreateAndInitializeReferenceAndSecondaryBuffers(const RSBFormat* formats, void* reference,
                                                                  unsigned int resourceType, void* palette,
                                                                  unsigned char powerOfTwo);

    /** Make this surface the target of the device and reset viewport and scissor to its size. */
    void SetAsTargetSurface()
    {
        CGraphics* graphics = (CGraphics*)ICGraphics::GetInstance();
        if (graphics->m_targetSurface != this) {
            graphics->m_targetSurface = this;
            unsigned int width;
            unsigned int height;
            GetWidthAndHeight(width, height);
            graphics->SetViewport(0, 0, (short)width, (short)height);
            graphics->GetRasterizerState()->SetScissor(0, 0, (short)width, (short)height);
        }
    }

    void* m_palette;                        // 0x0c Flag_OwnsPalette: allocated by the surface
    union {
        CRenderSurfaceBuffer* m_buffers[BufferCount];   // 0x10
        ICRenderSurface* m_referenceSurface;            // 0x10 Flag_Reference
    };
};

} } } }

#endif
