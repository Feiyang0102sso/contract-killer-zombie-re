// GamePlayManager.cpp - the current game mode and the saved player data.
#include "GamePlayManager.h"
#include "App.h"
#include "DGHelper.h"
#include "CDH_BasicGameData.h"
#include "CDH_WeaponsManager.h"
#include "CDH_Weapon.h"
#include "CSwerveGame.h"
#include "CBH_Player.h"

GamePlayManager::GamePlayManager()
    : m_gameType((game_type_t)0)
{
    m_fieldbc = false;
    m_fieldbd = true;
}

void GamePlayManager::init()
{
}

GamePlayManager::~GamePlayManager()
{
}

void GamePlayManager::Serialize(DataOutputStream& stream)
{
    m_playerData.Serialize(stream);
}

void GamePlayManager::DeSerialize(JDataInputStream& stream)
{
    m_playerData.DeSerialize(stream);
}

void GamePlayManager::RestoreAmmo()
{
    CDH_WeaponsManager* weaponsManager = ((App*)WindowApp::m_instance)->m_weaponsManager;
    Vector<CDH_Weapon*>& weapons = weaponsManager->GetWeapons();
    for (int index = 0; index < weapons.size(); index++) {
        weapons[index]->restoreAmmo();
    }
}

void GamePlayManager::SwitchWeapons()
{
    m_playerData.GetGameData(m_gameType)->SwitchWeapons();
    CSwerveGame* game = ((App*)WindowApp::m_instance)->m_helper->m_swerveGame;
    if (game != NULL) {
        game->onWeaponSwitch();
    }
}

void GamePlayManager::SetWeapon(int slot, CDH_Weapon* weapon)
{
    if (weapon != NULL) {
        int weaponId = weapon->GetId();
        m_playerData.GetGameData(m_gameType)->SetWeapon(slot, weaponId);
    } else {
        m_playerData.GetGameData(m_gameType)->SetWeapon(slot, -1);
    }
    CBH_Player::GetInstance()->Save(true);
}

void GamePlayManager::SetWeapon(int slot, int weaponId)
{
    m_playerData.GetGameData(m_gameType)->SetWeapon(slot, weaponId);
}
