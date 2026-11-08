// CUnitMind.h - owner, executor stack and the original modal event dispatch.
#ifndef CUNITMIND_H
#define CUNITMIND_H
#include "IExecutor.h"
#include "Vector_ss.h"
class CUnitMind {
public:
    CUnitMind(void* context);
    ~CUnitMind();
    void CancelAllExecutors(EExecutorResult result);
    void RemoveDeadExecutors();
    void Init(CUnit* unit);
    IExecutor* GetTop();
    void Tick(int elapsed);
    void AddExecutor(IExecutor*, int modal);
    void FinishExecutor(IExecutor*, EExecutorResult);
    void OnAnimationComplete(WeakIntrusivePtr<AnimationInstance>, bool);
    void OnPathComplete();
    void OnTurningComplete();
    void OnTargetInvalid();
    void OnSceneScriptedEvent(CStrChar&);
    void OnReceiveDamageFromPlayer(EHitType);
    void OnReceiveDamageFromUnit(CUnit*);
    void OnWeaponDestroyed();
    void OnDeath();
    void CancelChildrenExecutors(const IExecutor*);
    CUnit* m_owner;              // 0x00.
    void* m_context;             // 0x04.
    Vector<IExecutor*> m_executors; // 0x08.
    int m_counter18;             // 0x18; constructor observed, meaning not established.
    int m_counter1c;             // 0x1c; constructor observed, meaning not established.
    bool m_cancelling;           // 0x20.
};
#endif
