// Original u418 full owner; keeps the repeated queries in the role callback, the animation temporaries and the unguarded paths.
#include "CBrawlExecutor.h"
#include "CUnit.h"
#include "CUnitTarget.h"

extern float MaxSteeringForce;
extern float MaxSteeringDistance;

CBrawlExecutor::~CBrawlExecutor()
{
    m_mind->m_owner->GetBrawlInfo(); // The original destructor does call it; the return value is unused.
}

// Keep the requested force separate from the value sent to the body.
static inline vec3 LimitBrawlForce(const vec3& requested, float length, float limit)
{
    if (length > limit) {
        return requested * (limit / length);
    }
    return requested;
}

void CBrawlExecutor::Tick(int elapsed)
{
    EBrawlRole role = m_mind->m_owner->GetBrawlInfo()->m_role;
    if ((unsigned int)(m_previousRole - 2) <= 1) {
        if ((unsigned int)role < 2 || m_mind->m_owner->m_dead) {
            Finish(m_mind, (EExecutorResult)0);
        } else {
            m_previousRole = role; // Original e79b0 jumps to e7cb4; only the Finish path skips this write.
        }
        return;
    }
    if (role == (EBrawlRole)1 && m_mind->m_owner->GetBrawlInfo()->m_elapsed <= 0) {
        Finish(m_mind, (EExecutorResult)0);
        return;
    }
    if (role == (EBrawlRole)4 && m_animationDone) {
        if (m_waitRemaining > (unsigned int)elapsed) {
            m_waitRemaining -= elapsed;
        } else {
            m_waitRemaining = 0;
            m_mind->m_owner->ChangeBrawlRole((EBrawlRole)1, NULL);
        }
        m_previousRole = role;
        return;
    }
    if (m_mind->m_owner->GetBrawlInfo()->m_role == (EBrawlRole)8) {
        CUnit* victim = GetVictim();
        if (victim != NULL && (!victim->m_dead || !victim->m_corpseDisappear)) {
            m_attackElapsed += elapsed;
            if (m_attackElapsed > 500) {
                m_attackElapsed = 0;
                TryAttack(false);
            }
        } else {
            Finish(m_mind, (EExecutorResult)2);
        }
    } else if (m_mind->m_owner->GetBrawlInfo()->m_role == (EBrawlRole)9) {
        CUnit* victim = GetVictim();
        if (victim != NULL && (!victim->m_dead || !victim->m_corpseDisappear)) {
            m_attackElapsed += elapsed;
            float maxForce = MaxSteeringForce;
            float factor = (maxForce + maxForce) / MaxSteeringDistance;
            CUnit* owner = m_mind->m_owner;
            vec3 requestedForce = (victim->m_body.m_position - owner->m_body.m_position) * factor;
            float squared = requestedForce.m_x * requestedForce.m_x + requestedForce.m_y * requestedForce.m_y + requestedForce.m_z * requestedForce.m_z;
            float length = MathLib::InvSqrt(squared) * squared;
            float limit = maxForce * 0.5f;
            vec3 force = LimitBrawlForce(requestedForce, length, limit);
            owner->m_body.SetTaskForce(force);
            if (m_attackElapsed > 500) {
                m_attackElapsed = 0;
                TryAttack(false);
            }
        } else {
            Finish(m_mind, (EExecutorResult)2);
        }
    } else if (m_mind->m_owner->GetBrawlInfo()->m_role == (EBrawlRole)5) {
        CUnit* victim = GetVictim();
        CUnit* owner = m_mind->m_owner;
        if (victim != NULL && (!victim->m_dead || !victim->m_corpseDisappear)) {
            vec3 position = victim->m_body.m_position;
            float maxForce = MaxSteeringForce;
            float factor = (maxForce + maxForce) / MaxSteeringDistance;
            vec3 requestedForce = (position - owner->m_body.m_position) * factor;
            float squared = requestedForce.m_x * requestedForce.m_x + requestedForce.m_y * requestedForce.m_y + requestedForce.m_z * requestedForce.m_z;
            float length = MathLib::InvSqrt(squared) * squared;
            float limit = maxForce * 0.5f;
            vec3 force = LimitBrawlForce(requestedForce, length, limit);
            owner->m_body.SetTaskForce(force);
            m_mind->m_owner->m_body.SetOrientationToPoint(position, true, 0.2f);
        } else {
            Finish(m_mind, (EExecutorResult)2);
        }
    } else if (m_mind->m_owner->GetBrawlInfo()->m_role == (EBrawlRole)7) {
        CUnit* victim = GetVictim();
        CUnit* owner = m_mind->m_owner;
        if (victim == NULL || (victim->m_dead && victim->m_corpseDisappear)) {
            owner->ChangeBrawlRole((EBrawlRole)10, victim);
        }
    }
    m_previousRole = role;
}

