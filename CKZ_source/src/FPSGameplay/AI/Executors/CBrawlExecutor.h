// PROVISIONAL (created by b070g): owner AI/Executors/CBrawlExecutor.cpp not yet recovered.
#ifndef CBRAWLEXECUTOR_H
#define CBRAWLEXECUTOR_H
#include "IExecutor.h"

/** Original vtable 0x4dc0f0: inherits all IExecutor slots; OnChangeRole is the new +0x4c slot.
 * Only the external methods CUnit needs are declared here; no claim of the full field layout or allocatable size.
 */
// b067h promotion: the full size0x30 below comes from the original AddBrawlExecutor and per-field accesses; historical notes above are kept.
class CBrawlExecutor : public IExecutor {
public:
    explicit CBrawlExecutor(CUnitMind* mind)
        : m_idleStarted(false), m_mind(mind), m_attackElapsed(0),
          m_animationDone(true), m_waitRemaining(0) {}
    virtual ~CBrawlExecutor();
    virtual void Tick(int elapsed);
    CUnit* GetVictim();
    bool TryAttack(bool force);
    void OnStart(bool passive);
    virtual void OnTurningComplete();
    virtual bool OnChangeRole(EBrawlRole oldRole, EBrawlRole newRole, CUnit* otherUnit); // Original 0xe7f5c
    virtual void OnAnimationComplete(WeakIntrusivePtr<AnimationInstance> animation, bool completed);
    virtual void OnDeath();
    virtual void OnFinish(EExecutorResult result);
    virtual const wchar_t* GetName() const;
    virtual void DebugRender(CAuxRender* render);
    bool m_idleStarted; // +c
    CUnitMind* m_mind; // +10
    unsigned int m_attackElapsed; // +14
    EBrawlRole m_previousRole; // +18, not initialized by the original constructor.
    bool m_animationDone; // +1c
    unsigned int m_waitRemaining; // +20
    WeakIntrusivePtr<AnimationInstance> m_afraidAnimation; // +24
};
void AddBrawlExecutor(CUnitMind* mind, bool modal, bool passive);
#endif
