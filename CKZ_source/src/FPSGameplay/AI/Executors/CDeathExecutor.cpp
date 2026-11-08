// Original u419: death phase callbacks; the repeated side effects of pause and finish are kept as in the original.
#include "CDeathExecutor.h"
#include "CUnit.h"
#include "CSwerveGame.h"
#include "App.h"
#include "DGHelper.h"

CDeathExecutor::~CDeathExecutor()
{
    // Original e85dc naturally destructs the weak reference, without extra stopping or deleting of the animation.
}

void CDeathExecutor::OnAnimationComplete(WeakIntrusivePtr<AnimationInstance> animation, bool completed)
{
    Finish(m_mind, (EExecutorResult)0);
}

void CDeathExecutor::OnStart()
{
    CUnit* owner = m_mind->m_owner;
    if (owner != NULL && &owner->m_body != NULL) {
        CUnitBody* body = &owner->m_body;
        ((App*)WindowApp::m_instance)->m_helper->m_swerveGame->GetCurrentCameraPos();
        body->StopAllAnimations(1200);
        body->OnNeutralization();
        body->SetWorldPositionToMassCenter();
        m_animation = body->PlayDeathAnimation(owner->m_blasted);
        body->SetState((CUnitBody::UnitState)4);
        m_started = true;
    }
}

void CDeathExecutor::Tick(int elapsed)
{
    if (!m_started) {
        OnStart();
        return;
    }
    if (m_animation.get() == NULL) {
        Finish(m_mind, (EExecutorResult)0);
    }
}

void CDeathExecutor::OnPause()
{
    CUnit* owner = m_mind->m_owner;
    if (owner != NULL && &owner->m_body != NULL) {
        CUnitBody* body = &owner->m_body;
        Finish(m_mind, (EExecutorResult)0);
        owner->MarkAsDead();
        body->SetState((CUnitBody::UnitState)5);
        owner->OnDeath();
    }
}

void CDeathExecutor::OnFinish(EExecutorResult result)
{
    CUnit* owner = m_mind->m_owner;
    if (owner != NULL && &owner->m_body != NULL) {
        CUnitBody* body = &owner->m_body;
        owner->MarkAsDead();
        body->SetState((CUnitBody::UnitState)5);
        m_mind->CancelAllExecutors(EXECUTOR_CANCELLED);
        owner->OnDeath();
    }
}

void AddDeathExecutorExecutor(CUnitMind* mind)
{
    if (mind != NULL) {
        CDeathExecutor* executor = new CDeathExecutor(mind);
        mind->AddExecutor(executor, true);
    }
}
