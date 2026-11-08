// CUnitsController.cpp - original u439 lifecycle of units, scene objects, spawn queue and listeners.
#include "CUnitsController.h"
#include "CUnit.h"
#include "CSwerveGame.h"
#include "CGameAIMap.h"
#include "CSceneObject.h"
#include "CSceneSetupsManager.h"
#include "CSceneObjectManager.h"
#include "CMissionManager.h"
#include "CMission.h"
#include "CBH_PersonTypeManager.h"
#include "CBH_PersonType.h"
#include "CBalanceManager.h"
#include "CLocationManager.h"
#include "CLocation.h"
#include "CZombieMission.h"
#include "CHuntingInfo.h"
#include "CRandGen.h"
#include "CSceneLighting.h"
#include "DGCamera.h"
#include "App.h"
#include "DGHelper.h"
#include "CDH_Weapon.h"
#include "CBH_Player.h"
#include "CPlayerGameStats.h"
#include "CDH_BasicGameData.h"

using com::glu::platform::core::CRandGen;

// Three real external symbols in the original __DATA; the tunable parameters are not folded into local constants.
float MaxSteeringForce = 0.9f;
float MaxSteeringDistance = 2.0f;
float MaxObstacleRepulseForce = 4.0f;

vec3 CalculateUnitRepulse(const vec3& firstPosition, const vec3& firstSpeed, float firstRadius,
                          const vec3& secondPosition, const vec3& secondSpeed, float secondRadius);

CUnitsController::CUnitsController(CSwerveGame* game, const XString& missionName)
    : m_civiliansHp(0), m_civiliansMaxHp(0), m_zombieCount(0), m_game(game),
      m_updateCounter58(0), m_nextUniqueId(0), m_unitsById(8),
      m_maxSceneUnits(((App*)WindowApp::m_instance)->m_balanceManager->m_setups[
          ((App*)WindowApp::m_instance)->m_balanceManager->m_currentSetup].m_maxSceneUnitsCount),
      m_wavesPaused(false)
{
    CMission* mission = ((App*)WindowApp::m_instance)->m_missionManager->GetMissionByName(missionName);
    m_sceneName = mission->m_sceneSetupName;
    m_locationName = mission->m_locationName;
    m_units.ensureCapacity(100);
    m_hudUnits.ensureCapacity(10);
}

void CUnitsController::InitUnits()
{
    CSceneSetupsManager::SSceneSetup setup =
        *((App*)WindowApp::m_instance)->m_sceneSetupsManager->GetSceneSetupByName(m_sceneName);
    for (int index = 0; index < setup.m_persons.size(); ++index) {
        XString point = setup.m_persons[index].m_point;
        unsigned int probability = setup.m_persons[index].m_probability;
        if (CRandGen::GetInstance()->GetRand(100) < probability) {
            SpawnUnit(SSpawnInfo(SSpawnablePerson(setup.m_persons[index].m_name,
                CRandGen::GetInstance()->Generate(), XString(L"")), point, -1), -1);
        }
    }
    for (int index = 0; index < setup.m_objects.size(); ++index) {
        XString name = setup.m_objects[index].m_name;
        XString point = setup.m_objects[index].m_point;
        unsigned int probability = setup.m_objects[index].m_probability;
        if (CRandGen::GetInstance()->GetRand(100) < probability) {
            const CSceneObjectManager::SSceneObjectInfo* info =
                ((App*)WindowApp::m_instance)->m_sceneObjectManager->GetSceneObjectInfoByName(name);
            CSceneObject* object = ((App*)WindowApp::m_instance)->m_sceneObjectManager->CreateSceneObject(info);
            object->SetToPoint(m_game->GetGameAIMap(), point);
            m_objects.addElement(object);
        }
    }
    CLocation* location = ((App*)WindowApp::m_instance)->m_locationManager->GetLocation(m_locationName);
    if (location != NULL) {
        for (int index = 0; index < location->m_objects.size(); ++index) {
            XString name = location->m_objects[index].m_name;
            XString point = location->m_objects[index].m_pointName;
            const CSceneObjectManager::SSceneObjectInfo* info =
                ((App*)WindowApp::m_instance)->m_sceneObjectManager->GetSceneObjectInfoByName(name);
            CSceneObject* object = ((App*)WindowApp::m_instance)->m_sceneObjectManager->CreateSceneObject(info);
            object->SetToPoint(m_game->GetGameAIMap(), point);
            m_objects.addElement(object);
        }
    }
}

