// PROVISIONAL (created by b080): owner src/FPSGameplay/CUnitsController.cpp not written yet
// Listener ABI, spawn records and the active-unit list used by CZombieMission only.
// b070g: complete owner restored from original 0x108bd0/0x10a424; +0x38 is actually the zombie count, not a Vector.
#ifndef CUNITSCONTROLLER_H
#define CUNITSCONTROLLER_H

#include "Vector_ss.h"
#include "CPersonsGroupManager.h"
#include "HashTable.h"
#include "CCollision.h"
class CUnit;
class CSceneObject;
class CSwerveGame;
class CDH_Weapon;
class CGrenade;

/** Spawn request passed by value; constructor visible at 0x0f8410 (N_SOL CUnitsController.h). */
struct SSpawnInfo : public SSpawnablePerson {
    SSpawnInfo() {}
    SSpawnInfo(SSpawnablePerson person, XString point, int waveId)
    {
        static_cast<SSpawnablePerson&>(*this) = person;
        m_spawnPoint = point;
        m_waveId = waveId;
    }

    XString m_spawnPoint;      // 0x14
    int m_waveId;              // 0x18 (default constructor deliberately leaves it untouched)
};

class CUnitsController {
public:
    /** Original constructor/destructor register and detach from a controller when non-null. */
    class Listener {
    public:
        Listener(CUnitsController* controller);
        virtual ~Listener();
        virtual void OnUnitSpawned(CUnit* unit) {}                 // 0x08; original 0xf80e4 bx lr+nop
        virtual void OnUnitKilled(CUnit* unit) {}                  // 0x0c; original 0x10ae00 bx lr+nop
        virtual void OnUnitRemoved(CUnit* unit) {}                 // 0x10; original 0xf80e8 bx lr+nop
        virtual void OnUnitInjured(CUnit* unit, CUnit* attacker, int amount) {} // 0x14; original 0x10ae04 bx lr+nop

        CUnitsController* m_controller;                           // 0x04
    };

    // The remaining controller layout and methods belong to its owner.
    struct SHudUnitInfo {
        int m_id;
        int m_hp;
        int m_maxHp;
        int m_screenX;
        int m_screenY;
        bool m_civilian;
        float m_cameraDistance;
    };
    CUnitsController(CSwerveGame* game, const XString& missionName);
    ~CUnitsController();
    void InitUnits();
    int SpawnUnit(const SSpawnInfo& info, int flags); // 0x109078
    void AddUnitInSpawnQueue(const SSpawnInfo& info); // 0x109250
    CUnit* GetUnitByUniqueId(int id); // 0x10a3e4
    void Update(unsigned int elapsedMs);
    void UpdateMissionSpawns(unsigned int elapsedMs);
    void UpdateSpawnQueue(unsigned int unused);
    void UpdateUnitsDeath(unsigned int unused);
    void UpdateUnits(unsigned int elapsedMs);
    void UpdateObjects(unsigned int elapsedMs);
    CSceneObject* GetSceneObjectInPoint(const XString& point);
    void CallUnitDead(CUnit* unit);
    void CallUnitInjured(CUnit* unit, CUnit* attacker, int damage);
    void DeleteUnitByNum(int index);
    void AvoidCollision(CUnit* unit);
    void UpdateHudUnitsInfo();
    SHudUnitInfo CalcCivilianInfo(CUnit* unit);
    CUnit* GetNearestUnit(const vec3& position, bool civilian, bool aliveOnly) const;
    int Shot(float x, float y, const CDH_Weapon* weapon, const CCollision::Line& line);
    void SortUnitPickDistance(Vector<CUnit*>& units);
    void Blast(const vec3& position, float radius, int damage, int power,
               const CDH_Weapon* weapon, const CGrenade* grenade, const CUnit* source);
    void Blast(const vec3& position, float radius, const CDH_Weapon* weapon,
               const CGrenade* grenade, const CUnit* source);
    float GetCiviliansMeanRelativeHP();
    void SetCiviliansMeanRelativeHP(float relativeHp);
    void RemoveAllListeners();
    void ReviveCivilian();
    void DeleteAllZombies();
    void AddListener(Listener* listener);
    bool RemoveListener(Listener* listener);
    void ShootAllZombies();
    void SetWavesPaused(bool paused) { m_wavesPaused = paused; } // Scene 0xf7aa0
    /** Full unit list at +0x18, distinct from the active list at +0x38. */
    const Vector<CUnit*>& GetUnits() const
    {
        return m_units;
    }
    // The original m_data00[0x38] has been split into real fields from the constructor/destructor and calls.
    int m_civiliansHp;                // 0x00
    int m_civiliansMaxHp;             // 0x04
    Vector<CSceneObject*> m_objects;  // 0x08
    Vector<CUnit*> m_units;           // 0x18
    Vector<CUnit*> m_auxiliaryUnits;  // 0x28, list that is only constructed/released; actual use to be verified with the Game owner
    // Formerly m_activeUnits was declared as a Vector and guessed at +0x38; CheckWaveStartConditions only reads its first word; now confirmed to be a count.
    int m_zombieCount;                // 0x38, read by CheckWaveStartConditions @0x1423d4
    Vector<SHudUnitInfo> m_hudUnits;   // 0x3c
    CSwerveGame* m_game;              // 0x4c
    XString m_sceneName;              // 0x50
    XString m_locationName;           // 0x54
    int m_updateCounter58;            // 0x58, zeroed by the constructor/UpdateUnits; semantics of the original reserved field undetermined
    int m_nextUniqueId;               // 0x5c
    HashTable<HashTableKey<int>, CUnit*> m_unitsById; // 0x60
    int m_maxSceneUnits;              // 0x6c
    Vector<Listener*> m_listeners;    // 0x70
    // Initial m_data50 provisional name assumed a CVector; this is the 16-byte game Vector at +0x38.
    // The former temporary opaque layout m_data48[0x80 - 0x48] has been replaced by the fields above.
    bool m_wavesPaused;                // 0x80
    Vector<SSpawnInfo> m_spawnQueue;   // 0x84, total size 0x94
};

#endif
