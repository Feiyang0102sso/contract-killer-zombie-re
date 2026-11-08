// CUnit.cpp - complete person instance: model/mind composition, damage and targeting (u435).
#include "CUnit.h"
#include "CUnitTarget.h"
#include "CBH_PersonType.h"
#include "CBH_PersonTypeManager.h"
#include "CVisualCharacterType.h"
#include "CEnemyWeaponType.h"
#include "CBrawlExecutor.h"
#include "App.h"
#include "DGHelper.h"
#include "CSwerveGame.h"
#include "CUnitsController.h"
#include "CBalanceManager.h"
#include "CHuntingInfo.h"
#include "CDH_Weapon.h"
#include "CDH_SoundManager.h"
#include "CBH_Player.h"
#include "CPlayerGameStats.h"
#include "CGameAnalytics.h"
#include "CRandGen.h"
#include "Ballistics.h"
#include "DGWorld.h"
void AddScriptedWarriorLogicExecutor(CUnitMind*, const XString&);
using com::glu::platform::core::CRandGen;
namespace {
inline CSwerveGame* Game() { return ((App*)WindowApp::m_instance)->m_helper->m_swerveGame; }
inline float Length(const vec3& value)
{
    float square = value.LengthSqr();
    return MathLib::InvSqrt(square) * square;
}
}

CUnit::CUnit(int id, const SSpawnablePerson& person, void* context, const IGameAIMap* map, int wave, XString script)
    : m_person(person), m_id(id), m_body(person, map), m_mind(context), m_target(NULL),
      m_personType(((App*)WindowApp::m_instance)->m_personTypeManager->GetType(person.m_typeName)), m_script(script)
{
    m_brawl.m_elapsed = 0;
    m_brawl.m_role = BRAWL_ROLE_NONE;
    m_brawl.m_secondUnitId = -1;
    m_brawl.m_firstUnitId = -1;
    m_brawl.m_executor = NULL;
    m_pickHit = HIT_BODY;
    m_removed = false;
    m_flag3a5 = false;
    m_relativeHp = 0.0f;
    m_lowHp = false;
    m_dead = false;
    m_inactive = false;
    m_blowing = false;
    m_running = true;
    m_carrying = false;
    m_flag3a8 = false;
    m_deathNotified = false;
    m_flag3aa = false;
    m_enraged = false;
    m_deathCallbackTime = 0;
    m_waveId = wave;
    m_nearbyAttack = 0;
    m_nearbyFear = 0;
    m_nearbyDanger = 0;
    m_weaponHp = 0;
    m_throwId = -1;
    m_vulnerable = true;
    m_undying = false;
    m_blasted = false;
    m_corpseDisappear = true;
    m_specialDamageMultiplier = 1.0f;
    if (m_personType != NULL) {
        m_hp = (int)((float)m_personType->m_baseHp * person.m_waveMultiplier1);
        if (m_hp < 0) m_hp = 0;
    }
    if (m_personType->m_weapon != NULL) m_weaponHp = m_personType->m_weapon->m_hp;
    m_arcadeCash = (int)(m_personType->m_arcadeCash * person.m_waveMultiplier2);
    m_arcadeXp = (int)(m_personType->m_arcadeXp * person.m_waveMultiplier2);
    m_attackRange = m_personType->m_attackRange;
    m_fearRange = m_personType->m_fearRange;
    m_dangerRange = m_personType->m_dangerRange;
    if (m_personType->m_weapon != NULL) {
        float range = (float)m_personType->m_weapon->m_effectiveRange;
        if (m_attackRange < range) range = m_attackRange;
        m_attackRange = range;
    }
    CBalanceManager* balance = ((App*)WindowApp::m_instance)->m_balanceManager;
    const CBalanceManager::SBalanceSetup& setup = balance->m_setups[balance->m_currentSetup];
    float hpMultiplier = setup.m_zombieHPMultiplier;
    if (m_body.m_personType->m_isCivilian) hpMultiplier = setup.m_civilianHPMultiplier;
    m_hp = (int)((float)m_hp * hpMultiplier);
    m_maxHp = m_hp;
}

