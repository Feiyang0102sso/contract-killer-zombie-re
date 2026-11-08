// CEnergyManager.cpp - the energy items of the shop.
#include "CEnergyManager.h"
#include "tinyxml.h"
#include "CStatsBonusItem.h"
#include "CShopEnergyButton.h"

void CEnergyManager::AddItem(TiXmlNode* node)
{
    CStatsBonusItem* item = new CStatsBonusItem(node);
    m_items.addElement(item);
}

CEnergyManager::~CEnergyManager()
{
    freeResources();
}

void CEnergyManager::freeResources()
{
    for (int index = 0; index < m_items.size(); index++) {
        if (m_items[index] != NULL) {
            delete m_items[index];
            m_items[index] = NULL;
        }
    }
}

void CEnergyManager::loadResources()
{
    for (int index = 0; index < m_items.size(); index++) {
        m_items[index]->loadResources();
    }
}

CItem* CEnergyManager::getItemByInternalId(int index)
{
    CItem* item = NULL;
    if (index < m_items.size()) {
        item = m_items[index];
    }
    return item;
}

CShopItemButton* CEnergyManager::CreateShopItemButton(int id)
{
    return new CShopEnergyButton(getItemById(id));
}
