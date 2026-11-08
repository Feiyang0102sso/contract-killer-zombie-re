// SG_Presenter.h - plays one animation of an archetype character and draws, measures or
// collides its current frame through the SG_Instance accumulators. (created by b042)
#ifndef SG_PRESENTER_H
#define SG_PRESENTER_H

#include "SG_Defines.h"

class SG_Instance;
class SG_SpecialHandler;

class SG_Presenter {
public:
    /** m_specialFlags: which SG_SpecialHandler hooks are called. */
    enum SpecialFlag {
        SpecialFlag_FramePre = 0x01,
        SpecialFlag_FramePost = 0x02,
        SpecialFlag_LayerPre = 0x04,
        SpecialFlag_LayerPost = 0x08,
        SpecialFlag_SpritePre = 0x10,
        SpecialFlag_SpritePost = 0x20
    };

    SG_Presenter();
    ~SG_Presenter();

    void Init();
    void SetArchetypeCharacter(int archetype, int character);
    /** Initialized instance, loaded character and a selected animation. */
    unsigned char IsViable();
    void SetPosition(short x, short y);
    void SetAnimation(int animation);
    void SetAnimation(int animation, unsigned char loop);
    void SetAnimation(int animation, unsigned char loop, unsigned char transform);
    /** Use the character scale closest to `percent`. */
    void setScale(unsigned short percent);
    /** Advance by `elapsed` ms; 1 when a non looping animation just ended. */
    unsigned char Update(int elapsed);
    unsigned char HasFinished();
    /** Restart the animation for playing in `direction` (1 forward, -1 backward). */
    void ResetPoles(int direction);
    void Draw();
    void Draw(int x, int y);
    void TraverseCurrentFrame(SG_TraverseMode mode);

    /** Frame at which playing in the current direction ends. */
    int GetLastFrame()
    {
        int lastFrame = 0;
        if (m_direction >= 1) {
            lastFrame = m_frameCount - 1;
        }
        return lastFrame;
    }
    void Bounds(com::glu::platform::components::CRectangle<short>& bounds);
    void Reset();

    unsigned char m_collisionSpritesOnly;   // 0x00 collide only collision boxes of the tag
    SG_Instance* m_instance;                // 0x04
    SG_SpecialHandler* m_special;           // 0x08 hooks around frames, layers and sprites
    unsigned short m_scalePercent;          // 0x0c
    int m_scale;                            // 0x10 16.16
    unsigned char m_isScaled;               // 0x14 m_scalePercent != 100
    unsigned char m_archetype;              // 0x15
    unsigned char m_character;              // 0x16
    unsigned short m_animation;             // 0x18
    unsigned char m_transform;              // 0x1a SquareTransform of the whole character
    short m_x;                              // 0x1c
    short m_y;                              // 0x1e
    unsigned char m_specialFlags;           // 0x20 SpecialFlag bits
    unsigned short* m_frameDurations;       // 0x24 of the current animation
    int m_unused28;                         // 0x28 cleared, never read
    unsigned char m_loop;                   // 0x2c
    signed char m_direction;                // 0x2d 1 forward, -1 backward
    int m_time;                             // 0x30 ms into the animation
    unsigned short m_pole0;                 // 0x34 time range of the current frame
    unsigned short m_pole1;                 // 0x36
    unsigned char m_frameCount;             // 0x38
    unsigned char m_frame;                  // 0x39
    unsigned short m_totalDuration;         // 0x3a
};

#endif