void CUnit::RestoreWeaponHP() { if (m_personType->m_weapon != NULL) m_weaponHp = m_personType->m_weapon->m_hp; }
bool CUnit::IsCivilian() { return m_body.m_personType->m_isCivilian; }
CUnit::~CUnit() { if (m_target != NULL) { delete m_target; m_target = NULL; } }
void CUnit::Init(const XString& point) { m_mind.Init(this); m_body.Init(this, point); }
void CUnit::Spawn()
{
    m_body.Spawn();
    AddScriptedWarriorLogicExecutor(&m_mind, m_script);
    if (m_body.m_personType->m_isCivilian) m_corpseDisappear = false;
}
void CUnit::Tick(int elapsed)
{
    if (Game()->IsHeadShotEffectActive()) elapsed /= 3;
    if (m_target != NULL) m_target->Update();
    if ((m_dead || m_inactive || m_removed) && m_throwId == -1) return;
    if (m_maxHp > 0) {
        m_relativeHp = (float)m_hp / (float)m_maxHp;
        m_lowHp = m_relativeHp < 0.01f;
    }
    m_body.Tick(elapsed);
    m_mind.Tick(elapsed);
    if (m_target != NULL && !m_target->m_valid) {
        m_mind.OnTargetInvalid();
        if (m_target != NULL) { delete m_target; m_target = NULL; }
    }
    if (m_throwId != -1) {
        Ballistics::Projectile& projectile = Game()->GetUnitBodyBallistics()->m_projectiles[m_throwId];
        if (projectile.m_active) m_body.SetWorldPosition(projectile.m_position);
        else m_throwId = -1;
    }
}
bool CUnit::CanBeHandled() const
{
    if (!m_dead && !m_inactive && !m_removed) return true;
    return m_throwId != -1;
}
bool CUnit::Pick(float x, float y, float distance, swerve::Camera<CSwerveAssertingErrorHandler>& camera)
{
    EHitType hit;
    bool result = m_body.Pick(x, y, distance, camera, hit);
    m_flag3a8 = result;
    m_pickHit = hit;
    return result;
}
void CUnit::ResetPickInfo() { m_flag3a8 = false; }
bool CUnit::GetPickInfo(EHitType* hit) { *hit = m_pickHit; return m_flag3a8; }

