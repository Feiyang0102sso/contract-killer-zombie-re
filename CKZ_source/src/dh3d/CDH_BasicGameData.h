// CDH_BasicGameData.h - progress data shared by every game mode (name, equipped weapons).
#ifndef CDH_BASICGAMEDATA_H
#define CDH_BASICGAMEDATA_H

#include "Serializable.h"
#include "xString.h"

class CDH_Weapon;

class CDH_BasicGameData : public Serializable {
public:
    CDH_BasicGameData();
    virtual void Serialize(DataOutputStream& stream);
    virtual void DeSerialize(JDataInputStream& stream);
    virtual ~CDH_BasicGameData();
    /** Mode specific reset / end / start of a game. */
    virtual void ResetImpl() = 0;
    virtual void FinishGameImpl() = 0;
    virtual void StartGameImpl() = 0;

    /** Back to the defaults (no name, weapon 1 in the second slot). */
    void Init();
    /** Put weapon `weaponId` into slot `slot` (slot 0 also makes it active; it becomes active when none is). */
    void SetWeapon(int slot, int weaponId);
    /** Weapon of slot `slot`. */
    CDH_Weapon* GetWeapon(int slot);
    CDH_Weapon* GetActiveWeapon();
    /** Make the weapon of slot `slot` active (the other slot's one when it is empty). */
    void SetActiveWeapon(int slot);
    /** Activate the weapon of the other slot when it is not empty. */
    void SwitchWeapons();

    XString m_name;             // 0x04
    int m_field08;              // 0x08
    bool m_hasData;             // 0x0c the rest is saved only when set
    char m_field0d;             // 0x0d -1 by default
    bool m_field0e;             // 0x0e
    /** Weapon id of slot `slot` (0: first, otherwise second); ?: keeps the original address select. */
    int& WeaponIdOfSlot(int slot)
    {
        return (slot == 0) ? m_weaponIds[0] : m_weaponIds[1];
    }

    int m_weaponIds[2];         // 0x10 weapon id per slot (-1: empty)
    int m_activeWeaponId;       // 0x18
};

#endif
