// PROVISIONAL (created by b078): owner src/FPSGameplay/SceneObject/CSpawnSceneObject.cpp not written yet
#ifndef CSPAWNSCENEOBJECT_H
#define CSPAWNSCENEOBJECT_H
#include "CSceneObject.h"
#include "CSceneObjectManager.h"
/** Constructor declaration only; size 0x38 at 0x132090. */
class CSpawnSceneObject : public CSceneObject {
public:
    CSpawnSceneObject(const CSceneObjectManager::SSpawnSceneObjectInfo* info);
    virtual ~CSpawnSceneObject();
    virtual void SetToPoint(const IGameAIMap* map, int id);
    virtual bool TrySpawn();
    virtual void Tick(int elapsedMs);
    virtual vec3 GetSpawnPositionShift() const { return m_spawnShift; } // original 0x10bbf8
    virtual bool CanBeHandled() { return false; } // original 0x10bc08
    bool m_spawnBusy;              // 0x20
    bool m_spawnRequested;         // 0x21
    int m_spawnTimeMs;             // 0x24
    vec3 m_spawnShift;             // 0x28
    CDH_Animation* m_animation;    // 0x34, total size 0x38
};
#endif
