// PROVISIONAL (created by b068-b069): owner AI/IExecutor.cpp is not restored yet.
// Only the original executor ABI and proven empty inline callbacks are supplied here.
// b067h promoted: recovery of the complete owner has started; the original GetName constant is UTF32, not the narrow string inferred earlier.
#ifndef IEXECUTOR_H
#define IEXECUTOR_H
#include "AnimationInstance.h"
#include "CStrChar.h"
class CUnit;
class CUnitMind;
class CAuxRender;
using com::glu::platform::components::CStrChar;
/** Values observed by CUnit / CUnitBody; unnamed hit-zone values remain available by cast. */
enum EHitType { HIT_BODY=0, HIT_HEAD=1, HIT_WEAPON=4, HIT_SPECIAL=5 };
enum EExecutorResult { EXECUTOR_CANCELLED=4 };
enum EBrawlRole { BRAWL_ROLE_NONE=0 };
class IExecutor {
public:
    IExecutor(); // 0xeb448, initializes only modal and state.
    virtual ~IExecutor() {} // 0xeb490.
    virtual void Tick(int) {} // 0xe998c.
    virtual void OnAnimationComplete(WeakIntrusivePtr<AnimationInstance>, bool) {} // 0xe8994.
    virtual void OnPathComplete() {} // 0xe76bc.
    virtual void OnTurningComplete() {} // 0xe790c.
    virtual void OnReceiveDamageFromPlayer(EHitType) {} // 0xe76c0.
    virtual void OnWeaponDestroyed() {} // 0xe76c4.
    virtual void OnReceiveDamageFromUnit(CUnit*) {} // 0xe76c8.
    virtual void OnTranquillize() {} // 0xe76cc.
    virtual void OnDeath() {} // 0xe76d0.
    virtual void OnTargetInvalid() {} // 0xe76d4.
    virtual void OnSceneScriptedEvent(CStrChar&) {} // 0xe76d8.
    virtual void OnChildFinished(EExecutorResult) {} // 0xe76dc.
    virtual void OnPause() {} // 0xe7910.
    virtual void OnResume() {} // 0xe7914.
    virtual void OnFinish(EExecutorResult) {} // 0xe76e0.
    virtual const wchar_t* GetName() const = 0; // 0x44; derived GetName returns a string. The original literal is UTF32.
    virtual void DebugRender(CAuxRender*) {} // 0xe860c, slot 0x48.
    void Finish(CUnitMind*, EExecutorResult);
    void CancelChildren(CUnitMind*);
    bool m_modal; // 0x04.
    int m_state;  // 0x08: running 0, paused 1, finished 2; minimum size 0x0c.
};
#endif
