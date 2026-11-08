// CShopPackManager.cpp - the configured grenade and power-up bundles of the shop.
#include "CShopPackManager.h"
#include "CShopPack.h"
#include "CShopCommonItemButton.h"
#include "CShopGrenadeButton.h"
#include "CShopPowerUpButton.h"

CItem* CShopPackManager::getItemByInternalId(int index)
{
    if (index < m_items.size()) {
        return m_items[index];
    }
    return NULL;
}

CShopItemButton* CShopPackManager::CreateShopItemButton(int id)
{
    return new CShopCommonItemButton(getItemById(id));
}

CShopPackManager::~CShopPackManager()
{
    for (int index = 0; index < m_items.size(); index++) {
        if (m_items[index] != NULL) {
            delete m_items[index];
            m_items[index] = NULL;
        }
    }
}

CGrenadePackManager::CGrenadePackManager()
{
    LoadFromConfig("res/common/configs/grenade_packs.xml", "grenade_packs", "grenade_pack");
}

void CGrenadePackManager::AddItem(TiXmlNode* node)
{
    CShopPack* item = new CGrenadePack(node);
    m_items.addElement(item);
}

CShopItemButton* CGrenadePackManager::CreateShopItemButton(int id)
{
    return new CShopGrenadeButton(static_cast<CGrenadePack*>(getItemById(id)));
}

int CGrenadePackManager::GetUnlockedGrenadePackIdsOnLevel(int level, Vector<int>& ids)
{
    int count = 0;
    for (int index = 0; index < m_items.size(); index++) {
        if (m_items[index]->getUnlockLvl() == level) {
            count++;
            ids.addElement(m_items[index]->getId());
        }
    }
    return count;
}

CPowerUpPackManager::CPowerUpPackManager()
{
    LoadFromConfig("res/common/configs/powerup_packs.xml", "powerup_packs", "powerup_pack");
}

void CPowerUpPackManager::AddItem(TiXmlNode* node)
{
    CShopPack* item = new CPowerUpPack(node);
    m_items.addElement(item);
}

CShopItemButton* CPowerUpPackManager::CreateShopItemButton(int id)
{
    return new CShopPowerUpButton(static_cast<CPowerUpPack*>(getItemById(id)));
}
