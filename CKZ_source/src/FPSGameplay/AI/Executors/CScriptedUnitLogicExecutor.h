// PROVISIONAL (created by b079): owner src/FPSGameplay/AI/Executors/CScriptedUnitLogicExecutor.cpp not written yet
// Registration-facing declarations only; no allocation or complete vtable claim.
// b074i: the two lines above are old placeholder history; the full 0xcc layout is now recovered from original e999c and the two dual-base tables.
#ifndef CSCRIPTEDUNITLOGICEXECUTOR_H
#define CSCRIPTEDUNITLOGICEXECUTOR_H
#include "CStrChar.h"
#include "CScriptedObject.h"
#include "CHumanLogicExecutor.h"
#include "luabridge.h"
using com::glu::platform::components::CStrChar;
class CScriptedUnitLogicExecutor : public CScriptedObject, public CHumanLogicExecutor {
public:
    CScriptedUnitLogicExecutor(CUnitMind* mind, const XString& scriptName);
    virtual bool IsRunCommandBlocked();
    virtual void PushInstanceInLuaState(lua_State* state)
    {
        luabridge::TypeTable::tdstack<CScriptedUnitLogicExecutor>::push(state, *this, 0);
    }
    // Declaration order of the new primary slots comes from the 4dc510 dual tables; the destructor must come after SceneEvent.
    virtual void Tick(int elapsed);
    virtual const wchar_t* GetName() const;
    virtual void OnPathComplete();
    virtual void OnStart();
    virtual void OnChildFinished(EExecutorResult result);
    virtual void OnResume();
    virtual void OnReceiveDamageFromPlayer(EHitType hit);
    virtual void OnReceiveDamageFromUnit(CUnit* attacker);
    virtual void OnWeaponDestroyed();
    virtual void OnDeath();
    virtual void OnTargetInvalid();
    virtual void OnZombieEntersAttackRange();
    virtual void OnZombieEntersFearRange();
    virtual void OnZombieEntersDangerRange();
    virtual void OnSceneScriptedEvent(CStrChar& event);
    // Original eabf0 natural implicit virtual destructor, cleans up Vector, Human and ScriptedObject; no separate empty body written.
    void Init();
    void MoveToTargetImpl(bool modal, bool onAttackDistance);
    int GetHP() const; // 0x132cf4
    int GetMaxHP() const; // 0x132d10
    int GetState() const { return m_scriptState; } // 0x132d2a
    int GetWaypointsCount() const { return m_waypoints.size(); } // 0x132d44
    int GetLifeTimeMS() const { return m_lifeTimeMs; } // 0x132d5e
    int GetZombiesInAttackRange() const { return m_zombiesInAttackRange; } // 0x132d78
    int GetZombiesInFearRange() const { return m_zombiesInFearRange; } // 0x132d92
    int GetZombiesInDangerRange() const { return m_zombiesInDangerRange; } // 0x132dac
    bool HasBox() const; // 0x132dc8
    bool HasTarget() const; // 0x132de4
    bool IsTargetNear() const; // 0x132e00
    bool IsTargetInSpecialRange() const; // 0x132e1c
    bool IsAllZombiesKilled() const; // 0x132e38
    bool IsIdling() const { return m_scriptState == 5; } // 0x132e52
    bool IsMoving() const { return m_scriptState == 2; } // 0x132e6c
    bool IsAttacking() const { return m_scriptState == 3; } // 0x132e86
    bool IsSpawning() const { return m_scriptState == 1; } // 0x132ea0
    void SetNearestCivilianAsTarget(); // 0x132ebc
    void SetNearestZombieAsTarget(); // 0x132ed8
    void SetMovementCarrying(); // 0x132ef4
    void SetMovementRun(); // 0x132f10
    void SetMovementWalk(); // 0x132f2c
    bool IsNearestCivilianMyTarget(); // 0x132f48
    void TeleportInPointWithId(int); // 0x132f64
    void TeleportInPointWithName(CStrChar); // 0x132f80
    void SendScriptedEvent(CStrChar); // 0x132f9c
    void AddWaypointWithId(int); // 0x132fb8
    void AddWaypointWithName(CStrChar); // 0x132fd4
    void MoveToNearestPoint(); // 0x132ff0
    void MoveToRandomPoint(); // 0x13300c
    void MoveToNextWaypoint(); // 0x133028
    void MoveToPointWithName(CStrChar); // 0x133044
    void MoveToPointWithId(int); // 0x133060
    void AttackTarget(); // 0x13307c
    void BeginBrawl(); // 0x133098
    void AttackTargetModal(); // 0x1330b4
    void Idle(); // 0x1330d0
    void IdleMS(int); // 0x1330ec
    void Afraid(); // 0x133108
    void AfraidMS(int); // 0x133124
    void PlayWound(); // 0x133140
    void TakeBox(); // 0x13315c
    void PutBox(); // 0x133178
    void ThrowWeapon(); // 0x133194
    void PickUpWeapon(); // 0x1331b0
    void ThreatenWithWeapon(); // 0x1331cc
    void ShowWeapon(bool); // 0x1331ea
    void PrepareBox(CStrChar); // 0x133206
    void WalkAround(); // 0x133222
    void RunAway(); // 0x13323e
    void Disappear(); // 0x13325a
    void SwitchBrain(CStrChar); // 0x133276
    void MoveToTarget(); // 0x133292
    void MoveToTargetModal(); // 0x1332ae
    void MoveToTargetOnAttackDistance(); // 0x1332ca
    void SetCorpseDisappear(bool); // 0x1332e8
    void AutoKill(); // 0x133304
    void SetShouldBlow(); // 0x133320
    void FeintDeath(); // 0x13333c
    void Enrage(); // 0x133358
    void PlayRage(); // 0x133374
    void SetSpecialZoneDamageMult(float); // 0x133390
    // +0x90 is the CHumanLogicExecutor secondary base; owner must restore it.
    // unsigned char m_data90[0xa8 - 0x90]; // Historical placeholder, now the real Human base class and an independent mind.
    CUnitMind* m_scriptMind; // +a4; must not be merged with Human's +c mind.
    int m_scriptState; // +0xa8; must not be merged with the +8 state of the IExecutor secondary base.
    int m_lifeTimeMs; // +0xac
    int m_zombiesInAttackRange; // +0xb0
    int m_zombiesInFearRange; // +0xb4
    int m_zombiesInDangerRange; // +0xb8
    // int m_waypointsCount; // +0xbc, historical fake field, actually a full Vector<int>.
    Vector<int> m_waypoints; // +bc/c0/c4/c8
};
class CZombieExecutor : public CScriptedUnitLogicExecutor {
public:
    CZombieExecutor(CUnitMind* mind, const XString& name)
        : CScriptedUnitLogicExecutor(mind, name), m_beaconPresent(false) {}
    virtual void Tick(int elapsed);
    virtual bool IsRunCommandBlocked();
    void CheckBeaconPresence();
    bool m_beaconPresent; // +cc, sizeof=d0.
};
void AddScriptedWarriorLogicExecutor(CUnitMind*, const XString&);
#endif
