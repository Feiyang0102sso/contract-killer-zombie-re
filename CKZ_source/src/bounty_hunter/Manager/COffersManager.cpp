// COffersManager.cpp - configured one-time offers and rotating daily deals.
#include "COffersManager.h"
#include "CXmlHelper.h"
#include "App.h"
#include "ResMap.h"
#include "CIAPCurrencyManager.h"
#include "CShopItem.h"
#include "CBH_Player.h"
#include "CGameAnalytics.h"
#include "CStdUtil_Cocoa.h"
#include "MathLib.h"
#include "DGHelper.h"
#include "CGPSMapScreen.h"
#include "CGPSMapGame.h"
#include "COneTimeOfferDialogWindow.h"
#include "CDailyDealDialogWindow.h"

COffersManager::COffersManager() : m_lastOfferTime(0), m_showCount(0), m_probabilityRandom(-1.0f),
    m_activeDealIndex(-1), m_nextPurchaseTime(0), m_lastDialogTime(0), m_activeDialog(NULL)
{
    ReloadConfig();
}

void COffersManager::ReloadConfig()
{
    ResMap* resources = ((App*)WindowApp::m_instance)->m_resMap;
    char* text = resources->loadText(XString(L"res/common/configs/offers.xml"));
    TiXmlDocument document;
    document.Parse(text);
    delete[] text;
    ParseOTO(document.FirstChild("OneTimeOffer"));
    ParseDD(document.FirstChild("DailyDeal"));
}

void COffersManager::ParseOTO(TiXmlNode* node)
{
    if (node == NULL) {
        return;
    }
    TiXmlNode* config = node->FirstChild("config");
    m_maxShows = CXmlHelper::GetAttributeValueOrUseDefault(config, "MaxShows", "5").ToInt();
    m_showDelay = CXmlHelper::GetAttributeValueOrUseDefault(config, "ShowDelay", "900").ToInt();
    m_startingLevel = CXmlHelper::GetAttributeValueOrUseDefault(config, "StartingLevel", "7").ToInt();
    m_showProbability = CXmlHelper::GetAttributeValueOrUseDefault(config, "ShowProbability", "0.5").ToFloat();
    m_goldPackId = CXmlHelper::GetAttributeValueOrUseDefault(config, "GoldPackId", "12").ToInt();
    m_bonus = CXmlHelper::GetAttributeValueOrUseDefault(config, "Bonus", "0").ToInt();
    m_text = CXmlHelper::GetAttributeValue(config, "Description");
    m_configVersion = CXmlHelper::GetAttributeValueOrUseDefault(config, "ConfigVersion", "0").ToInt();
}

void COffersManager::ParseDD(TiXmlNode* node)
{
    if (node == NULL) {
        return;
    }
    TiXmlNode* config = node->FirstChild("config");
    m_dealDuration = CXmlHelper::GetAttributeValueOrUseDefault(config, "DurationTime", XString(1800)).ToInt();
    m_dialogShowDelay = CXmlHelper::GetAttributeValueOrUseDefault(config, "DialogShowDelay", XString(1800)).ToInt();
    m_dealStartingLevel = CXmlHelper::GetAttributeValueOrUseDefault(config, "StartingLevel", XString(4)).ToInt();
    TiXmlNode* data = node->FirstChild("data");
    if (data == NULL) {
        m_dailyDeals.setSize(0);
        return;
    }
    ClearOffers();
    for (TiXmlNode* child = data->FirstChild("item"); child != NULL;) {
        XString itemUid;
        XString discount;
        int id = CXmlHelper::GetAttributeValue(child, "ID").ToInt();
        itemUid = CXmlHelper::GetAttributeValue(child, "ItemUID");
        int start = CXmlHelper::GetAttributeValue(child, "StartTime").ToInt();
        int end = CXmlHelper::GetAttributeValue(child, "EndTime").ToInt();
        int newPrice = CXmlHelper::GetAttributeValue(child, "NewPrice").ToInt();
        discount = CXmlHelper::GetAttributeValue(child, "Discount");
        m_dailyDeals.addElement(SDailyDealOffer(id, itemUid, start, end, newPrice, discount));
        child = child->NextSibling(); // l: full TinyXML interface; keeps the original next read without name filtering.
    }
    m_offersChanged = true;
    m_activeDealIndex = -1;
}

