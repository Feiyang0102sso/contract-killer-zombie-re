// Original death executor: delayed start, weak animation lifetime and finish callback, size0x20.
#ifndef CDEATHEXECUTOR_H
#define CDEATHEXECUTOR_H
#include "IExecutor.h"

class CDeathExecutor : public IExecutor {
public:
    explicit CDeathExecutor(CUnitMind* mind) : m_mind(mind), m_flag10(true), m_started(false) {}
    virtual ~CDeathExecutor();
    virtual void OnAnimationComplete(WeakIntrusivePtr<AnimationInstance> animation, bool completed);
    void OnStart();
    virtual void Tick(int elapsed);
    virtual void OnPause();
    virtual void OnFinish(EExecutorResult result);
    virtual const wchar_t* GetName() const { return L"CDeathExecutor"; } // Original 45d400 UTF32.
    CUnitMind* m_mind; // +c
    bool m_flag10;
    bool m_started; // +11
    WeakIntrusivePtr<AnimationInstance> m_animation; // +14
};

void AddDeathExecutorExecutor(CUnitMind* mind); // The original symbol really repeats Executor.
#endif
