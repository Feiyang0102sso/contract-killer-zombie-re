// CRenderSurface_OGLES_Texture.h - render surface kept in OpenGL ES textures (large images are split
// into tiles of the maximum texture size, one texture per tile).
// (first drafted by b002 as PROVISIONAL; completed by b039x, the owner)
#ifndef CRENDERSURFACE_OGLES_TEXTURE_H
#define CRENDERSURFACE_OGLES_TEXTURE_H

#include "CRenderSurface_SW.h"

namespace com { namespace glu { namespace platform { namespace graphics {

class CRenderSurface_OGLES_Targetable;

class CRenderSurface_OGLES_Texture : public CRenderSurface_SW {
public:
    /** Texture filters (m_magFilter / m_minFilter). */
    enum Filter {
        Filter_Linear = 0,
        Filter_LinearMipmapLinear = 1,
        Filter_LinearMipmapNearest = 2,
        Filter_Nearest = 3,
        Filter_NearestMipmapLinear = 4,
        Filter_NearestMipmapNearest = 5
    };

    /** Texture wrap modes (m_wrapS / m_wrapT). */
    enum Wrap {
        Wrap_ClampToEdge = 0,
        Wrap_Repeat = 1
    };

    CRenderSurface_OGLES_Texture();
    virtual ~CRenderSurface_OGLES_Texture();

    virtual ICGraphicsAbstract::Abstraction GetAbstraction() const
    {
        return ICGraphicsAbstract::Abstraction_Hardware;
    }
    virtual unsigned char Initialize(const InitParam* params);
    virtual void Destroy();
    virtual void Lock(unsigned char** data, int& pitch, LockType lockType, const components::CRectangle<short>* area);
    virtual void Unlock();
    virtual components::ColorFormat GetColorFormat();
    virtual unsigned char GetWidthAndHeight(unsigned int& width, unsigned int& height);
    virtual unsigned char GetDescription(Description& description);
    virtual unsigned char SaveStateForFutureRecreation();
    virtual unsigned char Recreate();
    /** Create the buffers described by `params` (also used by the subclasses). */
    virtual unsigned char InitializeBuffers(const InitParam* params);

    /** (Re)create the OpenGL textures from the color buffer of the source texture. */
    void InitializeTextureObjects();
    void DestroyTextureObjects();
    /** The surface that owns the textures (follows m_sourceTexture). */
    CRenderSurface_OGLES_Texture* GetSourceTexture();

    /** OpenGL name of texture (tile) `index`. */
    unsigned int& GetTextureName(int index)
    {
        if (m_textureCount > 1) {
            return m_textures[index];
        }
        return m_texture;
    }

    int m_textureCount;                             // 0x1c textures (tiles) of the surface
    union {
        unsigned int m_texture;                     // 0x20 m_textureCount < 2
        unsigned int* m_textures;                   // 0x20 m_textureCount >= 2
    };
    unsigned int m_magFilter;                       // 0x24 Filter
    unsigned int m_minFilter;                       // 0x28 Filter
    unsigned int m_wrapS;                           // 0x2c Wrap
    unsigned int m_wrapT;                           // 0x30 Wrap
    unsigned char m_keepBufferData;                 // 0x34 keep the pixels after uploading them
    unsigned char m_contentsUndefined;              // 0x35 nothing was uploaded yet
    CRenderSurface_OGLES_Texture* m_sourceTexture;  // 0x38 InitParam_SourceTexture: textures of that surface are used
    CRenderSurface_OGLES_Targetable* m_lockSurface; // 0x3c render target used to read a locked target back
};

} } } }

#endif
