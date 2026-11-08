// GamePlayManager.h - the current game mode and the saved player data.
#ifndef GAMEPLAYMANAGER_H
#define GAMEPLAYMANAGER_H

#include "Serializable.h"
#include "CDH_PlayerData.h"
#include "CDH_BasicGameData.h"  // added by b079 (inline script setter needs the complete type)

class CDH_Weapon;

class GamePlayManager : public Serializable {
public:
    GamePlayManager();
    ~GamePlayManager();

    virtual void Serialize(DataOutputStream& stream);
    virtual void DeSerialize(JDataInputStream& stream);

    void init();
    /** Refill the ammo of every weapon. */
    void RestoreAmmo();
    /** Activate the other weapon and tell the running game. */
    void SwitchWeapons();
    /** Put `weapon` (NULL: none) into slot `slot` and save the player. */
    void SetWeapon(int slot, CDH_Weapon* weapon);
    void SetWeapon(int slot, int weaponId);
    /** Script binding at 0x135fc4; preserve the same player-data lookup. */  // added by b079
    void SetActiveWeapon(int slot)  // added by b079
    {
        m_playerData.GetGameData(m_gameType)->SetActiveWeapon(slot);
    }
    /** Data of the current game mode. */
    CDH_BasicGameData* GetGameData()
    {
        return m_playerData.GetGameData(m_gameType);
    }

    game_type_t m_gameType;         // 0x04
    CDH_PlayerData m_playerData;    // 0x08
    bool m_fieldbc;                 // 0xbc
    bool m_fieldbd;                 // 0xbd true by default
};

#endif
