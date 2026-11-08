// SG_Instance.h - one loaded spriteGlu resource set: sprites, tints, sprite maps, the texture
// atlas and the archetypes (loaded on demand through a time sliced ResourceQueue), plus the
// per frame accumulators used by SG_Presenter to draw, measure and collide. (created by b042)
#ifndef SG_INSTANCE_H
#define SG_INSTANCE_H

#include "SG_Defines.h"
#include "CStrChar.h"

class SG_Instance;
class SG_Presenter;

/** Archetype characters waiting to be loaded, then their image sprites, a slice at a time. */
class ResourceQueue {
public:
    /** m_state values. */
    enum State {
        State_Idle = 0,
        State_Queued = 1,
        State_Loading = 2
    };

    /** Progress of the sprite pass (LoadQueuedSprites); reset as a whole. */
    struct Progress {
        unsigned short m_current;       // 0x00 next image sprite
        unsigned short m_steps;         // 0x02 loop iterations
        unsigned short m_queued;        // 0x04 sprites that needed loading
        unsigned short m_loaded;        // 0x06 transforms newly loaded
        unsigned short m_alreadyLoaded; // 0x08
    };

    ResourceQueue()
    {
        m_spritesToLoad = NULL;
    }

    ~ResourceQueue();

    void Init(SG_Instance* owner);
    /** Load queued work for up to `timeBudget` ms; 0 when nothing is left. */
    unsigned char LoadQueued(unsigned int timeBudget, unsigned char& finished);
    /** Queue a character; 0 when it is queued already. */
    unsigned char QueueArchetypeCharacter(unsigned char archetype, unsigned char character,
                                          unsigned short* scales, unsigned short scaleCount);
    /** Forget all queued work. */
    void Clean();
    /** Load the image sprites marked in m_spritesToLoad; 0 when all were visited. */
    unsigned char LoadQueuedSprites(unsigned int timeBudget, unsigned char& finished);

    SG_Instance* m_owner;                                                           // 0x00
    int m_state;                                                                    // 0x04 State
    unsigned short m_queuedCount;                                                   // 0x08 characters at start
    unsigned short m_loadedCount;                                                   // 0x0a characters loaded
    com::glu::platform::core::CVector<int> m_archetypes;                            // 0x0c
    com::glu::platform::core::CVector<int> m_characters;                            // 0x24
    com::glu::platform::core::CVector< com::glu::platform::core::CVector<unsigned short> > m_scales; // 0x3c
    int m_loadTimeMs;                                                               // 0x54
    BitVector* m_spritesToLoad;                                                     // 0x58 image sprites still to load
    Progress m_progress;                                                            // 0x5c
    unsigned short m_spriteCount;                                                   // 0x66
    unsigned short m_spritesToLoadCount;                                            // 0x68
    unsigned int m_spriteLoadTimeMs;                                                // 0x6c
    unsigned short m_spritePasses;                                                  // 0x70
};

class SG_Instance {
public:
    SG_Instance();
    ~SG_Instance();

    void Destroy();
    unsigned char IsInitialized();
    /** Load the global resource of the set named `name` (NULL = the default set). */
    void Init(const char* name);
    void DumpAllArchetypes();
    unsigned char IsLoadedArchetypeCharacter(int archetype, int character);
    unsigned char LoadQueued(unsigned int timeBudget, unsigned char& finished);
    void QueueArchetypeCharacter(int archetype, int character, unsigned short* scales, unsigned short scaleCount);
    void DumpArchetypeCharacter(int archetype, int character);
    unsigned char DumpCharacter(unsigned char archetype, unsigned char character);
    void DumpArchetype(unsigned char archetype);
    void DumpAllArchetypeCharacters();
    unsigned char GetTag(int sprite);
    void GetSize(int sprite, int& width, int& height);
    unsigned char IsCollisionSprite(int sprite);
    unsigned char LoadArchetypeCharacter(unsigned char archetype, unsigned char character,
                                         com::glu::platform::core::CVector<unsigned short> scales);
    unsigned char LoadArchetype(unsigned char archetype);
    void LoadCharacter(unsigned char archetype, unsigned char character,
                       com::glu::platform::core::CVector<unsigned short> scales);
    /** Sprites to draw for `sprite` after the character's sprite map substitution. */
    void computeSpriteDrawInfos(SpriteDrawInfo*& infos, unsigned int& count, unsigned int archetype,
                                unsigned int character, unsigned short sprite);
    void drawInit(int x, int y, const com::glu::platform::components::CRectangle<short>& clip);
    void drawAccumulate(int x, int y, unsigned int width, unsigned int height, unsigned short sprite,
                        unsigned char presenterTransform, unsigned short scalePercent, int scale,
                        int rotation, int scaleX, int scaleY);
    void drawResolve(SG_Presenter& presenter);
    /** Bounding box of the rectangle after rotation / scaling (not applied yet). */
    static void translateRect(int& x, int& y, int& width, int& height, int rotation, int scaleX, int scaleY);
    void boundsInit();
    void boundsAccumulate(int x, int y, int width, int height, int rotation, int scaleX, int scaleY);
    void boundsResolve(com::glu::platform::components::CRectangle<short>& bounds, SG_Presenter& presenter);
    void collisionInit(int x, int y, int width, int height, int tag, int sprite);
    void collisionAccumulate(int x, int y, int width, int height, int sprite, int rotation, int scaleX,
                             int scaleY);

