// ICRenderSurface.h - render surface (texture / render target) interface.
// (first drafted by b011x and b002 as PROVISIONAL; slots completed by b037x from the vtables of
// CRenderSurface and its subclasses; flags, InitParam keys and Description completed by b039x.
// The factory lives in cocoa/src/CGraphicsAbstractionManager_Cocoa.cpp.)
#ifndef ICRENDERSURFACE_H
#define ICRENDERSURFACE_H

#include "ICGraphicsResource.h"
#include "CColor.h"

namespace com { namespace glu { namespace platform {

namespace components {
class CInputStream;
class COutputStream;
class CIdToObjectRouter;
template <class T> class CRectangle;
}

namespace graphics {

class ICRenderSurface : public ICGraphicsResource {
public:
    /** Resource type key of render surfaces. */
    enum { ResourceType = 0xc9403f77 };

    /** Surface kinds. */
    enum Type {
        Type_Default = 0,
        Type_Window = 1     // the window surface of a device
    };

    /** How the surface may be rendered into (values from CResourceRenderSurface). */
    enum Targetability {
        Targetability_0 = 0,
        Targetability_1 = 1,
        Targetability_2 = 2,
        Targetability_3 = 3
    };

    /** m_flags bits (CRenderSurface and subclasses). */
    enum Flag {
        Flag_Texture = 0x1,                 // sampled as a texture (Description::m_usage 0)
        Flag_Targetable = 0x2,              // the surface can be a render target (Description::m_usage 1)
        Flag_Window = 0x4,                  // InitParam_Window was given
        Flag_Format8 = 0x8,                 // Description::m_colorKind bits (8 / 0x10 / 0x20 / 0x40)
        Flag_Format10 = 0x10,
        Flag_Format20 = 0x20,
        Flag_Format40 = 0x40,
        Flag_Software = 0x100,              // Description::m_type 2 (CRenderSurface_SW)
        Flag_Hardware = 0x200,              // Description::m_type 1 (OpenGL ES textures)
        Flag_400 = 0x400,                   // Description::m_flag15
        Flag_Lockable = 0x800,              // Description::m_lockAccess bit 0
        Flag_Writable = 0x1000,             // Description::m_lockAccess bit 1
        Flag_PowerOfTwo = 0x4000,           // buffers are padded to power of two dimensions
        Flag_Mipmaps = 0x8000,              // InitParam_Mipmaps
        Flag_HasColorFormat = 0x10000,      // InitParam_ColorFormat was given
        Flag_HasDepthBuffer = 0x20000,      // InitParam_DepthFormat: m_buffers[1]
        Flag_HasStencilBuffer = 0x80000,    // InitParam_StencilFormat: m_buffers[2]
        Flag_MultisampleMask = 0x300000,    // multisampling of the target (0: none, 1..3)
        Flag_Multisample1 = 0x100000,
        Flag_Multisample2 = 0x200000,
        Flag_Multisample3 = 0x300000,
        Flag_Reference = 0x400000,          // m_buffers[0] is another surface whose buffers are used
        Flag_OwnsPalette = 0x800000,        // m_palette was allocated by the surface
        Flag_LockMask = 0x7000000,          // locked
        Flag_ContentsDiscarded = 0x8000000, // DiscardContents()
        Flag_Initialized = 0x80000000
    };

    /** How a locked range is going to be used. */
    enum LockType {
        LockType_ReadWrite = 0
    };

