// CSkyBox.h - sky box of a 3D scene: either a separate world (BIN_SKYBOX) or its mesh moved into the
// scene world and rendered with the camera's rotation only.
#ifndef CSKYBOX_H
#define CSKYBOX_H

#include "tws.h"

/** Sky box (0x0c bytes). */
class CSkyBox {
public:
    CSkyBox();
    ~CSkyBox();

    /** Load BIN_SKYBOX for `world`. */
    void Init(swerve::World<CSwerveAssertingErrorHandler>& world);
    /** Switch depth test and depth writes off for `appearance` (a compositing mode is created when missing). */
    void DisableZBuffer(swerve::Appearance<CSwerveAssertingErrorHandler> appearance);
    /** Keep the sky mesh around the active camera. */
    void Update();
    void Render(const swerve::Graphics3D<CSwerveAssertingErrorHandler>& graphics);

    swerve::World<CSwerveAssertingErrorHandler> m_skyWorld;     // 0x00 BIN_SKYBOX (NULL once its mesh moved)
    swerve::World<CSwerveAssertingErrorHandler> m_world;        // 0x04 the scene world
    swerve::Mesh<CSwerveAssertingErrorHandler> m_skyMesh;       // 0x08 sky mesh inside m_world
};

#endif
