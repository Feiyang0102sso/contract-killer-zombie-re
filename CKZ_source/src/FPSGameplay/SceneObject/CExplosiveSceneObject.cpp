// CExplosiveSceneObject.cpp - original u440 explosive barrel damage, delay and animation lifecycle.
#include "CExplosiveSceneObject.h"
#include "CDH_Animation.h"
#include "CSwerveGame.h"
#include "App.h"
#include "DGHelper.h"
#include "CDH_SoundManager.h"

CExplosiveSceneObject::CExplosiveSceneObject(const CSceneObjectManager::SExplosiveSceneObjectInfo* info)
    : CSceneObject(info, true)
{
    m_explosionDelayMs = 0;
    m_exploded = false;
    m_damage = info->m_damage;
    m_damageRadius = info->m_damageRadius;
    m_hp = info->m_hp;
    m_animation = new CDH_Animation(m_group);
    m_animation->addController(XString("barrel_blast"));
    ((App*)WindowApp::m_instance)->m_soundManager->PrecacheSound((CDH_SoundManager::eSound)1);
}

CExplosiveSceneObject::~CExplosiveSceneObject()
{
    if (m_animation != NULL) {
        delete m_animation;
        m_animation = NULL;
    }
}

void CExplosiveSceneObject::Tick(int elapsedMs)
{
    if (m_exploded) {
        return;
    }
    if (m_hp > 0) {
        return;
    }
    if (m_explosionDelayMs > 0) {
        m_explosionDelayMs -= elapsedMs;
        if (m_explosionDelayMs <= 0) {
            Explode();
        }
        return;
    }
    Explode();
}

void CExplosiveSceneObject::Explode()
{
    m_exploded = true;
    m_animation->setCurrentController(0, false);
    m_animation->animate();
    const vec3& position = m_position;
    vec3 normal(0.0f, 0.0f, 1.0f);
    ((App*)WindowApp::m_instance)->m_helper->m_swerveGame->Blast(position, normal,
        m_damageRadius, 0, m_damage, NULL, NULL, NULL);
    ((App*)WindowApp::m_instance)->m_soundManager->Play((CDH_SoundManager::eSound)1, false, false);
}

void CExplosiveSceneObject::ReceivedDamage(int damage, float distance)
{
    if (m_hp < 1) {
        return;
    }
    m_hp -= damage;
    if (m_hp <= 0) {
        m_explosionDelayMs = (int)(distance * 60.0f); // original literal0x10b250=00007042
    }
}

bool CExplosiveSceneObject::CanBeHandled()
{
    return !m_exploded;
}
