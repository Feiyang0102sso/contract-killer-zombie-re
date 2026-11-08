// Scene Lua controller: original mission parameters, unit events, spawn queue and helicopter actions.
#include "CScriptedSceneController.h"
#include "CSwerveGame.h"
#include "CHelicopter.h"
#include "CMission.h"
#include "CZombieMission.h"
#include "CUnit.h"
#include "App.h"
#include "CHuntingInfo.h"
#include "DGHelper.h"
#include "GamePlayManager.h"
#include "CDH_BasicGameData.h"
#include "CDH_Weapon.h"
#include "CBH_Player.h"
#include "CTutorialManager.h"
#include "CPopupController.h"
#include "CBH_PersonTypeManager.h"
#include "CBH_PersonType.h"
#include "CVisualCharacterType.h"
#include "CRandGen.h"
#include "AdBannerSizes.h"

const char* REWARD_PEOPLE_STR = "REWARD_PEOPLE";
const char* REWARD_PROVISION_STR = "REWARD_PROVISION";
const char* RESCUE_PEOPLE_STR = "RESCUE_PEOPLE";
const char* SAVED_PEOPLE_STR = "SAVED_PEOPLE";

CScriptedSceneController::CScriptedSceneController(CSwerveGame* game, const CMission* mission)
    : CScriptedObject(mission->m_scriptName), CUnitsController::Listener(NULL),
      m_cycleCompleteNotified(false), m_game(game), m_waveCyclesCount(1), m_spawnGroups(8),
      m_showHUD(true), m_showWeapon(true), m_forceLand(false), m_land(false),
      m_takeoff(false), m_initialized(false)
{
    SetParam(CStrChar(REWARD_PEOPLE_STR), mission->GetRewardPeople());
    SetParam(CStrChar(REWARD_PROVISION_STR), mission->GetRewardProvision());
    SetParam(CStrChar(RESCUE_PEOPLE_STR), ((App*)WindowApp::m_instance)->m_huntingInfo->PeopleToSend());
    SetParam(CStrChar(SAVED_PEOPLE_STR), 0);
    for (int index = 0; index < mission->m_scriptParams.size(); index++) {
        CStrChar name(XString::AnsiString(mission->m_scriptParams[index].m_name.c_str()));
        SetParamStr(name, CStrChar(XString::AnsiString(mission->m_scriptParams[index].m_value.c_str())));
    }
    RunLuaCommand(XString("Init"), CStrChar(""), 0);
    m_initialized = true;
}

void CScriptedSceneController::Update(int elapsedMs)
{
    if (m_game->GetZombieMission()->m_cycleComplete && !m_cycleCompleteNotified) {
        m_cycleCompleteNotified = true;
        OnZombieWaveCycleCompleted();
    }
    CScriptedObject::Update(elapsedMs);
    if (m_forceLand) {
        CHelicopter* helicopter = ((App*)WindowApp::m_instance)->m_helper->m_swerveGame->GetHelicopter();
        if (helicopter != NULL) {
            helicopter->ForceLand();
            m_forceLand = false;
        }
    }
    if (m_land) {
        CHelicopter* helicopter = ((App*)WindowApp::m_instance)->m_helper->m_swerveGame->GetHelicopter();
        if (helicopter != NULL && helicopter->IsFlying() == true) {
            helicopter->Land();
            m_land = false;
        }
    }
    if (m_takeoff) {
        CHelicopter* helicopter = ((App*)WindowApp::m_instance)->m_helper->m_swerveGame->GetHelicopter();
        if (helicopter != NULL && helicopter->IsLanded() == true) {
            helicopter->Takeoff();
            m_takeoff = false;
        }
    }
}

void CScriptedSceneController::OnZombieWaveCycleCompleted()
{
    RunLuaCommand(XString("OnZombieWaveCycleCompleted"), CStrChar(""), 0);
}

void CScriptedSceneController::OnStart()
{
    RunLuaCommand(XString("OnStart"), CStrChar(""), 0);
}

void CScriptedSceneController::OnCivilianScriptedEvent(CStrChar event)
{
    RunLuaCommand(XString("OnCivilianScriptedEvent"), event, 0);
}

void CScriptedSceneController::OnSceneEvent(CStrChar event)
{
    RunLuaCommand(XString("OnSceneEvent"), event, 0);
}

