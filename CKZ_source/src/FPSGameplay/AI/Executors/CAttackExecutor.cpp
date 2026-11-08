// Original attack executor: state, attack delay, target virtual calls, sound and tracer side effects.
#include "CAttackExecutor.h"
#include "CUnit.h"
#include "CUnitTarget.h"
#include "CBH_PersonType.h"
#include "CEnemyWeaponType.h"
#include "CSwerveGame.h"
#include "App.h"
#include "DGHelper.h"
#include "CAuxRenderText.h"

void CAttackExecutor::Tick(int elapsed)
{
    if (m_attackState == 2) {
        return;
    }
    if (m_attackDelay >= 0) {
        m_attackDelay -= elapsed;
        if (m_attackDelay < 0) {
            RealAttack();
        }
    }
}

void CAttackExecutor::RealAttack()
{
    int damage = m_mind->m_owner->GetAttackDamage();
    CUnit* owner = m_mind->m_owner;
    if (owner->m_target != NULL) {
        owner->m_target->Attack(owner->m_id, damage);
    }
}

void CAttackExecutor::OnAnimationComplete(WeakIntrusivePtr<AnimationInstance> animation, bool completed)
{
    if (animation == m_animation) {
        if (m_attackState == 4) {
            m_attackState = 1;
        } else if (m_attackState == 2) {
            m_attackState = 4;
        } else if (m_attackState == 1) {
            m_attackState = 2;
        }
        ProcessState();
    }
}

void CAttackExecutor::OnTurningComplete()
{
    ProcessState();
}

void CAttackExecutor::ProcessState()
{
    int state = m_attackState;
    while (state == 2) {
        state = 4;
    }
    m_attackState = state;
    if (state == 4) {
        RunAttackAnimation();
    } else if (state == 1) {
        Finish(m_mind, (EExecutorResult)0);
    }
}

void CAttackExecutor::OnStart()
{
    if (!m_mind->m_owner->IsTargetNear()) {
        Finish(m_mind, (EExecutorResult)2);
        return;
    }
    CUnit* owner = m_mind->m_owner;
    CUnitBody* body = &owner->m_body;
    if (body != NULL) {
        body->StopAllAnimations(1200);
        CEnemyWeaponType* weapon = owner->m_body.m_personType->m_weapon;
        if (weapon != NULL) {
            m_attackDelay = weapon->m_attackDelayMS;
        }
        body->PlayAttackSound(10000.0f);
        OrientBodyToTarget();
    }
}

void CAttackExecutor::OrientBodyToTarget()
{
    CUnit* owner = m_mind->m_owner;
    CUnitBody* body = &owner->m_body;
    if (body != NULL && owner->m_target != NULL) {
        vec3 target = owner->m_target->m_position;
        body->SetOrientationToPoint(target, false, .2f);
    }
}

void CAttackExecutor::RunAttackAnimation()
{
    CUnit* owner = m_mind->m_owner;
    CUnitBody* body = &owner->m_body;
    if (body == NULL) {
        return;
    }
    body->StopAllAnimations(1200);
    m_animation = body->PlayShotAnimation();
    body->PlayWeaponSound();
    body->ShowBulletMuzzleFlash();
    CUnitTarget* target = m_mind->m_owner->m_target;
    if (target != NULL) {
        vec3 source(owner->m_body.m_position.m_x + 0.0f, owner->m_body.m_position.m_y + 0.0f,
                    owner->m_body.m_position.m_z + 1.0f);
        vec3 destination(target->m_position.m_x + 0.0f, target->m_position.m_y + 0.0f,
                         target->m_position.m_z + 1.0f);
        vec3 direction = destination - source;
        direction = direction * MathLib::InvSqrt(direction.LengthSqr());
        source = source + direction * 1.5f;
        vec3 velocity = direction * 75.0f;
        CEnemyWeaponType* weapon = owner->m_body.m_personType->m_weapon;
        if (weapon != NULL && weapon->m_hasTracer) {
            ((App*)WindowApp::m_instance)->m_helper->m_swerveGame->AddTracer(source, velocity, 5.0f);
        }
    }
}

void CAttackExecutor::OnPause()
{
    Finish(m_mind, (EExecutorResult)2);
}

void CAttackExecutor::OnResume()
{
    Finish(m_mind, (EExecutorResult)2);
}

void CAttackExecutor::DebugRender(CAuxRender* render)
{
    switch (m_attackState) {
    case 0:
        CAuxRenderText::GetInstance()->AddTextInfo(L"Unit hides");
        break;
    case 1:
        CAuxRenderText::GetInstance()->AddTextInfo(L"Unit reloads");
        break;
    case 2:
        CAuxRenderText::GetInstance()->AddTextInfo(L"Unit idles");
        break;
    case 3:
        CAuxRenderText::GetInstance()->AddTextInfo(L"Unit unhides");
        break;
    case 4:
        CAuxRenderText::GetInstance()->AddTextInfo(L"Unit shots");
        break;
    }
}

void AddAttackExecutor(CUnitMind* mind, bool modal)
{
    if (mind != NULL) {
        CAttackExecutor* executor = new CAttackExecutor(mind);
        mind->AddExecutor(executor, modal);
        executor->OnStart();
    }
}
