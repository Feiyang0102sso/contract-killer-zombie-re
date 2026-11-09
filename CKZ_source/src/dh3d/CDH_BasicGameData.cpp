// CDH_BasicGameData.cpp - progress data shared by every game mode.
#include "CDH_BasicGameData.h"
#include <wchar.h>
#include "App.h"
#include "CDH_WeaponsManager.h"

CDH_BasicGameData::CDH_BasicGameData()
    : m_name("")
{
    m_field08 = 0;
    m_hasData = false;
    m_field0d = -1;
    m_field0e = false;
    m_weaponIds[0] = 0;
    m_weaponIds[1] = 1;
    m_activeWeaponId = -1;
    Init();
}

void CDH_BasicGameData::Init()
{
    m_name = XString("");
    m_field08 = 0;
    m_hasData = false;
    m_field0d = -1;
    m_weaponIds[0] = 0;
    m_weaponIds[1] = 1;
    m_activeWeaponId = 0;
    m_field0e = false;
}

CDH_BasicGameData::~CDH_BasicGameData()
{
}

void CDH_BasicGameData::SetWeapon(int slot, int weaponId)
{
    if (m_activeWeaponId == -1) {
        m_activeWeaponId = weaponId;
    }
    if (slot == 0) {
        m_activeWeaponId = weaponId;
        m_weaponIds[0] = weaponId;
    } else {
        m_weaponIds[1] = weaponId;
    }
}

CDH_Weapon* CDH_BasicGameData::GetWeapon(int slot)
{
    CDH_WeaponsManager* weapons = ((App*)WindowApp::m_instance)->m_weaponsManager;
    if (slot == 0) {
        return weapons->GetWeaponById(m_weaponIds[0]);
    }
    return weapons->GetWeaponById(m_weaponIds[1]);
}

CDH_Weapon* CDH_BasicGameData::GetActiveWeapon()
{
    CDH_WeaponsManager* weapons = ((App*)WindowApp::m_instance)->m_weaponsManager;
    return weapons->GetWeaponById(m_activeWeaponId);
}

void CDH_BasicGameData::SetActiveWeapon(int slot)
{
    m_activeWeaponId = WeaponIdOfSlot(slot);
    if (m_activeWeaponId == -1) {
        m_activeWeaponId = WeaponIdOfSlot(slot == 0);
    }
}

void CDH_BasicGameData::SwitchWeapons()
{
    int other = m_weaponIds[0];
    if (m_activeWeaponId == m_weaponIds[0]) {
        other = m_weaponIds[1];
    }
    if (other == -1) {
        other = m_activeWeaponId;
    }
    m_activeWeaponId = other;
}

void CDH_BasicGameData::Serialize(DataOutputStream& stream)
{
    stream.writeByte(m_hasData);
    stream.writeByte(m_field0e);
    if (!m_hasData) {
        return;
    }
    stream.writeWStr(m_name.c_str(), DataOutputStream::WCHAR_LENGTH_PREFIXED, 0);
    stream.writeInt(m_field08);
    stream.writeByte(m_field0d);
    stream.writeInt(m_weaponIds[0]);
    stream.writeInt(m_weaponIds[1]);
    stream.writeInt(m_activeWeaponId);
}

void CDH_BasicGameData::DeSerialize(JDataInputStream& stream)
{
    m_hasData = stream.readByte() != 0;
    m_field0e = stream.readByte() != 0;
    if (!m_hasData) {
        return;
    }
    wchar_t* name = stream.readWChar();
    m_name.Assign(name, wcslen(name) * sizeof(wchar_t));
    if (name != NULL) {
        delete[] name;
    }
    m_field08 = stream.readInt();
    m_field0d = stream.readByte();
    m_weaponIds[0] = stream.readInt();
    m_weaponIds[1] = stream.readInt();
    m_activeWeaponId = stream.readInt();
}
