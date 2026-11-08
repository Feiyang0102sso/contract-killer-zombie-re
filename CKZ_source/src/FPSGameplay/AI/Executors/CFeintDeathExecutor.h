// Original u420: weak references to the three animations feign-death, get-up and flee, plus the state machine, size0x3c.
#ifndef CFEINTDEATHEXECUTOR_H
#define CFEINTDEATHEXECUTOR_H
#include "IExecutor.h"

class CFeintDeathExecutor : public IExecutor {
public:
    enum EFeintDeathState {
        FEINT_FALL=0, FEINT_WAIT=1, FEINT_RISE=2,
        FEINT_FINISH=3, FEINT_ANIMATION=4, FEINT_DEAD=5
    };
    explicit CFeintDeathExecutor(CUnitMind* mind);
    virtual ~CFeintDeathExecutor();
    void SwitchState(EFeintDeathState state);
    virtual void Tick(int elapsed);
    virtual void OnPause();
    virtual void OnResume();
    virtual void OnAnimationComplete(WeakIntrusivePtr<AnimationInstance> animation, bool completed);
    virtual void OnFinish(EExecutorResult result);
    virtual void OnReceiveDamageFromPlayer(EHitType hit);
    virtual void OnReceiveDamageFromUnit(CUnit* unit);
    virtual void OnDeath();
    virtual const wchar_t* GetName() const { return L"CFeintDeathExecutor"; }
    CUnitMind* m_mind; // +c
    WeakIntrusivePtr<AnimationInstance> m_fallAnimation; // +10
    WeakIntrusivePtr<AnimationInstance> m_lastAnimation; // +1c
    WeakIntrusivePtr<AnimationInstance> m_riseAnimation; // +28
    EFeintDeathState m_feintState; // +34
    int m_elapsed; // +38, not initialized until SwitchState(0).
};

void AddFeintDeathExecutor(CUnitMind* mind, bool modal);
#endif