int CUnitsController::SpawnUnit(const SSpawnInfo& info, int flags)
{
    CBH_PersonType* type = ((App*)WindowApp::m_instance)->m_personTypeManager->GetType(info.m_typeName);
    if (type == NULL) {
        return -1;
    }
    if (type->m_isBoss) {
        for (int index = 0; index < m_units.size(); ++index) {
            if (m_units[index]->IsBoss()) {
                return -1;
            }
        }
    }
    XString script = type->m_scriptName;
    if (info.m_point.ByteSize() != 0) {
        script = info.m_point;
    }
    int id = ++m_nextUniqueId;
    SSpawnablePerson person = info;
    if (type->m_isCivilian) {
        CMission* mission = ((App*)WindowApp::m_instance)->m_missionManager->GetMissionByName(
            ((App*)WindowApp::m_instance)->m_huntingInfo->m_assignedData.m_missionName);
        if (mission != NULL) {
            person.m_waveMultiplier1 = mission->m_tutorialMultiplier;
        }
    }
    CUnit* unit = new CUnit(id, person, NULL, m_game->GetGameAIMap(), info.m_waveId, script);
    unit->Init(info.m_spawnPoint);
    unit->Spawn();
    if (flags >= 0) {
        unit->m_body.m_forcedLod = flags;
    }
    m_unitsById.Add(HashTableKey<int>(id), unit);
    m_units.addElement(unit);
    if (unit->IsCivilian() == true) {
        m_civiliansMaxHp += unit->m_maxHp;
        m_civiliansHp += unit->m_maxHp;
        unit->m_nextScript = ((App*)WindowApp::m_instance)->m_huntingInfo->m_assignedData.m_text0c;
    }
    for (int index = m_listeners.size() - 1; index >= 0; --index) {
        m_listeners[index]->OnUnitSpawned(unit);
    }
    return id;
}

void CUnitsController::AddUnitInSpawnQueue(const SSpawnInfo& info)
{
    m_spawnQueue.addElement(info);
}

void CUnitsController::Update(unsigned int elapsedMs)
{
    UpdateMissionSpawns(elapsedMs);
    UpdateSpawnQueue(elapsedMs);
    UpdateUnitsDeath(elapsedMs);
    UpdateUnits(elapsedMs);
    UpdateObjects(elapsedMs);
}

void CUnitsController::UpdateMissionSpawns(unsigned int elapsedMs)
{
    if (m_wavesPaused) {
        return;
    }
    CZombieMission* mission = m_game->GetZombieMission();
    mission->Update(elapsedMs);
    if (mission->m_spawns.size() < 1 || m_units.size() >= m_maxSceneUnits) {
        return;
    }
    XString point = mission->GetNextSpawnPointName();
    CSceneObject* object = GetSceneObjectInPoint(point);
    if (object == NULL || object->TrySpawn() == true) {
        SSpawnInfo spawn = mission->PopNextSpawn();
        SpawnUnit(spawn, -1);
    }
}

void CUnitsController::UpdateSpawnQueue(unsigned int unused)
{
    for (int index = 0; index < m_spawnQueue.size(); ++index) {
        SSpawnInfo spawn = m_spawnQueue[index];
        XString point = spawn.m_spawnPoint;
        CSceneObject* object = GetSceneObjectInPoint(point);
        if (object != NULL && object->TrySpawn() != true) {
            continue;
        }
        SpawnUnit(spawn, -1);
        m_spawnQueue.removeElementAt(index);
        return; // The original exits after spawning one entry; omitting this would change the per-frame spawn rate.
    }
}

