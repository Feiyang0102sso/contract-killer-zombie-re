// dgCamera.cpp - camera movement and effects, recovered from the shipped armv7 image.
#include "dgCamera.h"
#include "CSwerve.h"
#include "App.h"
#include "DGHelper.h"
#include "CSwerveGame.h"
#include "CGameplayHUD.h"
#include "CDH_BasicGameData.h"
#include "CRandGen.h"
#include "Fixed.h"
#include "CUnitBody.h"

using com::glu::platform::core::CRandGen;
typedef swerve::Group<CSwerveAssertingErrorHandler> CameraGroup;

DGCamera::DGCamera(Camera camera, float horizontalLimit, float verticalLimit)
    : m_camera(camera), m_baseTransform(CSwerve::GetInstance()->m_swerve),
      m_defaultFov(-1.0f), m_horizontalLimit(horizontalLimit), m_verticalLimit(verticalLimit + 15.0f),
      m_zoomEffect(false), m_savedFov(m_fov), m_effectState(0), m_recoilAngleX(0), m_recoilAngleY(0),
      m_recoilDuration(0), m_recoilElapsed(-1), m_fov(-1.0f), m_scaleH(1.0f), m_scaleV(1.0f),
      m_shotEffect(false), m_shotDuration(0), m_shotElapsed(0), m_wanderLimitH(0), m_wanderLimitV(0),
      m_wanderAmplitudeH(0), m_wanderAmplitudeV(0), m_wanderPeriodH(0), m_wanderPeriodV(0),
      m_wanderElapsedH(0), m_wanderElapsedV(0), m_wanderAngleH(0), m_wanderAngleV(0), m_frustum(camera),
      m_wanderDisabled(false)
{
    // The original copies m_fov to m_savedFov before initializing m_fov; retain that order.
    Init(camera, horizontalLimit, verticalLimit);
    ReInit();
}

void DGCamera::Init(Camera camera, float horizontalLimit, float verticalLimit)
{
    m_camera = camera;
    m_horizontalLimit = horizontalLimit;
    m_verticalLimit = verticalLimit + 15.0f;
    swerve::Background<CSwerveAssertingErrorHandler> background =
        ((App*)WindowApp::m_instance)->m_helper->m_swerveGame->m_cameraNode.getBackground();
    m_viewport.m_x = background.getCropX();
    m_viewport.m_y = background.getCropY();
    m_viewport.m_width = background.getCropWidth();
    m_viewport.m_height = background.getCropHeight();
    Transform projection = ((App*)WindowApp::m_instance)->m_helper->m_transform;
    m_camera.getProjection(projection);
    projection.invert();
    m_initialProjectionPoint.x = -1.0f;
    m_initialProjectionPoint.y = -1.0f;
    m_initialProjectionPoint.z = 0;
    DGHelper::TransformPoint(projection, m_initialProjectionPoint);
    m_projectionPoint = m_initialProjectionPoint;
    m_frustum.UpdatePlanes();
    background.setColorClearEnable(false);
    m_headShotFov = ((App*)WindowApp::m_instance)->m_helper->m_config.GetFloatValueByToken("HEAD_SHOT_EFFECT_FOV", true);
}

void DGCamera::ReInit()
{
    m_camera.getCompositeTransform(m_baseTransform);
    m_defaultFov = GetFOV();
    SetFOV(m_defaultFov);
    m_field0c = 0;
    m_horizontalAxis.x = 0;
    m_horizontalAxis.y = 1;
    m_horizontalAxis.z = 0;
    m_field1c = 0;
    m_verticalAxis.x = 1;
    m_verticalAxis.y = 0;
    m_verticalAxis.z = 0;
    m_previousAngleH = 0;
    m_previousAngleV = 0;
    m_returnTimeLeft = 0;
    m_returnSpeedV = 0;
    m_returnSpeedH = 0;
    m_returningToCenter = false;
    m_returningToSavedView = false;
    m_savedAngleH = 0;
    m_savedAngleV = 0;
    m_shakeAmplitude = 0;
    m_shakeEnvelope = 0;
    m_shakeTimeLeft = 0;
    m_effectState = 0;
}

