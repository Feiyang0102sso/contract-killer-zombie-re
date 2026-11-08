// PROVISIONAL (created by b079): owner src/FPSGameplay/CUnit.cpp not written yet
// Registration-facing declarations only; no allocation or complete vtable claim.
// b068-b069: complete concrete owner, size 0x3dc, with Body/Mind members; comments above retain history.
#ifndef CUNIT_H
#define CUNIT_H
#include "CStrChar.h"
#include "MathLib.h"
#include "CUnitBody.h"
#include "CUnitMind.h"
#include "CPersonsGroupManager.h"
using com::glu::platform::components::CStrChar;
class CUnitTarget;
class CBrawlExecutor;
class CDH_Weapon;
class CGrenade;
class CUnit {
public:
    struct CBrawlInfo {
        void Reset();
        EBrawlRole m_role;    // 0x00.
        int m_elapsed;        // 0x04.
        int m_firstUnitId;    // 0x08.
        int m_secondUnitId;   // 0x0c.
        CBrawlExecutor* m_executor; // 0x10.
    };
    CUnit(int, const SSpawnablePerson&, void*, const IGameAIMap*, int, XString);
    void RestoreWeaponHP();
    bool IsCivilian(); // 0x133650
    virtual ~CUnit(); // only establishes the known primary vptr
    void Init(const XString&);
    void Spawn();
    void Tick(int);
    bool CanBeHandled() const;
    bool Pick(float, float, float, swerve::Camera<CSwerveAssertingErrorHandler>&);
    void ResetPickInfo();
    bool GetPickInfo(EHitType*);
    void ReceivedDamageFromPlayer(EHitType, const CDH_Weapon*, int);
    void ReceivedDamageFromUnit(int, int);
    void ReceivedFragDamage(int, const CDH_Weapon*, const CGrenade*);
    void ReceivedBlastDamage(int, const vec3&, const CDH_Weapon*, const CGrenade*, bool);
    void Throw(const vec3&);
    void MarkAsRemoved(); // 0x103194 writes true to +0x3a4
    EPersonAnimType GetAnimWeapon() const;
    bool IsTargetNear();
    float GetAttackRange() const;
    bool IsTargetInSpecialRange();
    bool IsBoss();
    bool IsHudInfoShown();
    float GetFearRange() const;
    float GetDangerRange() const;
    void SetVulnerability(bool); // 0x1336a0
    void SetUndying(bool); // 0x1336bc
    bool IsThrowed();
    bool ChangeBrawlRole(EBrawlRole, CUnit*);
    CBrawlInfo* GetBrawlInfo();
    void SetTarget(CUnit*);
    void SetTarget(const vec3&);
    void SetTargetHandGrenade(int);
    CUnit* GetTargetUnit();
    void BlowUp();
    void MarkAsDead();
    void MarkAsBlowing();
    void MarkAsEnraged();
    static CUnit* GetUnitById(int id);
    void SendSceneScriptedEvent(CStrChar); // 0x1336d8
    int GetAttackDamage() const;
    void OnDeath();
    int GetHP() const { return m_hp; } // 0x13361a
    int GetHPMax() const { return m_maxHp; } // 0x133634
    bool IsDead() const { return m_dead; } // 0x13366a
    int GetId() const { return m_id; } // 0x133684
    void SetCorpseDisappear(bool value) { m_corpseDisappear = value; } // 0x1336ea
    bool IsMarkedAsRemoved() const { return m_removed; }
    /** Runtime wave id, distinct from the public id (+0x18); OnUnitKilled at 0x1423xx. */  // added by b080
    int GetWaveId() const { return m_waveId; } // added by b080
    /** Position read by FragGrenade::OnUpdate from the unit at +0x1b0. */
    const vec3& GetPosition() const { return m_body.m_position; }
    // Former provisional m_data04/m_data1c/m_data39c/m_data3a2 opaque areas are now resolved below.
    SSpawnablePerson m_person;  // 0x004.
    int m_id; // +0x18
    CUnitBody m_body;           // 0x01c, complete body including virtual kinematic base.
    CUnitMind m_mind;           // 0x340.
    CUnitTarget* m_target;      // 0x364.
    CBH_PersonType* m_personType; // 0x368.
    CBrawlInfo m_brawl;         // 0x36c.
    XString m_script;           // 0x380.
    XString m_nextScript;       // 0x384.
    EHitType m_pickHit;          // 0x388.
    int m_arcadeCash;           // 0x38c.
    int m_arcadeXp;             // 0x390.
    int m_hp; // +0x394
    int m_maxHp; // +0x398
    float m_relativeHp;         // 0x39c.
    bool m_lowHp;               // 0x3a0.
    bool m_dead; // +0x3a1
    bool m_inactive;            // 0x3a2.
    bool m_blowing;             // 0x3a3.
    bool m_removed;             // 0x3a4.
    bool m_flag3a5;             // 0x3a5.
    bool m_running;             // 0x3a6.
    bool m_carrying;            // 0x3a7.
    bool m_flag3a8;             // 0x3a8.
    bool m_deathNotified;       // 0x3a9.
    bool m_flag3aa;             // 0x3aa.
    bool m_enraged;             // 0x3ab.
    int m_deathCallbackTime;    // 0x3ac.
    int m_waveId;               // 0x3b0.
    int m_nearbyAttack;          // 0x3b4.
    int m_nearbyFear;            // 0x3b8.
    int m_nearbyDanger;          // 0x3bc.
    float m_attackRange;        // 0x3c0.
    float m_fearRange;          // 0x3c4.
    float m_dangerRange;        // 0x3c8.
    int m_weaponHp;             // 0x3cc.
    int m_throwId;              // 0x3d0.
    bool m_vulnerable;          // 0x3d4.
    bool m_undying;             // 0x3d5.
    bool m_blasted;             // 0x3d6.
    bool m_corpseDisappear; // +0x3d7
    float m_specialDamageMultiplier; // 0x3d8.
};
#endif

