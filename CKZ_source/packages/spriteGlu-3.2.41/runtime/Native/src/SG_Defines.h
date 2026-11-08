// SG_Defines.h - data model of the spriteGlu 2d sprite animation runtime (created by b042):
// the resource reader, the texture atlas with its transformed source images, image / rect
// sprites, sprite maps, layers, animations, characters and archetypes.
// Layouts come from the constructors, loaders and accessors in SG_Defines.cpp / SG_Instance.cpp.
#ifndef SG_DEFINES_H
#define SG_DEFINES_H

#include <stddef.h>
#include "CArrayInputStream.h"
#include "CRectangle.h"
#include "CVector.h"
#include "SquareTransform.h"

namespace com { namespace glu { namespace platform { namespace graphics {
class ICRenderSurface;
} } } }

class DataInputStream;
class SG_SrcImage;

/** What SG_Presenter::TraverseCurrentFrame does with each sprite of the current frame. */
enum SG_TraverseMode {
    SG_TraverseMode_Draw = 0,           // draw it (SG_Instance::drawAccumulate)
    SG_TraverseMode_Collide = 1,        // test it against the collision rectangle
    SG_TraverseMode_CollisionInit = 2,  // the first matching sprite becomes the collision rectangle
    SG_TraverseMode_Bounds = 3          // grow the bounding box
};

/** Fixed size bit set that also counts its members (one bit per image sprite). */
class BitVector {
public:
    BitVector()
    {
        m_byteCount = 0;
        m_bits = NULL;
        m_bitCount = 0;
        m_onCount = 0;
    }

    BitVector(unsigned int bitCount);
    virtual ~BitVector();

    /** Allocate `bitCount` bits and read them from `stream`. */
    void Init(int bitCount, DataInputStream* stream);
    unsigned char IsMember(unsigned int bit);
    /** Add (`member` != 0) or remove `bit`, keeping the member count. */
    void SetMember(unsigned int bit, unsigned char member);
    unsigned int GetOnCount();

    int m_byteCount;            // 0x04
    unsigned char* m_bits;      // 0x08
    unsigned int m_bitCount;    // 0x0c
    unsigned int m_onCount;     // 0x10 bits set
};

/** Input stream over a binary resource; the resource is released with the stream. */
class DataInputStream : public com::glu::platform::components::CArrayInputStream {
public:
    /** Create the resource `resourceName`; m_error is set when it does not exist. */
    DataInputStream(const char* resourceName);
    virtual ~DataInputStream();

    const char* m_resourceName;     // 0x20
};

/** 2d drawing state of an SG_Instance: clip rectangle and drawing offset. */
class Graphics {
public:
    /** How a source image is mirrored when drawn (an ICGraphics2d::Flip value). */
    struct RenderInfo {
        int m_flip;     // 0x00
    };

    Graphics() : m_clip()
    {
    }

    void SetOffset(short x, short y);
    void SetClip(const com::glu::platform::components::CRectangle<short>& clip);
    /**
     * Draw `region` of `image` at (x, y) + offset; `scale` scales the whole sprite, `rotation`
     * (fixed point degrees) and `scaleX` / `scaleY` (16.16) act around the region's center.
     */
    void DrawRegion(SG_SrcImage* image, const RenderInfo& info, int x, int y, int scale,
                    com::glu::platform::components::CRectangle<short>* region, int width, int height,
                    int rotation, int scaleX, int scaleY);
    /** Fill a rectangle with the 0xRRGGBB `color`; rotation and scale as in DrawRegion. */
    void FillRect(int x, int y, unsigned int width, unsigned int height, int color, int rotation,
                  int scaleX, int scaleY);

    com::glu::platform::components::CRectangle<short> m_clip;   // 0x00
    short m_offsetX;                                            // 0x08
    short m_offsetY;                                            // 0x0a
};

/** A sprite showing one atlas image. */
class SG_Imagesprite {
public:
    SG_Imagesprite();
    ~SG_Imagesprite();

    unsigned char Load(DataInputStream& stream);

    unsigned short m_tint;          // 0x00 index into SG_Instance::m_tints
    unsigned char m_tag;            // 0x02
    unsigned char m_transform;      // 0x03 SquareTransform of the image
};

/** A sprite that is a plain rectangle; color -1 marks a collision box (never drawn). */
class SG_Rectsprite {
public:
    SG_Rectsprite();
    ~SG_Rectsprite();

    unsigned char Load(DataInputStream& stream);

    int m_color;                    // 0x00 0xRRGGBB, -1 = collision box
    unsigned short m_width;         // 0x04
    unsigned short m_height;        // 0x06
    signed char m_tag;              // 0x08
};

/** One texture map of the atlas: a run of images and its rectangle on the surface. */
class SG_TextureMap {
public:
    ~SG_TextureMap()
    {
    }