void CUnit::ReceivedDamageFromPlayer(EHitType hit, const CDH_Weapon* weapon, int split)
{
    if (m_blowing || m_removed) return;
    int oldHp = m_hp;
    // The original reads weapon type before the later NULL check on this civilian branch.
    if (m_body.m_personType->m_isCivilian && !m_vulnerable && weapon->m_projectileType != 2) return;
    if (weapon == NULL) return;
    int damage = weapon->GetShotDamage(Length(m_body.m_position - Game()->GetCurrentCameraPos()));
    switch (weapon->m_weaponType) {
    case 1: case 7: damage /= 7; break;
    case 8: damage /= 2; break;
    case 9: damage /= 3; break;
    }
    if (hit == HIT_HEAD) {
        if (!m_body.m_personType->m_isCivilian) damage = (int)((float)damage * weapon->m_headshotMultiplier);
    } else if (hit == HIT_WEAPON) {
        if (m_weaponHp > 0 && m_personType->m_weapon->m_isDestructible) {
            m_weaponHp -= damage;
            if (m_weaponHp <= 0) { m_body.BlowWeapon(); m_mind.OnWeaponDestroyed(); }
        }
    } else if (hit == HIT_SPECIAL) {
        damage = (int)((float)damage * m_specialDamageMultiplier);
    }
    damage = (int)((float)damage * (1.0f / (float)(split + 1)));
    m_body.m_attackSource = Game()->GetCurrentCameraPos();
    int notifiedDamage = 0;
    if (weapon->m_projectileType == 2) {
        if (m_body.m_personType->m_isCivilian) {
            CBalanceManager* balance = ((App*)WindowApp::m_instance)->m_balanceManager;
            int maximum = (int)((float)m_body.m_personType->m_baseHp * balance->m_setups[balance->m_currentSetup].m_civilianHPMultiplier);
            int healing = 0;
            int available = maximum - oldHp - 1;
            if (available >= 0) {
                healing = available;
                if (healing > maximum - 1) healing = maximum - 1;
            }
            m_hp += healing;
            CBH_Player::GetInstance()->m_gameStats.AddNumHealingGunShots(1);
            notifiedDamage = damage;
        }
    } else {
        m_hp -= damage;
        notifiedDamage = damage;
    }
    int damagedHp = m_hp;
    if (m_undying && damagedHp < m_maxHp / 10) { damagedHp = m_maxHp / 10; m_hp = damagedHp; }
    int goreThreshold = -m_body.m_personType->m_baseHp;
    if (damagedHp <= 0) {
        if (damagedHp < 0) m_hp = 0;
        if (!m_body.m_personType->m_isCivilian) CBH_Player::GetInstance()->m_gameStats.NotifyKill();
    }
    if (m_maxHp > 0) { m_relativeHp = (float)m_hp / (float)m_maxHp; m_lowHp = m_relativeHp < 0.01f; }
    bool headshot = false;
    if (damagedHp < goreThreshold && weapon->m_projectileType != 2 && hit != HIT_HEAD) {
        m_body.Blow();
    } else if (hit == HIT_HEAD && weapon->m_projectileType != 2 && !m_body.m_personType->m_isCivilian && m_hp == 0) {
        headshot = true;
        m_body.Behead();
        WindowApp::HandleTunnelCommand((int)0xe3735e88, 0, 0, 0);
    }
    if (!m_body.m_personType->m_isCivilian) CBH_Player::GetInstance()->m_gameStats.NotifyHit();
    if (headshot) {
        if (!m_body.m_personType->m_isCivilian) {
            CBH_Player::GetInstance()->m_gameStats.NotifyHeadshot();
            if ((m_body.IsRunning() && m_body.m_visualType->m_runSpeed >= 2.99f) ||
                (!m_body.IsRunning() && m_body.m_visualType->m_walkSpeed >= 2.99f)) {
                CBH_Player::GetInstance()->m_gameStats.NotifyRunningHeadshot();
            }
        } else {
            CBH_Player::GetInstance()->m_gameStats.NotifyCivilianHeadshot();
        }
    }
    if (damagedHp < goreThreshold) CBH_Player::GetInstance()->m_gameStats.NotifyGoreshot();
    if (weapon->m_projectileType != 2) m_mind.OnReceiveDamageFromPlayer(hit);
    if (m_hp == 0) {
        int before = ((App*)WindowApp::m_instance)->m_huntingInfo->GetCounter5c();
        m_mind.OnDeath();
        m_body.PlayDeathSound(3000.0f);
        m_deathCallbackTime = ((App*)WindowApp::m_instance)->m_huntingInfo->GetCounter5c() - before;
        Game()->OnUnitKilled(this, hit == HIT_HEAD);
    }
    CSwerveGame* game = Game();
    game->ShowUnitInfo(this, notifiedDamage);
    game->GetUnitsController()->CallUnitInjured(this, NULL, notifiedDamage);
    if (hit == HIT_HEAD) ((App*)WindowApp::m_instance)->m_soundManager->Play((CDH_SoundManager::eSound)23, false, false);
    else if (m_hp > 0) m_body.PlayWoundSound(2000.0f);
    CUnitBody::EHitEffect effect = CUnitBody::HIT_EFFECT_HEAL;
    if (weapon->m_projectileType != 2) {
        effect = CUnitBody::HIT_EFFECT_SLIME;
        if (hit != HIT_SPECIAL) {
            effect = CUnitBody::HIT_EFFECT_BODY;
            if ((float)(oldHp - m_hp) / (float)m_maxHp < 0.05f && !m_body.m_personType->m_isCivilian) effect = CUnitBody::HIT_EFFECT_RICOCHET;
        }
    }
    m_body.DrawHitEffect(effect);
    if (m_hp == 0) {
        if (!m_body.m_personType->m_isCivilian) CGameAnalytics::logZombieKilled(m_body.m_personType->m_name, weapon->m_id, (CGameAnalytics::EKillType)headshot, true);
        else CBH_Player::GetInstance()->m_gameStats.NotifyCivilianKilled();
    }
}

