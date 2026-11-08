// Native Lua registration and script loading recovered from 0x132a1c..0x139e24.
#include "CScriptManager.h"
#include "App.h"
#include "ResMap.h"
#include "tinyxml.h"
#include "CXmlHelper.h"
#include "DGHelper.h"
#include "MathLib.h"
#include "CSwerveGame.h"
#include "dgCamera.h"
#include "CDH_WeaponsManager.h"
#include "CDH_Weapon.h"
#include "CDH_SoundManager.h"
#include "CBH_Player.h"
#include "CScriptedObject.h"
#include "CScriptedUnitLogicExecutor.h"
#include "CScriptedSceneController.h"
#include "CUnit.h"
#include "ICStdUtil.h"
#include "luabridge.h"
#include "AdBannerSizes.h"

// External owner src/CUtility.cpp: original binding targets at 0x133880..0x1338c0.
void ShowDialogMessage(const char* title, const char* text, const char* button);
void ShowFloatingMessage(const char* text);
void ResetFloatingMessage();

CScriptManager::CScriptManager()
{
    char* text = ((App*)WindowApp::m_instance)->m_resMap->loadText(
        XString(L"res/common/configs/scripts/scripts.xml"));
    TiXmlDocument document;
    document.Parse(text, NULL, TIXML_ENCODING_UNKNOWN);
    if (text != NULL) {
        free(text);
    }
    const TiXmlNode* root = document.FirstChild("scripts");
    for (const TiXmlNode* node = root->FirstChild("script"); node != NULL; node = node->NextSibling("script")) {
        m_scripts.addElement(CXmlHelper::GetAttributeValue(node, "fileName"));
    }
    LoadScripts();
}

