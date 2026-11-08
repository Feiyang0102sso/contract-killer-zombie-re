// Original animation timing, restart after completion and derived actions; keeps the original dereference boundary for null weak references.
#include "CPlayAnimationExecutor.h"
#include "CUnit.h"
#include "Fixed.h"

void CPlayAnimationExecutor::Tick(int elapsed)
{
    if (m_remainingMS > 0) {
        m_remainingMS -= elapsed;
        if (m_remainingMS <= 0) {
            if (m_animation.get() != NULL) {
                m_animation->setLooped(false);
            }
            Finish(m_mind, (EExecutorResult)0);
        }
    }
}

void CPlayAnimationExecutor::OnAnimationComplete(WeakIntrusivePtr<AnimationInstance> animation, bool completed)
{
    if (animation == m_animation) {
        if (m_remainingMS < 0) {
            Finish(m_mind, (EExecutorResult)0);
        } else {
            m_animation = m_mind->m_owner->m_body.StartAnimation(m_type, false);
        }
    }
}

void CPlayAnimationExecutor::OnResume()
{
    Finish(m_mind, (EExecutorResult)0);
}

void CPlayAnimationExecutor::OnStart()
{
    CUnitBody* body = &m_mind->m_owner->m_body;
    body->StopAllAnimations(1200);
    m_animation = body->StartAnimation(m_type, false);
    body->IgnoreSteering(true);
}

void CThrowWeaponExecutor::OnAnimationComplete(WeakIntrusivePtr<AnimationInstance> animation, bool completed)
{
    if (animation == m_animation) {
        Finish(m_mind, (EExecutorResult)0);
    }
}

void CThrowWeaponExecutor::Tick(int elapsed)
{
    if (!m_thrown) {
        // Original e9524 reads +14 even when the weak reference is null; no unproven null fallback added.
        float time = Fixed::toFloat(m_animation->m_time);
        if (time >= 1033.3333740234375f) { // Original e955c: ab2a8144.
            m_mind->m_owner->m_body.ThrowWeapon();
            m_thrown = true;
        }
    }
    CPlayAnimationExecutor::Tick(elapsed);
}

void CPutBoxExecutor::OnAnimationComplete(WeakIntrusivePtr<AnimationInstance> animation, bool completed)
{
    if (animation == m_animation) {
        m_mind->m_owner->m_body.ShowBox(false);
        Finish(m_mind, (EExecutorResult)0);
    }
}

void AddPlayAnimationExecutor(CUnitMind* mind, EPersonAnimType animation, bool modal)
{
    if (mind != NULL) {
        CPlayAnimationExecutor* executor = new CPlayAnimationExecutor(mind, animation);
        mind->AddExecutor(executor, modal);
        executor->OnStart();
    }
}

void AddPlayWoundAnimationModalExecutor(CUnitMind* mind)
{
    if (mind != NULL) {
        // The original allocation precedes GetPainAnimation; the argument evaluation position inside the new expression is kept.
        CPlayAnimationExecutor* executor = new CPlayAnimationExecutor(mind, mind->m_owner->m_body.GetPainAnimation());
        mind->AddExecutor(executor, true);
        executor->OnStart();
    }
}

void AddPlayAfraidAnimationModalExecutor(CUnitMind* mind, int duration)
{
    if (mind != NULL) {
        CPlayAnimationExecutor* executor = new CPlayAnimationExecutor(mind, (EPersonAnimType)1);
        executor->m_remainingMS = duration;
        mind->AddExecutor(executor, false); // Original e969c is 0; modal is not changed arbitrarily based on the function name.
        executor->OnStart();
    }
}

void AddThrowWeaponExecutor(CUnitMind* mind)
{
    if (mind != NULL) {
        CThrowWeaponExecutor* executor = new CThrowWeaponExecutor(mind);
        mind->AddExecutor(executor, true);
        executor->OnStart();
    }
}

void AddPutBoxExecutor(CUnitMind* mind)
{
    if (mind != NULL) {
        CPutBoxExecutor* executor = new CPutBoxExecutor(mind);
        mind->AddExecutor(executor, true);
        executor->OnStart();
    }
}
