// PROVISIONAL (created by b048): owner src/dh3d/dgCamera.cpp not written yet
// dgCamera.h - camera of the 3D game; only what the other src/dh3d units use is declared.
// b049: the declarations and layout below are now recovered from dgCamera.cpp.
#ifndef DGCAMERA_H
#define DGCAMERA_H

#include "FrustumCulling.h"
#include "App.h"

/** Camera orientation, recoil, wandering, shake and shot / head-shot effects. */
class DGCamera {
public:
    typedef swerve::Camera<CSwerveAssertingErrorHandler> Camera;
    typedef swerve::Transform<CSwerveAssertingErrorHandler> Transform;

    DGCamera(Camera camera, float horizontalLimit, float verticalLimit);
    void Init(Camera camera, float horizontalLimit, float verticalLimit);
    void ReInit();
    float GetFOV() const;
    void SetFOV(float fov);  // added by b055x
    ~DGCamera();
    /** Back to the normal field of view (scope off). */  // added by b055x
    void restoreView(bool immediately);  // added by b055x
    void updateTransform(bool immediately);
    bool rotateLeft(float angle, bool unconstrained);
    bool rotateRight(float angle, bool unconstrained);
    bool rotateUp(float angle, bool unconstrained);
    bool rotateDown(float angle, bool unconstrained);
    void SetWandering(float horizontalAmplitude, float verticalAmplitude, int horizontalPeriod, int verticalPeriod);
    float GetShakeTimeLeft();  // added by b055x
    void SetShake(float duration, float frequency, float amplitude);  // added by b055x
    void Update(int elapsedMs);
    void UpdateReturnToCenter(int elapsedMs);
    void updateWander(int elapsedMs);
    void HandleZoomEffect(int elapsedMs);
    void CheckConstraints();
    bool IsShotEffectInProgress() const;
    /** Set the camera wandering of the active weapon. */
    void SetWanderngByActiveWeapon();
    void SetAngleH(float angle, bool unconstrained);
    void SetAngleV(float angle, bool unconstrained);
    vec3 GetViewDirection();
    vec3 GetViewPosition();
    bool GetScreenPosition(int* x, int* y, const vec3& point);

    // Former m_data00 / m_data10 placeholders: layout not known yet (b048).
    Camera m_camera;                     // 0x00
    Transform m_baseTransform;           // 0x04
    float m_defaultFov;                  // 0x08
    float m_field0c;                     // 0x0c camera angle (degrees; DGWorld::update, CFPSCursor::onPointerPressed)
    swvVector3D m_horizontalAxis;         // 0x10
    float m_field1c;                     // 0x1c read by CFPSCursor::onPointerPressed
    swvVector3D m_verticalAxis;           // 0x20
    float m_horizontalLimit;             // 0x2c
    float m_verticalLimit;               // 0x30
    float m_previousAngleH;              // 0x34
    float m_previousAngleV;              // 0x38
    float m_savedAngleH;                 // 0x3c
    float m_savedAngleV;                 // 0x40
    bool m_zoomEffect;                   // 0x44
    float m_savedFov;                    // 0x48
    float m_effectFov;                   // 0x4c
    float m_headShotFov;                 // 0x50
    float m_shakeAmplitude;              // 0x54
    float m_shakeEnvelope;               // 0x58 180 / duration
    float m_shakeTimeLeft;               // 0x5c seconds
    swvVector3D m_shakeOffset;            // 0x60
    float m_shakeFrequencies[9];          // 0x6c three sine components per axis
    int m_effectState;                   // 0x90 only initialized in this unit; meaning unresolved
    float m_recoilAngleX;                // 0x94
    float m_recoilAngleY;                // 0x98
    int m_recoilDuration;                // 0x9c ms
    int m_recoilElapsed;                 // 0xa0 -1 means inactive
    float m_fov;                         // 0xa4
    float m_scaleH;                      // 0xa8 initialized to 1; use outside this unit unresolved
    float m_scaleV;                      // 0xac initialized to 1; use outside this unit unresolved
    swvVector3D m_shotDisplacement;       // 0xb0
    swvVector3D m_shotStartPosition;      // 0xbc
    bool m_shotEffect;                   // 0xc8
    int m_shotDuration;                  // 0xcc
    int m_shotElapsed;                   // 0xd0
    float m_wanderLimitH;                // 0xd4
    float m_wanderLimitV;                // 0xd8
    float m_wanderAmplitudeH;            // 0xdc
    float m_wanderAmplitudeV;            // 0xe0
    int m_wanderPeriodH;                 // 0xe4
    int m_wanderPeriodV;                 // 0xe8
    int m_wanderElapsedH;                // 0xec
    int m_wanderElapsedV;                // 0xf0
    float m_wanderAngleH;                // 0xf4
    float m_wanderAngleV;                // 0xf8
    FrustumCulling m_frustum;            // 0xfc
    SViewport m_viewport;                // 0x160 background crop rectangle
    swvVector3D m_initialProjectionPoint; // 0x168
    swvVector3D m_projectionPoint;        // 0x174
    swvVector3D m_bulletStartPosition;    // 0x180
    swvVector3D m_bulletEndPosition;      // 0x18c
    bool m_wanderDisabled;               // 0x198
    int m_returnTimeLeft;                // 0x19c
    float m_returnSpeedV;                // 0x1a0
    float m_returnSpeedH;                // 0x1a4
    bool m_returningToCenter;            // 0x1a8
    bool m_returningToSavedView;          // 0x1a9
};

#endif
