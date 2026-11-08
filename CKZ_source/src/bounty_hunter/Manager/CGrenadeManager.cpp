// CGrenadeManager.cpp - the grenade types of the shop, the counts the player owns and the active one.
#include "CGrenadeManager.h"
#include "tinyxml.h"
#include "CXmlHelper.h"
#include "CGrenade.h"

CGrenadeManager::CGrenadeManager()
{
    m_activeGrenadeId = -1;
    LoadFromConfig("res/common/configs/grenades.xml", "grenades", "grenade");
}

void CGrenadeManager::Save(TiXmlElement* root)
{
    TiXmlElement* grenades = CXmlHelper::ObtainElement(root, XString("grenades"));
    grenades->Clear();
    grenades->SetAttribute("active_gid", m_activeGrenadeId);
    for (int index = 0; index < m_items.size(); index++) {
        CGrenade* grenade = m_items[index];
        if (grenade->GetCount() > 0) {
            TiXmlElement* element = new TiXmlElement("g");
            element->SetAttribute("id", grenade->getId());
            element->SetAttribute("c", grenade->GetCount());
            grenades->InsertEndChild(*element);
            delete element;
        }
    }
}

void CGrenadeManager::Load(TiXmlElement* root)
{
    TiXmlElement* grenades = root->FirstChildElement("grenades");
    if (grenades == NULL) {
        return;
    }
    m_activeGrenadeId = CXmlHelper::GetAttributeValue(grenades, "active_gid").ToInt();
    TiXmlElement* element = grenades->FirstChildElement("g");
    while (element != NULL) {
        int id = CXmlHelper::GetAttributeValue(element, "id").ToInt();
        CGrenade* grenade = (CGrenade*)getItemById(id);
        if (grenade == NULL) {
            // As in the original: the element is not advanced (an unknown id never ends the loop).
            continue;
        }
        int count = CXmlHelper::GetAttributeValue(element, "c").ToInt();
        grenade->SetCount(count);
        element = element->NextSiblingElement("g");
    }
}

void CGrenadeManager::AddItem(TiXmlNode* node)
{
    CGrenade* grenade = new CGrenade(node);
    m_items.addElement(grenade);
}

CGrenade* CGrenadeManager::GetActiveGrenade()
{
    CGrenade* grenade = NULL;
    if (m_activeGrenadeId >= 0) {
        grenade = (CGrenade*)getItemById(m_activeGrenadeId);
    }
    return grenade;
}

void CGrenadeManager::SetActiveGrenade(int id)
{
    m_activeGrenadeId = id;
}

CItem* CGrenadeManager::getItemByInternalId(int index)
{
    CItem* item = NULL;
    if (index < m_items.size()) {
        item = m_items[index];
    }
    return item;
}

CShopItemButton* CGrenadeManager::CreateShopItemButton(int id)
{
    return NULL;
}

CGrenadeManager::~CGrenadeManager()
{
    for (int index = 0; index < m_items.size(); index++) {
        if (m_items[index] != NULL) {
            delete m_items[index];
            m_items[index] = NULL;
        }
    }
}