float DGCamera::GetFOV() const
{
    if (m_fov > 0) {
        return m_fov;
    }
    float params[4];
    m_camera.getProjection(params);
    return params[0];
}

void DGCamera::SetFOV(float fov)
{
    if (m_fov == fov) {
        return;
    }
    m_fov = fov;
    float params[4];
    m_camera.getProjection(params);
    m_camera.setPerspective(fov, params[1], params[2], params[3]);
    m_frustum.UpdatePlanes();
    Transform projection = ((App*)WindowApp::m_instance)->m_helper->m_transform;
    m_camera.getProjection(projection);
    projection.invert();
    static const swvVector3D v = {-1.0f, -1.0f, 0};
    swvVector3D point = v;
    DGHelper::TransformPoint(projection, point);
    m_projectionPoint.x = -1.0f;
    m_projectionPoint.y = -1.0f;
    m_projectionPoint.z = 0;
    DGHelper::TransformPoint(projection, m_projectionPoint);
    float factor = (m_fov - 10.0f) / 23.0f;
    if (factor < 0) {
        factor = 0;
    } else {
        // Constant-first minimum preserves the original NEON vmin, including unordered values.
        factor = (1.0f < factor) ? 1.0f : factor;
    }
    factor = factor + (1.0f - factor) * 0.5f;
    if (factor < 0) {
        factor = 0;
    }
    CUnitBody::m_LodDistanceModifier = factor;
}

DGCamera::~DGCamera()
{
    m_camera.setTransform(m_baseTransform);
    restoreView(false);
}

void DGCamera::restoreView(bool immediately)
{
    if (!immediately) {
        m_savedAngleH = m_field0c;
        m_savedAngleV = m_field1c;
        m_field1c = 0;
        m_field0c = 0;
        updateTransform(false);
        SetWandering(0, 0, 0, 0);
        rotateLeft(0, false);
        rotateRight(0, false);
        rotateUp(0, false);
        rotateDown(0, false);
    }
    SetFOV(m_defaultFov);
}

void DGCamera::updateTransform(bool immediately)
{
    Transform transform = ((App*)WindowApp::m_instance)->m_helper->m_transform;
    m_camera.setTransform(m_baseTransform);
    ((App*)WindowApp::m_instance)->m_helper->m_swerveGame->m_cameraNode.getTransformTo(m_camera, transform);
    static const swvVector3D axis_end = {0, 0, 1};
    swvVector3D axisEnd = axis_end;
    swvVector3D axisStart = {0, 0, 0};
    DGHelper::TransformPoint(transform, axisEnd);
    DGHelper::TransformPoint(transform, axisStart);
    m_horizontalAxis.x = axisEnd.x - axisStart.x;
    m_horizontalAxis.y = axisEnd.y - axisStart.y;
    m_horizontalAxis.z = axisEnd.z - axisStart.z;
    transform.setIdentity();
    transform.postRotate(&m_field0c);
    transform.postRotate(&m_field1c);
    transform.postMultiply(m_baseTransform);
    if (m_recoilElapsed >= 0) {
        float rotation[4];
        if (m_recoilElapsed < m_recoilDuration * 0.1) {
            rotation[0] = m_recoilAngleX * m_recoilElapsed / m_recoilDuration;
        } else {
            float duration = m_recoilDuration * 0.9f;
            rotation[0] = m_recoilAngleX * (1.0f - (m_recoilElapsed - duration) / duration);
        }
        rotation[1] = 1;
        rotation[2] = 0;
        rotation[3] = 0;
        transform.postRotate(rotation);
        if (m_recoilElapsed < m_recoilDuration * 0.1) {
            rotation[0] = m_recoilAngleY * m_recoilElapsed / m_recoilDuration;
        } else {
            float duration = m_recoilDuration * 0.9f;
            rotation[0] = m_recoilAngleY * (1.0f - (m_recoilElapsed - duration) / duration);
        }
        rotation[1] = 0;
        rotation[2] = 1;
        rotation[3] = 0;
        transform.postRotate(rotation);
    } else if (!m_wanderDisabled && (m_wanderLimitH > 0 || m_wanderLimitV > 0)) {
        float rotation[4];
        rotation[0] = m_wanderAngleH;
        rotation[1] = 0;
        rotation[2] = 1;
        rotation[3] = 0;
        transform.postRotate(rotation);
        rotation[0] = m_wanderAngleV;
        rotation[1] = 1;
        rotation[2] = 0;
        rotation[3] = 0;
        transform.postRotate(rotation);
    }
    if (m_shakeTimeLeft > 0) {
        transform.postTranslate(&m_shakeOffset.x);
    }
    m_camera.setTransform(transform);
    m_previousAngleH = m_field0c;
    m_previousAngleV = m_field1c;
}

