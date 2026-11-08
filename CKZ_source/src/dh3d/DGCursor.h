// DGCursor.h - aiming cursor of the 3D game (moved by touches and by the device tilt).
#ifndef DGCURSOR_H
#define DGCURSOR_H

#include "DGMovingEntity.h"

class CDH_Accelerometer;

class DGCursor : public MovingEntity {
public:
    DGCursor();
    virtual ~DGCursor();
    virtual void update(int elapsedMs);
    /** Current aim position; first two slots introduced by DGCursor. */
    virtual int GetAimX() const = 0;
    virtual int GetAimY() const = 0;
    /** Position of the tag (the aim position). */
    virtual int GetTagX() const
    {
        return GetAimX();
    }
    virtual int GetTagY() const
    {
        return GetAimY();
    }
    virtual void ShowCross(bool show)
    {
    }
    /** Hooks of the concrete cursor. */
    virtual void onWeaponSwitch() = 0;
    virtual void onFire() = 0;
    virtual void onPointerMove(int x, int y, int deltaX, int deltaY) = 0;
    virtual void onPointerBreak() = 0;
    virtual void onPointerPressed(int x, int y) = 0;
    virtual void onPointerReleased() = 0;
    virtual void DrawImpl() = 0;
    virtual void UpdateImpl(int elapsedMs) = 0;
    virtual void InitImpl() = 0;
    virtual void DeInitImpl() = 0;
    virtual void EnableSight(bool enable) = 0;
    virtual void ShowSight(bool show) = 0;

    /** Pointer moved by (deltaX, deltaY); with acceleration the tilt movement is added (12 bit fraction). */
    void PointerMove(int x, int y, int deltaX, int deltaY);
    void Init();
    void DeInit();
    void PointerBreak();
    void draw();
    void Hide();
    void PointerPressed(int x, int y);
    void PointerReleased();
    /** Shoot when the game allows it. */
    void Fire();

    CDH_Accelerometer* m_accelerometer;     // 0x44
    float m_lastTiltX;                      // 0x48 accelerometer position at the last move
    float m_lastTiltY;                      // 0x4c
    bool m_visible;                         // 0x50
};

#endif
