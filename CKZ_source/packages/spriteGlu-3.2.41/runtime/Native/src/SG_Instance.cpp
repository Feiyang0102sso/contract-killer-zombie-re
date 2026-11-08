// SG_Instance.cpp - loading, unloading and per frame accumulation of a spriteGlu resource set
// (see SG_Instance.h). (written by b042)
#include <ctype.h>
#include "SG_Instance.h"
#include "SG_Presenter.h"
#include "CApplet.h"
#include "CApp.h"
#include "CStdUtil_Cocoa.h"
#include "ICResourceManager.h"

using namespace com::glu::platform;
using com::glu::platform::components::CRectangle;
using com::glu::platform::components::CStrChar;
using com::glu::platform::core::CVector;
using com::glu::platform::systems::ICResourceManager;

// ---- constants ----
static const char* const DEFAULT_PREFIX = "SPRITEGLU__";       // resource name prefix without a set name
static const char* const NAMED_PREFIX_SUFFIX = "_SPRITEGLU__";  // appended to a set name
static const int SPRITE_STATES_PER_SPRITE = 8;
static const int FIXED_ONE = 0x10000;                           // 1.0 in 16.16 fixed point
static const short BOUNDS_EMPTY_MIN = 0x7fff;
static const short BOUNDS_EMPTY_MAX = -0x8000;

SG_Instance::SG_Instance()
{
    m_unused30 = 0;
    m_mapCount = 0;
    m_tintCount = 0;
    m_tints = NULL;
    m_imagespriteCount = 0;
    m_imagesprites = NULL;
    m_spriteStates = NULL;
    m_rectspriteCount = 0;
    m_rectsprites = NULL;
    m_spriteCount = 0;
    m_spritemapCount = 0;
    m_spritemaps = NULL;
    m_archetypeCount = 0;
    m_archetypes = NULL;
    m_atlas = NULL;
}

SG_Instance::~SG_Instance()
{
    Destroy();
}

void SG_Instance::Destroy()
{
    if (m_archetypes == NULL) {
        return;
    }
    m_atlas->DumpAllTransforms();
    DumpAllArchetypes();
    if (m_archetypes != NULL) {
        delete[] m_archetypes;
        m_archetypes = NULL;
    }
    if (m_spritemaps != NULL) {
        delete[] m_spritemaps;
        m_spritemaps = NULL;
    }
    if (m_tints != NULL) {
        delete[] m_tints;
        m_tints = NULL;
    }
    if (m_atlas != NULL) {
        delete m_atlas;
        m_atlas = NULL;
    }
    if (m_imagesprites != NULL) {
        delete[] m_imagesprites;
        m_imagesprites = NULL;
    }
    if (m_spriteStates != NULL) {
        delete[] m_spriteStates;
        m_spriteStates = NULL;
    }
    if (m_rectsprites != NULL) {
        delete[] m_rectsprites;
        m_rectsprites = NULL;
    }
}

unsigned char SG_Instance::IsInitialized()
{
    return m_archetypes != NULL;
}

