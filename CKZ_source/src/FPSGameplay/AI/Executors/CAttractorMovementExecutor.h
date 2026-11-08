// u417: target attractor movement, original 0xe78b4 allocates 0x18; IExecutor real slots unchanged.
#ifndef CATTRACTORMOVEMENTEXECUTOR_H
#define CATTRACTORMOVEMENTEXECUTOR_H
#include "IExecutor.h"
#include "CEnemyWeaponType.h"
class CAttractorMovementExecutor : public IExecutor {
public:
    CAttractorMovementExecutor(CUnitMind* mind, float distance) : m_mind(mind), m_finishDistance(distance) {}
    virtual void Tick(int elapsed);
    virtual void OnAnimationComplete(WeakIntrusivePtr<AnimationInstance>, bool);
    virtual void OnReceiveDamageFromPlayer(EHitType);
    virtual const wchar_t* GetName() const { return L"CAttractorMovementExecutor"; }
    virtual void DebugRender(CAuxRender*);
    void StartAnimation();
    CUnitMind* m_mind;             // +0x0c
    EPersonAnimType m_animation;  // +0x10, initialized after AddExecutor
    float m_finishDistance;      // +0x14
};
void AddUnitTargetAttractorMovementExecutor(CUnitMind*, float);
#endif