CUnit* CBrawlExecutor::GetVictim()
{
    int id = m_mind->m_owner->GetBrawlInfo()->m_firstUnitId;
    CUnit* victim = NULL;
    if (id >= 1) {
        CUnit* unit = CUnit::GetUnitById(id);
        if (unit != NULL) {
            if (!unit->m_dead || !unit->m_corpseDisappear) {
                victim = unit;
            }
        }
    }
    return victim;
}

bool CBrawlExecutor::TryAttack(bool force)
{
    unsigned int role = m_mind->m_owner->GetBrawlInfo()->m_role;
    if (role - 1 < 3) {
        return false;
    }
    CUnit* owner = m_mind->m_owner;
    CUnit* target = owner->GetTargetUnit();
    if (target != NULL && (!target->m_dead || !target->m_corpseDisappear)) {
        if (target->GetBrawlInfo()->m_role == BRAWL_ROLE_NONE) {
            AddBrawlExecutor(&target->m_mind, false, true);
        }
        owner->SetTarget(target);
        owner->m_body.PlayAttackSound(10000.0f);
        if (target->IsCivilian()) {
            target->m_body.PlayDeathSound(3000.0f);
        }
        // Original e7dac BPL also takes role 9 when unordered; the threshold is double 0.6.
        if (force || (double)owner->m_body.m_speed < 0.6) {
            owner->m_body.SetOrientationToPoint(target->m_body.m_position, false, 0.2f);
            if (owner->IsTargetNear()) {
                owner->ChangeBrawlRole((EBrawlRole)8, target);
                m_idleStarted = false;
            } else if (!m_idleStarted) {
                owner->m_body.StartAnimation((EPersonAnimType)0, true);
                m_idleStarted = true;
            }
        } else {
            m_idleStarted = false;
            owner->ChangeBrawlRole((EBrawlRole)9, target);
            owner->m_body.SetOrientationControl(CUnitBody::ORIENTATION_MOVEMENT);
        }
        return true;
    }
    Finish(m_mind, (EExecutorResult)2);
    return false;
}

void AddBrawlExecutor(CUnitMind* mind, bool, bool passive)
{
    // The original second bool is never read; AddExecutor fixes modal=1.
    if (mind != NULL) {
        CBrawlExecutor* executor = new CBrawlExecutor(mind);
        mind->AddExecutor(executor, true);
        executor->OnStart(passive);
    }
}

void CBrawlExecutor::OnStart(bool passive)
{
    CUnit* owner = m_mind->m_owner;
    owner->GetBrawlInfo()->m_executor = this;
    if (owner->IsCivilian()) {
        owner->SetVulnerability(false);
    }
    if (passive) {
        owner->ChangeBrawlRole((EBrawlRole)1, NULL);
        return;
    }
    CUnit* target = owner->GetTargetUnit();
    if (target != NULL && (!target->m_dead || !target->m_corpseDisappear)) {
        ++target->GetBrawlInfo()->m_elapsed;
    }
    TryAttack(true);
}

void CBrawlExecutor::OnTurningComplete()
{
    CUnit* owner = m_mind->m_owner;
    if (owner != NULL && &owner->m_body != NULL) {
        if (owner->GetBrawlInfo()->m_role != (EBrawlRole)8) {
            return;
        }
        CUnit* victim = GetVictim();
        if (victim != NULL) {
            CUnit::CBrawlInfo* info = victim->GetBrawlInfo();
            if (info->m_secondUnitId == -1 && info->m_role == (EBrawlRole)1) {
                owner->ChangeBrawlRole((EBrawlRole)6, victim);
                victim->ChangeBrawlRole((EBrawlRole)2, owner);
            } else {
                owner->ChangeBrawlRole((EBrawlRole)5, victim);
            }
        }
    }
}

