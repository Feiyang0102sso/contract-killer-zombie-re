// CUnitTarget.h - snapshots of units, map points and active hand-grenade projectiles.
#ifndef CUNITTARGET_H
#define CUNITTARGET_H
#include "CGameAIMap.h"
#include <string.h>
class CUnit;
class CUnitTarget {
public:
    CUnitTarget() : m_valid(false) { __builtin_memset(&m_location, 0, sizeof(m_location)); }
    // The original deletes this non-owning snapshot without a virtual destructor.
    virtual bool IsTargetUnit() const { return false; } // 0x108ba4.
    virtual bool Attack(int attackerId, int damage) const;
    virtual void Update() {} // 0x108ba8.
    virtual bool IsInterruptable() { return true; } // 0x108bac.
    virtual CUnitTarget* Clone() = 0;
    virtual CUnit* GetUnit() const { return NULL; } // 0x108bb0.
    IGameAIMap::Location m_location; // 0x04.
    bool m_valid;                   // 0x28.
    vec3 m_position;                // 0x2c.
    int m_id;                       // 0x38, point target deliberately leaves it untouched.
};
class CUnitTargetUnit : public CUnitTarget {
public:
    CUnitTargetUnit(CUnit*);
    virtual CUnitTarget* Clone();
    virtual bool Attack(int attackerId, int damage) const;
    virtual void Update();
    virtual CUnit* GetUnit() const;
    virtual bool IsTargetUnit() const { return true; } // inline original owner metadata.
};
class CUnitTargetPoint : public CUnitTarget {
public:
    CUnitTargetPoint(const vec3&);
    virtual CUnitTarget* Clone();
};
class CUnitTargetHandGrenade : public CUnitTarget {
public:
    CUnitTargetHandGrenade(int id);
    virtual void Update();
    virtual CUnitTarget* Clone();
    virtual bool Attack(int, int) const { return true; } // original inline returns 1.
    virtual bool IsInterruptable() { return false; } // original inline returns 0.
};
#endif
