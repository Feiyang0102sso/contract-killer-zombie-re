// CZombieMission.h - configured wave prototypes and independent runtime cycles.
#ifndef CZOMBIEMISSION_H
#define CZOMBIEMISSION_H

#include "CWaveManager.h"
#include "CUnitsController.h"

/** One runtime wave; copying deliberately does not copy its owned spawn events. */
class CZombieWave {
public:
    enum EStartEvent {
        START_AFTER_KILLS = 0,
        START_ON_TIME = 1
    };
    struct SSpawnEvent : public SSpawnablePerson {
        XString m_spawnPoint;       // 0x14
        int m_spawnTime;            // 0x18
        bool m_spawned;             // 0x1c
    };

    CZombieWave(unsigned int number, EStartEvent event, int threshold,
                CMission::EMissionType type, CMission::EMissionDifficulty difficulty, const XString& filter);
    CZombieWave(unsigned int number, int level, EStartEvent event, int threshold,
                CMission::EMissionType type, CMission::EMissionDifficulty difficulty, const XString& filter);
    CZombieWave(const CZombieWave& other)
        : m_wave(other.m_wave), m_runtimeId(0), m_number(other.m_number),
          m_startEvent(other.m_startEvent), m_startThreshold(other.m_startThreshold),
          m_countdown(other.m_countdown), m_countdownNotified(other.m_countdownNotified),
          m_startedNotified(other.m_startedNotified)
    {
        // State, elapsed time and death counters are initialized by Init, as in the original.
    }
    void Init();
    void Update(unsigned int elapsedMs, Vector<SSpawnInfo>* spawns);
    void GetExpectedVisualCharacters(Vector<XString>& names);
    ~CZombieWave();

    Vector<SSpawnEvent*> m_events;  // 0x00
    CWaveManager::SWave* m_wave;    // 0x10
    int m_runtimeId;               // 0x14
    unsigned int m_number;         // 0x18
    EStartEvent m_startEvent;      // 0x1c
    int m_startThreshold;          // 0x20
    int m_state;                  // 0x24: 0 waiting, 1 running, 2 finished
    unsigned int m_elapsedMs;     // 0x28
    int m_deathCount;              // 0x2c
    int m_totalCount;              // 0x30
    int m_countdown;               // 0x34
    bool m_countdownNotified;      // 0x38
    bool m_startedNotified;        // 0x39
};

class CZombieMission : public CUnitsController::Listener {
public:
    CZombieMission(CMission::EMissionType type, CMission::EMissionDifficulty difficulty,
                   int waveCount, const XString& filter);
    void Init(int cycleCount);
    void Cleanup();
    void CompleteAllWavesBeforeThis(int number);
    void RestartCurrentWave();
    void RunNextCycle();
    void Update(unsigned int elapsedMs);
    void CheckWaveStartConditions(CZombieWave* wave);
    const XString& GetNextSpawnPointName();
    SSpawnInfo PopNextSpawn();
    virtual void OnUnitKilled(CUnit* unit);
    void RegisterUnitDeath(int runtimeId);
    int Preload(int waveLimit, bool countOnly);
    virtual ~CZombieMission();

    CMission::EMissionType m_type;      // 0x08
    Vector<CZombieWave> m_prototypes;  // 0x0c
    Vector<SSpawnInfo> m_spawns;       // 0x1c
    Vector<Vector<CZombieWave*> > m_cycles; // 0x2c
    CWaveManager::SWave* m_currentWave; // 0x3c
    int m_currentWaveNumber;            // 0x40
    int m_cycleCount;                   // 0x44
    int m_cycleIndex;                   // 0x48
    XString m_field4c;                 // 0x4c constructed/destroyed; meaning unresolved
    int m_timeMs;                      // 0x50
    bool m_field54;                    // 0x54
    bool m_cycleComplete;              // 0x55
};

#endif
