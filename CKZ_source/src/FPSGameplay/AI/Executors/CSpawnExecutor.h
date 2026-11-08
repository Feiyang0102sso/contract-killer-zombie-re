// Original u427 spawn animation executor, size0x1c; the original weak member does not receive the animation returned by OnStart.
#ifndef CSPAWNEXECUTOR_H
#define CSPAWNEXECUTOR_H
#include "IExecutor.h"

class CSpawnExecutor : public IExecutor {
public:
    explicit CSpawnExecutor(CUnitMind* mind) : m_mind(mind) {}
    virtual ~CSpawnExecutor();
    virtual void Tick(int elapsed);
    virtual void OnPause();
    virtual void OnResume();
    virtual void OnAnimationComplete(WeakIntrusivePtr<AnimationInstance> animation, bool completed);
    virtual void OnFinish(EExecutorResult result);
    void OnStart();
    virtual const wchar_t* GetName() const { return L"CSpawnExecutor"; } // Original 45d6cc.
    CUnitMind* m_mind; // +c
    WeakIntrusivePtr<AnimationInstance> m_animation; // +10, the original only constructs/destructs it.
};

void AddSpawnExecutor(CUnitMind* mind);
#endif
