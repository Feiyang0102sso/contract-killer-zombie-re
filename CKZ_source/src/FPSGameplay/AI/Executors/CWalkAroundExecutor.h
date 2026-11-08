// u428: keeps the original ChooseFutherAction spelling and the two added virtual slots.
#ifndef CWALKAROUNDEXECUTOR_H
#define CWALKAROUNDEXECUTOR_H
#include "IExecutor.h"
class CWalkAroundExecutor : public IExecutor {
public:
    CWalkAroundExecutor(CUnitMind* mind) : m_mind(mind) {}
    virtual void Tick(int elapsed);
    virtual void OnChildFinished(EExecutorResult);
    virtual const wchar_t* GetName() const { return L"CWalkAroundExecutor"; }
    virtual void MoveToNewPoint();        // +0x4c
    virtual void SetPeaceIdleBehaviour(); // +0x50
    void ChooseFutherAction();
    CUnitMind* m_mind; // +0x0c, original 0xeb3fc allocates 0x10
};
void AddWalkAroundExecutor(CUnitMind*);
#endif
