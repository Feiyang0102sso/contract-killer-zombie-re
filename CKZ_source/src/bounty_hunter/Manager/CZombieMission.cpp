// CZombieMission.cpp - spawn scheduling, wave start conditions and runtime mission cycles.
#include "CZombieMission.h"
#include "App.h"
#include "CHuntingInfo.h"
#include "CMissionManager.h"
#include "CLocationManager.h"
#include "CLocation.h"
#include "CBH_Player.h"
#include "CBH_PersonTypeManager.h"
#include "CBH_PersonType.h"
#include "CPersonsGroupManager.h"
#include "CVisualCharacterType.h"
#include "CVisualCharacterTypeManager.h"
#include "CRandGen.h"
#include "CSwerveGame.h"
#include "DGHelper.h"
#include "CUnit.h"

using com::glu::platform::core::CRandGen;

CZombieWave::CZombieWave(unsigned int number, EStartEvent event, int threshold,
                         CMission::EMissionType type, CMission::EMissionDifficulty difficulty, const XString& filter)
    : m_wave(NULL), m_runtimeId(0), m_number(number), m_startEvent(event), m_startThreshold(threshold),
      m_totalCount(0), m_countdown(0), m_countdownNotified(false), m_startedNotified(false)
{
    int level = CBH_Player::GetInstance()->GetLevel();
    CWaveManager::SWave* wave = ((App*)WindowApp::m_instance)->m_waveManager->GetRandomWave(type, difficulty, level, filter);
    if (wave == NULL) {
        wave = ((App*)WindowApp::m_instance)->m_waveManager->GetWave(type, difficulty, 1);
    }
    m_wave = wave;
    if (type == CMission::MISSION_EM) {
        m_countdown = (int)(m_wave->m_pause * 1000.0f);
        if (m_startEvent == START_AFTER_KILLS) {
            m_startThreshold = (int)m_wave->m_threshold;
        }
    }
}

CZombieWave::CZombieWave(unsigned int number, int level, EStartEvent event, int threshold,
                         CMission::EMissionType type, CMission::EMissionDifficulty difficulty, const XString& filter)
    : m_wave(NULL), m_runtimeId(0), m_number(number), m_startEvent(event), m_startThreshold(threshold),
      m_totalCount(0), m_countdown(0), m_countdownNotified(false), m_startedNotified(false)
{
    CWaveManager::SWave* wave = ((App*)WindowApp::m_instance)->m_waveManager->GetRandomWave(type, difficulty, level, filter);
    if (wave == NULL) {
        wave = ((App*)WindowApp::m_instance)->m_waveManager->GetWave(type, difficulty, 1);
    }
    m_wave = wave;
    if (type == CMission::MISSION_EM) {
        m_countdown = (int)(m_wave->m_pause * 1000.0f);
        if (m_startEvent == START_AFTER_KILLS) {
            m_startThreshold = (int)m_wave->m_threshold;
        }
    }
}