void CUnitsController::UpdateUnitsDeath(unsigned int unused)
{
    for (int index = 0; index < m_units.size(); ++index) {
        CUnit* unit = m_units[index];
        float height = unit->m_body.m_position.m_z;
        if (unit->m_dead && !unit->IsCivilian()) {
            --m_zombieCount;
        }
        bool notify = false;
        if (unit->m_dead) {
            bool thrown = unit->IsThrowed();
            notify = height < -30.0f || !thrown;
        } else {
            notify = height < -30.0f;
        }
        if (notify && !unit->m_deathNotified) {
            CallUnitDead(unit);
            unit->m_deathNotified = true;
        }
        if ((unit->m_dead && !unit->IsThrowed() && unit->m_corpseDisappear) ||
            height < -30.0f || unit->m_removed) {
            DeleteUnitByNum(index);
            return;
        }
    }
}

void CUnitsController::UpdateUnits(unsigned int elapsedMs)
{
    int civiliansHp = 0;
    int civiliansMaxHp = 0;
    m_updateCounter58 = 0;
    m_zombieCount = 0;
    for (int index = 0; index < m_units.size(); ++index) {
        CUnit* unit = m_units[index];
        if (unit == NULL) {
            continue;
        }
        if (unit->IsCivilian() == true) {
            int hp = unit->m_hp;
            int maxHp = unit->m_maxHp;
            int dangerCount = 0;
            int fearCount = 0;
            int attackCount = 0;
            for (int other = 0; other < m_units.size(); ++other) {
                if (index != other && !m_units[other]->m_body.m_personType->m_isCivilian) {
                    float attackDistance = unit->m_body.GetDistanceToUnit(m_units[other]);
                    float attackRange = unit->GetAttackRange();
                    attackCount += attackDistance < attackRange;
                    float fearDistance = unit->m_body.GetDistanceToUnit(m_units[other]);
                    float fearRange = unit->GetFearRange();
                    float dangerDistance = unit->m_body.GetDistanceToUnit(m_units[other]);
                    fearCount += fearDistance < fearRange;
                    if (dangerDistance < unit->GetDangerRange()) {
                        ++dangerCount;
                    }
                }
            }
            civiliansMaxHp += maxHp;
            civiliansHp += hp;
            unit->m_nearbyAttack = attackCount;
            unit->m_nearbyFear = fearCount;
            unit->m_nearbyDanger = dangerCount;
        } else if (!unit->m_dead) {
            ++m_zombieCount;
        }
        unit->m_body.SetLuminance(m_game->GetSceneLighting()->GetLuminance(unit->m_body.m_position));
        AvoidCollision(unit);
        unit->ResetPickInfo();
        if (unit->CanBeHandled() == true) {
            unit->m_body.CheckVisibility(m_game->m_camera->m_frustum);
            unit->Tick(elapsedMs);
        }
    }
    m_civiliansHp = civiliansHp;
    m_civiliansMaxHp = civiliansMaxHp;
}

void CUnitsController::UpdateObjects(unsigned int elapsedMs)
{
    for (int index = 0; index < m_objects.size(); ++index) {
        m_objects[index]->Tick(elapsedMs);
        float luminance = m_game->GetSceneLighting()->GetLuminance(m_objects[index]->m_position);
        m_objects[index]->SetLuminance(luminance);
    }
}

CSceneObject* CUnitsController::GetSceneObjectInPoint(const XString& point)
{
    for (int index = 0; index < m_objects.size(); ++index) {
        if (m_objects[index]->m_pointName == point) {
            return m_objects[index];
        }
    }
    return NULL;
}

void CUnitsController::CallUnitDead(CUnit* unit)
{
    for (int index = m_listeners.size() - 1; index >= 0; --index) {
        m_listeners[index]->OnUnitKilled(unit);
    }
}