    unsigned char Load(DataInputStream& stream);

    unsigned short m_firstImage;    // 0x00 index of the map's first image (minus one, see SG_Tint)
    unsigned short m_x;             // 0x02
    unsigned short m_y;             // 0x04
    unsigned short m_width;         // 0x06
    unsigned short m_height;        // 0x08
};

/**
 * Selects one image: m_images[m_maps[m_map].m_firstImage + m_offset + 1] of the atlas.
 * Passed by value everywhere; 8-byte aligned (callers realign the stack for its temporaries).
 */
class SG_Tint {
public:
    SG_Tint();
    ~SG_Tint();

    unsigned char Load(DataInputStream& stream);

    /** Atlas image selected in `maps`. */
    int GetImageIndex(SG_TextureMap* maps)
    {
        return m_offset + maps[m_map].m_firstImage + 1;
    }

    unsigned int m_map;             // 0x00 texture map index
    signed char m_offset;           // 0x04 image offset in the map, -1 = the first image
} __attribute__((aligned(8)));

/** A loaded surface of one image transform group (normal or transposed). */
class SG_SrcImage {
public:
    SG_SrcImage()
    {
        m_surface = NULL;
        m_resourceName = NULL;
    }

    ~SG_SrcImage();

    com::glu::platform::graphics::ICRenderSurface* m_surface;  // 0x00
    const char* m_resourceName;                                 // 0x04 surface resource, released on destruction
};

/** One atlas image with its surfaces per scale, for normal and transposed transforms. */
class SG_Image {
public:
    SG_Image();
    ~SG_Image();

    /** Mark `transform` loaded; `image` becomes the group's surface if the group had none. */
    unsigned char LoadTransform(unsigned char transform, SG_SrcImage* image);
    /** Mark `transform` unloaded; 0 (and surfaces freed) when its group has no transform left. */
    unsigned char DumpTransform(unsigned char transform);
    /** Surface and mirroring for `transform` at `scale` percent; `isScaled` is set when not the first scale. */
    void GetSrcImageAndRenderInfo(unsigned char transform, SG_SrcImage*& image, Graphics::RenderInfo& info,
                                  unsigned short scale, unsigned char& isScaled);
    void ClearImageTransposedVectors();
    void ClearImageVectors();

    /** Index of `scale` in m_scales (0 when there is only one scale, -1 if absent). */
    int GetScaleIndex(unsigned short scale)
    {
        if (m_scales.Size() == 1) {
            return 0;
        }
        for (int index = 0; index < m_scales.Size(); index++) {
            if (m_scales[index] == scale) {
                return index;
            }
        }
        return -1;
    }

    /** Some transform of the group of `transform` (normal 0..3 / transposed 4..7) is loaded. */
    bool IsTransformGroupLoaded(unsigned char transform)
    {
        if (SquareTransform_IsTransposed(transform)) {
            return m_loadedTransforms > 0x0f;
        }
        return (m_loadedTransforms & 0x0f) != 0;
    }

    unsigned short m_unused00;                                          // 0x00 never accessed
    unsigned char m_loadedTransforms;                                   // 0x02 bit per loaded transform
    com::glu::platform::core::CVector<SG_SrcImage*> m_images;           // 0x04 per scale, transforms 0..3
    com::glu::platform::core::CVector<SG_SrcImage*> m_transposedImages; // 0x1c per scale, transforms 4..7
    com::glu::platform::core::CVector<unsigned short> m_scales;         // 0x34 percent, starts with 100
};

/** The images of a sprite resource file and their per-transform usage counts. */
class SG_TextureAtlas {
public:
    SG_TextureAtlas(unsigned int imageCount, const char* surfacePrefix, unsigned int mapCount,
                    DataInputStream& stream);
    ~SG_TextureAtlas();

    void GetImageSize(SG_Tint tint, unsigned char transform, int& width, int& height);
    /** No transform of the group is loaded yet, so the surface has to be created. */
    unsigned char LoadTransformRequiresRawImage(SG_Tint tint, unsigned char transform);
    unsigned char IsImageTransformLoaded(SG_Tint tint, unsigned char transform);
    unsigned char LoadImageTransform(SG_Tint tint, unsigned char transform);
    /** Drop one use; the transform is unloaded with its last use. */
    unsigned char DumpImageTransform(SG_Tint tint, unsigned char transform);
    /** Unload everything; returns the number of surface groups that were loaded. */
    unsigned int DumpAllTransforms();
    void GetSrcImageAndRenderInfo(SG_Tint tint, unsigned char transform, SG_SrcImage*& image,
                                  Graphics::RenderInfo& info,
                                  com::glu::platform::components::CRectangle<short>& region);
    /** Load the surface of the transform group when needed and mark the transform loaded. */
    unsigned char LoadPage(SG_Tint tint, unsigned char transform);
    void IncreaseUssagesCount(SG_Tint tint, unsigned char transform);

