// SG_Defines.cpp - spriteGlu data model: resource stream, texture atlas, sprites, layers,
// animations, characters and archetypes (see SG_Defines.h). (written by b042)
#include "SG_Defines.h"
#include "CApplet.h"
#include "CApp.h"
#include "CBinary.h"
#include "CColor.h"
#include "CResource.h"
#include "ICGraphics2d.h"
#include "ICResourceManager.h"
#include "WindowApp.h"

using namespace com::glu::platform;
using com::glu::platform::components::CRectangle;
using com::glu::platform::components::Color_RGBA;
using com::glu::platform::graphics::ICGraphics2d;
using com::glu::platform::graphics::ICRenderSurface;
using com::glu::platform::systems::CResource;
using com::glu::platform::systems::ICResourceManager;

// ---- constants ----
static const int FIXED_ONE = 0x10000;                       // 1.0 in 16.16 fixed point
static const float FIXED_TO_FLOAT = 1.0f / 65536.0f;        // 16.16 fixed point -> float
enum { DEFAULT_SCALE_PERCENT = 100 };                       // first scale of every image
static const int TRANSFORMS_PER_IMAGE = 8;                  // SquareTransform values

// ---------------------------------------------------------------------------------------------
// BitVector

BitVector::BitVector(unsigned int bitCount)
    : m_byteCount((int)(bitCount + 7) / 8), m_bits(NULL)
{
    m_bits = (unsigned char*)malloc(m_byteCount);
    memset(m_bits, 0, m_byteCount);
    m_bitCount = bitCount;
    m_onCount = 0;
}

BitVector::~BitVector()
{
    if (m_bits != NULL) {
        free(m_bits);
        m_bits = NULL;
    }
}

void BitVector::Init(int bitCount, DataInputStream* stream)
{
    m_byteCount = (bitCount + 7) / 8;
    m_bits = (unsigned char*)malloc(m_byteCount);
    memset(m_bits, 0, m_byteCount);
    m_bitCount = bitCount;
    stream->Read(m_bits, m_byteCount);

    // count the members that were read
    for (int bit = 0; bit < bitCount; bit++) {
        if (IsMember(bit)) {
            m_onCount++;
        }
    }
}

unsigned char BitVector::IsMember(unsigned int bit)
{
    return (m_bits[bit >> 3] & (1 << (bit & 7))) != 0;
}

void BitVector::SetMember(unsigned int bit, unsigned char member)
{
    unsigned int byteIndex = bit >> 3;
    int mask = 1 << (bit & 7);
    int wasMember = m_bits[byteIndex] & mask;
    if (member) {
        if (!wasMember) {
            m_onCount++;
            m_bits[byteIndex] |= mask;
        }
    } else if (wasMember) {
        m_onCount--;
        m_bits[byteIndex] &= ~mask;
    }
}

unsigned int BitVector::GetOnCount()
{
    return m_onCount;
}

// ---------------------------------------------------------------------------------------------
// DataInputStream

DataInputStream::DataInputStream(const char* resourceName)
{
    m_resourceName = resourceName;
    CResource* resource = NULL;
    ((ICResourceManager*)CApplet::m_pApp->m_resourceManager)->CreateResource(resourceName, &resource);
    if (resource == NULL) {
        m_error = 1;
    } else {
        components::CBinary* binary = (components::CBinary*)resource->GetData();
        Open(binary->m_data, binary->m_size);
    }
}

DataInputStream::~DataInputStream()
{
    ((ICResourceManager*)CApplet::m_pApp->m_resourceManager)->ReleaseResource(m_resourceName);
}

// ---------------------------------------------------------------------------------------------
// Graphics

void Graphics::SetOffset(short x, short y)
{
    m_offsetX = x;
    m_offsetY = y;
}

void Graphics::SetClip(const CRectangle<short>& clip)
{
    m_clip.m_x = clip.m_x;
    m_clip.m_y = clip.m_y;
    m_clip.m_width = clip.m_width;
    m_clip.m_height = clip.m_height;
}