bool DGCamera::rotateLeft(float angle, bool unconstrained)
{
    bool moved = false;
    if (!m_returningToCenter) {
        moved = true;
        m_field0c += angle;
        if (!unconstrained && m_field0c > m_horizontalLimit) {
            m_field0c = m_horizontalLimit;
            moved = false;
        }
        updateTransform(false);
    }
    return moved;
}

bool DGCamera::rotateRight(float angle, bool unconstrained)
{
    bool moved = false;
    if (!m_returningToCenter) {
        moved = true;
        m_field0c -= angle;
        if (!unconstrained && m_field0c < -m_horizontalLimit) {
            m_field0c = -m_horizontalLimit;
            moved = false;
        }
        updateTransform(false);
    }
    return moved;
}

bool DGCamera::rotateUp(float angle, bool unconstrained)
{
    bool moved = false;
    if (!m_returningToCenter) {
        moved = true;
        m_field1c += angle;
        if (!unconstrained && m_field1c > m_verticalLimit) {
            m_field1c = m_verticalLimit;
            moved = false;
        }
        updateTransform(false);
    }
    return moved;
}

bool DGCamera::rotateDown(float angle, bool unconstrained)
{
    bool moved = false;
    if (!m_returningToCenter) {
        moved = true;
        m_field1c -= angle;
        if (!unconstrained && m_field1c < -m_verticalLimit) {
            m_field1c = -m_verticalLimit;
            moved = false;
        }
        updateTransform(false);
    }
    return moved;
}

void DGCamera::SetWandering(float horizontalAmplitude, float verticalAmplitude, int horizontalPeriod, int verticalPeriod)
{
    m_wanderLimitH = horizontalAmplitude;
    m_wanderLimitV = verticalAmplitude;
    m_wanderPeriodH = horizontalPeriod;
    m_wanderPeriodV = verticalPeriod;
    m_wanderElapsedH = horizontalPeriod;
    m_wanderElapsedV = verticalPeriod;
    updateWander(1);
    m_wanderElapsedH = CRandGen::GetInstance()->GetRandRange(0, horizontalPeriod);
    m_wanderElapsedV = CRandGen::GetInstance()->GetRandRange(0, verticalPeriod);
    updateWander(0);
}

float DGCamera::GetShakeTimeLeft()
{
    return m_shakeTimeLeft;
}

void DGCamera::SetShake(float duration, float frequency, float amplitude)
{
    m_shakeAmplitude = amplitude;
    m_shakeTimeLeft = duration;
    m_shakeEnvelope = 180.0f / duration;
    CRandGen* random = CRandGen::GetInstance();
    for (int index = 0; index < 9; index++) {
        m_shakeFrequencies[index] = random->GetRandRange((int)frequency, (unsigned int)(frequency * 4.0f)) * 360.0f / duration;
    }
}

