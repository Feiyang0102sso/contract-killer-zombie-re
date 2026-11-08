// u422: the two real classes Path and Follow both belong to this owner; map output is consumed in reversed node order.
#ifndef CPATHMOVEMENTEXECUTOR_H
#define CPATHMOVEMENTEXECUTOR_H
#include "IExecutor.h"
#include "CGameAIMap.h"
class CPathExecutor : public IExecutor {
public:
    CPathExecutor(CUnitMind* mind);
    virtual void Tick(int elapsed);
    virtual void OnAnimationComplete(WeakIntrusivePtr<AnimationInstance>, bool);
    virtual void OnChildFinished(EExecutorResult);
    virtual void OnPause();
    virtual void OnResume();
    virtual void OnFinish(EExecutorResult);
    virtual const wchar_t* GetName() const { return L"CPathExecutor"; }
    virtual void DebugRender(CAuxRender*);
    virtual void SetTarget(const IGameAIMap::Location&, bool startAnimation); // +0x4c
    void StartAnimation();
    void PlayFootstepsSound();
    static bool IsNearLocation(const vec3&, const vec3&);
    static void MoveTo(CUnitMind*, IGameAIMap::Location, bool);
    CUnitMind* m_mind;                      // +0x0c
    CSimplePath m_path;                     // +0x10..1413
    int m_currentPoint;                     // +0x1414
    IGameAIMap::Location m_target;          // +0x1418..143b
    WeakIntrusivePtr<AnimationInstance> m_animation; // +0x143c
    bool m_footstepsEnabled;                // +0x1448
    int m_footstepDelay;                    // +0x144c
    int m_footstepSound;                    // +0x1450
    int m_stuckTime;                        // +0x1454
    vec3 m_lastPosition;                    // +0x1458, sizeof=1464
};
class CFollowTargetExecutor : public CPathExecutor {
public:
    CFollowTargetExecutor(CUnitMind*);
    virtual void Tick(int elapsed);
    virtual const wchar_t* GetName() const { return L"CFollowExecutor"; } // The original UTF32 really is this name
    static CFollowTargetExecutor* Create(CUnitMind*, bool modal);
    int m_field1464;              // Not written by the original constructor; its uncertain meaning is kept
    float m_stopDistanceSquared; // +0x1468, original 0xe8fea writes 0.7, sizeof=146c
};
#endif