void Graphics::DrawRegion(SG_SrcImage* image, const RenderInfo& info, int x, int y, int scale,
                          CRectangle<short>* region, int width, int height, int rotation, int scaleX,
                          int scaleY)
{
    int offsetX = m_offsetX;
    int offsetY = m_offsetY;
    ICGraphics2d* graphics2d = ICGraphics2d::GetInstance();
    graphics2d->PushTransform();

    bool unscaled = scaleX == FIXED_ONE && scaleY == FIXED_ONE;
    bool scaled = !unscaled;
    int drawX = offsetX + x;
    int drawY = offsetY + y;
    if (rotation != 0 || scaled) {
        // rotate and scale around the center of the region
        int centerX = drawX + (width >> 1);
        int centerY = drawY + (height >> 1);
        graphics2d->Translate((float)centerX, (float)centerY);
        if (rotation != 0) {
            graphics2d->Rotate((float)rotation * FIXED_TO_FLOAT);
        }
        if (!unscaled) {
            graphics2d->Scale((float)scaleX * FIXED_TO_FLOAT, (float)scaleY * FIXED_TO_FLOAT);
        }
        graphics2d->Translate((float)-centerX, (float)-centerY);
    }
    graphics2d->Translate((float)drawX, (float)drawY);
    if (scale != FIXED_ONE) {
        float factor = (float)scale * FIXED_TO_FLOAT;
        graphics2d->Scale(factor, factor);
    }
    graphics2d->Draw(*image->m_surface, region, &WindowApp::m_instance->Color(),
                     (ICGraphics2d::Flip)info.m_flip);
    graphics2d->PopTransform();
}

void Graphics::FillRect(int x, int y, unsigned int width, unsigned int height, int color, int rotation,
                        int scaleX, int scaleY)
{
    ICGraphics2d* graphics2d = ICGraphics2d::GetInstance();
    graphics2d->PushTransform();

    bool unscaled = scaleX == FIXED_ONE && scaleY == FIXED_ONE;
    bool scaled = !unscaled;
    if (rotation != 0 || scaled) {
        // rotate and scale around the center of the rectangle
        int centerX = x + width / 2 + m_offsetX;
        int centerY = y + height / 2 + m_offsetY;
        graphics2d->Translate((float)centerX, (float)centerY);
        if (rotation != 0) {
            graphics2d->Rotate((float)rotation * FIXED_TO_FLOAT);
        }
        if (!unscaled) {
            graphics2d->Scale((float)scaleX * FIXED_TO_FLOAT, (float)scaleY * FIXED_TO_FLOAT);
        }
        graphics2d->Translate((float)-centerX, (float)-centerY);
    }

    CRectangle<short> rectangle;
    rectangle.m_x = m_offsetX + x;
    rectangle.m_y = m_offsetY + y;
    rectangle.m_width = width;
    rectangle.m_height = height;
    Color_RGBA fill;
    fill.r = (unsigned char)(color >> 16);
    fill.g = (unsigned char)(color >> 8);
    fill.b = (unsigned char)color;
    fill.a = 1.0f;
    graphics2d->Draw(rectangle, fill);
    graphics2d->PopTransform();
}

// ---------------------------------------------------------------------------------------------
// SG_Imagesprite / SG_Rectsprite / SG_TextureMap

SG_Imagesprite::SG_Imagesprite()
    : m_tint(0), m_tag(0), m_transform(0)
{
}

SG_Imagesprite::~SG_Imagesprite()
{
}

unsigned char SG_Imagesprite::Load(DataInputStream& stream)
{
    m_tint = stream.ReadUInt16();
    m_transform = stream.ReadUInt8();
    m_tag = stream.ReadUInt8();
    return stream.m_error == 0;
}

SG_Rectsprite::SG_Rectsprite()
{
    m_color = -1;
    m_tag = 0;
    m_height = 0;
    m_width = 0;
}

SG_Rectsprite::~SG_Rectsprite()
{
}