    /** Result of computeSpriteDrawInfos: the one sprite `sprite` at (x, y). */
    void SetSingleDrawInfo(SpriteDrawInfo*& infos, unsigned int& count, unsigned short sprite, short x, short y)
    {
        m_singleInfo.m_sprite = sprite;
        m_singleInfo.m_x = x;
        m_singleInfo.m_y = y;
        infos = &m_singleInfo;
        count = 1;
    }

    SG_Imagesprite& GetImagesprite(unsigned short sprite)
    {
        return m_imagesprites[sprite];
    }

    /** Rect sprites are numbered after the image sprites. */
    SG_Rectsprite& GetRectsprite(unsigned short rectSprite)
    {
        return m_rectsprites[rectSprite];
    }

    com::glu::platform::components::CStrChar m_globalResource;     // 0x00
    com::glu::platform::components::CStrChar m_archetypeResource;  // 0x10
    com::glu::platform::components::CStrChar m_surfaceResource;    // 0x20
    int m_unused30;                         // 0x30 cleared, never read
    unsigned short m_mapCount;              // 0x34 texture maps
    unsigned char m_version;                // 0x36 first byte of the global resource
    SG_TextureAtlas* m_atlas;               // 0x38
    unsigned short m_tintCount;             // 0x3c
    SG_Tint* m_tints;                       // 0x40
    unsigned short m_imagespriteCount;      // 0x44
    SG_Imagesprite* m_imagesprites;         // 0x48
    unsigned short* m_spriteStates;         // 0x4c 8 per image sprite, cleared on load
    unsigned short m_rectspriteCount;       // 0x50
    SG_Rectsprite* m_rectsprites;           // 0x54
    unsigned short m_spriteCount;           // 0x58 image + rect sprites
    unsigned char m_spritemapCount;         // 0x5a
    SG_Spritemap* m_spritemaps;             // 0x5c
    unsigned char m_archetypeCount;         // 0x60
    SG_Archetype** m_archetypes;            // 0x64 NULL until Init(); entries NULL until loaded
    ResourceQueue m_queue;                  // 0x68
    const char* m_globalName;               // 0xdc
    const char* m_archetypeName;            // 0xe0 first archetype resource
    const char* m_surfaceName;              // 0xe4 first surface resource
    int m_unusedE8;                         // 0xe8
    Graphics m_graphics;                    // 0xec
    int m_collisionX;                       // 0xf8 collision rectangle (collisionInit)
    int m_collisionY;                       // 0xfc
    int m_collisionWidth;                   // 0x100
    int m_collisionHeight;                  // 0x104
    int m_collisionTag;                     // 0x108 -1 = any sprite collides
    int m_collisionSprite;                  // 0x10c last sprite found colliding
    short m_boundsMinX;                     // 0x110
    short m_boundsMinY;                     // 0x112
    short m_boundsMaxX;                     // 0x114
    short m_boundsMaxY;                     // 0x116
    SpriteDrawInfo m_singleInfo;            // 0x118
    SpriteDrawInfo m_pairInfos[2];          // 0x11e
};

#endif
