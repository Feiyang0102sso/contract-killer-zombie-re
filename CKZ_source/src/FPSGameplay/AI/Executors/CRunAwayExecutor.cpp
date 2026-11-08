// u425 full owner; normalization grouping for fleeing from the attack source kept per the original ARM.
#include "CRunAwayExecutor.h"
#include "CAttractorMovementExecutor.h"
#include "CUnitMind.h"
#include "CUnit.h"

void CRunAwayExecutor::OnChildFinished(EExecutorResult)
{
    Finish(m_mind, (EExecutorResult)0);
}
void CRunAwayExecutor::OnStart()
{
    CUnit* unit = m_mind->m_owner;
    vec3 delta = unit->m_body.m_position - unit->m_body.m_attackSource;
    float inverseLength = MathLib::InvSqrt(delta.LengthSqr());
    vec3 position = unit->m_body.m_position + (delta * inverseLength) * 4.0f;
    m_mind->m_owner->SetTarget(position);
    AddUnitTargetAttractorMovementExecutor(m_mind, CUnitBody::UNIT_BODY_RADIUS);
}
void AddRunAwayExecutor(CUnitMind* mind)
{
    if (mind == NULL) return;
    CRunAwayExecutor* executor = new CRunAwayExecutor(mind);
    mind->AddExecutor(executor, 0);
    executor->OnStart();
}
