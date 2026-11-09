// DGWorld.cpp - the 3D world of a level and its hit effects.
#include "DGWorld.h"
#include "CSwerve.h"
#include "CStdUtil_Cocoa.h"
#include "SwerveHelper.h"
#include "MathLib.h"
#include "App.h"
#include "DGHelper.h"
#include "CSwerveGame.h"
#include "dgCamera.h"

/** Rows of the visibility table start at camera angle -45 degrees. */
static const int VISIBILITY_ANGLE_OFFSET = 45;

DGWorld::DGWorld(swerve::World<CSwerveAssertingErrorHandler> world, const char* name)
    : m_world(world),
      m_field0c(false),
      m_visibility(NULL),
      m_nodes(NULL),
      m_nodeCount(0),
      m_rayIntersection(CSwerve::GetInstance()->m_swerve)
{
    m_fireGeometry = swerve::Group<CSwerveAssertingErrorHandler>(m_world.find(SwerveHelper::Id("firegeometryTransform")));
    m_fireTransform = SwerveHelper::NewTransform();
    // The by-value Transform temporary is released before loading effects.
    m_fireGeometry.getTransformTo(m_world, m_fireTransform);
    loadHitEffects();
}

void DGWorld::loadHitEffects()
{
    // Original 0x16571c releases and clears this slot directly.
    m_effectNodes[2].Clear();
    static_cast<swerve::QueryInterface<CSwerveAssertingErrorHandler>&>(m_effectNodes[1]) = (IQueryInterface*)NULL;
    m_effectNodes[0] = m_effectNodes[1];
    m_effectDurations[0] = 0x683;
    m_effectDurations[1] = 300;
    m_effectDurations[2] = 0x683;

    swerve::World<CSwerveAssertingErrorHandler> blastWorld = App::LoadObject3D("BIN_M3G_HIT_EFFECT_BLAST");
    m_effectNodes[EFFECT_BLAST] = swerve::Node<CSwerveAssertingErrorHandler>(blastWorld.find(SwerveHelper::Id("dummy_blastTransform")));
    m_effectDurations[EFFECT_BLAST] = 0x535;

    swerve::World<CSwerveAssertingErrorHandler> bodyBlowWorld = App::LoadObject3D("BIN_M3G_BODY_BLOW");
    m_effectNodes[EFFECT_BODY_BLOW] = swerve::Node<CSwerveAssertingErrorHandler>(bodyBlowWorld.find(SwerveHelper::Id("dummy_h_blastTransform")));
    m_effectDurations[EFFECT_BODY_BLOW] = 0x29a;

    swerve::World<CSwerveAssertingErrorHandler> headBlowWorld = App::LoadObject3D("BIN_M3G_HEAD_BLOW");
    m_effectNodes[EFFECT_HEAD_BLOW] = swerve::Node<CSwerveAssertingErrorHandler>(headBlowWorld.find(SwerveHelper::Id("dummy_h_blastTransform")));
    m_effectDurations[EFFECT_HEAD_BLOW] = 0x29a;

    swerve::World<CSwerveAssertingErrorHandler> bloodWorld = App::LoadObject3D("BIN_M3G_HIT_EFFECT_BLOOD");
    m_effectNodes[EFFECT_BODY_HIT] = swerve::Node<CSwerveAssertingErrorHandler>(bloodWorld.find(SwerveHelper::Id("dummy_dustTransform")));
    m_effectDurations[EFFECT_BODY_HIT] = 400;

    swerve::World<CSwerveAssertingErrorHandler> healWorld = App::LoadObject3D("BIN_M3G_HEAL_BLAST");
    m_effectNodes[EFFECT_HEAL_BLAST] = swerve::Node<CSwerveAssertingErrorHandler>(healWorld.find(SwerveHelper::Id("dummy_healTransform")));
    m_effectDurations[EFFECT_HEAL_BLAST] = 600;

    swerve::World<CSwerveAssertingErrorHandler> slimeWorld = App::LoadObject3D("BIN_M3G_SLIME_BLAST");
    m_effectNodes[EFFECT_SLIME_BLAST] = swerve::Node<CSwerveAssertingErrorHandler>(slimeWorld.find(SwerveHelper::Id("dummy_h_blastTransform")));
    m_effectDurations[EFFECT_SLIME_BLAST] = 600;

    swerve::World<CSwerveAssertingErrorHandler> stoneWorld = App::LoadObject3D("BIN_M3G_STONE_BLAST");
    m_effectNodes[EFFECT_STONE_BLAST] = swerve::Node<CSwerveAssertingErrorHandler>(stoneWorld.find(SwerveHelper::Id("stone_blastTransform")));
    m_effectDurations[EFFECT_STONE_BLAST] = 600;
}