/** Three sine components multiplied by a cosine envelope (angles in degrees). */
float calcWave(float time, float amplitude, float envelope, float* frequencies)
{
    float fade = MathLib::Cos(time * 180.0 / envelope);
    float first = MathLib::Sin(frequencies[0] * time);
    float second = MathLib::Sin(frequencies[1] * time);
    float third = MathLib::Sin(frequencies[2] * time);
    return (fade + 1.0) * amplitude * (first + second * 0.5f + third * 0.25f);
}

void DGCamera::Update(int elapsedMs)
{
    UpdateReturnToCenter(elapsedMs);
    float seconds = elapsedMs * 0.001;
    if (m_shakeTimeLeft > 0) {
        m_shakeOffset.x = calcWave(m_shakeTimeLeft, m_shakeAmplitude, m_shakeEnvelope, m_shakeFrequencies);
        m_shakeOffset.y = calcWave(m_shakeTimeLeft, m_shakeAmplitude, m_shakeEnvelope, m_shakeFrequencies + 3);
        m_shakeOffset.z = calcWave(m_shakeTimeLeft, m_shakeAmplitude, m_shakeEnvelope, m_shakeFrequencies + 6);
        if (m_shakeTimeLeft > seconds) {
            m_shakeTimeLeft -= seconds;
        } else {
            m_shakeTimeLeft = 0;
        }
    } else {
        m_shakeOffset.x = 0;
        m_shakeOffset.y = 0;
        m_shakeOffset.z = 0;
    }
    if (m_shotEffect) {
        if (m_shotElapsed >= m_shotDuration) {
            m_shotEffect = false;
            CameraGroup bullet(((App*)WindowApp::m_instance)->m_helper->m_swerveGame->m_cameraNode.find(DGHelper::getSwerveID("bulletTransform")));
            CameraGroup cylinder(((App*)WindowApp::m_instance)->m_helper->m_swerveGame->m_cameraNode.find(DGHelper::getSwerveID("Cylinder03Transform")));
            bullet.setRenderingEnable(false);
            cylinder.setRenderingEnable(true);
        } else {
            m_shotElapsed += elapsedMs;
            if (m_shotElapsed >= m_shotDuration) {
                m_shotElapsed = m_shotDuration;
            }
            float fraction = (float)m_shotElapsed / m_shotDuration;
            swvVector3D cameraPosition;
            cameraPosition.x = (m_shotDisplacement.x - 3.0f) * fraction + m_shotStartPosition.x;
            cameraPosition.y = (m_shotDisplacement.y + 3.0f) * fraction + m_shotStartPosition.y;
            cameraPosition.z = (m_shotDisplacement.z + 3.0f) * fraction + (m_shotStartPosition.z + 3.0f);
            float bulletFraction = fraction + 0.1f;
            if (1.0f < bulletFraction) {
                bulletFraction = 1.0f;
            }
            swvVector3D bulletPosition;
            bulletPosition.x = (m_bulletEndPosition.x - m_bulletStartPosition.x) * bulletFraction + m_bulletStartPosition.x;
            bulletPosition.y = (m_bulletEndPosition.y - m_bulletStartPosition.y) * bulletFraction + m_bulletStartPosition.y;
            bulletPosition.z = (m_bulletEndPosition.z - m_bulletStartPosition.z) * bulletFraction + m_bulletStartPosition.z;
            m_camera.setTranslation(&cameraPosition.x);
            CameraGroup bullet(((App*)WindowApp::m_instance)->m_helper->m_swerveGame->m_cameraNode.find(DGHelper::getSwerveID("bulletTransform")));
            bullet.setTranslation(&bulletPosition.x);
            swerve::World<CSwerveAssertingErrorHandler> world = ((App*)WindowApp::m_instance)->m_helper->m_swerveGame->m_cameraNode;
            static const swvVector3D scale = {-1.0f, 1.0f, -1.0f};
            swvVector3D shotScale = scale;
            CameraGroup target(CSwerve::GetInstance()->m_swerve);
            target.setTranslation(&m_bulletEndPosition.x);
            world.addChild(target);
            m_camera.setScale(&shotScale.x);
            m_camera.setAlignment(target, 0x91, world, 0x94);
            m_camera.align(bullet);
            world.removeChild(target);
        }
    } else {
        if (m_recoilElapsed >= 0) {
            m_recoilElapsed += elapsedMs;
            updateTransform(false);
            if (m_recoilElapsed > m_recoilDuration) {
                m_recoilElapsed = -1;
            }
        } else {
            updateWander(elapsedMs);
            updateTransform(false);
        }
    }
    HandleZoomEffect(elapsedMs);
    m_frustum.UpdatePlanes();
}