void COffersManager::InitiateOTOProcess()
{
    float value = m_probabilityRandom;
    if (value == -1.0f) {
        value = ((RandomValue*)&WindowApp::m_instance->m_randomSeed)->Float();
    }
    m_probabilityRandom = value;
}

void COffersManager::ClearOffers()
{
    if (m_dailyDeals.size() != 0) {
        for (int index = 0; index < m_dailyDeals.size(); ++index) {
            CItem* item = (CItem*)App::GetItemByStringUId(m_dailyDeals[index].m_itemUid);
            if (item != NULL) {
                item->SetDailyDealParams(0, 0, 0);
            }
        }
        m_dailyDeals.removeAllElements();
    }
}

void COffersManager::Load(TiXmlElement* root)
{
    if (root == NULL) {
        return;
    }
    InitiateOTOProcess();
    TiXmlNode* offer = root->FirstChildElement("OneTimeOffer");
    if (offer != NULL) {
        m_lastOfferTime = CXmlHelper::GetAttributeValueOrUseDefault(offer, "LastOneTimeOfferTimestamp", "0").ToInt();
        m_showCount = CXmlHelper::GetAttributeValueOrUseDefault(offer, "ShowCount", "0").ToInt();
        int probability = CXmlHelper::GetAttributeValueOrUseDefault(offer, "ProbabilityRandom", "0").ToInt();
        if (probability == 0) {
            probability = m_probabilityRandomBits;
        }
        m_probabilityRandomBits = probability;
        int version = CXmlHelper::GetAttributeValueOrUseDefault(offer, "LastActiveVersion", "-1").ToInt();
        if (version != m_configVersion) {
            m_lastOfferTime = 0;
            m_showCount = 0;
        }
    }
    TiXmlNode* daily = root->FirstChildElement("DailyDeal");
    if (daily != NULL) {
        m_nextPurchaseTime = CXmlHelper::GetAttributeValueOrUseDefault(daily, "nextDDPurchaseTime", "0").ToInt();
    }
    // The original does not restore the purchased-offer children written by Save.
}

void COffersManager::Save(TiXmlElement* root)
{
    if (root == NULL) {
        return;
    }
    TiXmlElement* offer = CXmlHelper::ObtainElement(root, XString("OneTimeOffer"));
    offer->Clear();
    offer->SetAttribute("LastOneTimeOfferTimestamp", m_lastOfferTime);
    offer->SetAttribute("ShowCount", m_showCount);
    offer->SetAttribute("ProbabilityRandom", m_probabilityRandomBits);
    offer->SetAttribute("LastActiveVersion", m_configVersion);
    TiXmlElement* daily = CXmlHelper::ObtainElement(root, XString("DailyDeal"));
    daily->Clear();
    daily->SetAttribute("nextDDPurchaseTime", m_nextPurchaseTime);
    for (int index = 0; index < m_dailyDeals.size(); ++index) {
        if (m_dailyDeals[index].m_wasPurchased) {
            TiXmlElement* child;
            {
                TiXmlElement element("Offer");
                child = daily->InsertEndChild(element)->ToElement();
            }
            child->SetAttribute("OfferID", XString::AnsiString(m_dailyDeals[index].GetID().c_str()));
            child->SetAttribute("WasPurchased", 1);
        }
    }
}

bool COffersManager::ShouldBeShown()
{
    CItem* item = ((App*)WindowApp::m_instance)->m_iapCurrencyManager->getItemById(m_goldPackId);
    if (item != NULL && item->isAvailable() == true &&
        CBH_Player::GetInstance()->GetLevel() >= m_startingLevel &&
        m_showDelay < (unsigned int)(CStdUtil_Cocoa::GetTimeSeconds() - m_lastOfferTime) &&
        m_showCount < m_maxShows) {
        ++m_showCount;
        m_lastOfferTime = CStdUtil_Cocoa::GetTimeSeconds();
        if (1.0f - m_probabilityRandom < m_showProbability) {
            return true;
        }
        CGameAnalytics::logOneTimeOfferResult(item->GetName(),
            (int)(m_probabilityRandom * 1000000000.0f), (CGameAnalytics::EOTOResult)2);
    }
    return false;
}

COneTimeOfferDialogWindow* COffersManager::ShowOTODialog()
{
    CItem* item = ((App*)WindowApp::m_instance)->m_iapCurrencyManager->getItemById(m_goldPackId);
    if (item != NULL && item->isAvailable() == true) {
        return new COneTimeOfferDialogWindow(m_goldPackId);
    }
    return NULL;
}