unsigned char SG_Rectsprite::Load(DataInputStream& stream)
{
    m_color = stream.ReadInt32();
    m_width = stream.ReadUInt16();
    m_height = stream.ReadUInt16();
    m_tag = stream.ReadInt8();
    return stream.m_error == 0;
}

unsigned char SG_TextureMap::Load(DataInputStream& stream)
{
    m_firstImage = stream.ReadUInt16();
    m_x = stream.ReadUInt16();
    m_y = stream.ReadUInt16();
    m_width = stream.ReadUInt16();
    m_height = stream.ReadUInt16();
    return stream.m_error == 0;
}

// ---------------------------------------------------------------------------------------------
// SG_TextureAtlas

SG_TextureAtlas::SG_TextureAtlas(unsigned int imageCount, const char* surfacePrefix, unsigned int mapCount,
                                 DataInputStream& stream)
{
    m_mapCount = mapCount;
    m_imageCount = imageCount;
    m_surfacePrefix = surfacePrefix;

    m_maps = new SG_TextureMap[m_mapCount];
    for (unsigned int map = 0; map < m_mapCount; map++) {
        m_maps[map].Load(stream);
    }

    m_images = new SG_Image[m_imageCount];
    m_usageCounts = new unsigned short[m_imageCount * TRANSFORMS_PER_IMAGE];
    for (unsigned int index = 0; index < m_imageCount * TRANSFORMS_PER_IMAGE; index++) {
        m_usageCounts[index] = 0;
    }
}

SG_TextureAtlas::~SG_TextureAtlas()
{
    if (m_usageCounts != NULL) {
        delete[] m_usageCounts;
        m_usageCounts = NULL;
    }
    if (m_images != NULL) {
        delete[] m_images;
        m_images = NULL;
    }
    if (m_maps != NULL) {
        delete[] m_maps;
        m_maps = NULL;
    }
}

void SG_TextureAtlas::GetImageSize(SG_Tint tint, unsigned char transform, int& width, int& height)
{
    if (SquareTransform_IsTransposed(transform)) {
        height = m_maps[tint.m_map].m_width;
        width = m_maps[tint.m_map].m_height;
    } else {
        height = m_maps[tint.m_map].m_height;
        width = m_maps[tint.m_map].m_width;
    }
}

unsigned char SG_TextureAtlas::LoadTransformRequiresRawImage(SG_Tint tint, unsigned char transform)
{
    return !GetImage(tint).IsTransformGroupLoaded(transform);
}

SG_Tint::~SG_Tint()
{
}

unsigned char SG_TextureAtlas::IsImageTransformLoaded(SG_Tint tint, unsigned char transform)
{
    return (GetImage(tint).m_loadedTransforms & (1 << transform)) != 0;
}

unsigned char SG_TextureAtlas::LoadImageTransform(SG_Tint tint, unsigned char transform)
{
    return GetImage(tint).LoadTransform(transform, NULL);
}

unsigned char SG_Image::LoadTransform(unsigned char transform, SG_SrcImage* image)
{
    // the first surface of a group is shared by all its transforms
    if (SquareTransform_IsTransposed(transform)) {
        if (m_transposedImages.Size() == 0) {
            m_transposedImages.Add(image);
        }
    } else {
        if (m_images.Size() == 0) {
            m_images.Add(image);
        }
    }
    unsigned char loaded = m_loadedTransforms;
    m_loadedTransforms = loaded | (1 << transform);
    return (loaded & (1 << transform)) == 0;
}

unsigned char SG_TextureAtlas::DumpImageTransform(SG_Tint tint, unsigned char transform)
{
    if ((GetImage(tint).m_loadedTransforms & (1 << transform)) == 0) {
        return 0;
    }
    GetUsageCount(tint, transform)--;
    if (GetUsageCount(tint, transform) != 0) {
        return 0;
    }
    return GetImage(tint).DumpTransform(transform);
}

