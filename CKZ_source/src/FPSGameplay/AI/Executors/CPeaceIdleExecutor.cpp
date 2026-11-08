// u423 full owner; the animation callback at original 0xe9278 is a 4B empty body.
#include "CPeaceIdleExecutor.h"
#include "CUnitMind.h"
#include "CUnit.h"
#include "WindowApp.h"

void CPeaceIdleExecutor::Tick(int elapsed)
{
    m_remaining -= elapsed;
    if (m_remaining < 0) Finish(m_mind, (EExecutorResult)0);
}
void CPeaceIdleExecutor::OnAnimationComplete(WeakIntrusivePtr<AnimationInstance>, bool) {} // e9278
void CPeaceIdleExecutor::OnFinish(EExecutorResult)
{
    if (m_animation != WeakIntrusivePtr<AnimationInstance>()) m_animation->setLooped(false);
}
void CPeaceIdleExecutor::OnStart(int duration)
{
    if (duration > 0) m_remaining = duration;
    else m_remaining = WindowApp::m_instance->GetRandomValue().Int(3000) + 500;
    m_animation = m_mind->m_owner->m_body.StartAnimation((EPersonAnimType)0, true);
}
void AddPeaceIdleExecutor(CUnitMind* mind, int duration)
{
    if (mind == NULL) return;
    CPeaceIdleExecutor* executor = new CPeaceIdleExecutor(mind);
    mind->AddExecutor(executor, 0);
    executor->OnStart(duration);
}
