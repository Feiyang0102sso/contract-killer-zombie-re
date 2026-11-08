// CDH_WeaponsManager.cpp - the weapons of the shop, read from res/common/configs/weapons.xml.
#include "CDH_WeaponsManager.h"
#include "CDH_Weapon.h"
#include "CShopWeaponButton.h"

// Weapon config: file, root element, item element.
static const char* const WEAPONS_CONFIG = "res/common/configs/weapons.xml";

CDH_WeaponsManager::CDH_WeaponsManager()
{
    ReloadConfig();
}

void CDH_WeaponsManager::ReloadConfig()
{
    if (IsConfigNew(WEAPONS_CONFIG) == true) {
        CleanupAll();
        LoadFromConfig(WEAPONS_CONFIG, "weapons", "weapon");
    }
}

void CDH_WeaponsManager::AddItem(TiXmlNode* node)
{
    CDH_Weapon* weapon = new CDH_Weapon(node);
    m_weapons.addElement(weapon);
}

void CDH_WeaponsManager::CleanupAll()
{
    releaseResource();
    for (int index = 0; index < m_weapons.size(); index++) {
        m_weapons[index]->Save(true);
        if (m_weapons[index] != NULL) {
            delete m_weapons[index];
            m_weapons[index] = NULL;
        }
    }
    m_weapons.removeAllElements();
}

CDH_WeaponsManager::~CDH_WeaponsManager()
{
    CleanupAll();
}

void CDH_WeaponsManager::releaseResource()
{
    for (int index = 0; index < m_weapons.size(); index++) {
        m_weapons[index]->releaseResources();
    }
}

void CDH_WeaponsManager::loadResources()
{
    for (int index = 0; index < m_weapons.size(); index++) {
        m_weapons[index]->loadResources();
    }
}

void CDH_WeaponsManager::release3dResources(int id)
{
    ((CDH_Weapon*)getItemById(id))->releaseWeaponModel();
}

CItem* CDH_WeaponsManager::getItemByInternalId(int index)
{
    if (index < m_weapons.size()) {
        return m_weapons[index];
    }
    return NULL;
}

int CDH_WeaponsManager::GetUnlockedWeaponsIdOnLevel(int level, Vector<int>& ids)
{
    int count = 0;
    for (CItem* item = firstItem(); item != NULL; item = nextItem()) {
        if (item->getUnlockLvl() == level && !item->isHidden()) {
            count++;
            int id = item->getId();
            ids.addElement(id);
        }
    }
    return count;
}

CShopItemButton* CDH_WeaponsManager::CreateShopItemButton(int index)
{
    return new CShopWeaponButton((CDH_Weapon*)getItemById(index));
}
