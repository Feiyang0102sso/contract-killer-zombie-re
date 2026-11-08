// Full owner based on original e8610..e8894; keeps the truly empty callbacks and the timing of uninitialized timer fields.
#include "CFeintDeathExecutor.h"
#include "CUnit.h"
#include "CPlayAnimationExecutor.h"

CFeintDeathExecutor::CFeintDeathExecutor(CUnitMind* mind)
    : m_mind(mind), m_feintState((EFeintDeathState)-1) {}

CFeintDeathExecutor::~CFeintDeathExecutor() {}

void CFeintDeathExecutor::SwitchState(EFeintDeathState state)
{
    // The original has a signed upper-bound check: negative values only write the state and do not access owner.
    if (state > FEINT_DEAD) {
        return;
    }
    if ((unsigned int)state <= (unsigned int)FEINT_DEAD) {
        CUnit* owner = m_mind->m_owner;
        CUnitBody* body = &owner->m_body;
        switch (state) {
        case FEINT_FALL:
            m_elapsed = 0;
            body->IgnoreSteering(true);
            body->StopAllAnimations(0);
            m_fallAnimation = body->StartAnimation((EPersonAnimType)28, false);
            break;
        case FEINT_RISE:
            body->StopAllAnimations(0);
            m_riseAnimation = body->StartAnimation((EPersonAnimType)29, false);
            break;
        case FEINT_FINISH:
            body->IgnoreSteering(false);
            Finish(m_mind, (EExecutorResult)0);
            break;
        case FEINT_ANIMATION:
            m_lastAnimation = body->StartAnimation((EPersonAnimType)22, false);
            break;
        case FEINT_DEAD:
            owner->m_body.m_flag239 = true;
            Finish(m_mind, (EExecutorResult)0);
            break;
        default:
            break;
        }
    }
    m_feintState = state;
}

void CFeintDeathExecutor::Tick(int elapsed)
{
    if (m_feintState == FEINT_WAIT) {
        m_elapsed += elapsed;
        if (m_elapsed >= 10001) {
            SwitchState(FEINT_RISE);
        }
    }
}

void CFeintDeathExecutor::OnAnimationComplete(WeakIntrusivePtr<AnimationInstance> animation, bool)
{
    if (animation == m_fallAnimation) {
        SwitchState(FEINT_WAIT);
    } else if (animation == m_riseAnimation) {
        SwitchState(FEINT_FINISH);
    } else if (animation == m_lastAnimation) {
        SwitchState(FEINT_DEAD);
    }
}

void CFeintDeathExecutor::OnPause() {} // Original e87cc is a real 4B empty function.
void CFeintDeathExecutor::OnResume() {} // Original e87d0 is a real 4B empty function.
void CFeintDeathExecutor::OnFinish(EExecutorResult) {} // Original e8820 is a real 4B empty function.

void CFeintDeathExecutor::OnReceiveDamageFromPlayer(EHitType)
{
    if (m_feintState == FEINT_WAIT) {
        AddPlayAnimationExecutor(m_mind, (EPersonAnimType)27, true);
    }
}

void CFeintDeathExecutor::OnReceiveDamageFromUnit(CUnit*)
{
    if (m_feintState == FEINT_WAIT) {
        AddPlayAnimationExecutor(m_mind, (EPersonAnimType)27, true);
    }
}

void CFeintDeathExecutor::OnDeath()
{
    SwitchState(FEINT_DEAD);
}

void AddFeintDeathExecutor(CUnitMind* mind, bool modal)
{
    if (mind != NULL) {
        CFeintDeathExecutor* executor = new CFeintDeathExecutor(mind);
        mind->AddExecutor(executor, modal);
        executor->SwitchState(CFeintDeathExecutor::FEINT_FALL);
    }
}