void DGCamera::UpdateReturnToCenter(int elapsedMs)
{
    if (m_returningToCenter) {
        m_returnTimeLeft -= elapsedMs;
        m_field1c -= m_returnSpeedV * elapsedMs;
        m_field0c -= m_returnSpeedH * elapsedMs;
        if (m_returnTimeLeft <= 0) {
            m_field1c = 0;
            m_field0c = 0;
            m_returningToCenter = false;
        }
        updateTransform(false);
    }
    if (m_returningToSavedView) {
        m_returnTimeLeft -= elapsedMs;
        m_field1c += m_returnSpeedV * elapsedMs;
        m_field0c += m_returnSpeedH * elapsedMs;
        if (m_returnTimeLeft <= 0) {
            m_field1c = m_savedAngleV;
            m_field0c = m_savedAngleH;
            m_returningToSavedView = false;
        }
        updateTransform(false);
    }
}

void DGCamera::updateWander(int elapsedMs)
{
    if (!(m_wanderLimitH > 0 || m_wanderLimitV > 0)) {
        return;
    }
    m_wanderElapsedH += elapsedMs;
    if (m_wanderElapsedH > m_wanderPeriodH) {
        m_wanderElapsedH = 0;
        m_wanderAmplitudeH = (CRandGen::GetInstance()->GetRand(10) + 1) * m_wanderLimitH / 10.0f;
    }
    m_wanderAngleH = Fixed::toFloat(Fixed::sin((int)(((float)m_wanderElapsedH / m_wanderPeriodH) * 360.0f * 4096.0f))) * m_wanderAmplitudeH;
    m_wanderElapsedV += elapsedMs;
    if (m_wanderElapsedV > m_wanderPeriodV) {
        m_wanderElapsedV = 0;
        m_wanderAmplitudeV = (CRandGen::GetInstance()->GetRand(10) + 1) * m_wanderLimitV / 10.0f;
    }
    m_wanderAngleV = Fixed::toFloat(Fixed::sin((int)(((float)m_wanderElapsedV / m_wanderPeriodV) * 360.0f * 4096.0f))) * m_wanderAmplitudeV;
    float skill = ((App*)WindowApp::m_instance)->m_helper->m_gamePlayManager.m_playerData.GetSkillLevelCoef((CDH_PlayerData::skill_type_t)0);
    float divisor = skill + skill + 1.0f;
    divisor += ((App*)WindowApp::m_instance)->m_helper->GetPlayerData()->m_fieldac;
    m_wanderAngleH = m_wanderAngleH / divisor * 0.05f;
    m_wanderAngleV = m_wanderAngleV / divisor * 0.05f;
    if (((App*)WindowApp::m_instance)->m_helper->m_swerveGame->GetHUD()->m_steadyAim) {
        float factor = ((App*)WindowApp::m_instance)->m_helper->m_config.GetFloatValueByToken("STEADY_REDUCE_WANDER_COEF", true);
        m_wanderAngleH *= factor;
        m_wanderAngleV *= factor;
    }
}

void DGCamera::HandleZoomEffect(int elapsedMs)
{
    if (((App*)WindowApp::m_instance)->m_helper->m_swerveGame->IsHeadShotEffectActive()) {
        if (!m_zoomEffect) {
            m_savedFov = m_fov;
            m_zoomEffect = true;
            m_effectFov = m_fov;
        }
        m_effectFov += elapsedMs * -100.0f / 1000.0f;
        if (m_effectFov < m_headShotFov) {
            m_effectFov = m_headShotFov;
        }
        SetFOV(m_effectFov);
    } else if (m_zoomEffect) {
        m_zoomEffect = false;
        SetFOV(m_savedFov);
    }
}