void CScriptedSceneController::OnWin()
{
    RunLuaCommand(XString("OnWin"), CStrChar(""), 0);
    CHuntingInfo* hunt = ((App*)WindowApp::m_instance)->m_huntingInfo;
    hunt->SetPeopleEarned(GetParam(CStrChar(REWARD_PEOPLE_STR)));
    hunt->SetProvisionEarned(GetParam(CStrChar(REWARD_PROVISION_STR)));
    hunt->SetPeopleSent(GetParam(CStrChar(SAVED_PEOPLE_STR)));
}

void CScriptedSceneController::OnUnitKilled(CUnit* unit)
{
    if (unit->IsCivilian() == true && !unit->IsMarkedAsRemoved()) {
        CStrChar parameter(XString::AnsiString(XString(unit->GetId()).c_str()));
        RunLuaCommand(XString("OnCivilianDeath"), parameter, 0);
    }
    // The original queries IsCivilian a second time; must not cache it or turn it into an else.
    if (unit->IsCivilian() == false && !unit->IsMarkedAsRemoved()) {
        CStrChar parameter(XString::AnsiString(XString(unit->GetId()).c_str()));
        RunLuaCommand(XString("OnZombieDeath"), parameter, 0);
    }
}

void CScriptedSceneController::OnUnitInjured(CUnit* unit, CUnit* attacker, int amount)
{
    if (unit->IsCivilian() == true && attacker == NULL) {
        CStrChar parameter(XString::AnsiString(XString(amount).c_str()));
        RunLuaCommand(XString("OnCivilianInjuredByPlayer"), parameter, 0);
    }
    if (unit->IsCivilian() == true && attacker != NULL) {
        CStrChar parameter(XString::AnsiString(XString(attacker->GetId()).c_str()));
        RunLuaCommand(XString("OnCivilianInjuredByUnit"), parameter, 0);
    }
}

void CScriptedSceneController::SetTutorialState(int state)
{
    CBH_Player::GetInstance()->SetTutorialState(state);
    ((App*)WindowApp::m_instance)->m_tutorialManager->ShowStateInitMessage();
}

int CScriptedSceneController::SpawnUnit(CStrChar personName, CStrChar spawnPoint)
{
    XString person(personName.GetCharPtr());
    XString point(spawnPoint.GetCharPtr());
    CUnitsController* units = m_game->GetUnitsController();
    return units->SpawnUnit(SSpawnInfo(SSpawnablePerson(person,
        com::glu::platform::core::CRandGen::GetInstance()->Generate(), XString(L"")), point, -1), -1);
}

int CScriptedSceneController::SpawnUnitWithScript(CStrChar personName, CStrChar scriptName, CStrChar spawnPoint)
{
    XString person(personName.GetCharPtr());
    XString point(spawnPoint.GetCharPtr());
    XString script(scriptName.GetCharPtr());
    CUnitsController* units = m_game->GetUnitsController();
    return units->SpawnUnit(SSpawnInfo(SSpawnablePerson(person,
        com::glu::platform::core::CRandGen::GetInstance()->Generate(), script), point, -1), -1);
}

void CScriptedSceneController::AddSpawnInfoInGroup(CStrChar groupName, CStrChar personName,
                                                 CStrChar scriptName, CStrChar spawnPoint)
{
    XString group(groupName.GetCharPtr());
    if (m_spawnGroups.Get(group) == NULL) {
        m_spawnGroups.Add(group, Vector<SSpawnInfo>());
    }
    unsigned int seed = com::glu::platform::core::CRandGen::GetInstance()->Generate();
    Vector<SSpawnInfo>* spawns = m_spawnGroups.Get(group);
    spawns->addElement(SSpawnInfo(SSpawnablePerson(XString(personName.GetCharPtr()), seed,
        XString(scriptName.GetCharPtr())), XString(spawnPoint.GetCharPtr()), -1));
}

void CScriptedSceneController::SpawnGroup(CStrChar groupName)
{
    XString name(groupName.GetCharPtr());
    Vector<SSpawnInfo>* spawns = m_spawnGroups.Get(name);
    if (spawns != NULL) {
        for (int index = 0; index < spawns->size(); index++) {
            m_game->GetUnitsController()->AddUnitInSpawnQueue((*spawns)[index]);
        }
    }
}

