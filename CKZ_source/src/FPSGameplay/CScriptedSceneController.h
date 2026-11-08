// PROVISIONAL (created by b079): owner src/FPSGameplay/CScriptedSceneController.cpp not written yet
// Registration-facing declarations only; no allocation or complete vtable claim.
// b066: the comment above keeps the history; the real secondary base Listener is at +0x90, total size 0xb8.
#ifndef CSCRIPTEDSCENECONTROLLER_H
#define CSCRIPTEDSCENECONTROLLER_H
#include "CStrChar.h"
#include "CScriptedObject.h"
#include "CUnitsController.h"
#include "luabridge.h"
using com::glu::platform::components::CStrChar;
class CSwerveGame;
class CMission;
class CScriptedSceneController : public CScriptedObject, public CUnitsController::Listener {
public:
    CScriptedSceneController(CSwerveGame* game, const CMission* mission);
    virtual void Update(int elapsedMs);
    virtual void PushInstanceInLuaState(lua_State* state)
    {
        luabridge::TypeTable::tdstack<CScriptedSceneController>::push(state, *this, 0);
    }
    virtual void OnUnitKilled(CUnit* unit);
    virtual void OnUnitInjured(CUnit* unit, CUnit* attacker, int amount);
    // The original destructor only runs the real member and dual base-class destructors; uses the natural implicit virtual destructor.
    void OnZombieWaveCycleCompleted();
    void OnStart();
    void OnCivilianScriptedEvent(CStrChar event);
    void OnSceneEvent(CStrChar event);
    void OnWin();
    void PreLoad();
    void HelicopterForceLand(); // 0x1333c6
    void HelicopterLand(); // 0x1333e2
    void HelicopterTakeoff(); // 0x1333fe
    void HelicopterEngineOn(); // 0x13341a
    void SetWaveCyclesCount(int); // 0x133436
    int SpawnUnitWithScript(CStrChar, CStrChar, CStrChar); // 0x133452
    int SpawnUnit(CStrChar, CStrChar); // 0x13346e
    void AddSpawnInfoInGroup(CStrChar, CStrChar, CStrChar, CStrChar); // 0x13348a
    void SpawnGroup(CStrChar); // 0x1334a6
    void SetWavesPaused(bool); // 0x1334c2
    int GetAliveZombiesCount(); // 0x1334de
    void RunFinalScene(); // 0x1334fa
    void WinAfterDelay(int); // 0x133516
    void DeleteUnit(int); // 0x133532
    void LoseAfterDelay(int); // 0x13354e
    void RunNextWaveCycle(); // 0x13356a
    void SetTutorialState(int); // 0x133586
    void PopupText(CStrChar); // 0x1335a2
    void PopupTextWithArgs(CStrChar, int); // 0x1335be
    void ShowHUD(bool); // 0x1335da
    void ShowWeapon(bool); // 0x1335ee
    void OnTutorialStateUpdate(); // CTutorialManager 0x13c004

    bool m_cycleCompleteNotified;  // +0x98
    CSwerveGame* m_game;           // +0x9c
    int m_waveCyclesCount;         // +0xa0
    HashTable<XString, Vector<SSpawnInfo> > m_spawnGroups; // +0xa4
    bool m_showHUD;                // +0xb0
    bool m_showWeapon;             // +0xb1
    bool m_forceLand;              // +0xb2
    bool m_land;                   // +0xb3
    bool m_takeoff;                // +0xb4
    bool m_initialized;            // +0xb5, set to true only after the Init script returns
};
#endif