bool CBrawlExecutor::OnChangeRole(EBrawlRole oldRole, EBrawlRole newRole, CUnit* otherUnit)
{
    CUnit* owner = m_mind->m_owner;
    CUnitBody* body = &owner->m_body;
    CUnit::CBrawlInfo* info = owner->GetBrawlInfo();
    if ((unsigned int)oldRole <= 10) {
        if (oldRole <= (EBrawlRole)4 || oldRole == (EBrawlRole)6 ||
            oldRole == (EBrawlRole)7 || oldRole == (EBrawlRole)10) {
            body->StopAllAnimations(0);
        }
    }
    info->Reset();
    info->m_executor = this;
    switch (newRole) {
    case 1:
        m_animationDone = true;
        body->IgnoreSteering(true);
        break;
    case 2:
        body->SetSameOrientation(&otherUnit->m_body, true);
        info->m_secondUnitId = otherUnit->m_id;
        body->StopAllAnimations(0);
        body->StartAnimation((EPersonAnimType)15, true);
        body->IgnoreSteering(true);
        m_animationDone = false;
        break;
    case 3:
        body->SetSameOrientation(&otherUnit->m_body, true);
        info->m_secondUnitId = otherUnit->m_id;
        body->StopAllAnimations(0);
        body->StartAnimation((EPersonAnimType)16, true);
        body->IgnoreSteering(true);
        break;
    case 4:
        body->StopAllAnimations(0);
        if (owner->m_carrying) {
            body->StartAnimation((EPersonAnimType)18, false);
        } else {
            body->StartAnimation((EPersonAnimType)17, false);
        }
        body->IgnoreSteering(true);
        m_waitRemaining = 0;
        break;
    case 5:
        info->m_firstUnitId = otherUnit->m_id;
        body->StopAllAnimations(0);
        body->IgnoreSteering(true);
        body->StartAnimation((EPersonAnimType)6, false);
        break;
    case 6:
        info->m_firstUnitId = otherUnit->m_id;
        body->StopAllAnimations(1200);
        body->StartAnimation((EPersonAnimType)15, false);
        body->IgnoreSteering(true);
        body->SetWorldPosition(otherUnit->m_body.m_position);
        body->SetOrientationControl(CUnitBody::ORIENTATION_NONE);
        break;
    case 7:
        info->m_firstUnitId = otherUnit->m_id;
        body->StopAllAnimations(0);
        body->StartAnimation((EPersonAnimType)16, false);
        body->IgnoreSteering(true);
        body->SetWorldPosition(otherUnit->m_body.m_position);
        body->SetOrientationControl(CUnitBody::ORIENTATION_NONE);
        break;
    case 8:
        info->m_firstUnitId = otherUnit->m_id;
        body->StopAllAnimations(0);
        body->IgnoreSteering(true);
        break;
    case 9:
        info->m_firstUnitId = otherUnit->m_id;
        body->IgnoreSteering(false);
        body->SetOrientationControl(CUnitBody::ORIENTATION_MOVEMENT);
        body->StartAnimation(body->GetMovementAnimation(), false);
        break;
    case 10:
        body->StartAnimation((EPersonAnimType)16, true);
        body->IgnoreSteering(true);
        break;
    case 11:
        body->IgnoreSteering(true);
        owner->m_body.m_flag239 = oldRole == (EBrawlRole)7 || oldRole == (EBrawlRole)3;
        newRole = BRAWL_ROLE_NONE;
        break;
    default:
        break;
    }
    info->m_role = newRole;
    return true;
}

