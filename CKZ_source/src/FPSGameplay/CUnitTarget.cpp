// CUnitTarget.cpp - snapshots are rebuilt on Clone, rather than copying stale map locations.
#include "CUnitTarget.h"
#include "CUnit.h"
#include "CUnitsController.h"
#include "App.h"
#include "DGHelper.h"
#include "CSwerveGame.h"
#include "Ballistics.h"

bool CUnitTarget::Attack(int, int) const { return false; }

CUnitTargetUnit::CUnitTargetUnit(CUnit* unit)
{
    m_valid = unit != NULL;
    m_id = -1;
    if (unit != NULL) {
        m_id = unit->m_id;
        m_position = unit->m_body.m_position;
        m_location = unit->m_body.m_location;
    }
}

CUnitTarget* CUnitTargetUnit::Clone()
{
    CUnit* unit = ((App*)WindowApp::m_instance)->m_helper->m_swerveGame->GetUnitsController()->GetUnitByUniqueId(m_id);
    return new CUnitTargetUnit(unit);
}

bool CUnitTargetUnit::Attack(int attackerId, int damage) const
{
    if (m_valid) {
        CUnit* unit = ((App*)WindowApp::m_instance)->m_helper->m_swerveGame->GetUnitsController()->GetUnitByUniqueId(m_id);
        unit->ReceivedDamageFromUnit(attackerId, damage);
        return true;
    }
    return false;
}

void CUnitTargetUnit::Update()
{
    if (!m_valid) return;
    CUnit* unit = ((App*)WindowApp::m_instance)->m_helper->m_swerveGame->GetUnitsController()->GetUnitByUniqueId(m_id);
    if (unit == NULL || ((unit->m_dead || unit->m_blowing) && unit->m_corpseDisappear)) {
        m_valid = false;
        return;
    }
    m_position = unit->m_body.m_position;
    m_location = unit->m_body.m_location;
}

CUnit* CUnitTargetUnit::GetUnit() const
{
    if (m_id < 1 || !m_valid) return NULL;
    return ((App*)WindowApp::m_instance)->m_helper->m_swerveGame->GetUnitsController()->GetUnitByUniqueId(m_id);
}

CUnitTargetPoint::CUnitTargetPoint(const vec3& position)
{
    m_valid = true;
    m_position = position;
    ((App*)WindowApp::m_instance)->m_helper->m_swerveGame->GetGameAIMap()->SyncLocation(m_location, m_position);
}

CUnitTarget* CUnitTargetPoint::Clone() { return new CUnitTargetPoint(m_position); }

CUnitTargetHandGrenade::CUnitTargetHandGrenade(int id)
{
    m_id = id;
    m_valid = true;
    Update();
}

void CUnitTargetHandGrenade::Update()
{
    // The original indexes the projectile pool directly: no id bounds test.
    Ballistics::Projectile& projectile = ((App*)WindowApp::m_instance)->m_helper->m_swerveGame->GetHandGrenadeBallistics()->m_projectiles[m_id];
    if (projectile.m_active) {
        m_position = projectile.m_position;
        ((App*)WindowApp::m_instance)->m_helper->m_swerveGame->GetGameAIMap()->SyncLocation(m_location, m_position);
    } else {
        m_valid = false;
    }
}

CUnitTarget* CUnitTargetHandGrenade::Clone() { return new CUnitTargetHandGrenade(m_id); }