SDailyDealOffer* COffersManager::GetActiveDDOffer()
{
    if (m_dailyDeals.size() > 0 && m_values.size() > 0 && m_activeDealIndex != -1) {
        int index = m_values[m_activeDealIndex];
        if (index >= 0 && index < m_dailyDeals.size()) {
            return &m_dailyDeals[index];
        }
    }
    return NULL;
}

void COffersManager::UpdateOffers()
{
    if (m_dailyDeals.size() <= 0) {
        return;
    }
    unsigned int now = CStdUtil_Cocoa::GetTimeSeconds();
    m_values.removeAllElements();
    m_values.ensureCapacity(m_dailyDeals.size());
    for (int index = 0; index < m_dailyDeals.size(); ++index) {
        CItem* item = (CItem*)App::GetItemByStringUId(m_dailyDeals[index].m_itemUid);
        bool valid = m_dailyDeals[index].IsValid();
        if (item == NULL || !valid) {
            int removedIndex = index;
            --index;
            m_dailyDeals.removeElementAt(removedIndex);
        } else if (item->isAvailable() == true) {
            m_dailyDeals[index].m_wasPurchased = true;
        } else if (m_dailyDeals[index].m_startTime < now && now < m_dailyDeals[index].m_endTime && !m_dailyDeals[index].m_wasPurchased &&
                   !item->IsLocked() && !item->isHidden()) {
            m_values.addElement(index);
        }
    }
    if (CBH_Player::GetInstance()->GetLevel() >= m_dealStartingLevel && m_values.size() > 0) {
        unsigned int duration = m_dealDuration;
        unsigned int remaining = now % duration;
        m_activeDealIndex = (now / m_dealDuration) % m_values.size();
        unsigned int boundary = now + duration - remaining;
        // UpdateOffers checks both index levels; GetActiveDDOffer does not check the first.
        if (m_activeDealIndex >= 0 && m_activeDealIndex < m_values.size()) {
            int index = m_values[m_activeDealIndex];
            if (index >= 0 && index < m_dailyDeals.size()) {
                SDailyDealOffer& offer = m_dailyDeals[index];
                CItem* item = (CItem*)App::GetItemByStringUId(offer.m_itemUid);
                if (item != NULL) {
                    item->SetDailyDealParams(offer.m_newPrice, now, boundary);
                }
            }
        }
    } else {
        m_activeDealIndex = -1;
    }
}

void COffersManager::CheckAndShowDD()
{
    App* app = (App*)WindowApp::m_instance;
    if (app->m_helper->m_gpsScreen != NULL && app->m_helper->m_gpsScreen->m_mapGame != NULL) {
        SDailyDealOffer* offer = GetActiveDDOffer();
        if (offer != NULL && App::GetItemByStringUId(offer->m_itemUid) != NULL) {
            m_activeDialog = new CDailyDealDialogWindow(offer);
            ((App*)WindowApp::m_instance)->m_helper->m_gpsScreen->m_mapGame->AddDialogInQueue(m_activeDialog);
        }
    }
}

void COffersManager::Tick(unsigned int now)
{
    if (m_offersChanged) {
        UpdateOffers();
        m_offersChanged = false;
    }
    if (CBH_Player::GetInstance()->GetLevel() >= m_dealStartingLevel &&
        now - m_lastDialogTime >= m_dialogShowDelay) {
        if (m_activeDialog != NULL) {
            return;
        }
        if (CBH_Player::GetInstance()->GetTutorialState() > 8 &&
            (m_nextPurchaseTime == 0 || m_nextPurchaseTime <= now)) {
            CheckAndShowDD();
            m_nextPurchaseTime = 0;
            m_lastDialogTime = now;
        }
    }
}

void COffersManager::DialogClosed()
{
    m_activeDialog = NULL;
    unsigned int now = CStdUtil_Cocoa::GetTimeSeconds();
    m_lastDialogTime = now;
    m_nextPurchaseTime = now + m_dialogShowDelay;
}

void COffersManager::OnDDPurchased(SDailyDealOffer* offer)
{
    // The original ignores the argument and only schedules the next rotation.
    unsigned int now = CStdUtil_Cocoa::GetTimeSeconds();
    m_nextPurchaseTime = now + m_dealDuration - now % m_dealDuration;
}
