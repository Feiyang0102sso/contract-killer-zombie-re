// u426: Lua-scripted unit policy, Zombie decoy behavior and its natural template closures.
#include "CScriptedUnitLogicExecutor.h"
#include "CUnit.h"
#include "CUnitTarget.h"
#include "CUnitsController.h"
#include "CBH_PersonType.h"
#include "CSwerveGame.h"
#include "CScriptedSceneController.h"
#include "CPathMovementExecutor.h"
#include "CAttackExecutor.h"
#include "CBrawlExecutor.h"
#include "CSpawnExecutor.h"
#include "CPlayAnimationExecutor.h"
#include "CPeaceIdleExecutor.h"
#include "CWalkAroundExecutor.h"
#include "CRunAwayExecutor.h"
#include "CFeintDeathExecutor.h"
#include "DGHelper.h"
#include "App.h"
#include "AdBannerSizes.h"
#include <string.h>

CScriptedUnitLogicExecutor::CScriptedUnitLogicExecutor(CUnitMind* mind, const XString& name)
    : CScriptedObject(name), CHumanLogicExecutor(mind, false), m_scriptMind(mind),
      m_scriptState(0), m_lifeTimeMs(0), m_zombiesInAttackRange(0),
      m_zombiesInFearRange(0), m_zombiesInDangerRange(0)
{
}
void CScriptedUnitLogicExecutor::Tick(int elapsed)
{
    m_lifeTimeMs += elapsed;
    CUnitMind* mind = m_scriptMind;
    CUnit* unit = mind->m_owner;
    if (unit->m_blowing) return;
    if (unit->m_body.m_personType->m_isCivilian) {
        int count = unit->m_nearbyAttack;
        if (count > m_zombiesInAttackRange) {
            OnZombieEntersAttackRange();
            mind = m_scriptMind;
        }
        m_zombiesInAttackRange = count;
        count = mind->m_owner->m_nearbyFear;
        if (count > m_zombiesInFearRange) {
            OnZombieEntersFearRange();
            mind = m_scriptMind;
        }
        m_zombiesInFearRange = count;
        count = mind->m_owner->m_nearbyDanger;
        if (count > m_zombiesInDangerRange) OnZombieEntersDangerRange();
        m_zombiesInDangerRange = count;
    }
    CScriptedObject::Update(elapsed);
}
void CScriptedUnitLogicExecutor::OnPathComplete()
{
    if (m_waypoints.size() > 0) {
        IGameAIMap::Location location;
        // Original e9ab0 clears all 36B; must not rely on the shared default constructor to guess the three scalar values.
        __builtin_memset(&location, 0, sizeof(location));
        m_scriptMind->m_owner->m_body.m_map->LocationFromId(location, m_waypoints[0]);
        if (CPathExecutor::IsNearLocation(location.m_position, m_scriptMind->m_owner->m_body.m_position)) {
            m_waypoints.removeElementAt(0);
            AddLuaCommand(XString("OnWaypointReached"));
        }
    }
}
void CScriptedUnitLogicExecutor::Init()
{
    m_scriptState = 1;
    AddSpawnExecutor(m_scriptMind);
    RunLuaCommand(XString("OnInit"), CStrChar(""), 0);
}
void CScriptedUnitLogicExecutor::OnStart()
{
    m_scriptState = 0;
    m_lifeTimeMs = 0;
    RunLuaCommand(XString("OnStart"), CStrChar(""), 0);
}
void CScriptedUnitLogicExecutor::OnResume() { AddLuaCommand(XString("OnTaskFinished")); }
void CScriptedUnitLogicExecutor::OnChildFinished(EExecutorResult)
{
    if (m_scriptState == 1) OnStart();
    else AddLuaCommand(XString("OnTaskFinished"));
}
void CScriptedUnitLogicExecutor::OnReceiveDamageFromPlayer(EHitType hit)
{
    if (HasLuaMethod("OnDamageFromPlayer")) {
        SLuaCommand command(XString("OnDamageFromPlayer"));
        command.m_parameter = XString::AnsiString(XString((int)hit).c_str());
        AddLuaCommand(command);
    } else {
        CHumanLogicExecutor::OnReceiveDamageFromPlayer(hit);
    }
}
void CScriptedUnitLogicExecutor::OnReceiveDamageFromUnit(CUnit* attacker)
{
    if (HasLuaMethod("OnDamageFromUnit")) AddLuaCommand(XString("OnDamageFromUnit"));
    else CHumanLogicExecutor::OnReceiveDamageFromUnit(attacker);
}
void CScriptedUnitLogicExecutor::OnWeaponDestroyed() { AddLuaCommand(XString("OnWeaponDestroyed")); }
void CScriptedUnitLogicExecutor::OnDeath()
{
    RunLuaCommand(XString("OnDeath"), CStrChar(""), 0);
    CUnit* unit = m_scriptMind->m_owner;
    if (!unit->m_flag3aa) {
        CHumanLogicExecutor::OnDeath();
    } else {
        CUnitBody* body = &unit->m_body;
        body->StopAllAnimations(1200);
        body->OnNeutralization();
        body->SetWorldPositionToMassCenter();
        RegisterAsDead();
        unit->MarkAsDead();
        body->SetState((CUnitBody::UnitState)5);
        unit->OnDeath();
    }
}
void CScriptedUnitLogicExecutor::OnTargetInvalid() { AddLuaCommand(XString("OnTargetInvalid")); }
void CScriptedUnitLogicExecutor::OnZombieEntersAttackRange() { AddLuaCommand(XString("OnZombieEntersAttackRange")); }
void CScriptedUnitLogicExecutor::OnZombieEntersFearRange() { AddLuaCommand(XString("OnZombieEntersFearRange")); }
void CScriptedUnitLogicExecutor::OnZombieEntersDangerRange() { AddLuaCommand(XString("OnZombieEntersDangerRange")); }
void CScriptedUnitLogicExecutor::OnSceneScriptedEvent(CStrChar& event)
{
    RunLuaCommand(XString("OnSceneScriptedEvent"), event, 0);
}
void CScriptedUnitLogicExecutor::SetCorpseDisappear(bool value) { m_scriptMind->m_owner->m_corpseDisappear = value; }
int CScriptedUnitLogicExecutor::GetHP() const { return m_scriptMind->m_owner->m_hp; }
int CScriptedUnitLogicExecutor::GetMaxHP() const { return m_scriptMind->m_owner->m_maxHp; }
bool CScriptedUnitLogicExecutor::HasTarget() const
{
    CUnitTarget* target = m_scriptMind->m_owner->m_target;
    if (target == NULL) return false;
    if (target->IsTargetUnit() != true) return false;
    return m_scriptMind->m_owner->m_target->m_valid;
}
bool CScriptedUnitLogicExecutor::IsTargetNear() const { return m_scriptMind->m_owner->IsTargetNear(); }
bool CScriptedUnitLogicExecutor::HasBox() const { return m_scriptMind->m_owner->m_body.HasBox(); }
bool CScriptedUnitLogicExecutor::IsAllZombiesKilled() const
{
    return ((App*)WindowApp::m_instance)->m_helper->m_swerveGame->IsMissionCompleted();
}
bool CScriptedUnitLogicExecutor::IsTargetInSpecialRange() const { return m_scriptMind->m_owner->IsTargetInSpecialRange(); }
void CScriptedUnitLogicExecutor::MoveToNearestPoint()
{
    CancelChildren(m_scriptMind);
    m_scriptState = 2;
    CUnit* unit = m_scriptMind->m_owner;
    if (unit != NULL) {
        CUnitBody* body = &unit->m_body;
        if (body != NULL) {
            int second;
            int first;
            body->GetCurrentPosition(&first, &second);
        }
    }
}
void CScriptedUnitLogicExecutor::MoveToRandomPoint()
{
    CancelChildren(m_scriptMind);
    m_scriptState = 2;
    int point;
    if (!m_scriptMind->m_owner->m_body.m_map->FindAnotherPoint(&point, 0)) point = 0;
    MoveToPointWithId(point);
}
void CScriptedUnitLogicExecutor::MoveToPointWithId(int point)
{
    CancelChildren(m_scriptMind);
    m_scriptState = 2;
    IGameAIMap::Location location;
    __builtin_memset(&location, 0, sizeof(location));
    m_scriptMind->m_owner->m_body.m_map->LocationFromId(location, point);
    CPathExecutor::MoveTo(m_scriptMind, location, true);
}
void CScriptedUnitLogicExecutor::MoveToPointWithName(CStrChar name)
{
    int point = m_scriptMind->m_owner->m_body.m_map->GetPointIDByName(XString(name.GetCharPtr()));
    MoveToPointWithId(point);
}
void CScriptedUnitLogicExecutor::MoveToNextWaypoint()
{
    if (m_waypoints.size() > 0) MoveToPointWithId(m_waypoints[0]);
}
void CScriptedUnitLogicExecutor::AddWaypointWithName(CStrChar name)
{
    int point = m_scriptMind->m_owner->m_body.m_map->GetPointIDByName(XString(name.GetCharPtr()));
    m_waypoints.addElement(point);
}
void CScriptedUnitLogicExecutor::AddWaypointWithId(int point) { m_waypoints.addElement(point); }
void CScriptedUnitLogicExecutor::PlayWound()
{
    if (!m_scriptMind->m_owner->m_body.m_spawning) AddPlayWoundAnimationModalExecutor(m_scriptMind);
}
void CScriptedUnitLogicExecutor::AttackTarget()
{
    CancelChildren(m_scriptMind);
    m_scriptState = 3;
    // Original ea340 cmn owner,#1c: checks the member Body address; not replaced by an owner null check.
    CUnitBody* body = &m_scriptMind->m_owner->m_body;
    if (body != NULL) AddAttackExecutor(m_scriptMind, true);
}
void CScriptedUnitLogicExecutor::AutoKill()
{
    CUnit* unit = m_scriptMind->m_owner;
    if (unit != NULL) unit->ReceivedDamageFromUnit(unit->m_id, unit->m_maxHp + 1);
}
void CScriptedUnitLogicExecutor::SetShouldBlow()
{
    CUnit* unit = m_scriptMind->m_owner;
    if (unit != NULL) unit->MarkAsBlowing();
}
void CScriptedUnitLogicExecutor::BeginBrawl()
{
    CancelChildren(m_scriptMind);
    m_scriptState = 4;
    CUnitBody* body = &m_scriptMind->m_owner->m_body;
    if (body != NULL) AddBrawlExecutor(m_scriptMind, false, false);
}
void CScriptedUnitLogicExecutor::AttackTargetModal()
{
    CUnitBody* body = &m_scriptMind->m_owner->m_body;
    if (body != NULL) AddAttackExecutor(m_scriptMind, true);
}
void CScriptedUnitLogicExecutor::MoveToTarget() { MoveToTargetImpl(false, false); }
void CScriptedUnitLogicExecutor::MoveToTargetImpl(bool modal, bool onAttackDistance)
{
    CancelChildren(m_scriptMind);
    if (m_scriptMind->m_owner->m_target == NULL) return;
    CFollowTargetExecutor* follow = CFollowTargetExecutor::Create(m_scriptMind, modal);
    if (onAttackDistance) {
        float distance = m_scriptMind->m_owner->GetAttackRange() * 0.9f;
        follow->m_stopDistanceSquared = distance * distance;
    }
    m_scriptState = 2;
}
void CScriptedUnitLogicExecutor::MoveToTargetModal() { MoveToTargetImpl(true, false); }
void CScriptedUnitLogicExecutor::MoveToTargetOnAttackDistance() { MoveToTargetImpl(false, true); }
void CScriptedUnitLogicExecutor::Idle()
{
    CancelChildren(m_scriptMind);
    m_scriptState = 5;
    AddPeaceIdleExecutor(m_scriptMind, -1);
}
void CScriptedUnitLogicExecutor::IdleMS(int time)
{
    CancelChildren(m_scriptMind);
    m_scriptState = 5;
    AddPeaceIdleExecutor(m_scriptMind, time);
}
void CScriptedUnitLogicExecutor::Afraid()
{
    CancelChildren(m_scriptMind);
    m_scriptState = 5;
    AddPlayAfraidAnimationModalExecutor(m_scriptMind, -1);
}
void CScriptedUnitLogicExecutor::AfraidMS(int time)
{
    CancelChildren(m_scriptMind);
    m_scriptState = 5;
    AddPlayAfraidAnimationModalExecutor(m_scriptMind, time);
}
void CScriptedUnitLogicExecutor::TakeBox() { m_scriptMind->m_owner->m_body.ShowBox(true); }
void CScriptedUnitLogicExecutor::PickUpWeapon()
{
    m_scriptMind->m_owner->RestoreWeaponHP();
    m_scriptMind->m_owner->m_body.ShowWeapon(true);
    CancelChildren(m_scriptMind);
    m_scriptState = 5;
    AddPlayAnimationExecutor(m_scriptMind, (EPersonAnimType)31, false);
}
void CScriptedUnitLogicExecutor::ThreatenWithWeapon()
{
    CUnit* unit = m_scriptMind->m_owner;
    unit->m_body.SetOrientationToPoint(((App*)WindowApp::m_instance)->m_helper->m_swerveGame->GetCurrentCameraPos(), false, 0.07f);
    CancelChildren(m_scriptMind);
    m_scriptState = 5;
    AddPlayAnimationExecutor(m_scriptMind, (EPersonAnimType)33, false);
}
void CScriptedUnitLogicExecutor::ShowWeapon(bool value) { m_scriptMind->m_owner->m_body.ShowWeapon(value); }
void CScriptedUnitLogicExecutor::ThrowWeapon()
{
    CUnit* unit = m_scriptMind->m_owner;
    unit->m_body.SetOrientationToPoint(((App*)WindowApp::m_instance)->m_helper->m_swerveGame->GetCurrentCameraPos(), false, 0.07f);
    CancelChildren(m_scriptMind);
    m_scriptState = 5;
    AddThrowWeaponExecutor(m_scriptMind);
}
void CScriptedUnitLogicExecutor::PutBox()
{
    CancelChildren(m_scriptMind);
    m_scriptState = 5;
    AddPutBoxExecutor(m_scriptMind);
}
void CScriptedUnitLogicExecutor::PrepareBox(CStrChar name) { m_scriptMind->m_owner->m_body.BindBoxToHand(XString(name.GetCharPtr())); }
void CScriptedUnitLogicExecutor::WalkAround()
{
    CancelChildren(m_scriptMind);
    m_scriptState = 2;
    AddWalkAroundExecutor(m_scriptMind);
}
void CScriptedUnitLogicExecutor::RunAway()
{
    CancelChildren(m_scriptMind);
    m_scriptState = 2;
    AddRunAwayExecutor(m_scriptMind);
}
void CScriptedUnitLogicExecutor::Disappear() { m_scriptMind->m_owner->MarkAsRemoved(); }
void CScriptedUnitLogicExecutor::SwitchBrain(CStrChar name)
{
    m_scriptNames.removeAllElements();
    m_scriptNames.addElement(XString(name.GetCharPtr()));
    m_pendingCommands.removeAllElements();
    CancelChildren(m_scriptMind);
    OnStart();
}
void CScriptedUnitLogicExecutor::SetMovementRun()
{
    CUnit* unit = m_scriptMind->m_owner;
    unit->m_running = true;
    unit->m_carrying = false;
}
void CScriptedUnitLogicExecutor::SetMovementCarrying() { m_scriptMind->m_owner->m_carrying = true; }
void CScriptedUnitLogicExecutor::SetMovementWalk()
{
    CUnit* unit = m_scriptMind->m_owner;
    unit->m_running = false;
    unit->m_carrying = false;
}
bool CScriptedUnitLogicExecutor::IsNearestCivilianMyTarget()
{
    CUnit* nearest = ((App*)WindowApp::m_instance)->m_helper->m_swerveGame->GetUnitsController()->GetNearestUnit(m_scriptMind->m_owner->m_body.m_position, true, true);
    return m_scriptMind->m_owner->GetTargetUnit() == nearest;
}
void CScriptedUnitLogicExecutor::SetNearestCivilianAsTarget()
{
    CUnit* nearest = ((App*)WindowApp::m_instance)->m_helper->m_swerveGame->GetUnitsController()->GetNearestUnit(m_scriptMind->m_owner->m_body.m_position, true, true);
    m_scriptMind->m_owner->SetTarget(nearest);
}
void CScriptedUnitLogicExecutor::SetNearestZombieAsTarget()
{
    CUnit* nearest = ((App*)WindowApp::m_instance)->m_helper->m_swerveGame->GetUnitsController()->GetNearestUnit(m_scriptMind->m_owner->m_body.m_position, false, true);
    m_scriptMind->m_owner->SetTarget(nearest);
}
void CScriptedUnitLogicExecutor::TeleportInPointWithName(CStrChar name)
{
    int point = m_scriptMind->m_owner->m_body.m_map->GetPointIDByName(XString(name.GetCharPtr()));
    TeleportInPointWithId(point);
}
void CScriptedUnitLogicExecutor::TeleportInPointWithId(int point) { m_scriptMind->m_owner->m_body.SetToPoint(XString(point)); }
void CScriptedUnitLogicExecutor::SendScriptedEvent(CStrChar event)
{
    ((App*)WindowApp::m_instance)->m_helper->m_swerveGame->GetScriptController()->OnCivilianScriptedEvent(event);
}
bool CScriptedUnitLogicExecutor::IsRunCommandBlocked() { return m_scriptMind->m_owner->m_blowing; }
void CScriptedUnitLogicExecutor::FeintDeath()
{
    CancelChildren(m_scriptMind);
    AddFeintDeathExecutor(m_scriptMind, true);
}
void CScriptedUnitLogicExecutor::PlayRage()
{
    CUnit* unit = m_scriptMind->m_owner;
    unit->m_body.SetOrientationToPoint(((App*)WindowApp::m_instance)->m_helper->m_swerveGame->GetCurrentCameraPos(), false, 0.2f);
    CancelChildren(m_scriptMind);
    m_scriptState = 5;
    AddPlayAnimationExecutor(m_scriptMind, (EPersonAnimType)34, false);
}
void CScriptedUnitLogicExecutor::Enrage()
{
    CUnit* unit = m_scriptMind->m_owner;
    if (!unit->m_enraged) unit->MarkAsEnraged();
}
void CScriptedUnitLogicExecutor::SetSpecialZoneDamageMult(float value) { m_scriptMind->m_owner->m_specialDamageMultiplier = value; }
const wchar_t* CScriptedUnitLogicExecutor::GetName() const { return L"CScriptedUnitLogicExecutor"; }