    /** Image index selected by `tint`. */
    int GetImageIndex(SG_Tint& tint)
    {
        return m_maps[tint.m_map].m_firstImage + tint.m_offset + 1;
    }

    SG_Image& GetImage(SG_Tint& tint)
    {
        return m_images[GetImageIndex(tint)];
    }

    /** Use count of one transform of the image selected by `tint`. */
    unsigned short& GetUsageCount(SG_Tint& tint, unsigned char transform)
    {
        return m_usageCounts[GetImageIndex(tint) * 8 + transform];
    }

    unsigned int m_mapCount;            // 0x00
    unsigned int m_imageCount;          // 0x04
    const char* m_surfacePrefix;        // 0x08 first surface resource name (two per image)
    SG_TextureMap* m_maps;              // 0x0c
    SG_Image* m_images;                 // 0x10
    unsigned short* m_usageCounts;      // 0x14 8 transforms per image
};

/** Replacement of one sprite inside a sprite map. */
struct SG_SpritemapEntry {
    unsigned short m_sprite;        // 0x00 replaced sprite (entries sorted by it)
    unsigned short m_replacement;   // 0x02
    int m_mode;                     // 0x04 0 = replace, 1 = draw on top, other = draw below
    short m_x;                      // 0x08 offset of the replacement
    short m_y;                      // 0x0a
};

/** Per character sprite substitutions. */
class SG_Spritemap {
public:
    SG_Spritemap();
    ~SG_Spritemap();

    unsigned char Load(DataInputStream& stream);

    unsigned short m_count;             // 0x00
    SG_SpritemapEntry* m_entries;       // 0x04
};

/** One sprite placed in a layer. */
struct SG_LayerEntry {
    unsigned short m_sprite;    // 0x00
    short m_x;                  // 0x02
    short m_y;                  // 0x04
    int m_rotation;             // 0x08 fixed point degrees
    int m_scaleX;               // 0x0c 16.16
    int m_scaleY;               // 0x10 16.16
};

/** The sprites of one animation frame. */
class SG_Layer {
public:
    SG_Layer()
    {
        m_count = 0;
        m_entries = NULL;
    }

    ~SG_Layer()
    {
        if (m_entries != NULL) {
            delete[] m_entries;
            m_entries = NULL;
        }
    }

    unsigned char Load(DataInputStream& stream);

    unsigned short m_count;         // 0x00
    SG_LayerEntry* m_entries;       // 0x04
};

/** A sequence of layers with a duration per frame. */
class SG_Animation {
public:
    SG_Animation()
    {
        m_frameCount = 0;
    }

    ~SG_Animation();

    unsigned char Load(DataInputStream& stream);

    unsigned char m_id;                 // 0x00
    unsigned char m_frameCount;         // 0x01
    unsigned short* m_frameLayers;      // 0x04 layer per frame
    unsigned short* m_frameDurations;   // 0x08 milliseconds per frame
};

/** A variant of an archetype: which image sprites it uses and its sprite map. */
class SG_Character {
public:
    /** m_spritemap value of a character without sprite map. */
    enum { NoSpritemap = 0xff };

    SG_Character()
    {
        m_loaded = 0;
        m_spritemap = NoSpritemap;
    }

    ~SG_Character();

    /** The available scale closest to `scale` percent (the scales are sorted, largest first). */
    unsigned short GetScaleFactorForCharacter(unsigned short scale);

    BitVector m_sprites;                                        // 0x00 image sprites used
    com::glu::platform::core::CVector<unsigned short> m_scales; // 0x14
    unsigned char m_loaded;                                     // 0x2c
    unsigned char m_spritemap;                                  // 0x2d NoSpritemap = none
};

/** Layers, animations and characters loaded from one archetype resource. */
class SG_Archetype {
public:
    SG_Archetype();
    ~SG_Archetype();

    unsigned char LoadLayers(DataInputStream& stream);
    unsigned char LoadAnimations(DataInputStream& stream);
    unsigned char LoadCharacters(DataInputStream& stream, unsigned short spriteCount);

    unsigned short m_layerCount;        // 0x00
    SG_Layer* m_layers;                 // 0x04
    unsigned short m_animationCount;    // 0x08
    SG_Animation* m_animations;         // 0x0c
    unsigned char m_characterCount;     // 0x10
    SG_Character* m_characters;         // 0x14
};

/** One sprite to draw for a layer entry after sprite map substitution. */
struct SpriteDrawInfo {
    unsigned short m_sprite;    // 0x00
    short m_x;                  // 0x02
    short m_y;                  // 0x04
};

#endif