void CBrawlExecutor::OnAnimationComplete(WeakIntrusivePtr<AnimationInstance>, bool)
{
    // The original callback does not compare animation/completed; it operates on the current role.
    CUnit* owner = m_mind->m_owner;
    CUnit* victim = GetVictim();
    CUnitBody* body = &owner->m_body;
    int damage = owner->GetAttackDamage();
    EBrawlRole role = owner->GetBrawlInfo()->m_role;
    if (role == (EBrawlRole)5) {
        if (victim == NULL || (victim->m_dead && victim->m_corpseDisappear)) {
            Finish(m_mind, (EExecutorResult)2);
            return;
        }
        CUnitTarget* target = owner->m_target;
        if (target != NULL) {
            float speed = owner->m_body.m_speed;
            if (target->Attack(owner->m_id, damage / 5)) {
                // Original e823a BPL sends unordered together with >=0.3 to role 9.
                if ((double)speed < 0.3) {
                    CUnit::CBrawlInfo* info = victim->GetBrawlInfo();
                    if (info->m_secondUnitId == -1 && info->m_role == (EBrawlRole)1) {
                        owner->ChangeBrawlRole((EBrawlRole)6, victim);
                        victim->ChangeBrawlRole((EBrawlRole)2, owner);
                    } else {
                        owner->ChangeBrawlRole((EBrawlRole)5, victim);
                        body->SetOrientationToPoint(victim->m_body.m_position, true, 0.2f);
                    }
                } else {
                    owner->ChangeBrawlRole((EBrawlRole)9, victim);
                    body->SetOrientationControl(CUnitBody::ORIENTATION_MOVEMENT);
                }
            }
        }
    } else if (role == (EBrawlRole)4) {
        if (!m_animationDone) {
            m_animationDone = true;
            if (owner->GetBrawlInfo()->m_elapsed >= 1) {
                m_waitRemaining = 2000;
                body->StopAllAnimations(0);
                m_afraidAnimation = body->PlayAfraidAnimation();
                m_afraidAnimation->setLooped(true); // The original has no guard for a null weak reference.
            } else {
                m_waitRemaining = 0;
                owner->ChangeBrawlRole((EBrawlRole)1, NULL);
            }
        }
    } else if ((unsigned int)(role - 6) <= 1) {
        if (victim == NULL || (victim->m_dead && victim->m_corpseDisappear)) {
            owner->ChangeBrawlRole((EBrawlRole)10, victim);
        } else {
            CUnitTarget* target = owner->m_target;
            if (target == NULL || !target->Attack(m_mind->m_owner->m_id, damage)) {
                Finish(m_mind, (EExecutorResult)2);
                return;
            }
            if (victim->m_hp >= 1) {
                victim->ChangeBrawlRole((EBrawlRole)3, owner);
                owner->ChangeBrawlRole((EBrawlRole)7, GetVictim());
            } else {
                owner->ChangeBrawlRole((EBrawlRole)10, victim);
            }
        }
    }
}

void CBrawlExecutor::OnDeath()
{
    m_mind->m_owner->GetBrawlInfo();
    Finish(m_mind, (EExecutorResult)2);
}

void CBrawlExecutor::OnFinish(EExecutorResult)
{
    if (m_mind->m_owner->IsCivilian()) {
        m_mind->m_owner->SetVulnerability(true);
    }
    CUnit* owner = m_mind->m_owner;
    CUnit* victim = GetVictim();
    if (victim != NULL) {
        if (victim->GetBrawlInfo()->m_secondUnitId == owner->m_id) {
            if (victim->m_hp >= 1) {
                victim->ChangeBrawlRole((EBrawlRole)4, NULL);
            } else if (victim->GetBrawlInfo()->m_role != BRAWL_ROLE_NONE) {
                victim->ChangeBrawlRole(BRAWL_ROLE_NONE, NULL);
            }
        }
        --victim->GetBrawlInfo()->m_elapsed;
    }
    EBrawlRole role = BRAWL_ROLE_NONE;
    if (owner->m_hp < 1) {
        role = (EBrawlRole)11;
    }
    owner->ChangeBrawlRole(role, NULL);
    owner->GetBrawlInfo()->m_executor = NULL;
}

const wchar_t* CBrawlExecutor::GetName() const
{
    return L"CBrawlExecutor"; // Original 45d3c4 full UTF32 literal.
}

void CBrawlExecutor::DebugRender(CAuxRender*) {} // Original e8438 is a real 2B empty function.