void CUnitsController::CallUnitInjured(CUnit* unit, CUnit* attacker, int damage)
{
    for (int index = m_listeners.size() - 1; index >= 0; --index) {
        m_listeners[index]->OnUnitInjured(unit, attacker, damage);
    }
}

void CUnitsController::DeleteUnitByNum(int index)
{
    CUnit* unit = m_units[index];
    for (int listener = 0; listener < m_listeners.size(); ++listener) {
        m_listeners[listener]->OnUnitRemoved(unit);
    }
    m_unitsById.Remove(HashTableKey<int>(unit->m_id));
    delete unit;
    m_units.removeElementAt(index);
}

void CUnitsController::AvoidCollision(CUnit* unit)
{
    if (unit == NULL) {
        return;
    }
    vec3 force;
    vec3 speed = unit->m_body.GetSpeed();
    m_game->GetGameAIMap()->FindObstacles(unit->m_body.m_location, unit->m_body.m_obstacles);
    for (int index = 0; index < m_units.size(); ++index) {
        if (unit->IsCivilian()) {
            break; // The original checks civilian first on every iteration; not hoisted out of the loop.
        }
        CUnit* other = m_units[index];
        if (other == NULL || other->m_dead) {
            continue;
        }
        bool handled = other->CanBeHandled();
        if (other != unit && handled == true) {
            vec3 otherSpeed = other->m_body.GetSpeed();
            float firstRadius = unit->m_body.GetRadius();
            float secondRadius = other->m_body.GetRadius();
            force = force + CalculateUnitRepulse(unit->m_body.m_position, speed, firstRadius,
                other->m_body.m_position, otherSpeed, secondRadius);
        }
    }
    for (int index = 0; index < m_objects.size(); ++index) {
        CSceneObject* object = m_objects[index];
        float radius = unit->m_body.GetRadius();
        vec3 objectSpeed;
        force = force + CalculateUnitRepulse(unit->m_body.m_position, speed, radius,
            object->m_position, objectSpeed, 1.0f);
    }
    float maxForce = MaxSteeringForce;
    float forceSquared = force.LengthSqr();
    float forceLength = MathLib::InvSqrt(forceSquared) * forceSquared;
    if (forceLength > maxForce) {
        force = force * (maxForce / forceLength);
    }
    float radius = unit->m_body.GetRadius();
    float halfRadius = radius * 0.5f;
    for (int index = 0; index < unit->m_body.m_obstacles.m_static.size(); ++index) {
        ObstaclesContainer::Static& obstacle = unit->m_body.m_obstacles.m_static[index];
        bool inside = false;
        vec3 displacement = MathLib::DistanceToSegment(unit->m_body.m_position,
            obstacle.m_first, obstacle.m_second, inside);
        if (!inside) {
            continue;
        }
        // The original indeed compares the squared distance with radius; do not change it to sqrt or radius squared on our own.
        float distance = MathLib::Fabs(displacement.LengthSqr());
        if (displacement.Dot(obstacle.m_direction) < 0.0f) {
            distance = 0.0f;
        }
        if (!(distance < radius)) {
            continue;
        }
        float magnitude = (float)((1.0 - (double)(distance / halfRadius)) * (double)MaxObstacleRepulseForce);
        if (!(magnitude > 0.0f)) {
            magnitude = 0.0f;
        }
        force = force + obstacle.m_direction * magnitude;
    }
    force.m_z = 0.0f;
    maxForce = MaxObstacleRepulseForce;
    forceSquared = force.LengthSqr();
    forceLength = MathLib::InvSqrt(forceSquared) * forceSquared;
    if (forceLength > maxForce) {
        force = force * (maxForce / forceLength);
    }
    unit->m_body.SetSteeringForce(force);
}

void CUnitsController::UpdateHudUnitsInfo()
{
    m_hudUnits.removeAllElements();
    for (int index = 0; index < m_units.size(); ++index) {
        CUnit* unit = m_units[index];
        if (unit != NULL && unit->IsHudInfoShown() == true) {
            m_hudUnits.addElement(CalcCivilianInfo(unit));
        }
    }
}