unsigned char SG_Image::DumpTransform(unsigned char transform)
{
    m_loadedTransforms &= ~(1 << transform);
    if (IsTransformGroupLoaded(transform)) {
        return 1;
    }
    if (SquareTransform_IsTransposed(transform)) {
        ClearImageTransposedVectors();
    } else {
        ClearImageVectors();
    }
    return 0;
}

unsigned int SG_TextureAtlas::DumpAllTransforms()
{
    unsigned int dumpedGroups = 0;
    for (unsigned int index = 0; index < m_imageCount; index++) {
        SG_Image& image = m_images[index];
        image.m_loadedTransforms = 0;
        int normalCount = image.m_images.Size();
        int transposedCount = image.m_transposedImages.Size();
        image.ClearImageVectors();
        image.ClearImageTransposedVectors();
        for (int transform = 0; transform < TRANSFORMS_PER_IMAGE; transform++) {
            m_usageCounts[index * TRANSFORMS_PER_IMAGE + transform] = 0;
        }
        if (normalCount > 0) {
            dumpedGroups++;
        }
        if (transposedCount > 0) {
            dumpedGroups++;
        }
    }
    return dumpedGroups;
}

void SG_TextureAtlas::GetSrcImageAndRenderInfo(SG_Tint tint, unsigned char transform, SG_SrcImage*& image,
                                               Graphics::RenderInfo& info, CRectangle<short>& region)
{
    unsigned char isScaled = 0;
    GetImage(tint).GetSrcImageAndRenderInfo(transform, image, info, DEFAULT_SCALE_PERCENT, isScaled);
    if (SquareTransform_IsTransposed(transform)) {
        region.m_x = m_maps[tint.m_map].m_y;
        region.m_y = m_maps[tint.m_map].m_x;
        region.m_width = m_maps[tint.m_map].m_height;
        region.m_height = m_maps[tint.m_map].m_width;
    } else {
        region.m_x = m_maps[tint.m_map].m_x;
        region.m_y = m_maps[tint.m_map].m_y;
        region.m_width = m_maps[tint.m_map].m_width;
        region.m_height = m_maps[tint.m_map].m_height;
    }
}

void SG_Image::GetSrcImageAndRenderInfo(unsigned char transform, SG_SrcImage*& image, Graphics::RenderInfo& info,
                                        unsigned short scale, unsigned char& isScaled)
{
    isScaled = GetScaleIndex(scale) != 0;
    int mirrorVertical = transform & SquareTransform_MirrorVertical;
    if (SquareTransform_IsTransposed(transform)) {
        // transposed surface: the mirror bits swap axes
        if (mirrorVertical && (transform & SquareTransform_MirrorHorizontal)) {
            info.m_flip = ICGraphics2d::Flip_Both;
        } else if (transform & SquareTransform_MirrorHorizontal) {
            info.m_flip = ICGraphics2d::Flip_Vertical;
        } else if (mirrorVertical) {
            info.m_flip = ICGraphics2d::Flip_Horizontal;
        } else {
            info.m_flip = ICGraphics2d::Flip_None;
        }
        image = m_transposedImages[GetScaleIndex(scale)];
    } else {
        if (mirrorVertical) {
            if (transform & SquareTransform_MirrorHorizontal) {
                info.m_flip = ICGraphics2d::Flip_Both;
            } else {
                info.m_flip = ICGraphics2d::Flip_Vertical;
            }
        } else {
            if (transform & SquareTransform_MirrorHorizontal) {
                info.m_flip = ICGraphics2d::Flip_Horizontal;
            } else {
                info.m_flip = ICGraphics2d::Flip_None;
            }
        }
        image = m_images[GetScaleIndex(scale)];
    }
}