void SG_Instance::Init(const char* name)
{
    if (m_archetypes != NULL) {
        return;
    }

    // resource names: "<NAME>_SPRITEGLU__..." or "SPRITEGLU__...", upper case
    CStrChar prefix;
    prefix.ReleaseMemory();
    if (name == NULL) {
        prefix.Concatenate(DEFAULT_PREFIX);
    } else {
        prefix.Concatenate(name);
        prefix.Concatenate(NAMED_PREFIX_SUFFIX);
    }
    for (char* character = prefix.m_string; *character != 0; character++) {
        *character = toupper(*character);
    }
    m_globalResource = prefix + "BINARY_GLOBAL";
    m_globalName = m_globalResource.m_string;
    m_archetypeResource = prefix + "BINARY_ARCHETYPE_000";
    m_archetypeName = m_archetypeResource.m_string;
    m_surfaceResource = prefix + "SURFACE_00000";
    m_surfaceName = m_surfaceResource.m_string;

    DataInputStream stream(m_globalName);
    m_version = stream.ReadUInt8();

    m_mapCount = stream.ReadUInt16();
    if (m_mapCount != 0) {
        unsigned int imageCount = stream.ReadUInt16();
        m_atlas = new SG_TextureAtlas(imageCount, m_surfaceName, m_mapCount, stream);
    }

    m_tintCount = stream.ReadUInt16();
    if (m_tintCount != 0) {
        m_tints = new SG_Tint[m_tintCount];
        for (int tint = 0; tint < m_tintCount; tint++) {
            m_tints[tint].Load(stream);
        }
    }

    m_imagespriteCount = stream.ReadUInt16();
    if (m_imagespriteCount != 0) {
        m_imagesprites = new SG_Imagesprite[m_imagespriteCount];
        m_spriteStates = new unsigned short[m_imagespriteCount * SPRITE_STATES_PER_SPRITE];
        for (unsigned short sprite = 0; sprite < m_imagespriteCount; sprite++) {
            m_imagesprites[sprite].Load(stream);
            for (unsigned short state = 0; state < SPRITE_STATES_PER_SPRITE; state++) {
                m_spriteStates[sprite * SPRITE_STATES_PER_SPRITE + state] = 0;
            }
        }
    }

    m_rectspriteCount = stream.ReadUInt16();
    if (m_rectspriteCount != 0) {
        m_rectsprites = new SG_Rectsprite[m_rectspriteCount];
        for (unsigned short sprite = 0; sprite < m_rectspriteCount; sprite++) {
            m_rectsprites[sprite].Load(stream);
        }
    }
    m_spriteCount = m_imagespriteCount + m_rectspriteCount;

    m_spritemapCount = stream.ReadUInt8();
    if (m_spritemapCount != 0) {
        m_spritemaps = new SG_Spritemap[m_spritemapCount];
        for (unsigned short spritemap = 0; spritemap < m_spritemapCount; spritemap++) {
            m_spritemaps[spritemap].Load(stream);
        }
    }

    // archetypes are loaded on demand
    m_archetypeCount = stream.ReadUInt8();
    m_archetypes = new SG_Archetype*[m_archetypeCount];
    memset(m_archetypes, 0, m_archetypeCount * sizeof(SG_Archetype*));

    m_queue.Init(this);
    stream.Close();
}

void ResourceQueue::Init(SG_Instance* owner)
{
    m_owner = owner;
    m_state = State_Idle;
    m_spritesToLoad = new BitVector(m_owner->m_imagespriteCount);
    m_progress.m_current = m_owner->m_imagespriteCount;
    m_queuedCount = 0;
    m_loadedCount = 0;
}

void SG_Instance::DumpAllArchetypes()
{
    for (int archetype = 0; archetype < m_archetypeCount; archetype++) {
        DumpArchetype(archetype);
    }
}

unsigned char SG_Instance::IsLoadedArchetypeCharacter(int archetype, int character)
{
    SG_Archetype* loaded = m_archetypes[(unsigned char)archetype];
    if (loaded == NULL) {
        return 0;
    }
    return loaded->m_characters[character].m_loaded != 0;
}

unsigned char SG_Instance::LoadQueued(unsigned int timeBudget, unsigned char& finished)
{
    return m_queue.LoadQueued(timeBudget, finished);
}

unsigned char ResourceQueue::LoadQueued(unsigned int timeBudget, unsigned char& finished)
{
    finished = 1;
    if (timeBudget == 0) {
        finished = 0;
        return 0;
    }
    if (m_state == State_Idle) {
        return 0;
    }
    CVector<int>& archetypes = m_archetypes;
    CVector<int>& characters = m_characters;
    CVector< CVector<unsigned short> >& scaleLists = m_scales;
    if (m_state == State_Queued) {
        m_state = State_Loading;
        m_queuedCount = characters.Size();
        m_loadedCount = 0;
        m_loadTimeMs = 0;
    }

    unsigned char result;
    if (characters.Size() > 0) {
        // one character per call, last queued first
        unsigned int start = CStdUtil_Cocoa::GetUpTimeMS();
        CVector<unsigned short> scales(scaleLists[scaleLists.Size() - 1]);
        unsigned char archetype = archetypes[archetypes.Size() - 1];
        archetypes.RemoveAt(archetypes.Size() - 1);
        unsigned char character = characters[characters.Size() - 1];
        characters.RemoveAt(characters.Size() - 1);
        scaleLists.RemoveAt(scaleLists.Size() - 1);
        if (!m_owner->LoadArchetypeCharacter(archetype, character, scales)) {
            result = 0;
            finished = 0;
        } else {
            m_loadedCount++;
            m_loadTimeMs += CStdUtil_Cocoa::GetUpTimeMS() - start;
            result = 1;
            if (characters.Size() <= 0 && m_spritesToLoad->GetOnCount() == 0) {
                result = 0;
                m_state = State_Idle;
            }
        }
    } else {
        // then the image sprites of every loaded character
        result = 1;
        if (!LoadQueuedSprites(timeBudget, finished)) {
            result = 0;
            m_state = State_Idle;
        }
    }
    return result;
}