CUnitsController::SHudUnitInfo CUnitsController::CalcCivilianInfo(CUnit* unit)
{
    SHudUnitInfo info;
    info.m_hp = unit->m_hp;
    info.m_maxHp = unit->m_maxHp;
    int x;
    int y;
    unit->m_body.GetScreenPos(x, y, 2.0f);
    info.m_screenX = x;
    info.m_screenY = y;
    info.m_cameraDistance = unit->m_body.m_cameraDistance;
    info.m_id = unit->m_id;
    info.m_civilian = unit->IsCivilian();
    return info;
}

CUnit* CUnitsController::GetNearestUnit(const vec3& position, bool civilian, bool aliveOnly) const
{
    CUnit* nearest = NULL;
    float nearestDistance = 2147483648.0f; // original literal0x109cfc, 0000004f
    for (int index = 0; index < m_units.size(); ++index) {
        CUnit* unit = m_units[index];
        if (unit->m_body.m_personType->m_isCivilian != civilian) {
            continue;
        }
        if (aliveOnly == true && unit->m_dead) {
            continue;
        }
        float distance = unit->m_body.GetDistanceToPoint(position);
        if (distance < nearestDistance) {
            nearest = m_units[index];
            nearestDistance = distance;
        }
    }
    return nearest;
}

int CUnitsController::Shot(float x, float y, const CDH_Weapon* weapon, const CCollision::Line& line)
{
    Vector<CUnit*> picked;
    swerve::Camera<CSwerveAssertingErrorHandler> camera = m_game->m_cameraNode.getActiveCamera();
    float collisionDistance = m_game->GetDistanceToCollision(x, y);
    for (int index = 0; index < m_units.size(); ++index) {
        CUnit* unit = m_units[index];
        if (unit == NULL) {
            continue;
        }
        unit->ResetPickInfo();
        if (unit->CanBeHandled() != true || unit->m_hp == 0) {
            continue;
        }
        unit->m_body.CheckVisibility(m_game->m_camera->m_frustum);
        if (unit->m_body.PickBounds(line) == true && unit->Pick(x, y, collisionDistance, camera) == true) {
            picked.addElement(unit);
        }
    }
    SortUnitPickDistance(picked);
    int pickedCount = picked.size();
    CUnit** items = picked.m_items;
    for (int index = 0; index < pickedCount; ++index) {
        CUnit* unit = items[index];
        if (unit == NULL) {
            return 0;
        }
        EHitType hit;
        unit->GetPickInfo(&hit);
        if (m_game->DefineShotProbability(weapon, unit->m_body.m_position) == true) {
            unit->IsCivilian(); // The original return value is unused, but the external call must be kept.
            unit->ReceivedDamageFromPlayer(hit, weapon, index);
        }
    }
    int objectHits = 0;
    for (int index = 0; index < m_objects.size(); ++index) {
        if (m_objects[index]->CanBeHandled() != true || m_objects[index]->IsPickable() != true) {
            continue;
        }
        if (m_objects[index]->Pick(x, y, -1.0f, camera) != true) {
            continue;
        }
        if (m_game->DefineShotProbability(weapon, m_objects[index]->m_position) != true) {
            continue;
        }
        ++objectHits;
        CSceneObject* object = m_objects[index];
        vec3 difference = object->m_position - m_game->GetCurrentCameraPos();
        float square = difference.LengthSqr();
        float distance = MathLib::InvSqrt(square) * square;
        object->ReceivedDamage(weapon->GetShotDamage(distance), 0.0f);
    }
    return pickedCount + objectHits;
}

void CUnitsController::SortUnitPickDistance(Vector<CUnit*>& units)
{
    for (int index = 0; index < units.size(); ++index) {
        CUnit* unit = units[index];
        if (unit == NULL) {
            return;
        }
        float distance = unit->m_body.m_pickDistance;
        int position = index;
        // The original BLE also stops movement when unordered, so the loop only accepts an ordered greater-than.
        while (position > 0 && units[position - 1]->m_body.m_pickDistance > distance) {
            units[position] = units[position - 1];
            --position;
        }
        units[position] = unit;
    }
}