unsigned char SG_TextureAtlas::LoadPage(SG_Tint tint, unsigned char transform)
{
    SG_Image& image = GetImage(tint);
    SG_SrcImage* source = NULL;
    if (!image.IsTransformGroupLoaded(transform)) {
        // two surfaces per image: normal, then transposed
        source = new SG_SrcImage();
        ICResourceManager* manager = (ICResourceManager*)CApplet::m_pApp->m_resourceManager;
        ICResourceManager::CConsecutiveResourceIdItr* iterator =
            manager->CreateAndInitConsecutiveResourceIdItr(m_surfacePrefix, m_imageCount * 2);
        int surface = tint.GetImageIndex(m_maps) * 2 + SquareTransform_IsTransposed(transform);
        const char* name =
            iterator->Seek(ICResourceManager::CConsecutiveResourceIdItr::SeekFrom_Current, surface).GetName();
        delete iterator;
        CResource* resource = NULL;
        ((ICResourceManager*)CApplet::m_pApp->m_resourceManager)->CreateResource(name, &resource);
        source->m_surface = (ICRenderSurface*)resource->GetData();
        source->m_resourceName = name;
    }
    return image.LoadTransform(transform, source);
}

void SG_TextureAtlas::IncreaseUssagesCount(SG_Tint tint, unsigned char transform)
{
    GetUsageCount(tint, transform)++;
}

// ---------------------------------------------------------------------------------------------
// SG_Tint / SG_SrcImage / SG_Image

SG_Tint::SG_Tint()
{
    m_offset = -1;
}

unsigned char SG_Tint::Load(DataInputStream& stream)
{
    m_map = stream.ReadUInt16();
    m_offset = stream.ReadInt8();
    return stream.m_error == 0;
}

SG_SrcImage::~SG_SrcImage()
{
    if (m_surface != NULL) {
        ((ICResourceManager*)CApplet::m_pApp->m_resourceManager)->ReleaseResource(m_resourceName);
    }
    m_resourceName = NULL;
}

SG_Image::SG_Image()
{
    m_loadedTransforms = 0;
    m_scales.Add(DEFAULT_SCALE_PERCENT);
}

SG_Image::~SG_Image()
{
}

void SG_Image::ClearImageTransposedVectors()
{
    for (int index = 0; index < m_transposedImages.Size(); index++) {
        if (m_transposedImages[index] != NULL) {
            delete m_transposedImages[index];
            m_transposedImages[index] = NULL;
        }
    }
    m_transposedImages.Clear();
}

void SG_Image::ClearImageVectors()
{
    for (int index = 0; index < m_images.Size(); index++) {
        if (m_images[index] != NULL) {
            delete m_images[index];
            m_images[index] = NULL;
        }
    }
    m_images.Clear();
}

// ---------------------------------------------------------------------------------------------
// SG_Spritemap / SG_Layer / SG_Animation

SG_Spritemap::SG_Spritemap()
{
    m_count = 0;
    m_entries = NULL;
}

SG_Spritemap::~SG_Spritemap()
{
    if (m_entries != NULL) {
        delete[] m_entries;
        m_entries = NULL;
    }
}

unsigned char SG_Spritemap::Load(DataInputStream& stream)
{
    m_count = stream.ReadUInt16();
    if (m_count == 0) {
        return 1;
    }
    m_entries = new SG_SpritemapEntry[m_count];
    if (m_entries == NULL) {
        return 0;
    }
    for (int index = 0; index < m_count; index++) {
        m_entries[index].m_sprite = stream.ReadUInt16();
        m_entries[index].m_replacement = stream.ReadUInt16();
        m_entries[index].m_mode = stream.ReadUInt8();
        m_entries[index].m_x = stream.ReadInt16();
        m_entries[index].m_y = stream.ReadInt16();
    }
    return stream.m_error == 0;
}

unsigned char SG_Layer::Load(DataInputStream& stream)
{
    m_count = stream.ReadUInt8();
    if (m_count == 0) {
        return 1;
    }
    m_entries = new SG_LayerEntry[m_count];
    if (m_entries == NULL) {
        return 0;
    }
    for (int index = 0; index < m_count; index++) {
        m_entries[index].m_sprite = stream.ReadUInt16();
        m_entries[index].m_x = stream.ReadInt16();
        m_entries[index].m_y = stream.ReadInt16();
        m_entries[index].m_rotation = stream.ReadInt32();
        m_entries[index].m_scaleX = stream.ReadInt32();
        m_entries[index].m_scaleY = stream.ReadInt32();
    }
    return stream.m_error == 0;
}

