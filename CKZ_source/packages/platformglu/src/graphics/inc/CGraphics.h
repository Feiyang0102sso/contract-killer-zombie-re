// CGraphics.h - device independent part of the graphics devices: resource registry, the clear
// table, the cached display program instructions and context loss handling.
// (first drafted by b002 as PROVISIONAL; completed by b037x, the owner)
//
// Key function note: SetDefaultColorFormat is the first non-inline virtual of the class and is
// defined `inline` below the class, so (as in the original) the vtable and the inline virtuals
// are emitted weak wherever this header is used.
#ifndef CGRAPHICS_H
#define CGRAPHICS_H

#include "ICGraphics.h"
#include "ICRenderSurface.h"
#include "CTypedVariableTable.h"
#include "CRegistry.h"
#include "CVector.h"
#include "CColor.h"
#include "CDisplayProgram.h"

namespace com { namespace glu { namespace platform {

namespace systems {
class CEvent;
class CEventListener;
}

namespace components {
template <class T> class CRectangle;
}

namespace graphics {

class ICShaderProgram;

class CGraphics : public ICGraphics {
public:
    /** Device event callback: (event, CEventListener*) -> handled. */
    typedef unsigned char (*EventHandler)(systems::CEvent& event, void* listener);

    /** Number of texture units the device state keeps. */
    enum { TextureUnitCount = 16 };

    /** Listeners of the debug messages of the device. */
    class DebugMessagingBridge {
    public:
        DebugMessagingBridge() : m_listeners(20, 20) {}
        ~DebugMessagingBridge();

        core::CVector<systems::CEventListener*> m_listeners;   // 0x00
    };

    /** A shader program the device created for its fixed function emulation. */
    struct IntrinsicShaderProgramInfo {
        unsigned int m_id;                  // 0x00 IntrinsicShaders id
        ICShaderProgram* m_program;         // 0x04
        unsigned char m_fromResources;      // 0x08 owned by the resource manager (released, not deleted)

        // user-declared: CVector copies the infos without zero-filling the slot first
        IntrinsicShaderProgramInfo() {}
    };

    /** Texture bound to a unit. */
    struct TextureUnit {
        ICRenderSurface* m_surface;         // 0x00
        unsigned int m_level;               // 0x04 mip level / texture index of the surface
        unsigned char m_dirty;              // 0x08 not yet sent to the device
    };

    /** "Target" instruction. */
    struct TargetInstruction {
        CDisplayProgram::Instruction::Header m_header;  // 0x00
        ICRenderSurface* m_surface;                     // 0x10
    };

    /** "Viewport" instruction. */
    struct ViewportInstruction {
        CDisplayProgram::Instruction::Header m_header;  // 0x00
        int m_x;                                        // 0x10
        int m_y;                                        // 0x14
        int m_width;                                    // 0x18
        int m_height;                                   // 0x1c
    };

    /** "Shaders" instruction. */
    struct ShadersInstruction {
        CDisplayProgram::Instruction::Header m_header;  // 0x00
        ICShaderProgram* m_program;                     // 0x10
    };

    /** "Textures" instruction: the units that changed. */
    struct TexturesInstruction {
        /** One changed unit. */
        struct Unit {
            unsigned int m_unit;            // 0x00
            ICRenderSurface* m_surface;     // 0x04
            unsigned int m_level;           // 0x08
        };

        CDisplayProgram::Instruction::Header m_header;  // 0x00 m_size grows with m_count
        unsigned int m_count;                           // 0x10
        Unit m_units[TextureUnitCount];                 // 0x14
    };

    /** Entries of the clear table (m_clearTable, also the "ClearBuffers" instruction). */
    enum ClearEntry {
        ClearEntry_Stamp = 0,           // time of the ClearBuffers() call
        ClearEntry_ScissorTest = 1,
        ClearEntry_Scissor = 2,         // x, y, width, height
        ClearEntry_Flags = 3,           // ClearFlag bits
        ClearEntry_Color = 4,           // r, g, b, a
        ClearEntry_ColorMask = 5,       // r<<24 | g<<16 | b<<8 | a
        ClearEntry_Depth = 6,
        ClearEntry_Stencil = 7
    };

    explicit CGraphics(EventHandler eventHandler);

    virtual void SetDefaultColorFormat(components::ColorFormat format);
    virtual components::ColorFormat GetDefaultColorFormat() const;

    virtual ~CGraphics();

    /** Create the window surface and make it the render target. */
    virtual unsigned char CreateAndInitializeWindowSurface(ICRenderSurface::Targetability targetability);
    /** Platform part of the window surface set-up; 1 on success. */
    virtual unsigned char InitializeWindowSurface(ICRenderSurface& surface) = 0;
    /** Context lost: save what is needed and release the device. */
    virtual unsigned char SaveStateAndReleaseDevice();
    /** Context back: rebuild the device and its resources. */
    virtual unsigned char Recreate();

