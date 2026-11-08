// u422 full owner: Path/Follow, including the unused GetMovementSpeed call kept by the original.
#include "CPathMovementExecutor.h"
#include "CUnitMind.h"
#include "CUnit.h"
#include "CUnitTarget.h"
#include "CBH_PersonType.h"
#include "CVisualCharacterType.h"
#include "CSwerveGame.h"
#include "DGHelper.h"
#include "CAuxRender.h"
#include "CDH_SoundManager.h"

/** Target scalar initialization precedes animation binding in the original constructor. */
static IGameAIMap::Location CreateEmptyPathLocation()
{
    IGameAIMap::Location location;
    location.m_map = NULL;
    location.m_pointId = 0;
    location.m_syncDistance = 0.0f;
    return location;
}

CPathExecutor::CPathExecutor(CUnitMind* mind) : m_mind(mind), m_currentPoint(0),
    m_target(CreateEmptyPathLocation()), m_stuckTime(0)
{
    // Per-point default construction of m_path only clears vec3; original 0xe89c4 does not initialize the two scalars and pointCount.
    // Original e89e0 zeroes all 36B of the target; the shared default Location leaves scalars undefined, so the owner restores these three explicitly.
    // The actual memset also covers the adjacent currentPoint and WeakPtr, 52B in total; only then bind(NULL).
    m_footstepDelay = 10;
    m_footstepSound = 137;
    CUnit* unit = m_mind->m_owner;
    m_footstepsEnabled = unit->m_body.m_personType->GetVisualObject(unit->m_body.m_seed)->m_name == L"Zombie Bs Man";
}
void CPathExecutor::StartAnimation()
{
    CUnitBody* body = &m_mind->m_owner->m_body;
    if (body != NULL) {
        body->StopAllAnimations(1200);
        m_animation = body->StartAnimation(body->GetMovementAnimation(), false);
    }
}
void CPathExecutor::OnPause() {} // Original e8ab8 bx lr/nop
void CPathExecutor::OnResume() { StartAnimation(); }
void CPathExecutor::OnAnimationComplete(WeakIntrusivePtr<AnimationInstance> animation, bool)
{
    if (animation == m_animation) StartAnimation();
}
void CPathExecutor::PlayFootstepsSound()
{
    if (m_footstepDelay > 0) return;
    ((App*)WindowApp::m_instance)->m_soundManager->Play((CDH_SoundManager::eSound)m_footstepSound, false, false);
    if (m_footstepSound == 137) m_footstepSound = 138;
    else m_footstepSound = 137;
    m_footstepDelay = 660;
}
void CPathExecutor::Tick(int elapsed)
{
    if (m_currentPoint >= m_path.m_pointCount || m_path.m_pointCount == 0) {
        Finish(m_mind, (EExecutorResult)1);
        return;
    }
    CUnitBody* body = &m_mind->m_owner->m_body;
    vec3 position = body->m_position;
    vec3 delta;
    float square;
    while (true) {
        delta = m_path.m_points[m_currentPoint].m_position - position;
        square = delta.LengthSqr();
        // e8bbe BPL includes unordered; the path point is consumed only when strictly less than 0.7.
        if (!(square < 0.7f)) break;
        int previous = m_currentPoint - 1;
        if (previous == -1) {
            m_mind->OnPathComplete();
            Finish(m_mind, (EExecutorResult)0);
            return;
        }
        m_currentPoint = previous;
    }
    body->GetMovementSpeed(); // Original e8bd8 kept; its return value is not used for the subsequent normalization.
    float distance = MathLib::InvSqrt(square) * square;
    if (distance > 0.0f) {
        float inverseDistance = 1.0f / distance;
        body->SetTaskForce((delta * 3.0f) * inverseDistance);
        body->SetOrientationControl(CUnitBody::ORIENTATION_MOVEMENT);
    } else {
        body->SetOrientationControl(CUnitBody::ORIENTATION_NONE);
    }
    float moved = (m_lastPosition - position).LengthSqr();
    if (!(moved < 0.15f) || m_mind->m_owner->m_body.m_spawning) {
        m_stuckTime = 0;
        m_lastPosition = position;
    } else {
        m_stuckTime += elapsed;
        if (m_stuckTime >= 11000) m_mind->OnDeath();
    }
    if (m_footstepsEnabled) {
        m_footstepDelay -= elapsed;
        PlayFootstepsSound();
    }
}
bool CPathExecutor::IsNearLocation(const vec3& first, const vec3& second)
{
    return (first - second).LengthSqr() < 0.7f;
}
void CPathExecutor::SetTarget(const IGameAIMap::Location& target, bool startAnimation)
{
    m_target = target;
    m_path.m_pointCount = 0;
    CUnit* unit = m_mind->m_owner;
    unit->m_body.SyncLocation();
    unit->m_body.IgnoreSteering(false);
    IGameAIMap* map = ((App*)WindowApp::m_instance)->m_helper->m_swerveGame->GetGameAIMap();
    if (map->FindPath(m_path, unit->m_body.m_location, target, 5.0f, unit->m_body.m_seed)) {
        m_currentPoint = m_path.m_pointCount - 1;
    }
    if (startAnimation) StartAnimation();
}
void CPathExecutor::OnFinish(EExecutorResult)
{
    if (m_animation != WeakIntrusivePtr<AnimationInstance>()) m_animation->setLooped(false);
}
void CPathExecutor::OnChildFinished(EExecutorResult) { SetTarget(m_target, true); }
void CPathExecutor::DebugRender(CAuxRender* render)
{
    if (render == NULL || m_currentPoint < 1) return;
    for (int index = 0; index < m_currentPoint; ++index) {
        render->DrawLine(m_path.m_points[index].m_position,
                         m_path.m_points[index].m_position + vec3(0.0f, 0.0f, 1.0f), 0.03f, (int)0xffffff00);
        render->DrawLine(m_path.m_points[index].m_position + vec3(0.0f, 0.0f, 1.0f),
                         m_path.m_points[index + 1].m_position + vec3(0.0f, 0.0f, 1.0f), 0.03f, (int)0xff00ff00);
    }
}
void CPathExecutor::MoveTo(CUnitMind* mind, IGameAIMap::Location target, bool)
{
    CPathExecutor* executor = new CPathExecutor(mind);
    mind->AddExecutor(executor, 0); // The original third bool parameter is never read; always non-modal.
    executor->SetTarget(target, true);
}
CFollowTargetExecutor::CFollowTargetExecutor(CUnitMind* mind) : CPathExecutor(mind)
{
    m_stopDistanceSquared = 0.7f;
    CUnitTarget* target = mind->m_owner->m_target;
    if (target != NULL) {
        m_target = target->m_location;
        CPathExecutor::SetTarget(m_target, true);
    }
}
void CFollowTargetExecutor::Tick(int elapsed)
{
    CUnit* unit = m_mind->m_owner;
    CUnitTarget* target = unit->m_target;
    if (target == NULL || !target->m_valid) {
        Finish(m_mind, (EExecutorResult)2);
        return;
    }
    if ((target->m_position - unit->m_body.m_position).LengthSqr() < m_stopDistanceSquared) {
        Finish(m_mind, (EExecutorResult)2);
        return;
    }
    IGameAIMap::Location location = target->m_location;
    if (location.m_pointId != m_target.m_pointId) SetTarget(location, false);
    else m_path.m_points[0].m_position = location.m_position;
    CPathExecutor::Tick(elapsed);
}
CFollowTargetExecutor* CFollowTargetExecutor::Create(CUnitMind* mind, bool modal)
{
    CFollowTargetExecutor* executor = new CFollowTargetExecutor(mind);
    mind->AddExecutor(executor, modal);
    return executor;
}
