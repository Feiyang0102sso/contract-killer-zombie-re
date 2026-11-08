// u428 complete owner; ChooseFutherAction keeps the original spelling and two independent CFloat.
#include "CWalkAroundExecutor.h"
#include "CAttractorMovementExecutor.h"
#include "CPeaceIdleExecutor.h"
#include "CUnitMind.h"
#include "CUnit.h"
#include "WindowApp.h"
#include "CRandGen.h"

void CWalkAroundExecutor::Tick(int) {} // original 0xeb2d0 bx lr/nop
void CWalkAroundExecutor::OnChildFinished(EExecutorResult) { ChooseFutherAction(); }
void CWalkAroundExecutor::ChooseFutherAction()
{
    unsigned int action = com::glu::platform::core::CRandGen::GetInstance()->GetRand(100);
    if (action <= 29) SetPeaceIdleBehaviour();
    else MoveToNewPoint();
}
void CWalkAroundExecutor::MoveToNewPoint()
{
    CUnit* unit = m_mind->m_owner;
    if (unit == NULL || &unit->m_body == NULL) return;
    float x = WindowApp::m_instance->GetRandomValue().CFloat();
    float y = WindowApp::m_instance->GetRandomValue().CFloat();
    vec3 position = unit->m_body.m_position + vec3(x * 3.0f, y * 3.0f, 0.0f);
    unit->SetTarget(position);
    AddUnitTargetAttractorMovementExecutor(m_mind, CUnitBody::UNIT_BODY_RADIUS);
}
void CWalkAroundExecutor::SetPeaceIdleBehaviour() { AddPeaceIdleExecutor(m_mind, -1); }
void AddWalkAroundExecutor(CUnitMind* mind)
{
    if (mind == NULL) return;
    CWalkAroundExecutor* executor = new CWalkAroundExecutor(mind);
    mind->AddExecutor(executor, 0);
    executor->ChooseFutherAction();
}