void CUnitsController::Blast(const vec3& position, float radius, int damage, int power,
    const CDH_Weapon* weapon, const CGrenade* grenade, const CUnit* source)
{
    for (int index = 0; index < m_units.size(); ++index) {
        if (m_units[index] == source) {
            continue;
        }
        m_units[index]->IsCivilian();
        CUnit* unit = m_units[index];
        float centerZ = unit->m_body.m_position.m_z +
            (unit->m_body.m_bounds.m_max.m_z - unit->m_body.m_bounds.m_min.m_z) * 0.5f;
        vec3 difference(unit->m_body.m_position.m_x - position.m_x,
                        unit->m_body.m_position.m_y - position.m_y, centerZ - position.m_z);
        float square = difference.LengthSqr();
        float distance = MathLib::InvSqrt(square) * square;
        if (!(distance <= radius)) {
            continue; // The original BHI includes unordered; must not be changed to distance>radius.
        }
        float factor = MathLib::Pow(1.0f - distance / radius, 2.0f);
        int amount = (int)((float)damage * (1.0f - factor) + (float)power * factor);
        m_units[index]->ReceivedBlastDamage(amount, position, weapon, grenade, source != NULL);
    }
    for (int index = 0; index < m_objects.size(); ++index) {
        if (m_objects[index]->CanBeHandled() != true) {
            continue;
        }
        vec3 difference = m_objects[index]->m_position - position;
        float square = difference.LengthSqr();
        float distance = MathLib::InvSqrt(square) * square;
        float factor = MathLib::Pow(1.0f - distance / radius, 2.0f);
        if (distance <= radius) {
            // Keeps the original call order of the object branch: compute Pow first, then check radius.
            float powerPart = (float)power * factor;
            int amount = (int)((float)damage * (1.0f - factor) + powerPart);
            m_objects[index]->ReceivedDamage(amount, distance);
        }
    }
}

void CUnitsController::Blast(const vec3& position, float radius, const CDH_Weapon* weapon,
    const CGrenade* grenade, const CUnit* source)
{
    int damage = 0;
    int power = 1;
    if (weapon != NULL) {
        damage = weapon->m_damageMin;
        power = weapon->m_damageMax;
    }
    Blast(position, radius, damage, power, weapon, grenade, source);
}

vec3 CalculateUnitRepulse(const vec3& firstPosition, const vec3& firstSpeed, float firstRadius,
    const vec3& secondPosition, const vec3& secondSpeed, float secondRadius)
{
    float threshold = (float)((double)firstRadius * 0.5 + (double)secondRadius * 0.5) + 1.5f;
    vec3 difference = secondPosition - firstPosition;
    float square = difference.LengthSqr();
    // The original lays out the in-range branch first; the zero result block comes last.
    if (square < threshold * threshold) {
        // The original two speed arguments are not read; the two InvSqrt each have a real call, not merged.
        float inverseLength = MathLib::InvSqrt(square);
        float distance = MathLib::InvSqrt(square) * square;
        float magnitude = (float)(((double)(distance / -1.5f) + 1.0) * (double)MaxSteeringForce);
        if (!(magnitude > 0.0f)) {
            magnitude = 0.0f;
        }
        float zero = magnitude * 0.0f;
        float tangent = magnitude * -0.9f;
        float dx = difference.m_x * inverseLength;
        float dy = difference.m_y * inverseLength;
        float dz = difference.m_z * inverseLength;
        return vec3(dx * tangent - zero * dy, dy * tangent + dx * zero,
                    dz * tangent + dz * zero);
    }
    return vec3();
}

