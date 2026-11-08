// Original spawn executor: discards the temporary weak reference after starting the animation, keeps handling of all completion events.
#include "CSpawnExecutor.h"
#include "CUnit.h"

CSpawnExecutor::~CSpawnExecutor()
{
}

void CSpawnExecutor::Tick(int elapsed)
{
    // Original eb1e4 is only bx lr/nop.
}

void CSpawnExecutor::OnPause()
{
    // Original eb1e8 is only bx lr/nop.
}

void CSpawnExecutor::OnResume()
{
    Finish(m_mind, (EExecutorResult)0);
}

void CSpawnExecutor::OnAnimationComplete(WeakIntrusivePtr<AnimationInstance> animation, bool completed)
{
    Finish(m_mind, (EExecutorResult)0);
}

void CSpawnExecutor::OnFinish(EExecutorResult result)
{
    if (result == 0) {
        m_mind->m_owner->m_body.m_spawning = false;
    }
}

void CSpawnExecutor::OnStart()
{
    CUnitBody* body = &m_mind->m_owner->m_body;
    m_mind->m_owner->m_body.m_spawning = true;
    body->IgnoreSteering(true);
    EPersonAnimType animation = body->GetSpawnAnimation();
    if (animation >= 0) {
        body->StopAllAnimations(0);
        body->StartAnimation(animation, false); // Original eb258/eb25e only destructs the returned temporary, does not assign m_animation.
    } else {
        Finish(m_mind, (EExecutorResult)0);
    }
}

void AddSpawnExecutor(CUnitMind* mind)
{
    if (mind != NULL) {
        CSpawnExecutor* executor = new CSpawnExecutor(mind);
        mind->AddExecutor(executor, false);
        executor->OnStart();
    }
}
