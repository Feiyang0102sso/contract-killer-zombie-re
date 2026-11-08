// PROVISIONAL (created by b048): owner src/dh3d/career/DGMovingEntity.cpp not written yet
// b050: the implementation and fields below now replace the provisional layout.
// DGMovingEntity.h - entity moving towards a target inside constraints; only what src/dh3d uses.
#ifndef DGMOVINGENTITY_H
#define DGMOVINGENTITY_H

class MovingEntity {
public:
    MovingEntity();
    virtual ~MovingEntity();
    /** Called when a constraint stops the movement. */
    virtual void onConstraintReached(int constraint)
    {
    }
    virtual void update(int elapsedMs);
    /** Current aim position. */
    // b118: the original MovingEntity ends at update; the aiming interface is introduced by DGCursor.

    /** Unconstrained movement completes when either directed axis reaches its target. */
    bool isTargetReached() const;
    /** Clamp crossed bounds, stop the current movement and notify the crossed sides. */
    void correctPositionByConstraints();
    bool checkConstraints(int x, int y) const;

    int m_positionX;                  // 0x04
    int m_positionY;                  // 0x08
    int m_startX;                  // 0x0c
    int m_startY;                  // 0x10
    // m_data14 was the provisional byte array: layout not known yet (b048).
    int m_targetX;                  // 0x14
    int m_targetY;                  // 0x18
    int m_minX;                     // 0x1c constraint bit 8
    int m_minY;                     // 0x20 constraint bit 1
    int m_maxX;                     // 0x24 constraint bit 2
    int m_maxY;                     // 0x28 constraint bit 4
    int m_velocityX;                // 0x2c fixed point units per second
    int m_velocityY;                // 0x30
    int m_durationMs;               // 0x34 duration of interpolated movement
    int m_elapsedMs;                  // 0x38
    int m_constraints;                  // 0x3c
    int m_movementMode;                  // 0x40 -1 by default
};

#endif