void CScriptedSceneController::RunFinalScene()
{
    m_forceLand = false;
    m_land = false;
    m_takeoff = true;
}

void CScriptedSceneController::HelicopterTakeoff()
{
    m_forceLand = false;
    m_land = false;
    m_takeoff = true;
}

void CScriptedSceneController::LoseAfterDelay(int delay)
{
    m_game->FinishGameWithResultAndDelayMS((EBountyResult)1, delay);
}

void CScriptedSceneController::WinAfterDelay(int delay)
{
    m_game->FinishGameWithResultAndDelayMS((EBountyResult)3, delay);
}

void CScriptedSceneController::DeleteUnit(int id)
{
    CUnit* unit = m_game->GetUnitsController()->GetUnitByUniqueId(id);
    if (unit != NULL) {
        unit->MarkAsRemoved();
    }
}

void CScriptedSceneController::RunNextWaveCycle()
{
    m_cycleCompleteNotified = false;
    m_game->GetZombieMission()->RunNextCycle();
}

void CScriptedSceneController::ShowHUD(bool show)
{
    m_showHUD = show;
}

void CScriptedSceneController::ShowWeapon(bool show)
{
    m_showWeapon = show;
    CDH_Weapon* weapon = ((App*)WindowApp::m_instance)->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon();
    if (show == true) {
        weapon->fetchWeaponModel();
    }
    else {
        weapon->unfetchWeaponModel();
    }
}

void CScriptedSceneController::HelicopterForceLand()
{
    m_forceLand = true;
    m_land = false;
    m_takeoff = false;
}

void CScriptedSceneController::HelicopterLand()
{
    m_forceLand = false;
    m_land = true;
    m_takeoff = false;
}

void CScriptedSceneController::HelicopterEngineOn()
{
    CHelicopter* helicopter = ((App*)WindowApp::m_instance)->m_helper->m_swerveGame->GetHelicopter();
    if (helicopter != NULL && helicopter->IsLanded() == true) {
        helicopter->EngineOn();
    }
}

int CScriptedSceneController::GetAliveZombiesCount()
{
    return m_game->GetUnitsController()->m_zombieCount; // original f7a9c reads the count at Controller+38 directly.
}

void CScriptedSceneController::SetWavesPaused(bool paused)
{
    m_game->GetUnitsController()->SetWavesPaused(paused);
}

void CScriptedSceneController::SetWaveCyclesCount(int count)
{
    m_waveCyclesCount = count;
}

void CScriptedSceneController::PopupText(CStrChar resourceName)
{
    XString text = Window::ResString(resourceName.GetCharPtr());
    if (text.ByteSize() == 0) {
        text = XString(resourceName.GetCharPtr());
    }
    ((App*)WindowApp::m_instance)->m_popupController->AddTextPopup(text,
        CPopupController::Position3, CPopupController::AnimationStyle0, false, 17);
}

void CScriptedSceneController::PopupTextWithArgs(CStrChar resourceName, int argument)
{
    XString text = Window::ResString(resourceName.GetCharPtr());
    if (text.ByteSize() == 0) {
        text = XString(resourceName.GetCharPtr());
    }
    text = XString::Format(text.c_str(), argument);
    ((App*)WindowApp::m_instance)->m_popupController->AddTextPopup(text,
        CPopupController::Position3, CPopupController::AnimationStyle0, false, 17);
}

void CScriptedSceneController::PreLoad()
{
    for (int group = 0; group < m_spawnGroups.m_count; group++) {
        Vector<SSpawnInfo>* spawns = m_spawnGroups.GetAt(group, NULL);
        if (spawns == NULL) {
            break;
        }
        for (int index = 0; index < spawns->size(); index++) {
            SSpawnInfo& spawn = (*spawns)[index];
            ((App*)WindowApp::m_instance)->m_personTypeManager->GetType(spawn.m_typeName)
                ->GetVisualObject(spawn.m_seed)->LoadModel();
        }
    }
}

void CScriptedSceneController::OnTutorialStateUpdate()
{
    CStrChar parameter(XString::AnsiString(XString(CBH_Player::GetInstance()->GetTutorialState()).c_str()));
    RunLuaCommand(XString("OnTutorialStateUpdate"), parameter, 0);
}
