// Original Human death registration, damage reaction and switch to the death executor.
#include "CHumanLogicExecutor.h"
#include "CDeathExecutor.h"
#include "CPlayAnimationExecutor.h"
#include "CUnit.h"
#include "CBH_PersonType.h"
#include "CProgressSystem.h"
#include "App.h"

CHumanLogicExecutor::CHumanLogicExecutor(CUnitMind* mind, bool flag)
    : m_mind(mind), m_flag10(false), m_flag11(flag)
{
}

void CHumanLogicExecutor::OnDeath()
{
    RegisterAsDead();
    CancelChildren(m_mind);
    AddDeathExecutorExecutor(m_mind);
}

void CHumanLogicExecutor::RegisterAsDead()
{
    if (m_mind != NULL) {
        EHitType hit;
        m_mind->m_owner->GetPickInfo(&hit);
        int experience = CProgressSystem::CalcXpForKilledUnit(m_mind->m_owner);
        ((App*)WindowApp::m_instance)->m_progressSystem->RegisterGameEvent(
            CProgressSystem::EVENT_ENEMY_KILLED, experience, hit == HIT_HEAD);
        CUnit* owner = m_mind->m_owner;
        if (owner != NULL) {
            owner->m_blowing = true;
        }
    }
}

void CHumanLogicExecutor::OnReceiveDamageFromPlayer(EHitType hit)
{
    if (!m_mind->m_owner->m_body.m_spawning) {
        AddPlayWoundAnimationModalExecutor(m_mind);
    }
}

void CHumanLogicExecutor::OnReceiveDamageFromUnit(CUnit* attacker)
{
    if (m_mind->m_owner->m_body.m_spawning) {
        return;
    }
    CEnemyWeaponType* weapon = attacker->m_body.m_personType->m_weapon;
    if (weapon != NULL && weapon->m_isMelee) {
        AddPlayWoundAnimationModalExecutor(m_mind);
    }
}

void CHumanLogicExecutor::Tick(int elapsed)
{
    // Original e8980 is a real empty body; it does not stand in for any unrecovered logic.
}