float CUnitsController::GetCiviliansMeanRelativeHP()
{
    float sum = 0.0f;
    int count = 0;
    for (int index = 0; index < m_units.size(); ++index) {
        if (m_units[index]->IsCivilian() == true) {
            ++count;
            sum += (float)m_units[index]->m_hp / (float)m_units[index]->m_maxHp;
        }
    }
    if (count >= 1) {
        sum = sum / (float)count;
    }
    if (sum != 0.0f) {
        return sum;
    }
    return 0.0f;
}

void CUnitsController::SetCiviliansMeanRelativeHP(float relativeHp)
{
    if (!(relativeHp >= 0.0f && relativeHp <= 1.0f)) {
        return;
    }
    for (int index = 0; index < m_units.size(); ++index) {
        if (m_units[index]->IsCivilian() == true) {
            m_units[index]->m_hp = (int)((float)m_units[index]->m_maxHp * relativeHp);
        }
    }
}

CUnit* CUnitsController::GetUnitByUniqueId(int id)
{
    CUnit** value = m_unitsById.Get(HashTableKey<int>(id));
    if (value != NULL) {
        return *value;
    }
    return NULL;
}

CUnitsController::~CUnitsController()
{
    RemoveAllListeners();
    SAFE_DELETE_VECTOR(m_units);
    SAFE_DELETE_VECTOR(m_objects);
}

void CUnitsController::RemoveAllListeners()
{
    while (m_listeners.size() != 0) {
        RemoveListener(m_listeners[0]);
    }
}

void CUnitsController::ReviveCivilian()
{
    for (int index = 0; index < m_units.size(); ++index) {
        if (m_units[index]->IsCivilian() == true) {
            m_units[index]->m_corpseDisappear = true;
            SSpawnInfo spawn;
            {
                SSpawnablePerson person = m_units[index]->m_person;
                static_cast<SSpawnablePerson&>(spawn) = person;
            }
            spawn.m_spawnPoint = m_units[index]->m_body.m_spawnPoint;
            SpawnUnit(spawn, -1); // The original default constructor does not initialize waveId; no assignment added.
            return;
        }
    }
}

void CUnitsController::DeleteAllZombies()
{
    int count = 0;
    for (int index = 0; index < m_units.size(); ++index) {
        if (!m_units[index]->IsCivilian() && !m_units[index]->IsBoss()) {
            ++count;
            m_units[index]->MarkAsDead();
            m_units[index]->m_corpseDisappear = true;
        }
    }
    // Each UnitsDeath removes at most one; the original calls once even with zero matches.
    for (int remaining = count + 1; remaining >= 1; --remaining) {
        UpdateUnitsDeath(0);
    }
}

void CUnitsController::AddListener(Listener* listener)
{
    if (listener->m_controller != NULL) {
        listener->m_controller->RemoveListener(listener);
    }
    listener->m_controller = this;
    m_listeners.addElement(listener);
}

bool CUnitsController::RemoveListener(Listener* listener)
{
    unsigned int count = m_listeners.size();
    unsigned int index = 0;
    while (index < count && m_listeners[index] != listener) {
        ++index;
    }
    if (index == count) {
        return false;
    }
    if (count >= 2) {
        m_listeners[index] = m_listeners[count - 1];
    }
    m_listeners.setSize(count - 1);
    listener->m_controller = NULL;
    return true;
}

void CUnitsController::ShootAllZombies()
{
    for (int index = 0; index < m_units.size(); ++index) {
        CUnit* unit = m_units[index];
        if (!unit->IsCivilian()) {
            CDH_Weapon* weapon = ((App*)WindowApp::m_instance)->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon();
            CBH_Player::GetInstance()->m_gameStats.NotifyShotBegin(weapon, NULL, 1);
            unit->ReceivedDamageFromPlayer(HIT_HEAD, weapon, 0);
        }
    }
}

CUnitsController::Listener::Listener(CUnitsController* controller) : m_controller(NULL)
{
    if (controller != NULL) {
        controller->AddListener(this);
    }
}

CUnitsController::Listener::~Listener()
{
    if (m_controller != NULL) {
        m_controller->RemoveListener(this);
    }
}