void CUnit::ReceivedDamageFromUnit(int attackerId, int damage)
{
    if (m_blowing || m_removed) return;
    CUnit* attacker = Game()->GetUnitsController()->GetUnitByUniqueId(attackerId);
    if (attacker != NULL) m_body.m_attackSource = attacker->m_body.m_position;
    m_mind.OnReceiveDamageFromUnit(attacker);
    Game()->GetUnitsController()->CallUnitInjured(this, attacker, damage);
    int damagedHp = m_hp - damage;
    m_hp = damagedHp;
    if (m_undying && damagedHp < m_maxHp / 10) { damagedHp = m_maxHp / 10; m_hp = damagedHp; }
    if (damagedHp <= 0) {
        int baseHp = m_body.m_personType->m_baseHp;
        m_hp = 0;
        CBH_Player::GetInstance()->m_gameStats.AddZombiesKilledByEvelyn(1);
        if (m_hp == 0) {
            if (damagedHp < -baseHp) m_body.Blow();
            m_mind.OnDeath();
            Game()->OnUnitKilled(this, false);
        }
    }
    m_body.DrawHitEffect(CUnitBody::HIT_EFFECT_BODY);
    if (m_hp == 0 && m_body.m_personType->m_isCivilian) CBH_Player::GetInstance()->m_gameStats.NotifyCivilianKilled();
}

void CUnit::ReceivedFragDamage(int damage, const CDH_Weapon* weapon, const CGrenade* grenade)
{
    if (grenade == NULL && weapon == NULL) return;
    if (m_blowing || m_removed) return;
    m_hp -= damage;
    if (m_undying && m_hp < m_maxHp / 10) {
        m_hp = m_maxHp / 10;
        if ((unsigned int)m_maxHp + 9u <= 18u) m_hp = 1;
    }
    if (m_hp < 0) m_hp = 0;
    if (m_maxHp > 0) { m_relativeHp = (float)m_hp / (float)m_maxHp; m_lowHp = m_relativeHp < 0.01f; }
    m_mind.OnReceiveDamageFromPlayer(HIT_BODY);
    if (m_hp == 0) {
        m_hp = 0;
        if (!m_body.m_personType->m_isCivilian) {
            CBH_Player::GetInstance()->m_gameStats.NotifyKill();
            CBH_Player::GetInstance()->m_gameStats.NotifyGoreshot();
        }
        int before = ((App*)WindowApp::m_instance)->m_huntingInfo->GetCounter5c();
        m_mind.OnDeath();
        m_deathCallbackTime = ((App*)WindowApp::m_instance)->m_huntingInfo->GetCounter5c() - before;
        if (CRandGen::GetInstance()->GetRand(100) <= 10) m_body.Blow();
        Game()->OnUnitKilled(this, false);
    }
    Game()->ShowUnitInfo(this, damage);
    if (m_hp > 0) m_body.PlayWoundSound(2000.0f);
    else m_body.PlayDeathSound(3000.0f);
    if (m_hp == 0) {
        if (!m_body.m_personType->m_isCivilian) {
            int weaponId = 0;
            if (weapon != NULL) weaponId = weapon->m_id;
            if (grenade != NULL) CGameAnalytics::logZombieKilled(m_body.m_personType->m_name, weaponId, (CGameAnalytics::EKillType)3, true);
            else CGameAnalytics::logZombieKilled(m_body.m_personType->m_name, weaponId, (CGameAnalytics::EKillType)0, true);
        } else {
            CBH_Player::GetInstance()->m_gameStats.NotifyCivilianKilled();
        }
    }
}