void CZombieWave::Init()
{
    // Original discards the old pointer storage without deleting its pointed-to events.
    m_events.removeAllElements();
    m_state = 0;
    m_elapsedMs = 0;
    m_deathCount = 0;
    m_totalCount = 0;
    if (m_wave == NULL) {
        m_state = 2;
        return;
    }
    CMission* mission = ((App*)WindowApp::m_instance)->m_missionManager->GetMissionByName(
        ((App*)WindowApp::m_instance)->m_huntingInfo->m_assignedData.m_missionName);
    if (mission == NULL) {
        return;
    }
    CLocation* location = ((App*)WindowApp::m_instance)->m_locationManager->GetLocation(mission->m_locationName);
    if (location == NULL) {
        return;
    }
    for (int index = 0; index < m_wave->m_events.size(); index++) {
        const CWaveManager::SSpawnEvent& configured = m_wave->m_events[index];
        Vector<XString> points = location->GetSectorPoints(configured.m_sector);
        Vector<SSpawnablePerson> persons;
        if (((App*)WindowApp::m_instance)->m_personTypeManager->TypeIsExists(configured.m_type) == false) {
            // Empty generated groups can keep this loop running forever in the original.
            while (persons.size() < configured.m_count) {
                CPersonsGroupManager::SPersonsGroup* group = ((App*)WindowApp::m_instance)->m_personsGroupManager->GetGroup(configured.m_type);
                if (mission->m_type == CMission::MISSION_EM) {
                    ((App*)WindowApp::m_instance)->m_personsGroupManager->GeneratePersonsSelection(persons, *group, m_wave);
                } else {
                    ((App*)WindowApp::m_instance)->m_personsGroupManager->GeneratePersonsSelection(persons, *group, NULL);
                }
            }
        } else {
            SSpawnablePerson person;
            person.m_typeName = configured.m_type;
            person.m_seed = 0;
            person.m_point = L"";
            if (mission->m_type == CMission::MISSION_EM) {
                person.m_waveMultiplier1 = m_wave->m_personMultiplier;
                person.m_waveMultiplier2 = m_wave->m_cashXpMultiplier;
            }
            persons.addElement(person);
        }
        int personIndex = 0;
        int delay = 0;
        for (int count = 0; count < configured.m_count; count++) {
            int pointIndex = CRandGen::GetInstance()->GetRand(points.size());
            SSpawnEvent* event = new SSpawnEvent();
            static_cast<SSpawnablePerson&>(*event) = persons[personIndex];
            event->m_spawnPoint = points[pointIndex];
            event->m_spawnTime = delay + CRandGen::GetInstance()->GetRandRange(1, 1500);
            event->m_spawned = false;
            points.removeElementAt(pointIndex);
            if (points.size() == 0) {
                points = location->GetSectorPoints(configured.m_sector);
                delay += CRandGen::GetInstance()->GetRandRange(1000, 3000);
            }
            m_totalCount++;
            m_events.addElement(event);
            personIndex++;
            if (personIndex > persons.size() - 1) {
                personIndex = 0;
            }
        }
    }
}

void CZombieWave::Update(unsigned int elapsedMs, Vector<SSpawnInfo>* spawns)
{
    if (m_countdownNotified == false) {
        m_countdownNotified = true;
        WindowApp::HandleTunnelCommand(0xb962267b, m_countdown, 0, 0);
    }
    m_countdown -= elapsedMs;
    if (m_countdown > 0) {
        return;
    }
    if (m_startedNotified == false) {
        m_startedNotified = true;
        WindowApp::HandleTunnelCommand(0x60042695, m_number, 0, 0);
    }
    m_elapsedMs += elapsedMs;
    bool allSpawned = true;
    for (int index = 0; index < m_events.size(); index++) {
        SSpawnEvent* event = m_events[index];
        if (event->m_spawned == false) {
            allSpawned = false;
            if (event->m_spawnTime <= (int)m_elapsedMs) {
                spawns->addElement(SSpawnInfo(*event, event->m_spawnPoint, m_runtimeId));
                event->m_spawned = true;
            }
        }
    }
    if (allSpawned) {
        m_state = 2;
    }
}

void CZombieWave::GetExpectedVisualCharacters(Vector<XString>& names)
{
    for (int index = 0; index < m_events.size(); index++) {
        const SSpawnEvent* event = m_events[index];
        const XString& typeName = event->m_typeName;
        unsigned int seed = event->m_seed;
        if (((App*)WindowApp::m_instance)->m_personTypeManager->TypeIsExists(typeName) == true) {
            CVisualCharacterType* visual = ((App*)WindowApp::m_instance)->m_personTypeManager->GetType(typeName)->GetVisualObject(seed);
            if (names.indexOf(visual->m_name, 0) == -1) {
                names.addElement(visual->m_name);
            }
        } else {
            Vector<SSpawnablePerson> persons;
            CPersonsGroupManager::SPersonsGroup* group = ((App*)WindowApp::m_instance)->m_personsGroupManager->GetGroup(typeName);
            ((App*)WindowApp::m_instance)->m_personsGroupManager->GeneratePersonsSelection(persons, *group, NULL);
            for (int personIndex = 0; personIndex < persons.size(); personIndex++) {
                XString name = persons[personIndex].m_typeName;
                CVisualCharacterType* visual = ((App*)WindowApp::m_instance)->m_personTypeManager->GetType(name)->GetVisualObject(seed);
                if (names.indexOf(visual->m_name, 0) == -1) {
                    names.addElement(visual->m_name);
                }
            }
        }
    }
}

CZombieWave::~CZombieWave()
{
    SAFE_DELETE_VECTOR(m_events);
}