void CScriptManager::LoadScripts()
{
    /** Local helper names are retained in the shipped symbol table. */
    struct Helper {
        static GamePlayManager* GetGamePlayManager()
        {
            return &((App*)WindowApp::m_instance)->m_helper->m_gamePlayManager;
        }
        static CBH_Player* GetPlayer() { return CBH_Player::GetInstance(); }
        static int GetWeaponIdByName(const XString& name)
        {
            CItem* item = ((App*)WindowApp::m_instance)->m_weaponsManager->getItemByName(name);
            if (item != NULL) {
                return item->getId();
            }
            return -1;
        }
        static void SetWeapon(int slot, int id)
        {
            ((App*)WindowApp::m_instance)->m_helper->m_gamePlayManager.SetWeapon(slot, id);
        }
        static int GetWeapon(int slot)
        {
            DGHelper* helper = ((App*)WindowApp::m_instance)->m_helper;
            return helper->m_gamePlayManager.GetGameData()->GetWeapon(slot)->getId();
        }
        static CScriptedSceneController* GetScene()
        {
            return ((App*)WindowApp::m_instance)->m_helper->m_swerveGame->GetScriptController();
        }
        static void ShakeCamera(float amplitude)
        {
            ((App*)WindowApp::m_instance)->m_helper->m_swerveGame->m_camera->SetShake(0.8f, 2.0f, amplitude);
        }
        static void SetActiveWeapon(int slot)
        {
            DGHelper* helper = ((App*)WindowApp::m_instance)->m_helper;
            helper->m_gamePlayManager.GetGameData()->SetActiveWeapon(slot);
        }
        static void SetWeaponByName(int slot, const XString& name)
        {
            CItem* item = ((App*)WindowApp::m_instance)->m_weaponsManager->getItemByName(name);
            int weaponId = item->getId();
            ((App*)WindowApp::m_instance)->m_helper->m_gamePlayManager.SetWeapon(slot, weaponId);
            item->loadResources();
        }
        static void SwitchWeapons()
        {
            ((App*)WindowApp::m_instance)->m_helper->m_gamePlayManager.SwitchWeapons();
        }
        static void MakeWeaponAvailable(int id, bool available)
        {
            CItem* item = ((App*)WindowApp::m_instance)->m_weaponsManager->getItemById(id);
            item->setAvailable(available);
        }
        static XString StringFromRes(const char* name) { return Window::ResString(name); }
        static void PlaySound(const char* name)
        {
            ((App*)WindowApp::m_instance)->m_soundManager->PlayTrack(name, false);
        }
        static int GetRandInt(int limit)
        {
            return ((RandomValue*)&WindowApp::m_instance->m_randomSeed)->Int(limit);
        }
        static int debugMsg(lua_State* state)
        {
            // Original diagnostic consumes the conversion but does not print a message.
            if (lua_type(state, -1) != 0) {
                lua_tostring(state, -1);
            }
            return 0;
        }
        static int int2string(lua_State* state)
        {
            char text[64];
            com::glu::platform::core::ICStdUtil::SPrintF_S(text, 64, "%d", lua_tointeger(state, -1));
            // Preserve the original full-buffer length, including bytes after the terminator.
            lua_pushlstring(state, text, 64);
            return 1;
        }
    };
    lua_State* state = ((App*)WindowApp::m_instance)->m_lua;
    luabridge::module globals(state);
    globals.class_<CScriptedObject>("CScriptedObject")
        .method("RunTimer", &CScriptedObject::RunTimer)
        .method("FlushTimer", &CScriptedObject::FlushTimer)
        .method("IsTimerRunning", &CScriptedObject::IsTimerRunning)
        .method("SetBaseScript", &CScriptedObject::SetBaseScript)
        .method("Super", &CScriptedObject::Super)
        .method("SetParam", &CScriptedObject::SetParam)
        .method("GetParam", &CScriptedObject::GetParam)
        .method("SetParamStr", &CScriptedObject::SetParamStr)
        .method("GetParamStr", &CScriptedObject::GetParamStr)
        .method("PlaySound", &CScriptedObject::PlaySound)
        .method("Print", &CScriptedObject::Print)
        .property_rw("param1", &CScriptedObject::m_param1);
    globals.class_<CScriptedUnitLogicExecutor, CScriptedObject>("CScriptedUnitLogicExecutor")
        .property_ro("hp", &CScriptedUnitLogicExecutor::GetHP)
        .property_ro("maxhp", &CScriptedUnitLogicExecutor::GetMaxHP)
        .property_ro("state", &CScriptedUnitLogicExecutor::GetState)
        .property_ro("waypointsCount", &CScriptedUnitLogicExecutor::GetWaypointsCount)
        .property_ro("lifeTime", &CScriptedUnitLogicExecutor::GetLifeTimeMS)
        .property_ro("zombiesInAttackRange", &CScriptedUnitLogicExecutor::GetZombiesInAttackRange)
        .property_ro("zombiesInFearRange", &CScriptedUnitLogicExecutor::GetZombiesInFearRange)
        .property_ro("zombiesInDangerRange", &CScriptedUnitLogicExecutor::GetZombiesInDangerRange)
        .property_ro("hasBox", &CScriptedUnitLogicExecutor::HasBox)
        .property_ro("hasTarget", &CScriptedUnitLogicExecutor::HasTarget)
        .property_ro("isTargetNear", &CScriptedUnitLogicExecutor::IsTargetNear)
        .property_ro("isTargetInSpecialRange", &CScriptedUnitLogicExecutor::IsTargetInSpecialRange)
        .property_ro("isAllZombiesKilled", &CScriptedUnitLogicExecutor::IsAllZombiesKilled)
        .property_ro("isIdling", &CScriptedUnitLogicExecutor::IsIdling)
        .property_ro("isMoving", &CScriptedUnitLogicExecutor::IsMoving)
        .property_ro("isAttacking", &CScriptedUnitLogicExecutor::IsAttacking)
        .property_ro("isSpawning", &CScriptedUnitLogicExecutor::IsSpawning)
        .method("SetNearestCivilianAsTarget", &CScriptedUnitLogicExecutor::SetNearestCivilianAsTarget)
        .method("SetNearestZombieAsTarget", &CScriptedUnitLogicExecutor::SetNearestZombieAsTarget)
        .method("SetMovementCarrying", &CScriptedUnitLogicExecutor::SetMovementCarrying)
        .method("SetMovementRun", &CScriptedUnitLogicExecutor::SetMovementRun)
        .method("SetMovementWalk", &CScriptedUnitLogicExecutor::SetMovementWalk)
        .method("IsNearestCivilianMyTarget", &CScriptedUnitLogicExecutor::IsNearestCivilianMyTarget)
        .method("TeleportInPointWithId", &CScriptedUnitLogicExecutor::TeleportInPointWithId)
        .method("TeleportInPointWithName", &CScriptedUnitLogicExecutor::TeleportInPointWithName)
        .method("SendScriptedEvent", &CScriptedUnitLogicExecutor::SendScriptedEvent)
        .method("AddWaypointWithId", &CScriptedUnitLogicExecutor::AddWaypointWithId)
        .method("AddWaypointWithName", &CScriptedUnitLogicExecutor::AddWaypointWithName)
        .method("MoveToNearestPoint", &CScriptedUnitLogicExecutor::MoveToNearestPoint)
        .method("MoveToRandomPoint", &CScriptedUnitLogicExecutor::MoveToRandomPoint)
        .method("MoveToNextWaypoint", &CScriptedUnitLogicExecutor::MoveToNextWaypoint)
        .method("MoveToPointWithName", &CScriptedUnitLogicExecutor::MoveToPointWithName)
        .method("MoveToPointWithId", &CScriptedUnitLogicExecutor::MoveToPointWithId)
        .method("AttackTarget", &CScriptedUnitLogicExecutor::AttackTarget)
        .method("BeginBrawl", &CScriptedUnitLogicExecutor::BeginBrawl)
        .method("AttackTargetModal", &CScriptedUnitLogicExecutor::AttackTargetModal)
        .method("Idle", &CScriptedUnitLogicExecutor::Idle)
        .method("IdleMS", &CScriptedUnitLogicExecutor::IdleMS)
        .method("Afraid", &CScriptedUnitLogicExecutor::Afraid)
        .method("AfraidMS", &CScriptedUnitLogicExecutor::AfraidMS)
        .method("PlayWound", &CScriptedUnitLogicExecutor::PlayWound)
        .method("TakeBox", &CScriptedUnitLogicExecutor::TakeBox)
        .method("PutBox", &CScriptedUnitLogicExecutor::PutBox)
        .method("ThrowWeapon", &CScriptedUnitLogicExecutor::ThrowWeapon)
        .method("PickUpWeapon", &CScriptedUnitLogicExecutor::PickUpWeapon)
        .method("ThreatenWithWeapon", &CScriptedUnitLogicExecutor::ThreatenWithWeapon)
        .method("ShowWeapon", &CScriptedUnitLogicExecutor::ShowWeapon)
        .method("PrepareBox", &CScriptedUnitLogicExecutor::PrepareBox)
        .method("WalkAround", &CScriptedUnitLogicExecutor::WalkAround)
        .method("RunAway", &CScriptedUnitLogicExecutor::RunAway)
        .method("Disappear", &CScriptedUnitLogicExecutor::Disappear)
        .method("SwitchBrain", &CScriptedUnitLogicExecutor::SwitchBrain)
        .method("MoveToTarget", &CScriptedUnitLogicExecutor::MoveToTarget)
        .method("MoveToTargetModal", &CScriptedUnitLogicExecutor::MoveToTargetModal)
        .method("MoveToTargetOnAttackDistance", &CScriptedUnitLogicExecutor::MoveToTargetOnAttackDistance)
        .method("SetCorpseDisappear", &CScriptedUnitLogicExecutor::SetCorpseDisappear)
        .method("AutoKill", &CScriptedUnitLogicExecutor::AutoKill)
        .method("SetShouldBlow", &CScriptedUnitLogicExecutor::SetShouldBlow)
        .method("FeintDeath", &CScriptedUnitLogicExecutor::FeintDeath)
        .method("Enrage", &CScriptedUnitLogicExecutor::Enrage)
        .method("PlayRage", &CScriptedUnitLogicExecutor::PlayRage)
        .method("SetSpecialZoneDamageMult", &CScriptedUnitLogicExecutor::SetSpecialZoneDamageMult);
    globals.class_<CScriptedSceneController, CScriptedObject>("CScriptedSceneController")
        .method("HelicopterForceLand", &CScriptedSceneController::HelicopterForceLand)
        .method("HelicopterLand", &CScriptedSceneController::HelicopterLand)
        .method("HelicopterTakeoff", &CScriptedSceneController::HelicopterTakeoff)
        .method("HelicopterEngineOn", &CScriptedSceneController::HelicopterEngineOn)
        .method("SetWaveCyclesCount", &CScriptedSceneController::SetWaveCyclesCount)
        .method("SpawnUnitWithScript", &CScriptedSceneController::SpawnUnitWithScript)
        .method("SpawnUnit", &CScriptedSceneController::SpawnUnit)
        .method("AddSpawnInfoInGroup", &CScriptedSceneController::AddSpawnInfoInGroup)
        .method("SpawnGroup", &CScriptedSceneController::SpawnGroup)
        .method("SetWavesPaused", &CScriptedSceneController::SetWavesPaused)
        .method("GetAliveZombiesCount", &CScriptedSceneController::GetAliveZombiesCount)
        .method("RunFinalScene", &CScriptedSceneController::RunFinalScene)
        .method("WinAfterDelay", &CScriptedSceneController::WinAfterDelay)
        .method("DeleteUnit", &CScriptedSceneController::DeleteUnit)
        .method("LoseAfterDelay", &CScriptedSceneController::LoseAfterDelay)
        .method("RunNextWaveCycle", &CScriptedSceneController::RunNextWaveCycle)
        .method("SetTutorialState", &CScriptedSceneController::SetTutorialState)
        .method("PopupText", &CScriptedSceneController::PopupText)
        .method("PopupTextWithArgs", &CScriptedSceneController::PopupTextWithArgs)
        .method("ShowHUD", &CScriptedSceneController::ShowHUD)
        .method("ShowWeapon", &CScriptedSceneController::ShowWeapon);
    globals.class_<CUnit>("CUnit")
        .property_ro("hp", &CUnit::GetHP)
        .property_ro("hpMax", &CUnit::GetHPMax)
        .method("IsCivillian", &CUnit::IsCivilian)
        .method("IsDead", &CUnit::IsDead)
        .method("GetID", &CUnit::GetId)
        .method("SetVulnerability", &CUnit::SetVulnerability)
        .method("SetUndying", &CUnit::SetUndying)
        .method("SendSceneScriptedEvent", &CUnit::SendSceneScriptedEvent)
        .method("SetCorpseDisappear", &CUnit::SetCorpseDisappear);
    globals.class_<GamePlayManager>("GamePlayManager")
        .method("SetWeapon", static_cast<void (GamePlayManager::*)(int, int)>(&GamePlayManager::SetWeapon))
        .method("SetActiveWeapon", &GamePlayManager::SetActiveWeapon)
        .method("SwitchWeapons", &GamePlayManager::SwitchWeapons);
    globals.class_<CBH_Player>("CPlayer");
    globals.function("GetGamePlayManager", &Helper::GetGamePlayManager);
    globals.function("GetPlayer", &Helper::GetPlayer);
    globals.function("GetWeaponIdByName", &Helper::GetWeaponIdByName);
    globals.function("SetWeapon", &Helper::SetWeapon);
    globals.function("GetWeapon", &Helper::GetWeapon);
    globals.function("GetScene", &Helper::GetScene);
    globals.function("ShakeCamera", &Helper::ShakeCamera);
    globals.function("SetActiveWeapon", &Helper::SetActiveWeapon);
    globals.function("SetWeaponByName", &Helper::SetWeaponByName);
    globals.function("SwitchWeapons", &Helper::SwitchWeapons);
    globals.function("MakeWeaponAvailable", &Helper::MakeWeaponAvailable);
    globals.function("ShowDialogMessage", &ShowDialogMessage);
    globals.function("ShowFloatingMessage", &ShowFloatingMessage);
    globals.function("ResetFloatingMessage", &ResetFloatingMessage);
    globals.function("GetUnitByID", &CUnit::GetUnitById);
    globals.function("StringFromRes", &Helper::StringFromRes);
    globals.function("PlaySound", &Helper::PlaySound);
    globals.function("GetRandInt", &Helper::GetRandInt);
    lua_pushcfunction(state, Helper::debugMsg);
    lua_setfield(state, LUA_GLOBALSINDEX, "DDD");
    lua_pushcfunction(state, Helper::int2string);
    lua_setfield(state, LUA_GLOBALSINDEX, "int2string");
    for (int index = 0; index < m_scripts.size(); ++index) {
        XString path = XString(L"res/common/configs/scripts/") + m_scripts[index];
        char* script = ((App*)WindowApp::m_instance)->m_resMap->loadText(path);
        // Both calls run regardless of Lua's error codes in the original.
        luaL_loadstring(state, script);
        lua_pcall(state, 0, LUA_MULTRET, 0);
        if (script != NULL) {
            free(script);
        }
    }
}

void CScriptManager::ParseScriptParams(Vector<SScriptParam>& params, XString text)
{
    Vector<XString> entries;
    DGHelper::SplitStringAndTrim(entries, text, XString(","));
    for (int index = 0; index < entries.size(); ++index) {
        XString* entry = &entries[index];
        if (entry->ByteSize() != 0) {
            Vector<XString> fields;
            DGHelper::SplitStringAndTrim(fields, *entry, XString(":"));
            SScriptParam param;
            // The original reads two fields without checking fields.size().
            param.m_name = fields[0];
            param.m_value = fields[1];
            params.addElement(param);
        }
    }
}

CScriptManager::~CScriptManager()
{
}
