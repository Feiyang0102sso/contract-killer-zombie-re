// PROVISIONAL (created by b078): owner src/FPSGameplay/SceneObject/CExplosiveSceneObject.cpp not written yet
#ifndef CEXPLOSIVESCENEOBJECT_H
#define CEXPLOSIVESCENEOBJECT_H
#include "CSceneObject.h"
#include "CSceneObjectManager.h"
/** Constructor declaration only; size 0x38 at 0x1320a2. */
class CExplosiveSceneObject : public CSceneObject {
public:
    CExplosiveSceneObject(const CSceneObjectManager::SExplosiveSceneObjectInfo* info);
    virtual ~CExplosiveSceneObject();
    virtual void Tick(int elapsedMs);
    void Explode();
    virtual void ReceivedDamage(int damage, float distance);
    virtual bool CanBeHandled();
    virtual bool IsPickable() { return true; } // original 0x10b270
    int m_damage;                 // 0x20, original ctor copies from info+1c
    float m_damageRadius;         // 0x24, info+20
    int m_hp;                     // 0x28, info+24
    int m_explosionDelayMs;        // 0x2c
    CDH_Animation* m_animation;    // 0x30, original new0x30/destructor virtual release
    bool m_exploded;              // 0x34, total size 0x38
};
#endif