CZombieMission::CZombieMission(CMission::EMissionType type, CMission::EMissionDifficulty difficulty,
                               int waveCount, const XString& filter)
    : CUnitsController::Listener(NULL), m_type(type), m_currentWave(NULL)
{
    XString firstFilter = filter;
    firstFilter = firstFilter + L"F";
    int level = CBH_Player::GetInstance()->GetLevel();
    if (((App*)WindowApp::m_instance)->m_waveManager->GetRandomWave(type, difficulty, level, firstFilter) == NULL) {
        firstFilter = filter;
    }
    if (type == CMission::MISSION_EM) {
        for (int index = 1; index < 51; index++) {
            if (index == 1) {
                m_prototypes.addElement(CZombieWave(index, index, CZombieWave::START_ON_TIME, 0, type, difficulty, filter));
            } else {
                m_prototypes.addElement(CZombieWave(index, index, CZombieWave::START_AFTER_KILLS, 100, type, difficulty, filter));
            }
        }
    } else {
        m_prototypes.addElement(CZombieWave(1, CZombieWave::START_ON_TIME, 0, type, difficulty, firstFilter));
        int threshold;
        switch (type) {
        case CMission::MISSION_CB:
            threshold = 75;
            break;
        case CMission::MISSION_KA:
        case CMission::MISSION_IC:
        case CMission::MISSION_ST:
        case CMission::MISSION_QU:
            threshold = 100;
            break;
        default:
            return;
        }
        if (waveCount > 1) {
            int number = 1;
            do {
                number++;
                m_prototypes.addElement(CZombieWave(number, CZombieWave::START_AFTER_KILLS, threshold, type, difficulty, filter));
            } while (number != waveCount);
        }
    }
}

void CZombieMission::Init(int cycleCount)
{
    m_cycleCount = cycleCount;
    m_cycleIndex = 0;
    m_currentWaveNumber = 1;
    m_field54 = false;
    m_cycleComplete = false;
    m_timeMs = 0;
    m_spawns.removeAllElements();
    Cleanup();
    m_cycles.ensureCapacity(cycleCount);
    int runtimeId = 1;
    for (int cycle = 0; cycle < cycleCount; cycle++) {
        m_cycles.addElement(Vector<CZombieWave*>());
        for (int index = 0; index < m_prototypes.size(); index++) {
            CZombieWave* wave = new CZombieWave(m_prototypes[index]);
            wave->m_runtimeId = runtimeId;
            runtimeId++;
            wave->Init();
            m_cycles[cycle].addElement(wave);
        }
    }
}

void CZombieMission::Cleanup()
{
    for (int cycle = 0; cycle < m_cycles.size(); cycle++) {
        SAFE_DELETE_VECTOR(m_cycles[cycle]);
    }
}

void CZombieMission::CompleteAllWavesBeforeThis(int number)
{
    for (int index = 0; index < m_cycles[m_cycleIndex].size(); index++) {
        CZombieWave* wave = m_cycles[m_cycleIndex][index];
        if ((int)wave->m_number < number) {
            wave->m_state = 2;
            wave->m_deathCount = wave->m_totalCount;
        }
    }
}

void CZombieMission::RestartCurrentWave()
{
    for (int index = 0; index < m_cycles[m_cycleIndex].size(); index++) {
        CZombieWave* wave = m_cycles[m_cycleIndex][index];
        if ((int)wave->m_number == m_currentWaveNumber) {
            wave->Init();
            wave->m_state = 1;
            return;
        }
    }
}

void CZombieMission::RunNextCycle()
{
    if (m_cycleIndex + 1 < m_cycleCount) {
        m_cycleIndex++;
        m_cycleComplete = false;
    }
}

void CZombieMission::Update(unsigned int elapsedMs)
{
    if (m_spawns.size() > 0) {
        return;
    }
    bool allComplete = true;
    for (int index = 0; index < m_cycles[m_cycleIndex].size(); index++) {
        CZombieWave* wave = m_cycles[m_cycleIndex][index];
        int state = wave->m_state;
        bool killed = true;
        if (wave->m_totalCount != 0) {
            int percent = (int)(((float)wave->m_deathCount / (float)wave->m_totalCount) * 100.0f);
            killed = false;
            if (percent >= 0 && percent > 99) {
                killed = true;
            }
        }
        allComplete = (state == 2) & allComplete & killed;
        if (state == 0) {
            CheckWaveStartConditions(wave);
        } else if (state == 1) {
            wave->Update(elapsedMs, &m_spawns);
        }
    }
    if (allComplete) {
        m_cycleComplete = true;
    }
}

