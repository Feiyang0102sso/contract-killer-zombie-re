// CPowerUpManager.cpp - power-up inventory, persistence and running timer.
#include "CPowerUpManager.h"
#include "CPowerUp.h"
#include "CXmlHelper.h"
#include "tinyxml.h"
#include "App.h"
#include "CDH_SoundManager.h"

CPowerUpManager::CPowerUpManager() : m_activeId(-1), m_timeLeft(0)
{
    LoadFromConfig("res/common/configs/powerups.xml", "powerups", "powerup");
}

void CPowerUpManager::Save(TiXmlElement* parent)
{
    TiXmlElement* powerups = CXmlHelper::ObtainElement(parent, XString("powerups"));
    powerups->Clear();
    powerups->SetAttribute("active_pid", m_activeId);
    for (int index = 0; index < m_items.size(); index++) {
        CPowerUp* item = m_items[index];
        if (item->GetCount() > 0) {
            TiXmlElement* element = new TiXmlElement("p");
            element->SetAttribute("id", item->getId());
            element->SetAttribute("c", item->GetCount());
            powerups->InsertEndChild(*element);
            delete element;
        }
    }
}

void CPowerUpManager::Load(TiXmlElement* parent)
{
    TiXmlElement* powerups = parent->FirstChildElement("powerups");
    if (powerups == NULL) {
        return;
    }
    m_activeId = CXmlHelper::GetAttributeValue(powerups, "active_pid").ToInt();
    for (TiXmlElement* element = powerups->FirstChildElement("p"); element != NULL;
         element = element->NextSiblingElement("p")) {
        CPowerUp* item;
        // Original @0x1300b0 retries the same node forever when its id is unknown.
        do {
            int id = CXmlHelper::GetAttributeValue(element, "id").ToInt();
            item = static_cast<CPowerUp*>(getItemById(id));
        } while (item == NULL);
        int count = CXmlHelper::GetAttributeValue(element, "c").ToInt();
        item->m_count.Set(count);
    }
}

void CPowerUpManager::AddItem(TiXmlNode* node)
{
    CPowerUp* item = new CPowerUp(node);
    m_items.addElement(item);
}

CPowerUp* CPowerUpManager::GetActivePowerUp()
{
    if (m_activeId >= 0) {
        return static_cast<CPowerUp*>(getItemById(m_activeId));
    }
    return NULL;
}

void CPowerUpManager::SetActivePowerUp(int id)
{
    m_activeId = id;
}

CItem* CPowerUpManager::getItemByInternalId(int index)
{
    if (index < m_items.size()) {
        return m_items[index];
    }
    return NULL;
}

CShopItemButton* CPowerUpManager::CreateShopItemButton(int index)
{
    // Original @0x1301f4 returns NULL; these items have no direct shop button.
    return NULL;
}

void CPowerUpManager::Update(int elapsedMs)
{
    if (m_timeLeft > 0) {
        m_timeLeft -= elapsedMs;
        if (m_timeLeft <= 0) {
            FinishRunningPowerUp();
        }
    }
}

void CPowerUpManager::FinishRunningPowerUp()
{
    m_timeLeft = 0;
    CPowerUp* item = GetActivePowerUp();
    if (item != NULL) {
        item->StopVisualEffect();
    }
}

bool CPowerUpManager::IsPowerUpAvailable()
{
    return GetActivePowerUp() != NULL && m_timeLeft <= 0 && GetActivePowerUp()->GetCount() > 0;
}

bool CPowerUpManager::IsPowerUpRunning()
{
    return GetActivePowerUp() != NULL && m_timeLeft > 0;
}

float CPowerUpManager::GetPowerUpLeftPercent()
{
    CPowerUp* item = GetActivePowerUp();
    if (item == NULL || m_timeLeft <= 0) {
        return 0.0f;
    }
    return (float)m_timeLeft / (float)item->m_effectTime;
}

void CPowerUpManager::RunPowerUp()
{
    CPowerUp* item = GetActivePowerUp();
    if (item == NULL || IsPowerUpAvailable() != true || item->GetCount() <= 0) {
        return;
    }
    m_timeLeft = item->m_effectTime;
    item->m_count.Set(item->m_count.Get() - 1);
    item->RunVisualEffect();
    switch (m_activeId) {
    case 0:
        ((App*)WindowApp::m_instance)->m_soundManager->Play((CDH_SoundManager::eSound)0x7d, false, false);
        break;
    case 1:
        ((App*)WindowApp::m_instance)->m_soundManager->Play((CDH_SoundManager::eSound)0x7e, false, false);
        break;
    case 2:
        ((App*)WindowApp::m_instance)->m_soundManager->Play((CDH_SoundManager::eSound)0x7f, false, false);
        break;
    }
}

CPowerUpManager::~CPowerUpManager()
{
    for (int index = 0; index < m_items.size(); index++) {
        if (m_items[index] != NULL) {
            delete m_items[index];
            m_items[index] = NULL;
        }
    }
}

