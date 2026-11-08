// Original u421 abstract Human executor, still keeps the IExecutor pure GetName slot; full size0x14.
#ifndef CHUMANLOGICEXECUTOR_H
#define CHUMANLOGICEXECUTOR_H
#include "IExecutor.h"

class CHumanLogicExecutor : public IExecutor {
public:
    CHumanLogicExecutor(CUnitMind* mind, bool flag);
    virtual ~CHumanLogicExecutor() {} // Original e8984 has no member cleanup.
    virtual void OnDeath();
    void RegisterAsDead();
    virtual void OnReceiveDamageFromPlayer(EHitType hit);
    virtual void OnReceiveDamageFromUnit(CUnit* attacker);
    virtual void Tick(int elapsed); // Original e8980 is only bx lr; the abstract policy itself does no per-frame work.
    CUnitMind* m_mind; // +c
    bool m_flag10;
    bool m_flag11;
};
#endif