void CZombieMission::CheckWaveStartConditions(CZombieWave* wave)
{
    if (wave->m_startEvent == CZombieWave::START_ON_TIME) {
        // Original comparison is threshold >= time, rather than elapsed >= threshold.
        if (wave->m_startThreshold < m_timeMs) {
            return;
        }
        wave->m_state = 1;
    } else if (wave->m_startEvent == CZombieWave::START_AFTER_KILLS) {
        if (wave->m_wave->m_mustBeClear &&
            ((App*)WindowApp::m_instance)->m_helper->m_swerveGame->GetUnitsController()->m_zombieCount > 0) {
            return;
        }
        CZombieWave* previous = NULL;
        int distance = 0xffff;
        for (int index = 0; index < m_cycles[m_cycleIndex].size(); index++) {
            CZombieWave* other = m_cycles[m_cycleIndex][index];
            if (other->m_totalCount != 0) {
                int difference = (int)(wave->m_number - other->m_number);
                if (difference > 0 && difference < distance) {
                    previous = other;
                    distance = difference;
                }
            }
        }
        if (previous == NULL || previous->m_state == 0) {
            return;
        }
        int percent = 100;
        if (previous->m_totalCount != 0) {
            int value = (int)(((float)previous->m_deathCount / (float)previous->m_totalCount) * 100.0f);
            percent = 0;
            if (value >= 0) {
                if (value > 100) {
                    value = 100;
                }
                percent = value;
            }
        }
        if (percent < wave->m_startThreshold) {
            return;
        }
        wave->m_state = 1;
        m_currentWaveNumber = wave->m_number;
    } else {
        return;
    }
    m_currentWave = wave->m_wave;
}

const XString& CZombieMission::GetNextSpawnPointName()
{
    // Original does not check whether the spawn vector is empty.
    return m_spawns[0].m_spawnPoint;
}

SSpawnInfo CZombieMission::PopNextSpawn()
{
    if (m_spawns.size() < 1) {
        return SSpawnInfo();
    }
    SSpawnInfo spawn = m_spawns[0];
    m_spawns.removeElementAt(0);
    return spawn;
}

void CZombieMission::OnUnitKilled(CUnit* unit)
{
    RegisterUnitDeath(unit->GetWaveId());
}

void CZombieMission::RegisterUnitDeath(int runtimeId)
{
    for (int index = 0; index < m_cycles[m_cycleIndex].size(); index++) {
        CZombieWave* wave = m_cycles[m_cycleIndex][index];
        if (wave->m_runtimeId == runtimeId) {
            wave->m_deathCount++;
        }
    }
}

int CZombieMission::Preload(int waveLimit, bool countOnly)
{
    Vector<XString> loadedNames;
    int count = 0;
    int examined = 0;
    for (int cycle = 0; cycle < m_cycleCount; cycle++) {
        for (int index = 0; index < m_cycles[cycle].size(); index++) {
            Vector<XString> names;
            CZombieWave* wave = m_cycles[cycle][index];
            if (wave->m_state != 2) {
                wave->GetExpectedVisualCharacters(names);
                examined++;
                int nameCount = names.size();
                const XString* namePointer = names.m_items;
                for (int nameIndex = 0; nameIndex < nameCount; nameIndex++) {
                    const XString& name = *namePointer;
                    if (loadedNames.indexOf(name, 0) == -1 &&
                        ((App*)WindowApp::m_instance)->m_visualCharacterTypeManager->GetType(name)->m_loaded == false) {
                        if (countOnly == false) {
                            ((App*)WindowApp::m_instance)->m_visualCharacterTypeManager->GetType(name)->LoadModel();
                        }
                        count++;
                        loadedNames.addElement(name);
                    }
                    if (count > 7) {
                        return count;
                    }
                    namePointer++;
                }
                if (waveLimit > 0 && examined >= waveLimit) {
                    return count;
                }
            }
        }
    }
    return count;
}

CZombieMission::~CZombieMission()
{
    Cleanup();
}
