// CSpawnSceneObject.cpp - original u442 manhole spawn animation, waiting state and actual spawn point offset.
#include "CSpawnSceneObject.h"
#include "CGameAIMap.h"
#include "CSwerveGame.h"
#include "App.h"
#include "DGHelper.h"
#include "SwerveHelper.h"
#include "CSwerve.h"
#include "CDH_Animation.h"

typedef swerve::Group<CSwerveAssertingErrorHandler> Group;
typedef swerve::Transform<CSwerveAssertingErrorHandler> Transform;

CSpawnSceneObject::CSpawnSceneObject(const CSceneObjectManager::SSpawnSceneObjectInfo* info)
    : CSceneObject(info, true), m_spawnBusy(false), m_spawnRequested(false), m_spawnTimeMs(0)
{
    m_animation = new CDH_Animation(m_group);
    m_animation->addController(XString("manhole_open"));
    m_spawnShift = vec3(); // The original clears it once before and once after creating the animation; keeps the second write.
}

void CSpawnSceneObject::SetToPoint(const IGameAIMap* map, int id)
{
    CSceneObject::SetToPoint(map, id);
    Group actualSpawn(SwerveHelper::Find(m_group, "actual_spawn01Transform"));
    if (actualSpawn.m_interface != NULL) {
        map->GetMapPointOrientation(id); // The original does not use the output, but the call and its order must be kept.
        map->GetMapPointPostition(id);
        CSwerveGame* game = ((App*)WindowApp::m_instance)->m_helper->m_swerveGame;
        Group world(game->m_cameraNode.find(DGHelper::getSwerveID("worldTransform")));
        Transform transform(CSwerve::GetInstance()->m_swerve);
        actualSpawn.getTransformTo(world, transform);
        m_spawnShift = SwerveHelper::GetTranslation(transform) - m_position;
    }
}

bool CSpawnSceneObject::TrySpawn()
{
    if (!m_spawnBusy) {
        if (!m_spawnRequested) {
            m_spawnRequested = true;
            m_animation->setCurrentController(0, false);
            m_animation->animate();
        }
        return false;
    }
    if (m_spawnTimeMs <= 0) {
        m_spawnTimeMs = 0;
        return true;
    }
    return false;
}

void CSpawnSceneObject::Tick(int elapsedMs)
{
    if (m_spawnRequested && m_animation->isAnimationComplete() == true) {
        m_spawnBusy = true;
        m_spawnRequested = false;
    }
    if (m_spawnTimeMs >= 1) {
        m_spawnTimeMs -= elapsedMs;
    }
}

CSpawnSceneObject::~CSpawnSceneObject()
{
    if (m_animation != NULL) {
        delete m_animation;
        m_animation = NULL;
    }
}
