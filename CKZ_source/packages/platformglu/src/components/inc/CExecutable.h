// CExecutable.h - something that runs a number of times, optionally on a timer
// (base of events and messages).
#ifndef CEXECUTABLE_H
#define CEXECUTABLE_H

#include "CPriorityClass.h"

namespace com { namespace glu { namespace platform { namespace components {

class CExecutable : public core::CPriorityClass {
public:
    static const int InfinitelyRunning;     // m_runCount value: never stops
    static const int OneShot;               // m_runCount value: run once

    /** Run() results. */
    enum RunResult {
        RunResult_Pending = 0,      // more runs to go
        RunResult_Finished = 1,     // done, object still alive
        RunResult_Deleted = 2       // done and deleted itself
    };

    CExecutable()
    {
        m_runCount = OneShot;
        m_interval = 0;
        m_elapsed = 0;
        m_isExecuting = 0;
        m_deleteWhenDone = 0;
    }

    /** Executable running `runCount` times. */  // added by b011x
    explicit CExecutable(int runCount)  // added by b011x
    {  // added by b011x
        m_runCount = runCount;  // added by b011x
        m_interval = 0;  // added by b011x
        m_elapsed = 0;  // added by b011x
        m_isExecuting = 0;  // added by b011x
        m_deleteWhenDone = 0;  // added by b011x
    }  // added by b011x

    virtual ~CExecutable() {}

    /** The work itself. */
    virtual void OnExecute() = 0;

    /** True while runs are left. */
    bool IsRunning() const { return m_runCount != 0; }

    /** Advance the timer and execute when due; see RunResult. */
    int Run();

    int m_runCount;                 // 0x0c remaining runs, InfinitelyRunning for endless
    int m_interval;                 // 0x10 16.16 seconds between runs
    int m_elapsed;                  // 0x14 16.16 seconds since the last run
    unsigned char m_isExecuting;    // 0x18 inside OnExecute()
    unsigned char m_deleteWhenDone; // 0x19 delete itself after the last run
};

} } } }

#endif
