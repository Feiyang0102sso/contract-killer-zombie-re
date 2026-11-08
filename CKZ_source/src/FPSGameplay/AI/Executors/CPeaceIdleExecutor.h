// u423: peaceful idle; original 0xe9336 allocates 0x20.
#ifndef CPEACEIDLEEXECUTOR_H
#define CPEACEIDLEEXECUTOR_H
#include "IExecutor.h"
class CPeaceIdleExecutor : public IExecutor {
public:
    CPeaceIdleExecutor(CUnitMind* mind) : m_mind(mind) {}
    virtual void Tick(int elapsed);
    virtual void OnAnimationComplete(WeakIntrusivePtr<AnimationInstance>, bool);
    virtual void OnFinish(EExecutorResult);
    virtual const wchar_t* GetName() const { return L"CPeaceIdleExecutor"; }
    void OnStart(int duration);
    WeakIntrusivePtr<AnimationInstance> m_animation; // +0x0c
    CUnitMind* m_mind;                            // +0x18
    int m_remaining;                            // +0x1c, assigned in OnStart
};
void AddPeaceIdleExecutor(CUnitMind*, int);
#endif
