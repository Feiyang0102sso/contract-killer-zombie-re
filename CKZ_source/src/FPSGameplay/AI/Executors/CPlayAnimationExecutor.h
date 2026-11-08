// Original u424: plain animation, throw weapon and place box, three classes in one translation unit.
#ifndef CPLAYANIMATIONEXECUTOR_H
#define CPLAYANIMATIONEXECUTOR_H
#include "IExecutor.h"
#include "CEnemyWeaponType.h"

class CPlayAnimationExecutor : public IExecutor {
public:
    CPlayAnimationExecutor(CUnitMind* mind, EPersonAnimType animation)
        : m_mind(mind), m_type(animation), m_remainingMS(-1) {}
    virtual ~CPlayAnimationExecutor() {} // Original e9780 only destructs the weak reference naturally.
    virtual void Tick(int elapsed);
    virtual void OnAnimationComplete(WeakIntrusivePtr<AnimationInstance> animation, bool completed);
    virtual void OnResume();
    void OnStart();
    virtual const wchar_t* GetName() const { return L"CPlayAnimationExecutor"; } // Original 45d5c0.
    CUnitMind* m_mind; // +c
    EPersonAnimType m_type; // +10
    WeakIntrusivePtr<AnimationInstance> m_animation; // +14
    int m_remainingMS; // +20; -1 ends on completion, 0 restarts continuously, positive values are timed.
};

class CThrowWeaponExecutor : public CPlayAnimationExecutor {
public:
    explicit CThrowWeaponExecutor(CUnitMind* mind)
        : CPlayAnimationExecutor(mind, (EPersonAnimType)32), m_thrown(false) {}
    virtual ~CThrowWeaponExecutor() {}
    virtual void OnAnimationComplete(WeakIntrusivePtr<AnimationInstance> animation, bool completed);
    virtual void Tick(int elapsed);
    bool m_thrown; // +24
};

class CPutBoxExecutor : public CPlayAnimationExecutor {
public:
    explicit CPutBoxExecutor(CUnitMind* mind) : CPlayAnimationExecutor(mind, (EPersonAnimType)19) {}
    virtual ~CPutBoxExecutor() {}
    virtual void OnAnimationComplete(WeakIntrusivePtr<AnimationInstance> animation, bool completed);
};

void AddPlayAnimationExecutor(CUnitMind* mind, EPersonAnimType animation, bool modal);
void AddPlayWoundAnimationModalExecutor(CUnitMind* mind);
void AddPlayAfraidAnimationModalExecutor(CUnitMind* mind, int duration);
void AddThrowWeaponExecutor(CUnitMind* mind);
void AddPutBoxExecutor(CUnitMind* mind);
#endif