void SG_Instance::QueueArchetypeCharacter(int archetype, int character, unsigned short* scales,
                                          unsigned short scaleCount)
{
    if (IsLoadedArchetypeCharacter(archetype, character)) {
        return;
    }
    m_queue.QueueArchetypeCharacter(archetype, character, scales, scaleCount);
}

unsigned char ResourceQueue::QueueArchetypeCharacter(unsigned char archetype, unsigned char character,
                                                     unsigned short* scales, unsigned short scaleCount)
{
    CVector<int>& archetypes = m_archetypes;
    CVector<int>& characters = m_characters;
    CVector< CVector<unsigned short> >& scaleLists = m_scales;
    for (int index = 0; index < archetypes.Size(); index++) {
        if (archetype == archetypes[index] && character == characters[index]) {
            return 0;
        }
    }
    if (m_state == State_Idle) {
        m_state = State_Queued;
    }
    archetypes.Add(archetype);
    characters.Add(character);
    CVector<unsigned short> scaleList;
    if (scales != NULL && scaleCount != 0) {
        for (int index = 0; index < scaleCount; index++) {
            scaleList.Add(scales[index]);
        }
    }
    scaleLists.Add(scaleList);
    return 1;
}

void SG_Instance::DumpArchetypeCharacter(int archetype, int character)
{
    unsigned char archetypeIndex = archetype;
    SG_Archetype* loaded = m_archetypes[archetypeIndex];
    if (loaded == NULL) {
        return;
    }
    if (!DumpCharacter(archetypeIndex, character)) {
        return;
    }
    // the archetype goes with its last loaded character
    for (int index = 0; index < loaded->m_characterCount; index++) {
        if (loaded->m_characters[index].m_loaded) {
            return;
        }
    }
    DumpArchetype(archetypeIndex);
}

unsigned char SG_Instance::DumpCharacter(unsigned char archetype, unsigned char character)
{
    SG_Character& dumped = m_archetypes[archetype]->m_characters[character];
    if (!dumped.m_loaded) {
        return 0;
    }
    dumped.m_loaded = 0;
    for (int sprite = 0; sprite < m_imagespriteCount; sprite++) {
        if (dumped.m_sprites.IsMember(sprite)) {
            SG_Imagesprite& imagesprite = GetImagesprite(sprite);
            m_atlas->DumpImageTransform(m_tints[imagesprite.m_tint], imagesprite.m_transform);
        }
    }
    return 1;
}

void SG_Instance::DumpArchetype(unsigned char archetype)
{
    SG_Archetype* loaded = m_archetypes[archetype];
    if (loaded != NULL) {
        delete loaded;
        m_archetypes[archetype] = NULL;
    }
}

void SG_Instance::DumpAllArchetypeCharacters()
{
    m_queue.Clean();
    m_atlas->DumpAllTransforms();
    DumpAllArchetypes();
}

void ResourceQueue::Clean()
{
    m_state = State_Idle;
    if (m_spritesToLoad != NULL) {
        delete m_spritesToLoad;
        m_spritesToLoad = NULL;
    }
    m_archetypes.Clear();
    m_characters.Clear();
    m_scales.Clear();
    m_spritesToLoad = new BitVector(m_owner->m_imagespriteCount);
    m_progress.m_current = m_owner->m_imagespriteCount;
    m_queuedCount = 0;
    m_loadedCount = 0;
}

