// CNGSLoginFlow.cpp - runs the login steps queued by the game one after another.
#include "CNGSLoginFlow.h"
#include "CNGS.h"
#include "CNGSLocalUser.h"
#include "CNotificationHandler.h"

CNGSLoginFlow::CNGSLoginFlow()
    : CSingleton(ClassId)
    , m_currentStep(-1)
{
    m_unused28 = -1;
    Reset();
}

void CNGSLoginFlow::Reset()
{
    m_finished = 0;
    m_currentStep = -1;
    for (int index = 0; index < m_steps.Size(); index++) {
        if (m_steps[index] != NULL) {
            delete m_steps[index];
            m_steps[index] = NULL;
        }
    }
    m_steps.Clear();
}

CNGSLoginFlow::~CNGSLoginFlow()
{
    Reset();
}

unsigned char CNGSLoginFlow::IsRunning()
{
    return m_currentStep >= 0 && m_currentStep < m_steps.Size();
}

void CNGSLoginFlow::Queue(CNGSLoginStep* step)
{
    m_steps.Add(step);
}

void CNGSLoginFlow::Start()
{
    if (m_steps.Size() > 0) {
        m_finished = 0;
        PrepareStep(0);
    } else {
        // nothing queued: the flow is done at once
        m_finished = 1;
    }
}

void CNGSLoginFlow::PrepareStep(int step)
{
    if (step != -1) {
        m_currentStep = step;
    }
    if (m_currentStep >= 0 && m_currentStep < m_steps.Size()) {
        m_steps[m_currentStep]->Prepare();
    }
}

void CNGSLoginFlow::OnEvent(eLoginFlowEvent event, const char* caller)
{
    if (m_currentStep >= 0 && m_currentStep < m_steps.Size()) {
        m_steps[m_currentStep]->OnEvent(event, caller);
    }
}

void CNGSLoginFlow::NextStep()
{
    if (m_currentStep >= 0 && m_currentStep < m_steps.Size()) {
        m_currentStep++;
        CNGS::GetInstance()->GetLocalUser()->m_notificationHandler->HandleLoginFlowNextStep();
        if (m_currentStep != m_steps.Size()) {
            PrepareStep(m_currentStep);
        } else {
            m_finished = 1;
        }
    }
}

void CNGSLoginStep_CheckVersion::Excute()
{
    CNGS::GetInstance()->GetLocalUser()->CheckServerVersionCompatibility();
}
