// CSkyBox.cpp - sky box of a 3D scene.
#include "CSkyBox.h"
#include "AdBannerSizes.h"
#include "App.h"
#include "DGHelper.h"
#include "SwerveHelper.h"
#include "CSwerve.h"

typedef CSwerveAssertingErrorHandler ErrorHandler;

/** Camera handle of the sky box (not used by the functions of this file). */
swerve::Camera<ErrorHandler> cam;

/** Number of faces (appearances) of the sky box mesh. */
static const int SKYBOX_FACES = 6;

CSkyBox::CSkyBox()
{
}

CSkyBox::~CSkyBox()
{
    m_skyWorld.Clear();
    m_skyMesh.Clear();
}

void CSkyBox::Init(swerve::World<ErrorHandler>& world)
{
    m_world = world;
    m_skyWorld = swerve::World<ErrorHandler>(App::LoadObject3D("BIN_SKYBOX"));
    if (m_skyWorld.m_interface == NULL) {
        return;
    }
    m_skyWorld.setPickingEnable(false);
    if (world.m_interface == NULL) {
        return;
    }

    swerve::Mesh<ErrorHandler> skyMesh = m_skyWorld.find(DGHelper::getSwerveID("SkyBox"));
    if (skyMesh.m_interface == NULL) {
        return;
    }
    skyMesh.setPickingEnable(false);

    // The faces take the appearances of the dummy mesh of the scene when it has all six.
    swerve::Mesh<ErrorHandler> dummy = world.find(DGHelper::getSwerveID("SkyBoxMaterialDummy"));
    if (dummy.m_interface == NULL) {
        // No dummy: only a copy of the sky mesh goes into the scene.
        swerve::Object3D<ErrorHandler> copy = skyMesh.duplicate();
        world.addChild(swerve::Node<ErrorHandler>(copy));
    } else {
        swerve::Appearance<ErrorHandler> face0 = dummy.getAppearance(0);
        swerve::Appearance<ErrorHandler> face1 = dummy.getAppearance(1);
        swerve::Appearance<ErrorHandler> face2 = dummy.getAppearance(2);
        swerve::Appearance<ErrorHandler> face3 = dummy.getAppearance(3);
        swerve::Appearance<ErrorHandler> face4 = dummy.getAppearance(4);
        swerve::Appearance<ErrorHandler> face5 = dummy.getAppearance(5);
        if (face0.m_interface != NULL && face1.m_interface != NULL && face2.m_interface != NULL &&
            face3.m_interface != NULL && face4.m_interface != NULL && face5.m_interface != NULL) {
            DisableZBuffer(face0);
            DisableZBuffer(face1);
            DisableZBuffer(face2);
            DisableZBuffer(face3);
            DisableZBuffer(face4);
            DisableZBuffer(face5);
            skyMesh.setAppearance(0, face0);
            skyMesh.setAppearance(1, face1);
            skyMesh.setAppearance(2, face2);
            skyMesh.setAppearance(3, face3);
            skyMesh.setAppearance(4, face4);
            skyMesh.setAppearance(5, face5);
            swerve::Object3D<ErrorHandler> copy = skyMesh.duplicate();
            world.addChild(swerve::Node<ErrorHandler>(copy));
            m_skyMesh = swerve::Mesh<ErrorHandler>(world.find(DGHelper::getSwerveID("SkyBox")));
            dummy.setRenderingEnable(false);
            m_skyWorld.Clear();
        }
    }
}

void CSkyBox::DisableZBuffer(swerve::Appearance<ErrorHandler> appearance)
{
    swerve::CompositingMode<ErrorHandler> mode = appearance.getCompositingMode();
    if (mode.m_interface == NULL) {
        mode = SwerveHelper::NewCompositingMode();
        mode.setDepthTestEnable(false);
        mode.setDepthWriteEnable(false);
        appearance.setCompositingMode(mode);
    } else {
        appearance.getCompositingMode().setDepthWriteEnable(false);
        appearance.getCompositingMode().setDepthTestEnable(false);
    }
}

void CSkyBox::Update()
{
    if (m_skyMesh.m_interface == NULL) {
        return;
    }
    float rotation[4];
    swerve::Camera<ErrorHandler> camera = m_world.getActiveCamera();
    swerve::Transform<ErrorHandler> cameraTransform(CSwerve::GetInstance()->m_swerve);
    camera.getTransformTo(m_world, cameraTransform);
    swerve::Transform<ErrorHandler> skyTransform(CSwerve::GetInstance()->m_swerve);
    m_skyMesh.setTransform(skyTransform);

    float cameraMatrix[16];
    cameraTransform.get(cameraMatrix);
    rotation[0] = -90.0f;
    rotation[1] = 1.0f;
    rotation[2] = 0.0f;
    rotation[3] = 0.0f;
    skyTransform.postRotate(rotation);
    float skyMatrix[16];
    skyTransform.get(skyMatrix);
    // Follow the camera position (slightly below and behind it).
    skyMatrix[3] = cameraMatrix[3];
    skyMatrix[7] = cameraMatrix[7] + -5.0f;
    skyMatrix[11] = cameraMatrix[11] + 5.0f;
    skyTransform.set(skyMatrix);
    m_skyMesh.setTransform(skyTransform);
    cameraTransform.Clear();
    skyTransform.Clear();
}

void CSkyBox::Render(const swerve::Graphics3D<ErrorHandler>& graphics)
{
    if (m_skyMesh.m_interface != NULL) {
        // Render the scene with the sky mesh on and the scene root off, then restore.
        m_skyMesh.setRenderingEnable(true);
        swerve::Group<ErrorHandler> root = m_world.find(DGHelper::getSwerveID("RootNode"));
        root.setRenderingEnable(false);
        graphics.render(m_world.m_interface);
        m_skyMesh.setRenderingEnable(false);
        root.setRenderingEnable(true);
    } else {
        graphics.render(m_skyWorld.m_interface);
    }
}
