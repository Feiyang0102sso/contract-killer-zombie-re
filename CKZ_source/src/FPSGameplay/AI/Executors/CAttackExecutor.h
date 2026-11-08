// Original u416 attack state machine, size0x24; GetName string verified against the original UTF32 data.
#ifndef CATTACKEXECUTOR_H
#define CATTACKEXECUTOR_H
#include "IExecutor.h"

class CAttackExecutor : public IExecutor {
public:
    explicit CAttackExecutor(CUnitMind* mind) : m_mind(mind), m_attackDelay(0), m_attackState(2) {}
    virtual ~CAttackExecutor() {}
    virtual void Tick(int elapsed);
    void RealAttack();
    virtual void OnAnimationComplete(WeakIntrusivePtr<AnimationInstance> animation, bool completed);
    virtual void OnTurningComplete();
    void ProcessState();
    void OnStart();
    void OrientBodyToTarget();
    void RunAttackAnimation();
    virtual void OnPause();
    virtual void OnResume();
    virtual void DebugRender(CAuxRender* render);
    virtual const wchar_t* GetName() const { return L"CAttackExecutor"; } // Original 45d318.
    CUnitMind* m_mind; // +c
    int m_attackDelay; // +10
    WeakIntrusivePtr<AnimationInstance> m_animation; // +14
    int m_attackState; // +20
};

void AddAttackExecutor(CUnitMind* mind, bool modal);
#endif