unsigned char SG_Instance::GetTag(int sprite)
{
    if (sprite < m_imagespriteCount) {
        return m_imagesprites[(unsigned short)sprite].m_tag;
    }
    return m_rectsprites[(unsigned short)(sprite - m_imagespriteCount)].m_tag;
}

void SG_Instance::GetSize(int sprite, int& width, int& height)
{
    if (sprite < m_imagespriteCount) {
        SG_Imagesprite& imagesprite = GetImagesprite(sprite);
        m_atlas->GetImageSize(m_tints[imagesprite.m_tint], imagesprite.m_transform, width, height);
    } else {
        SG_Rectsprite& rectsprite = GetRectsprite(sprite - m_imagespriteCount);
        width = rectsprite.m_width;
        height = rectsprite.m_height;
    }
}

unsigned char SG_Instance::IsCollisionSprite(int sprite)
{
    if (sprite < m_imagespriteCount) {
        return 0;
    }
    return GetRectsprite(sprite - m_imagespriteCount).m_color == -1;
}

unsigned char SG_Instance::LoadArchetypeCharacter(unsigned char archetype, unsigned char character,
                                                  CVector<unsigned short> scales)
{
    if (!LoadArchetype(archetype)) {
        return 0;
    }
    LoadCharacter(archetype, character, scales);
    return 1;
}

unsigned char SG_Instance::LoadArchetype(unsigned char archetype)
{
    if (m_archetypes[archetype] != NULL) {
        return 1;
    }
    m_archetypes[archetype] = new SG_Archetype();
    SG_Archetype* loaded = m_archetypes[archetype];

    // archetype resources follow each other after the first one
    ICResourceManager* manager = (ICResourceManager*)CApplet::m_pApp->m_resourceManager;
    ICResourceManager::CConsecutiveResourceIdItr* iterator =
        manager->CreateAndInitConsecutiveResourceIdItr(m_archetypeName, m_archetypeCount);
    const char* name =
        iterator->Seek(ICResourceManager::CConsecutiveResourceIdItr::SeekFrom_Current, archetype).GetName();
    DataInputStream stream(name);
    delete iterator;

    unsigned char result = 0;
    if (!stream.m_error) {
        loaded->LoadLayers(stream);
        loaded->LoadAnimations(stream);
        loaded->LoadCharacters(stream, m_imagespriteCount);
        stream.Close();
        result = 1;
    }
    return result;
}

void SG_Instance::LoadCharacter(unsigned char archetype, unsigned char character, CVector<unsigned short> scales)
{
    SG_Character& loaded = m_archetypes[archetype]->m_characters[character];
    if (loaded.m_loaded) {
        return;
    }
    loaded.m_loaded = 1;
    for (int sprite = 0; sprite < m_imagespriteCount; sprite++) {
        SG_Imagesprite* imagesprites = m_imagesprites;
        if (!loaded.m_sprites.IsMember(sprite)) {
            continue;
        }
        unsigned short index = sprite;
        unsigned char transform = imagesprites[index].m_transform;
        m_atlas->IncreaseUssagesCount(m_tints[imagesprites[index].m_tint], transform);
        if (m_atlas->IsImageTransformLoaded(m_tints[imagesprites[index].m_tint], transform)) {
            continue;
        }
        if (m_atlas->LoadTransformRequiresRawImage(m_tints[imagesprites[index].m_tint], transform)) {
            // the surface is loaded later, a slice at a time (ResourceQueue::LoadQueuedSprites)
            m_queue.m_spritesToLoad->SetMember(sprite, 1);
        } else {
            m_atlas->LoadImageTransform(m_tints[imagesprites[index].m_tint], transform);
        }
    }
}