DGWorld::~DGWorld()
{
    if (m_nodes != NULL) {
        delete[] m_nodes;
        m_nodes = NULL;
    }
    if (m_visibility != NULL) {
        delete[] m_visibility;
        m_visibility = NULL;
    }
}

void DGWorld::update()
{
    int animationTime = WindowApp::m_instance->m_updateTime;
    int validity = 0;
    ((swerve::IObject3DInterface*)m_world.m_interface)->animate(animationTime, &validity);
    CStdUtil_Cocoa::GetUpTimeMS();
    if (m_nodeCount < 1) {
        return;
    }
    DGCamera* camera = ((App*)WindowApp::m_instance)->m_helper->m_swerveGame->m_camera;
    int row = (int)camera->m_field0c + VISIBILITY_ANGLE_OFFSET;
    int rowStart = m_nodeCount * row;
    for (int index = 0; index < m_nodeCount; index++) {
        m_nodes[index].node()->setRenderingEnable(m_visibility[rowStart + index] & 1);
    }
}

void DGWorld::BlastImpl(int type, const vec3& position, const vec3& direction)
{
    swerve::Transform<CSwerveAssertingErrorHandler> transform = SwerveHelper::NewTransform();
    transform.transformInterface()->setIdentity();
    vec3 forward = direction;
    float lengthSquared = forward.m_x * forward.m_x + forward.m_y * forward.m_y + forward.m_z * forward.m_z;
    // Original checks the length and passes the unscaled direction to Angles.
    float length = MathLib::InvSqrt(lengthSquared) * lengthSquared;
    if (length == 0.0f) {
        forward.m_x = 0;
        forward.m_y = 0;
        forward.m_z = 1.0f;
    }
    Angles angles(forward, forward);
    Quaternion quaternion;
    quaternion.m_x = 0;
    quaternion.m_y = 0;
    quaternion.m_z = 0;
    quaternion.m_w = 0;
    angles.ToQuaternion(quaternion);
    float rotation[4];
    rotation[0] = quaternion.m_w;
    rotation[1] = quaternion.m_x;
    rotation[2] = quaternion.m_y;
    rotation[3] = quaternion.m_z;
    transform.transformInterface()->postRotateQuat(rotation);
    float translation[3];
    translation[0] = position.m_x;
    translation[1] = position.m_y;
    translation[2] = position.m_z;
    transform.transformInterface()->postTranslate(translation);
    EffectInstance effect = { type, transform, 0 };
    m_effects.addElement(effect);
}

void DGWorld::blast(const vec3& position, const vec3& direction)
{
    BlastImpl(EFFECT_BLAST, position, direction);
}

void DGWorld::slime_blast(const vec3& position, const vec3& direction)
{
    BlastImpl(EFFECT_SLIME_BLAST, position, direction);
}

void DGWorld::stoneBlast(const vec3& position, const vec3& direction)
{
    BlastImpl(EFFECT_STONE_BLAST, position, direction);
}

void DGWorld::bodyBlow(const vec3& position, const vec3& direction)
{
    BlastImpl(EFFECT_BODY_BLOW, position, direction);
}

void DGWorld::headBlow(const vec3& position, const vec3& direction)
{
    BlastImpl(EFFECT_HEAD_BLOW, position, direction);
}

void DGWorld::bodyHit(const vec3& position, const vec3& direction)
{
    BlastImpl(EFFECT_BODY_HIT, position, direction);
}

void DGWorld::healBlast(const vec3& position, const vec3& direction)
{
    BlastImpl(EFFECT_HEAL_BLAST, position, direction);
}

void DGWorld::paintHitEffects(swerve::Graphics3D<CSwerveAssertingErrorHandler>& graphics)
{
    for (int index = m_effects.size() - 1; index >= 0; index--) {
        EffectInstance& effect = m_effects[index];
        effect.m_time += WindowApp::m_instance->m_updateFrameTime;
        if (effect.m_time < m_effectDurations[effect.m_type]) {
            if (m_effectNodes[effect.m_type] != swerve::Object3D<CSwerveAssertingErrorHandler>()) {
                int validity = 0;
                ((swerve::IObject3DInterface*)m_effectNodes[effect.m_type].m_interface)->animate(effect.m_time, &validity);
                if (graphics.hasTarget()) {  // changed by fx2 (Graphics3D is a CSwerveGraphics3D handle)
                    graphics.renderer()->RenderNode(m_effectNodes[effect.m_type].m_interface, effect.m_transform.m_interface);
                }
            }
        } else {
            m_effects.removeElementAt(index);
        }
    }
}
