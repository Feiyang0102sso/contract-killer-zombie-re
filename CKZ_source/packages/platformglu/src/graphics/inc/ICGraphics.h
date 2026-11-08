// ICGraphics.h - rendering device interface and the base of every device resource.
// (first drafted by b000x as PROVISIONAL; completed by b037x, the owner)
// Virtual slot order follows the vtable of CGraphics_OGLES (ICGraphics slots +0x08 .. +0x94).
#ifndef ICGRAPHICS_H
#define ICGRAPHICS_H

#include "CClass.h"
#include "CColor.h"
#include "ICBuffer.h"
#include "ICDisplayProgram.h"
#include "ICGraphicsAbstractionManager.h"
#include "ICGraphicsResource.h"

namespace com { namespace glu { namespace platform {

namespace components {
class CStrWChar;
struct Color_RGBA;
}

namespace graphics {

class ICRasterizerState;
class ICRenderSurface;
class ICShaderProgram;

class ICGraphics : public core::CClass {
public:
    /** Runtime class id of the graphics devices. */
    enum { ClassId = 0x0466e194 };

    /** One (key, value) initialization setting; a list ends with key 0. */
    struct InitParam {
        unsigned int m_key;     // 0x00
        unsigned int m_value;   // 0x04
    };

    /** Primitive kinds of Draw(). */
    enum Primitive {
        Primitive_Lines = 0,
        Primitive_LineStrip = 1,
        Primitive_Triangles = 2,
        Primitive_TriangleStrip = 3
    };

    /** Texture filtering hint per sampler. */
    enum TextureSampleHint {
        TextureSampleHint_Default = 0
    };

    /** ClearBuffers() flags (bits of the "Flags" clear table entry). */
    enum ClearFlag {
        ClearFlag_Color = 0x010000,
        ClearFlag_Depth = 0x020000,
        ClearFlag_Stencil = 0x040000
    };

    /** Capabilities of the device (filled in by the platform layer). */
    struct Description {
        unsigned int m_abstraction;                 // 0x00
        const components::CStrWChar* m_renderer;    // 0x04 device name
        unsigned int m_maxTextureSize;              // 0x08
        unsigned int m_maxTextureWidth;             // 0x0c
        unsigned int m_maxTextureHeight;            // 0x10
        unsigned int m_maxRenderTargetSize;         // 0x14
        unsigned int m_maxRenderTargetWidth;        // 0x18
        unsigned int m_maxRenderTargetHeight;       // 0x1c
        unsigned int m_maxViewportWidth;            // 0x20
        unsigned int m_maxViewportHeight;           // 0x24
        unsigned int m_maxTextureUnits;             // 0x28
        unsigned int m_maxVertexAttributes;         // 0x2c
        unsigned int m_maxShaderParameters;         // 0x30
    };

    /** The graphics device of the application's abstraction manager. */
    static ICGraphics* GetInstance();

    ICGraphics() : core::CClass(ClassId) {}

    virtual ~ICGraphics() {}

    virtual ICGraphicsAbstract::Abstraction GetAbstraction() const = 0;
    virtual unsigned char Initialize(const InitParam* params) = 0;
    virtual void Destroy() = 0;
    /** Start recording into the device's own display program; returns it (NULL without one). */
    virtual ICDisplayProgram* Begin(ICDisplayProgram::Mode& mode, unsigned char keepState, unsigned char runOnEnd) = 0;
    virtual void Begin(ICDisplayProgram& program, ICDisplayProgram::Mode& mode, unsigned char keepState,
                       unsigned char runOnEnd) = 0;
    virtual void End(ICDisplayProgram& program) = 0;
    virtual void Run(ICDisplayProgram& program) = 0;
    virtual ICRasterizerState* GetRasterizerState() const = 0;
    virtual void SetTargetSurface(ICRenderSurface& surface) = 0;
    virtual ICRenderSurface* GetTargetSurface() const = 0;
    /** Clear the buffers selected by `flags` (ClearFlag bits). */
    virtual void ClearBuffers(unsigned int flags) = 0;
    virtual void SwapBuffers() = 0;
    virtual void SetDefaultColorFormat(components::ColorFormat format) = 0;
    virtual components::ColorFormat GetDefaultColorFormat() const = 0;
    virtual void EnableClearScissor(unsigned char enable) = 0;
    virtual void SetClearScissor(short x, short y, short width, short height) = 0;
    virtual void SetClearColor(float red, float green, float blue, float alpha) = 0;
    virtual void SetClearColor(const components::Color_RGBA& color) = 0;
    virtual void SetClearColorMask(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha) = 0;
    virtual void SetClearDepth(float depth) = 0;
    virtual void SetClearStencil(int stencil) = 0;
    virtual void SetViewport(short x, short y, short width, short height) = 0;
    virtual void SetShaderProgram(ICShaderProgram& program) = 0;
    virtual ICShaderProgram* GetShaderProgram() = 0;
    virtual void SetTextureSamplerHint(unsigned int unit, TextureSampleHint hint) = 0;
    virtual void SetTexture2d(unsigned int unit, ICRenderSurface& surface) = 0;
    virtual void SetVertexStream(unsigned int index, ICBuffer::Stream& stream) = 0;
    virtual void EnableVertexStream(unsigned int index, unsigned char enable) = 0;
    virtual void Draw(Primitive primitive) = 0;
    virtual void Draw(Primitive primitive, unsigned int first, unsigned int count) = 0;
    virtual void Draw(Primitive primitive, ICBuffer::Stream& indices) = 0;
    virtual void Draw(Primitive primitive, ICBuffer::Stream& indices, unsigned int first, unsigned int count) = 0;
    virtual const Description& GetDescription() const = 0;
    virtual const components::CStrWChar& GetDeviceVersion() const = 0;
    virtual void GetDeviceVersion(unsigned int& major, unsigned int& minor) const = 0;
    /** Push the whole cached device state to the hardware again. */
    virtual void Sync() = 0;
};

} } } }

#endif
