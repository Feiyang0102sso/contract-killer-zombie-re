// u425: first creates a four-meter flee target, then attaches the Attractor child executor.
#ifndef CRUNAWAYEXECUTOR_H
#define CRUNAWAYEXECUTOR_H
#include "IExecutor.h"
class CRunAwayExecutor : public IExecutor {
public:
    CRunAwayExecutor(CUnitMind* mind) : m_mind(mind) {}
    virtual void OnChildFinished(EExecutorResult);
    virtual const wchar_t* GetName() const { return L"CRunAwayExecutor"; }
    void OnStart();
    CUnitMind* m_mind; // +0x0c, original 0xe994c allocates 0x10
};
void AddRunAwayExecutor(CUnitMind*);
#endif