void CUnit::ReceivedBlastDamage(int damage, const vec3& source, const CDH_Weapon* weapon, const CGrenade* grenade, bool silent)
{
    if (m_blowing || m_removed) return;
    m_hp -= damage;
    if (m_undying && m_hp < m_maxHp / 10) {
        m_hp = m_maxHp / 10;
        if ((unsigned int)m_maxHp + 9u <= 18u) m_hp = 1;
    }
    if (m_hp < 0) m_hp = 0;
    if (m_maxHp > 0) { m_relativeHp = (float)m_hp / (float)m_maxHp; m_lowHp = m_relativeHp < 0.01f; }
    if (m_weaponHp > 0 && m_personType->m_weapon->m_isDestructible) {
        m_weaponHp -= damage;
        if (m_weaponHp <= 0) { m_body.BlowWeapon(); m_mind.OnWeaponDestroyed(); }
    }
    if (!silent) {
        bool barrel = weapon == NULL && grenade == NULL;
        if (!barrel) CBH_Player::GetInstance()->m_gameStats.NotifyHit();
        m_mind.OnReceiveDamageFromPlayer(HIT_BODY);
        if (m_hp == 0) {
            m_hp = 0;
            if (!m_body.m_personType->m_isCivilian) {
                CBH_Player::GetInstance()->m_gameStats.NotifyKill();
                CBH_Player::GetInstance()->m_gameStats.NotifyGoreshot();
            }
            if (barrel) CBH_Player::GetInstance()->m_gameStats.AddBarrelBlow(1);
            m_deathCallbackTime = 0;
            Game()->OnUnitKilled(this, false);
        }
    }
    if (m_hp == 0) {
        m_mind.OnDeath();
        if (CRandGen::GetInstance()->GetRand(100) <= 33) {
            m_body.Blow();
        } else {
            vec3 velocity(m_body.m_position.m_x - source.m_x, m_body.m_position.m_y - source.m_y, 0.0f);
            float length = Length(velocity);
            float heightFactor = length * 0.01f;
            velocity.m_z = heightFactor * (float)CRandGen::GetInstance()->GetRandRange(25, 33);
            float inverse = MathLib::InvSqrt(velocity.LengthSqr());
            velocity = velocity * inverse;
            int impulse = CRandGen::GetInstance()->GetRandRange(500, 1000);
            // Keep the component * 0.01 * impulse grouping of the original.
            velocity.m_x = (velocity.m_x * 0.01f) * (float)impulse;
            velocity.m_y = (velocity.m_y * 0.01f) * (float)impulse;
            velocity.m_z = (velocity.m_z * 0.01f) * (float)impulse;
            Throw(velocity);
        }
        m_blasted = true;
    }
    Game()->ShowUnitInfo(this, damage);
    if (m_hp > 0) m_body.PlayWoundSound(2000.0f);
    else m_body.PlayDeathSound(3000.0f);
    if (m_hp == 0 && !silent) {
        if (!m_body.m_personType->m_isCivilian) {
            int killType = 3;
            int weaponId = 0;
            if (weapon == NULL) killType = 2;
            else weaponId = weapon->m_id;
            CGameAnalytics::logZombieKilled(m_body.m_personType->m_name, weaponId, (CGameAnalytics::EKillType)killType, true);
        } else {
            CBH_Player::GetInstance()->m_gameStats.NotifyCivilianKilled();
        }
    }
}