SG_Animation::~SG_Animation()
{
    if (m_frameLayers != NULL) {
        delete[] m_frameLayers;
        m_frameLayers = NULL;
    }
    if (m_frameDurations != NULL) {
        delete[] m_frameDurations;
        m_frameDurations = NULL;
    }
}

unsigned char SG_Animation::Load(DataInputStream& stream)
{
    m_id = stream.ReadUInt8();
    m_frameCount = stream.ReadUInt8();
    if (m_frameCount == 0) {
        return 1;
    }
    m_frameLayers = new unsigned short[m_frameCount];
    m_frameDurations = new unsigned short[m_frameCount];
    if (m_frameLayers == NULL || m_frameDurations == NULL) {
        return 0;
    }
    for (unsigned int frame = 0; frame < m_frameCount; frame++) {
        m_frameLayers[frame] = stream.ReadUInt16();
        m_frameDurations[frame] = stream.ReadUInt16() * 10;     // stored in 1/100 s
    }
    return stream.m_error == 0;
}

// ---------------------------------------------------------------------------------------------
// SG_Character / SG_Archetype

SG_Character::~SG_Character()
{
}

unsigned short SG_Character::GetScaleFactorForCharacter(unsigned short scale)
{
    int count = m_scales.Size();
    if (count < 1) {
        return scale;
    }
    for (int index = 0; index < count; index++) {
        unsigned short smaller = m_scales[index];
        if (smaller < scale) {
            // between the previous (larger) scale and this one: take the closer
            unsigned short larger = DEFAULT_SCALE_PERCENT;
            if (index != 0) {
                larger = m_scales[index - 1];
            }
            unsigned short distanceDown = scale - smaller;
            unsigned short distanceUp = larger - scale;
            if (distanceUp > distanceDown) {
                return smaller;
            }
            return larger;
        }
    }
    return m_scales[count - 1];
}

SG_Archetype::SG_Archetype()
{
    m_layerCount = 0;
    m_layers = NULL;
    m_animationCount = 0;
    m_animations = NULL;
    m_characterCount = 0;
    m_characters = NULL;
}

SG_Archetype::~SG_Archetype()
{
    if (m_layers != NULL) {
        delete[] m_layers;
        m_layers = NULL;
    }
    if (m_animations != NULL) {
        delete[] m_animations;
        m_animations = NULL;
    }
    if (m_characters != NULL) {
        delete[] m_characters;
        m_characters = NULL;
    }
}

unsigned char SG_Archetype::LoadLayers(DataInputStream& stream)
{
    m_layerCount = stream.ReadUInt16();
    if (m_layerCount == 0) {
        return 1;
    }
    m_layers = new SG_Layer[m_layerCount];
    for (int layer = 0; layer < m_layerCount; layer++) {
        m_layers[layer].Load(stream);
    }
    return stream.m_error == 0;
}

unsigned char SG_Archetype::LoadAnimations(DataInputStream& stream)
{
    m_animationCount = stream.ReadUInt16();
    if (m_animationCount == 0) {
        return 1;
    }
    m_animations = new SG_Animation[m_animationCount];
    for (unsigned int animation = 0; animation < m_animationCount; animation++) {
        m_animations[animation].Load(stream);
    }
    return stream.m_error == 0;
}

unsigned char SG_Archetype::LoadCharacters(DataInputStream& stream, unsigned short spriteCount)
{
    m_characterCount = stream.ReadUInt8();
    if (m_characterCount == 0) {
        return 1;
    }
    m_characters = new SG_Character[m_characterCount];
    for (unsigned int index = 0; index < m_characterCount; index++) {
        SG_Character& character = m_characters[index];
        character.m_loaded = 0;
        character.m_sprites.Init(spriteCount, &stream);
        character.m_spritemap = stream.ReadInt8();
    }
    return stream.m_error == 0;
}
