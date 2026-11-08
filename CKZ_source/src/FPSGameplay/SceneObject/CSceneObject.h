// PROVISIONAL (created by b078): owner src/FPSGameplay/SceneObject/CSceneObject.cpp not written yet
#ifndef CSCENEOBJECT_H
#define CSCENEOBJECT_H
#include "CSceneObjectManager.h"
#include "MathLib.h"
#include "tws.h"
class IGameAIMap;
class CDH_Animation;
/** Opaque runtime object; only allocation of its 0x38-byte derived objects is used here. */
// b070g: the real base class at original 0x10b278 is only 0x20, the two derived classes are 0x38 each; the earlier opaque view was not evidence of the base-class size.
class CSceneObject {
public:
    CSceneObject(const CSceneObjectManager::SSceneObjectInfo* info, bool pickingEnabled);
    virtual ~CSceneObject();
    virtual vec3 GetSpawnPositionShift() const { return vec3(); } // original 0x10b264, sret of three zero words
    virtual bool CanBeHandled() = 0; // slot0x0c, base class is abstract and has no C1
    virtual void ReceivedDamage(int damage, float distance) {} // original 0x10b8c0 empty callback
    virtual bool IsPickable() { return false; } // original 0x10b8c4
    virtual void Tick(int elapsedMs) {} // original 0x10b8c8 empty callback
    virtual bool TrySpawn() { return true; } // original 0x10b274
    virtual void SetToPoint(const IGameAIMap* map, int id); // slot0x20
    void BindToWorld();
    void SetToPoint(const IGameAIMap* map, XString point);
    bool Pick(float x, float y, float distance, swerve::Camera<CSwerveAssertingErrorHandler>& camera);
    void SetLuminance(float luminance);
    // The original unsigned char m_data[0x38] restored into the fields below from the constructor/destructor; derived parts belong to their respective owners.
    CDH_Animation* m_lightAnimation; // 0x04
    // Early m_runtimeValue08 was only seen being zeroed and was tentatively an int; the complete ctor10b3c4/dtor10b714 proves it is a Group holding an owning reference.
    swerve::Group<CSwerveAssertingErrorHandler> m_mesh; // 0x08, subtree used for pick
    vec3 m_position;               // 0x0c
    XString m_pointName;           // 0x18
    swerve::Group<CSwerveAssertingErrorHandler> m_group; // 0x1c
};
#endif