void CUnit::Throw(const vec3& velocity)
{
    if (m_throwId != -1 || m_body.m_personType->m_isCivilian) return;
    Ballistics* ballistics = Game()->GetUnitBodyBallistics();
    if (ballistics->m_activeCount >= ballistics->m_capacity) return;
    vec3 position = m_body.m_position;
    m_throwId = ballistics->AddProjectile(position, velocity, 0.0f, this);
}
void CUnit::MarkAsRemoved() { m_removed = true; }
EPersonAnimType CUnit::GetAnimWeapon() const
{
    if (m_personType->m_weapon != NULL) return m_personType->m_weapon->GetCharacterAnimationType();
    return (EPersonAnimType)6;
}
bool CUnit::IsTargetNear()
{
    if (m_target == NULL) return false;
    return m_body.GetDistanceToTarget(m_target) <= m_attackRange * 1.1f;
}
float CUnit::GetAttackRange() const { return m_attackRange; }
bool CUnit::IsTargetInSpecialRange()
{
    if (m_target == NULL) return false;
    return m_body.GetDistanceToTarget(m_target) <= m_body.m_personType->m_specialAttackRange;
}
bool CUnit::IsBoss() { return m_body.m_personType->m_isBoss; }
bool CUnit::IsHudInfoShown() { return m_body.m_personType->m_showHudInfo && !m_body.m_spawning; }
float CUnit::GetFearRange() const { return m_fearRange; }
float CUnit::GetDangerRange() const { return m_dangerRange; }
void CUnit::SetVulnerability(bool value) { m_vulnerable = value; }
void CUnit::SetUndying(bool value) { m_undying = value; }
bool CUnit::IsThrowed() { return m_throwId != -1; }
void CUnit::CBrawlInfo::Reset()
{
    m_role = BRAWL_ROLE_NONE;
    m_firstUnitId = -1;
    m_secondUnitId = -1;
    m_executor = NULL; // m_elapsed intentionally retained.
}
bool CUnit::ChangeBrawlRole(EBrawlRole role, CUnit* other)
{
    if (m_brawl.m_executor->OnChangeRole(m_brawl.m_role, role, other)) { m_brawl.m_role = role; return true; }
    return false;
}
CUnit::CBrawlInfo* CUnit::GetBrawlInfo() { return &m_brawl; }
void CUnit::SetTarget(CUnit* unit)
{
    if (m_target != NULL) { delete m_target; m_target = NULL; }
    m_target = new CUnitTargetUnit(unit);
}
void CUnit::SetTarget(const vec3& point)
{
    if (m_target != NULL) { delete m_target; m_target = NULL; }
    m_target = new CUnitTargetPoint(point);
}
void CUnit::SetTargetHandGrenade(int id)
{
    if (m_target != NULL) { delete m_target; m_target = NULL; }
    m_target = new CUnitTargetHandGrenade(id);
}
CUnit* CUnit::GetTargetUnit()
{
    CUnitTarget* target = m_target;
    if (target == NULL || !target->IsTargetUnit()) return NULL;
    return target->GetUnit();
}
void CUnit::BlowUp()
{
    vec3 position = m_body.m_position;
    vec3 direction(0.0f, 0.0f, 1.0f);
    Game()->GetDGWorld()->slime_blast(position, direction);
    int radius = (int)m_body.m_personType->m_specialAttackRange;
    int damage = m_body.m_personType->m_blowDamage;
    Game()->GetUnitsController()->Blast(position, (float)radius, damage, damage, NULL, NULL, this);
}
void CUnit::MarkAsDead() { m_dead = true; }
void CUnit::MarkAsBlowing() { m_flag3aa = true; }
void CUnit::MarkAsEnraged() { m_enraged = true; }
CUnit* CUnit::GetUnitById(int id) { return Game()->GetUnitsController()->GetUnitByUniqueId(id); }
void CUnit::SendSceneScriptedEvent(CStrChar event) { m_mind.OnSceneScriptedEvent(event); }
int CUnit::GetAttackDamage() const { return m_personType->GetAttackDamage(); }
void CUnit::OnDeath() { if (m_flag3aa && !m_body.m_personType->m_isCivilian) BlowUp(); }