void SG_Instance::computeSpriteDrawInfos(SpriteDrawInfo*& infos, unsigned int& count, unsigned int archetype,
                                         unsigned int character, unsigned short sprite)
{
    unsigned char spritemapIndex = m_archetypes[(unsigned char)archetype]->m_characters[character].m_spritemap;
    if (spritemapIndex == SG_Character::NoSpritemap) {
        SetSingleDrawInfo(infos, count, sprite, 0, 0);
        return;
    }

    // binary search of the sprite map entries (sorted by sprite)
    SG_Spritemap& spritemap = m_spritemaps[spritemapIndex];
    SG_SpritemapEntry* entries = spritemap.m_entries;
    int found = -1;
    int low = 0;
    int high = spritemap.m_count - 1;
    while (low <= high) {
        int middle = (low + high) >> 1;
        if (entries[middle].m_sprite == sprite) {
            found = middle;
            break;
        } else if (entries[middle].m_sprite > sprite) {
            high = middle - 1;
        } else if (entries[middle].m_sprite < sprite) {
            low = middle + 1;
        }
    }
    if (found == -1) {
        SetSingleDrawInfo(infos, count, sprite, 0, 0);
        return;
    }

    SG_SpritemapEntry entry = entries[found];
    if (entry.m_mode == 0) {
        SetSingleDrawInfo(infos, count, entry.m_replacement, entry.m_x, entry.m_y);
    } else {
        // mode 1: replacement drawn over the sprite, otherwise under it
        int spriteSlot = entry.m_mode != 1;
        int replacementSlot = spriteSlot ^ 1;
        m_pairInfos[spriteSlot].m_sprite = sprite;
        m_pairInfos[spriteSlot].m_x = 0;
        m_pairInfos[spriteSlot].m_y = 0;
        m_pairInfos[replacementSlot].m_sprite = entry.m_replacement;
        m_pairInfos[replacementSlot].m_x = entry.m_x;
        m_pairInfos[replacementSlot].m_y = entry.m_y;
        infos = m_pairInfos;
        count = 2;
    }
}

void SG_Instance::drawInit(int x, int y, const CRectangle<short>& clip)
{
    m_graphics.SetOffset(x, y);
    m_graphics.SetClip(clip);
}

void SG_Instance::drawAccumulate(int x, int y, unsigned int width, unsigned int height, unsigned short sprite,
                                 unsigned char presenterTransform, unsigned short scalePercent, int scale,
                                 int rotation, int scaleX, int scaleY)
{
    if (sprite < m_imagespriteCount) {
        SG_Imagesprite& imagesprite = m_imagesprites[sprite];
        unsigned char transform = SquareTransform_Compose(imagesprite.m_transform, presenterTransform);
        SG_SrcImage* image = NULL;
        Graphics::RenderInfo info;
        info.m_flip = 0;
        CRectangle<short> region = CRectangle<short>();
        m_atlas->GetSrcImageAndRenderInfo(m_tints[imagesprite.m_tint], transform, image, info, region);
        m_graphics.DrawRegion(image, info, x, y, scale, &region, width, height, rotation, scaleX, scaleY);
    } else {
        SG_Rectsprite& rectsprite = m_rectsprites[sprite - m_imagespriteCount];
        if (rectsprite.m_color == -1) {
            return;     // collision boxes are not drawn
        }
        m_graphics.FillRect(x, y, width * scalePercent / 100, height * scalePercent / 100, rectsprite.m_color,
                            rotation, scaleX, scaleY);
    }
}

void SG_Instance::drawResolve(SG_Presenter& presenter)
{
}

void SG_Instance::translateRect(int& x, int& y, int& width, int& height, int rotation, int scaleX, int scaleY)
{
    // corners of the rectangle
    int cornersX[4];
    int cornersY[4];
    cornersX[0] = x;
    cornersY[0] = y;
    cornersY[1] = y;
    cornersX[1] = x + width;
    cornersX[2] = x + width;
    cornersY[2] = y + height;
    cornersY[3] = y + height;
    cornersX[3] = x;

    int minX = cornersX[0];
    int maxX = cornersX[0];
    int minY = cornersY[0];
    int maxY = cornersY[0];
    for (int corner = 1; corner < 4; corner++) {
        if (cornersX[corner] > maxX) {
            maxX = cornersX[corner];
        } else if (cornersX[corner] < minX) {
            minX = cornersX[corner];
        }
        if (cornersY[corner] > maxY) {
            maxY = cornersY[corner];
        } else if (cornersY[corner] < minY) {
            minY = cornersY[corner];
        }
    }
    x = minX;
    y = minY;
    width = maxX - minX;
    height = maxY - minY;
}

