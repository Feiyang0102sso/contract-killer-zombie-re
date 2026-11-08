// CExecutable.cpp - timed / counted execution.
#include "CExecutable.h"
#include "CApplet.h"
#include "CApp.h"
#include "CAppExecutor.h"

namespace com { namespace glu { namespace platform { namespace components {

const int CExecutable::InfinitelyRunning = -1;
const int CExecutable::OneShot = 1;

int CExecutable::Run()
{
    if (IsRunning()) {
        framework::CAppExecutor* executor = CApplet::m_pApp->GetExecutor();
        m_isExecuting = 1;
        if (executor->GetMode() == framework::CAppExecutor::Mode_Running) {
            // timed: execute once the interval has elapsed
            m_elapsed += executor->GetFrameTime();
            if (m_elapsed >= m_interval) {
                OnExecute();
                if (m_runCount != InfinitelyRunning && m_runCount != 0) {
                    m_runCount--;
                }
                m_elapsed = 0;
            }
        } else {
            OnExecute();
            if (m_runCount != InfinitelyRunning && m_runCount != 0) {
                m_runCount--;
            }
        }
        m_isExecuting = 0;
    }
    if (m_deleteWhenDone && !IsRunning()) {
        delete this;
        return RunResult_Deleted;
    }
    return !m_deleteWhenDone && !IsRunning();
}

} } } }