void DGCamera::CheckConstraints()
{
    bool unchanged = true;
    if (m_field0c > m_horizontalLimit) {
        m_field0c = m_horizontalLimit;
        unchanged = false;
    } else if (m_field0c < -m_horizontalLimit) {
        m_field0c = -m_horizontalLimit;
        unchanged = false;
    }
    if (m_field1c > m_verticalLimit) {
        m_field1c = m_verticalLimit;
    } else if (m_field1c < -m_verticalLimit) {
        m_field1c = -m_verticalLimit;
    } else if (unchanged) {
        return;
    }
    updateTransform(false);
}

bool DGCamera::IsShotEffectInProgress() const
{
    return m_shotEffect;
}

void DGCamera::SetWanderngByActiveWeapon()
{
    // The shipped function only obtains the active weapon; no wandering setter follows it.
    ((App*)WindowApp::m_instance)->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon();
}

void DGCamera::SetAngleH(float angle, bool unconstrained)
{
    if (!m_returningToCenter && !m_returningToSavedView) {
        m_field0c = angle;
        if (!unconstrained) {
            CheckConstraints();
        }
    }
}

void DGCamera::SetAngleV(float angle, bool unconstrained)
{
    if (!m_returningToCenter && !m_returningToSavedView) {
        m_field1c = angle;
        if (!unconstrained) {
            CheckConstraints();
        }
    }
}

vec3 DGCamera::GetViewDirection()
{
    Transform transform(CSwerve::GetInstance()->m_swerve);
    m_camera.getTransformTo(((App*)WindowApp::m_instance)->m_helper->m_swerveGame->m_cameraNode, transform);
    float matrix[16];
    transform.get(matrix);
    vec3 direction;
    direction.m_x = matrix[2];
    direction.m_y = matrix[6];
    direction.m_z = matrix[10];
    return direction;
}

vec3 DGCamera::GetViewPosition()
{
    Transform transform(CSwerve::GetInstance()->m_swerve);
    m_camera.getTransformTo(((App*)WindowApp::m_instance)->m_helper->m_swerveGame->m_cameraNode, transform);
    float matrix[16];
    transform.get(matrix);
    vec3 position;
    position.m_x = matrix[3];
    position.m_y = matrix[7];
    position.m_z = matrix[11];
    return position;
}

bool DGCamera::GetScreenPosition(int* x, int* y, const vec3& point)
{
    Transform view(CSwerve::GetInstance()->m_swerve);
    Transform projection(CSwerve::GetInstance()->m_swerve);
    ((App*)WindowApp::m_instance)->m_helper->m_swerveGame->m_cameraNode.getTransformTo(m_camera, view);
    m_camera.getProjection(projection);
    float position[4];
    position[0] = point.m_x;
    position[1] = point.m_y;
    position[2] = point.m_z;
    position[3] = 1.0f;
    view.transform(4, position);
    projection.transform(4, position);
    // b147: the original loads x/y in the w-test block (all four read up front, then sunk).
    float clipX = position[0];
    float clipY = position[1];
    float clipZ = position[2];
    float clipW = position[3];
    bool visible = false;
    // 0x169014 branches on ordered <= 0: unordered z proceeds to the w test.
    if (!(clipZ <= 0)) {
        if (!(clipW > -1e-12f && clipW < 1e-12f)) {
            visible = true;
            float normalizedX = clipX / clipW;
            float normalizedY = clipY / clipW;
            int width = ((App*)WindowApp::m_instance)->m_helper->m_screenWidth;
            int height = ((App*)WindowApp::m_instance)->m_helper->m_screenHeight;
            *x = (normalizedX + 1.0f) * 0.5f * width;
            *y = (1.0f - normalizedY) * 0.5f * height;
        }
    }
    return visible;
}