void SG_Instance::boundsInit()
{
    m_boundsMinX = BOUNDS_EMPTY_MIN;
    m_boundsMinY = BOUNDS_EMPTY_MIN;
    m_boundsMaxX = BOUNDS_EMPTY_MAX;
    m_boundsMaxY = BOUNDS_EMPTY_MAX;
}

void SG_Instance::boundsAccumulate(int x, int y, int width, int height, int rotation, int scaleX, int scaleY)
{
    if (scaleX != FIXED_ONE || scaleY != FIXED_ONE || rotation != 0) {
        translateRect(x, y, width, height, rotation, scaleX, scaleY);
    }
    // ?: on purpose: the bounds are stored unconditionally (selects in the original)
    m_boundsMinX = (m_boundsMinX > x) ? x : m_boundsMinX;
    m_boundsMaxX = (m_boundsMaxX < x + width) ? x + width : m_boundsMaxX;
    m_boundsMinY = (m_boundsMinY > y) ? y : m_boundsMinY;
    m_boundsMaxY = (m_boundsMaxY < y + height) ? y + height : m_boundsMaxY;
}

void SG_Instance::boundsResolve(CRectangle<short>& bounds, SG_Presenter& presenter)
{
    bounds.m_x = presenter.m_x + m_boundsMinX;
    bounds.m_y = presenter.m_y + m_boundsMinY;
    bounds.m_width = m_boundsMaxX - m_boundsMinX;
    bounds.m_height = m_boundsMaxY - m_boundsMinY;
}

void SG_Instance::collisionInit(int x, int y, int width, int height, int tag, int sprite)
{
    m_collisionX = x;
    m_collisionY = y;
    m_collisionWidth = width;
    m_collisionHeight = height;
    m_collisionTag = tag;
    m_collisionSprite = sprite;
}

void SG_Instance::collisionAccumulate(int x, int y, int width, int height, int sprite, int rotation, int scaleX,
                                      int scaleY)
{
    if (scaleX != FIXED_ONE || scaleY != FIXED_ONE || rotation != 0) {
        translateRect(x, y, width, height, rotation, scaleX, scaleY);
    }
    if (m_collisionX + m_collisionWidth > x && m_collisionX < x + width &&
        m_collisionY + m_collisionHeight > y && m_collisionY < y + height) {
        m_collisionSprite = sprite;
    }
}

ResourceQueue::~ResourceQueue()
{
    if (m_spritesToLoad != NULL) {
        delete m_spritesToLoad;
        m_spritesToLoad = NULL;
    }
}

unsigned char ResourceQueue::LoadQueuedSprites(unsigned int timeBudget, unsigned char& finished)
{
    if (m_progress.m_current == m_owner->m_imagespriteCount) {
        // start a new pass
        m_progress = Progress();
        m_spriteCount = m_owner->m_imagespriteCount;
        m_spritesToLoadCount = m_spritesToLoad->GetOnCount();
        m_spriteLoadTimeMs = 0;
        m_spritePasses = 0;
    }
    m_spritePasses++;

    unsigned int start = CStdUtil_Cocoa::GetUpTimeMS();
    unsigned int elapsed = 0;
    for (; m_progress.m_current < m_owner->m_imagespriteCount; m_progress.m_current++) {
        m_progress.m_steps++;
        if (elapsed >= timeBudget) {
            m_spriteLoadTimeMs += elapsed;
            return 1;
        }
        if (m_spritesToLoad->IsMember(m_progress.m_current)) {
            m_progress.m_queued++;
            SG_Imagesprite& imagesprite = m_owner->m_imagesprites[m_progress.m_current];
            if (!m_owner->m_atlas->LoadPage(m_owner->m_tints[imagesprite.m_tint], imagesprite.m_transform)) {
                m_progress.m_alreadyLoaded++;
            } else {
                m_progress.m_loaded++;
            }
            m_spritesToLoad->SetMember(m_progress.m_current, 0);
        }
        elapsed = CStdUtil_Cocoa::GetUpTimeMS() - start;
    }
    m_spriteLoadTimeMs += elapsed;
    return 0;
}