    /** Keys of the creation settings (Initialize). */
    enum InitParamKey {
        InitParam_End = 0,
        InitParam_Width = 1,
        InitParam_Height = 2,
        InitParam_Window = 3,               // only the window surface accepts it
        InitParam_Mipmaps = 5,
        InitParam_ColorFormat = 6,          // RSBFormat
        InitParam_DepthFormat = 7,          // RSBFormat
        InitParam_StencilFormat = 8,        // RSBFormat
        InitParam_ColorKey = 9,             // Color_A8R8G8B8 made transparent
        InitParam_Palette = 0xb,            // void* palette of the pixel data
        InitParam_Reference = 0xc,          // ICGraphicsResource* whose buffers are shared
        InitParam_ReferenceType = 0xd,      // resource type of InitParam_Reference
        InitParam_SourceStream = 0xe,       // CInputStream* to decode the pixels from
        InitParam_SourceType = 0xf,         // mime type key of the stream
        InitParam_HighColor = 0x10,         // decode to 16 bit formats
        InitParam_Orientation = 0x11,       // 0..6: transpose / flip of the decoded image
        InitParam_Multisample = 0x12,       // 0..3
        InitParam_MagFilter = 0x13,
        InitParam_MinFilter = 0x14,
        InitParam_WrapS = 0x15,
        InitParam_WrapT = 0x16,
        InitParam_SourceTexture = 100       // ICRenderSurface* (same abstraction) whose textures are used
    };

    /** One (key, value) creation setting; a list ends with key 0. */  // added by b002
    struct InitParam {  // added by b002
        unsigned int m_key;     // 0x00  // added by b002
        unsigned int m_value;   // 0x04  // added by b002
    };  // added by b002

    /** Description of a surface (GetDescription). */
    struct Description {
        unsigned int m_type;                // 0x00 1 software, 2 hardware
        unsigned int m_usage;               // 0x04 0 texture, 1 target
        unsigned int m_lockAccess;          // 0x08 0..3
        unsigned int m_colorKind;           // 0x0c 0..4
        unsigned short m_width;             // 0x10 0xffff when unknown
        unsigned short m_height;            // 0x12
        unsigned char m_window;             // 0x14 Flag_Window
        unsigned char m_flag15;             // 0x15 Flag_400
        unsigned char m_locked;             // 0x16 IsLocked()
        unsigned char m_mipmaps;            // 0x17 Flag_Mipmaps
        unsigned char m_hasColorFormat;     // 0x18 Flag_HasColorFormat
        unsigned char m_hasDepthBuffer;     // 0x19
        unsigned char m_hasStencilBuffer;   // 0x1a
        unsigned char m_ownsPalette;        // 0x1b Flag_OwnsPalette
        components::ColorFormat m_colorFormat;      // 0x1c
        components::ColorFormat m_depthFormat;      // 0x20
        components::ColorFormat m_stencilFormat;    // 0x24
        unsigned int m_multisample;         // 0x28 0..3
        unsigned int m_magFilter;           // 0x2c
        unsigned int m_minFilter;           // 0x30
        unsigned int m_wrapS;               // 0x34
        unsigned int m_wrapT;               // 0x38
        void* m_palette;                    // 0x3c
        ICRenderSurface* m_reference;       // 0x40 Flag_Reference
    };

    /** Platform factory. */
    static ICRenderSurface* CreateInstance(Type type, Targetability targetability);

    ICRenderSurface() : ICGraphicsResource(ResourceType) {}

    virtual ~ICRenderSurface() {}

    /** Load the image of mime type `type` from `stream`. */
    virtual unsigned char Load(components::CInputStream& stream, unsigned int type,
                               components::CIdToObjectRouter* router) = 0;
    virtual unsigned char Save(components::COutputStream& stream, unsigned int type)
    {
        return 0;
    }
    virtual unsigned char Initialize(const InitParam* params) = 0;  // added by b002
    virtual void Destroy() = 0;  // added by b002
    virtual void DiscardContents() = 0;  // added by b002
    virtual void UpdateTextureWithTarget() = 0;
    virtual void Lock(unsigned char** data, int& pitch, LockType lockType, const components::CRectangle<short>* area) = 0;
    virtual void Unlock() = 0;
    virtual unsigned char IsLocked() = 0;
    virtual components::ColorFormat GetColorFormat() = 0;
    virtual unsigned char GetWidthAndHeight(unsigned int& width, unsigned int& height) = 0;  // added by b002
    virtual unsigned char GetDescription(Description& description) = 0;
    /** Make the surface the render target of the device. */
    virtual void Bind() = 0;
    virtual unsigned char SaveStateForFutureRecreation() = 0;
    virtual unsigned char Recreate() = 0;

    unsigned int m_flags;   // 0x08 Flag bits
};

} } } }

#endif