void AddScriptedWarriorLogicExecutor(CUnitMind* mind, const XString& name)
{
    if (mind == NULL) return;
    CScriptedUnitLogicExecutor* executor;
    if (mind->m_owner->IsCivilian()) executor = new CScriptedUnitLogicExecutor(mind, name);
    else executor = new CZombieExecutor(mind, name);
    mind->AddExecutor(executor, 1);
    executor->Init();
}
void CZombieExecutor::Tick(int elapsed)
{
    CUnit* unit = m_scriptMind->m_owner;
    if (unit->CanBeHandled() && !unit->m_body.m_spawning && !unit->m_body.m_personType->m_ignoreLureGrenades) CheckBeaconPresence();
    CScriptedUnitLogicExecutor::Tick(elapsed);
}
void CZombieExecutor::CheckBeaconPresence()
{
    m_beaconPresent = false;
    int canInterrupt = !m_scriptMind->m_owner->IsCivilian();
    CUnitTarget* target = m_scriptMind->m_owner->m_target;
    if (target != NULL) {
        canInterrupt = target->IsInterruptable() ? canInterrupt : 0;
    }
    if (canInterrupt == 1) {
        int grenade = ((App*)WindowApp::m_instance)->m_helper->m_swerveGame->m_pickupGrenade;
        if (grenade >= 0) {
            m_beaconPresent = true;
            m_scriptMind->m_owner->SetTargetHandGrenade(grenade);
            if (m_scriptMind->m_owner->IsTargetNear()) {
                CUnitBody* body = &m_scriptMind->m_owner->m_body;
                if (body != NULL) AddAttackExecutor(m_scriptMind, true);
            } else {
                MoveToTargetImpl(true, false);
            }
        }
    }
}
bool CZombieExecutor::IsRunCommandBlocked()
{
    return m_beaconPresent || m_scriptMind->m_owner->m_blowing;
}
