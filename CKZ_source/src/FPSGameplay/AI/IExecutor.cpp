// original u429: the executor initializes only its own state, and finishes after cancelling child nodes in the original order.
#include "IExecutor.h"
#include "CUnitMind.h"

IExecutor::IExecutor()
{
    m_modal = false;
    m_state = 0;
}

void IExecutor::Finish(CUnitMind* mind, EExecutorResult result)
{
    mind->CancelChildrenExecutors(this);
    mind->FinishExecutor(this, result);
}

void IExecutor::CancelChildren(CUnitMind* mind)
{
    mind->CancelChildrenExecutors(this);
}