    /** Register `resource` in the resource registry (0 if it already is). */
    unsigned char AddToResourceRegistry(ICGraphicsResource& resource);
    /** Unregister `resource`; texture units still using it are cleared. */
    unsigned char RemoveFromResourceRegistry(ICGraphicsResource& resource);
    /** Common device set-up (clear table, event listener, cached instructions). */
    void InitializeCore();
    /** Undo InitializeCore(). */
    void DestroyCore();
    /** Map `rectangle` (y down) to device coordinates of the current target and orientation. */
    void OrientationReinterpretationYFlipTransform(components::CRectangle<short>& rectangle,
                                                   components::CRectangle<short>& transformed);
    unsigned char SaveResourceStateForFutureRecreation(ICGraphicsResource* resource);
    unsigned char RecreateResource(ICGraphicsResource* resource);

    /** The registry the resources are kept in (calls go through the vtable). */
    systems::CRegistry& GetResourceRegistry()
    {
        return m_resourceRegistry;
    }

    ICRasterizerState* m_rasterizerState;               // 0x008
    TargetInstruction* m_targetInstruction;             // 0x00c
    ViewportInstruction* m_viewportInstruction;         // 0x010
    components::CTypedVariableTable m_clearTable;       // 0x014 ClearEntry values ("ClearBuffers")
    ShadersInstruction* m_shadersInstruction;           // 0x08c
    TexturesInstruction* m_texturesInstruction;         // 0x090
    TextureSampleHint m_textureSampleHints[TextureUnitCount];  // 0x094
    TextureUnit m_textureUnits[TextureUnitCount];       // 0x0d4
    ICDisplayProgram* m_displayProgram;                 // 0x194 program being recorded (NULL: immediate)
    ICDisplayProgram* m_defaultDisplayProgram;          // 0x198 program Begin(Mode&) records into
    unsigned char m_runOnEnd;                           // 0x19c End() runs the program
    ICShaderProgram* m_shaderProgram;                   // 0x1a0
    unsigned char** m_targetLevel;                      // 0x1a4 pending "Target" slot of the program
    unsigned char** m_clearLevel;                       // 0x1a8 pending "ClearBuffers" slot
    unsigned char** m_viewportLevel;                    // 0x1ac pending "Viewport" slot
    unsigned char** m_shadersLevel;                     // 0x1b0 pending "Shaders" slot
    unsigned char** m_texturesLevel;                    // 0x1b4 pending "Textures" slot
    unsigned char** m_shaderParamsLevel;                // 0x1b8 pending "ShaderParams" slot
    unsigned char* m_shaderParamsInstruction;           // 0x1bc last executed "ShaderParams" instruction
    systems::CRegistry m_resourceRegistry;              // 0x1c0 every live ICGraphicsResource
    unsigned char m_deviceInitialized;                  // 0x210 Initialize() of the device succeeded
    unsigned char m_coreInitialized;                    // 0x211 InitializeCore() ran
    unsigned char m_state212;                           // 0x212 (only cleared)
    unsigned char m_initialized;                        // 0x213 platform Initialize() succeeded so far
    unsigned char m_intrinsicShadersInitialized;        // 0x214
    components::ColorFormat m_defaultColorFormat;       // 0x218
    unsigned int m_state21c;                            // 0x21c (unused)
    EventHandler m_eventHandler;                        // 0x220 callback of m_eventListener
    systems::CEventListener* m_eventListener;           // 0x224
    unsigned int m_width;                               // 0x228 init parameter 1
    int m_displayScaleX;                                // 0x22c 16.16, init parameter 4
    int m_displayScaleY;                                // 0x230 16.16, init parameter 5
    components::Color_A8R8G8B8_t m_backgroundColor;     // 0x234 init parameter 3
    unsigned char m_initializeFailed;                   // 0x238 init parameter 2 rejected
    unsigned char m_state239;                           // 0x239 (only cleared)
    ICRenderSurface* m_windowSurface;                   // 0x23c
    ICRenderSurface* m_targetSurface;                   // 0x240
    core::CVector<IntrinsicShaderProgramInfo> m_intrinsicShaderPrograms;   // 0x244
    DebugMessagingBridge* m_debugMessagingBridge;       // 0x25c
};

inline void CGraphics::SetDefaultColorFormat(components::ColorFormat format)
{
    m_defaultColorFormat = format;
}

inline components::ColorFormat CGraphics::GetDefaultColorFormat() const
{
    return m_defaultColorFormat;
}

} } } }

#endif
