// u417 full owner. OnReceiveDamage/DebugRender are 4B empty bodies at original 0xe78a0/a4.
#include "CAttractorMovementExecutor.h"
#include "CUnitMind.h"
#include "CUnit.h"
#include "CUnitTarget.h"

void CAttractorMovementExecutor::Tick(int elapsed)
{
    CUnitTarget* target = m_mind->m_owner->m_target;
    if (target == NULL || !target->m_valid) {
        Finish(m_mind, (EExecutorResult)2);
        return;
    }
    if (m_animation != m_mind->m_owner->m_body.GetMovementAnimation()) {
        m_animation = m_mind->m_owner->m_body.GetMovementAnimation();
        StartAnimation();
    }
    CUnitBody* body = &m_mind->m_owner->m_body;
    body->MakeStep(elapsed, target->m_position, body->GetMovementSpeed());
    // Original e77f2 re-reads the mind's owner after MakeStep; the body from before the call must not be reused.
    vec3 delta = m_mind->m_owner->m_body.m_position - target->m_position;
    float square = delta.LengthSqr();
    float distance = MathLib::InvSqrt(square) * square;
    // Original e784a BHI also skips Finish for unordered; it completes only on ordered <=.
    if (distance <= m_finishDistance) Finish(m_mind, (EExecutorResult)0);
}

void CAttractorMovementExecutor::StartAnimation()
{
    CUnitBody* body = &m_mind->m_owner->m_body;
    if (body != NULL) {
        body->StopAllAnimations(1200);
        body->StartAnimation(m_animation, false);
    }
}

void CAttractorMovementExecutor::OnAnimationComplete(WeakIntrusivePtr<AnimationInstance>, bool)
{
    StartAnimation();
}
void CAttractorMovementExecutor::OnReceiveDamageFromPlayer(EHitType) {} // Original e78a0 bx lr/nop
void CAttractorMovementExecutor::DebugRender(CAuxRender*) {} // Original e78a4 bx lr/nop

void AddUnitTargetAttractorMovementExecutor(CUnitMind* mind, float distance)
{
    if (mind == NULL) return;
    CAttractorMovementExecutor* executor = new CAttractorMovementExecutor(mind, distance);
    mind->AddExecutor(executor, 0);
    executor->m_animation = executor->m_mind->m_owner->m_body.GetMovementAnimation();
    executor->StartAnimation();
}
