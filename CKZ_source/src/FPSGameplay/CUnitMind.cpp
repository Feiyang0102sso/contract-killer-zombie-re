// CUnitMind.cpp - executor ownership, modal suspension and callback dispatch (original u437).
#include "CUnitMind.h"
#include "CUnit.h"
#include "WindowApp.h"

CUnitMind::CUnitMind(void* context)
    : m_owner(NULL), m_context(context)
{
    m_counter18 = 0;
    m_counter1c = 0;
    m_cancelling = false;
    m_executors.ensureCapacity(10);
}

CUnitMind::~CUnitMind()
{
    CancelAllExecutors(EXECUTOR_CANCELLED);
    RemoveDeadExecutors();
    SAFE_DELETE_VECTOR(m_executors);
}

void CUnitMind::CancelAllExecutors(EExecutorResult result)
{
    if (m_cancelling) {
        return;
    }
    m_cancelling = true;
    for (int index = 0; index < m_executors.size(); ++index) {
        IExecutor* executor = m_executors[index];
        if (executor != NULL && executor->m_state != 2) {
            executor->m_state = 2;
            executor->OnFinish(result);
        }
    }
    for (int index = 0; index < m_executors.size(); ++index) {
        delete m_executors[index];
        m_executors[index] = NULL;
    }
    m_executors.setSize(0);
    m_cancelling = false;
}

void CUnitMind::RemoveDeadExecutors()
{
    int originalCount = m_executors.size();
    int removed = 0;
    for (int index = 0; index != originalCount; ++index) {
        IExecutor* executor = m_executors[index];
        // The original reads state before the delete's NULL test.
        if (executor->m_state == 2) {
            delete executor;
            m_executors[index] = NULL;
            ++removed;
        } else if (removed != 0) {
            m_executors[index - removed] = executor;
        }
    }
    m_executors.setSize(originalCount - removed);
}

void CUnitMind::Init(CUnit* unit) { m_owner = unit; }

IExecutor* CUnitMind::GetTop()
{
    for (int index = m_executors.size() - 1; index >= 0; --index) {
        IExecutor* executor = m_executors[index];
        if (executor->m_state != 2) {
            return executor;
        }
    }
    return NULL;
}

void CUnitMind::Tick(int elapsed)
{
    RemoveDeadExecutors();
    for (int index = m_executors.size() - 1; index >= 0; --index) {
        IExecutor* executor = m_executors[index];
        executor->Tick(elapsed);
        if (executor->m_modal) {
            break;
        }
    }
}

void CUnitMind::AddExecutor(IExecutor* executor, int modal)
{
    if (modal != 0) {
        IExecutor* top;
        while ((top = GetTop()) != NULL) {
            if (top->m_modal) {
                if (top->m_state == 0) {
                    top->m_state = 1;
                    top->OnPause();
                }
                break;
            }
            FinishExecutor(top, EXECUTOR_CANCELLED);
        }
    }
    executor->m_modal = modal != 0;
    m_executors.addElement(executor);
}

void CUnitMind::FinishExecutor(IExecutor* executor, EExecutorResult result)
{
    if (executor == NULL) {
        return;
    }
    int count = m_executors.size();
    int index = 0;
    while (index < count && m_executors[index] != executor) {
        ++index;
    }
    if (index < 0 || index != count - 1) {
        return;
    }
    executor->m_state = 2;
#ifdef CKZ_PC
    // PORT: a death executor can cancel and delete itself in OnFinish.
    const bool modal = executor->m_modal;
    executor->OnFinish(result);
    if (index >= 1 && !modal && m_executors.size() > index - 1) {
#else
    executor->OnFinish(result);
    if (index >= 1 && !executor->m_modal && m_executors.size() > index - 1) {
#endif
        m_executors[index - 1]->OnChildFinished(result);
    }
    IExecutor* top = GetTop();
    if (top != NULL && top->m_state == 1) {
        top->m_state = 0;
        top->OnResume();
    }
}

void CUnitMind::OnAnimationComplete(WeakIntrusivePtr<AnimationInstance> instance, bool completed)
{
    for (int index = m_executors.size() - 1; index >= 0; --index) {
        IExecutor* executor = m_executors[index];
        if (executor->m_state != 2) {
            executor->OnAnimationComplete(instance, completed);
            break;
        }
    }
}

void CUnitMind::OnPathComplete()
{
    for (int index = m_executors.size() - 1; index >= 0; --index) {
        IExecutor* executor = m_executors[index];
        if (executor->m_state != 2) executor->OnPathComplete();
        if (executor->m_modal) break;
    }
}

void CUnitMind::OnTurningComplete()
{
    for (int index = m_executors.size() - 1; index >= 0; --index) {
        IExecutor* executor = m_executors[index];
        if (executor->m_state != 2) executor->OnTurningComplete();
        if (executor->m_modal) break;
    }
}

void CUnitMind::OnTargetInvalid()
{
    IExecutor* top = GetTop();
    if (top != NULL) top->OnTargetInvalid();
}

void CUnitMind::OnSceneScriptedEvent(CStrChar& event)
{
    // Unlike the other event broadcasts, modal executors do not stop this loop.
    for (int index = m_executors.size() - 1; index >= 0; --index) {
        IExecutor* executor = m_executors[index];
        if (executor->m_state != 2) executor->OnSceneScriptedEvent(event);
    }
}

void CUnitMind::OnReceiveDamageFromPlayer(EHitType hit)
{
    for (int index = m_executors.size() - 1; index >= 0; --index) {
        IExecutor* executor = m_executors[index];
        if (executor->m_state != 2) executor->OnReceiveDamageFromPlayer(hit);
        if (executor->m_modal) break;
    }
}

void CUnitMind::OnReceiveDamageFromUnit(CUnit* attacker)
{
    if (m_owner->IsCivilian()) {
        WindowApp::HandleTunnelCommand(0x58611016, m_owner->GetId(), 0, 0);
    }
    for (int index = m_executors.size() - 1; index >= 0; --index) {
        IExecutor* executor = m_executors[index];
        if (executor->m_state != 2) executor->OnReceiveDamageFromUnit(attacker);
        if (executor->m_modal) break;
    }
}

void CUnitMind::OnWeaponDestroyed()
{
    for (int index = m_executors.size() - 1; index >= 0; --index) {
        IExecutor* executor = m_executors[index];
        if (executor->m_state != 2) executor->OnWeaponDestroyed();
        if (executor->m_modal) break;
    }
}

void CUnitMind::OnDeath()
{
    // Callbacks can shrink the vector. The original returns rather than clamping the index.
    int index = m_executors.size() - 1;
    while (index >= 0 && index < m_executors.size()) {
        IExecutor* executor = m_executors[index];
        if (executor != NULL && executor->m_state != 2) executor->OnDeath();
        --index;
    }
}

void CUnitMind::CancelChildrenExecutors(const IExecutor* parent)
{
    int count = m_executors.size();
    if (parent == NULL) return;
    int parentIndex = -1;
    for (int index = 0; index < count; ++index) {
        if (m_executors[index] == parent) {
            parentIndex = index;
            break;
        }
    }
    if (parentIndex < 0) return;
    int first = parentIndex + 1;
    for (int child = first; child < count; ++child) {
        IExecutor* executor = m_executors[child];
        if (executor != NULL && executor->m_state != 2) {
            // No state write precedes this callback in the original.
            executor->OnFinish(EXECUTOR_CANCELLED);
        }
    }
    for (int child = first; child < count; ++child) delete m_executors[child];
    m_executors.setSize(first);
}
