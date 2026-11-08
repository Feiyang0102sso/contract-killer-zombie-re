// CShopItem.cpp: original 12020c..120d2c, full CItem/CItemManager owner; keeps the repeated lookups and the charge timing.
#include "CShopItem.h"
#include "App.h"
#include "ResMap.h"
#include "CCrc32.h"
#include "tinyxml.h"
#include "CXmlHelper.h"
#include "COverrideManager.h"
#include "CBH_Player.h"
#include "COffersManager.h"
#include "CStdUtil_Cocoa.h"
#include <stdlib.h>

CItemManager::CItemManager()
    : m_overrideVersion(0), m_configCrc(0)
{
    // The original constructor does not write m_iterator; firstItem is responsible for resetting it.
}

bool CItemManager::IsConfigNew(const char* path)
{
    char* text = ((App*)WindowApp::m_instance)->m_resMap->loadText(XString(path));
    unsigned int length = 0;
    while (text[length] != 0) {
        ++length;
    }
    unsigned int crc = com::glu::platform::components::CCrc32::GetInstance()->Crc32((unsigned char*)text, length);
    if (text != NULL) {
        free(text);
    }
    return m_configCrc != crc;
}

void CItemManager::LoadFromConfig(const char* path, const char* rootTag, const char* itemTag)
{
    char* text = ((App*)WindowApp::m_instance)->m_resMap->loadText(XString(path));
    unsigned int length = 0;
    while (text[length] != 0) {
        ++length;
    }
    unsigned int crc = com::glu::platform::components::CCrc32::GetInstance()->Crc32((unsigned char*)text, length);
    TiXmlDocument document;
    document.Parse(text, NULL, TIXML_ENCODING_UNKNOWN);
    if (text != NULL) {
        free(text);
    }
    TiXmlNode* root = &document;
    if (rootTag != NULL) {
        root = document.FirstChild(rootTag);
    }
    TiXmlNode* item = root->FirstChild(itemTag);
    while (item != NULL) {
        AddItem(item);
        item = item->NextSibling(itemTag);
    }
    m_configCrc = crc;
}

CItem* CItemManager::getItemByName(XString name)
{
    CheckOverride();
    CItem* item = firstItem();
    while (item != NULL) {
        if (item->GetName() == name) {
            return item;
        }
        item = nextItem();
    }
    return NULL;
}

void CItemManager::CheckOverride()
{
    COverrideManager* overrides = ((App*)WindowApp::m_instance)->m_overrideManager;
    int version = overrides->m_configCrc;
    if (version == m_overrideVersion) {
        return;
    }
    TiXmlNode* document = &overrides->m_document;
    TiXmlNode* items = NULL;
    if (document != NULL) {
        items = document->FirstChild("items");
    }
    // The original loop looks up 0 first, then still looks up 0 again before incrementing the index; this repeated access is not eliminated.
    int index = 0;
    CItem* item = getItemByInternalId(0);
    while (item != NULL) {
        item->Override(items);
        item = getItemByInternalId(index);
        ++index;
    }
    m_overrideVersion = version;
}

CItem* CItemManager::getItemByStringUId(const XString& id)
{
    CheckOverride();
    CItem* item = firstItem();
    while (item != NULL) {
        if (item->GetStringUId() == id) {
            return item;
        }
        item = nextItem();
    }
    return NULL;
}

XString& CItem::GetStringUId()
{
    if (m_stringUId.ByteSize() == 0) {
        // Original 451fd0 is a UTF32 underscore; the four temporaries are destroyed in reverse order at the end of the full expression.
        m_stringUId = XString(GetItemTypeName()) + L"_" + XString(getId());
    }
    return m_stringUId;
}

CItem* CItemManager::firstItem()
{
    CheckOverride();
    m_iterator = 0;
    return getItemByInternalId(0);
}

CItem* CItemManager::nextItem()
{
    CheckOverride();
    ++m_iterator;
    return getItemByInternalId(m_iterator);
}

CItem* CItemManager::getItemById(int id)
{
    CheckOverride();
    int index = 0;
    CItem* item = getItemByInternalId(0);
    while (item != NULL) {
        if (item->getId() == id) {
            return item;
        }
        item = getItemByInternalId(index);
        ++index;
    }
    return NULL;
}

void CItemManager::loadResources()
{
    CItem* item = firstItem();
    while (item != NULL) {
        item->loadResources();
        item = nextItem();
    }
}

void CItemManager::releaseResources()
{
    CItem* item = firstItem();
    while (item != NULL) {
        item->releaseResources();
        item = nextItem();
    }
}

void CItem::Override(TiXmlNode* node)
{
    if (node == NULL) {
        return;
    }
    TiXmlNode* item;
    {
        XString::AnsiString id(GetStringUId().c_str());
        item = node->FirstChild(id);
    }
    if (item != NULL) {
        ParseOverridable(item, false);
    }
}

CItem::CItem()
    : m_isHot(false), m_isNew(false), m_isSale(false), m_saleStartMS(0), m_saleEndMS(0),
      m_dailyDealPoints(0), m_dailyDealCredits(0), m_dailyDealStartMS(0), m_dailyDealEndMS(0),
      m_isDailyDeal(false)
{
    // The original constructor does not write m_hidden; the original value fields are Set once more after the default Encrypted construction.
    m_origHidden = false;
    m_origPriceCredits.Set(0);
    m_origPricePoints.Set(0);
    m_origIsSale = false;
    m_origIsHot = false;
    m_origIsNew = false;
    m_origSaleStartMS = 0;
    m_origSaleEndMS = 0;
    m_origSalePriceCredits.Set(0);
    m_origSalePricePoints.Set(0);
}

int CItem::Buy()
{
    XString name(GetName());
    if (getPriceCredits() >= 1) {
        int balance = CBH_Player::GetInstance()->GetStats(PLAYER_STATS_HARD_CURRENCY);
        if (balance < getPriceCredits()) {
            return 0;
        }
        CBH_Player* player = CBH_Player::GetInstance();
        int price = getPriceCredits();
        player->ReduceCredits(price, name.c_str());
    }
    if (getPricePoints() >= 1) {
        int balance = CBH_Player::GetInstance()->GetStats(PLAYER_STATS_SOFT_CURRENCY);
        if (balance < getPricePoints()) {
            return 0;
        }
        CBH_Player* player = CBH_Player::GetInstance();
        int price = getPricePoints();
        player->ReduceMoney(price);
    }
    COffersManager* offers = ((App*)WindowApp::m_instance)->m_offersManager;
    offers->m_offersChanged = true;
    offers->m_activeDealIndex = -1;
    return 1;
}

bool CItem::IsLocked() const
{
    int level = getUnlockLvl();
    return level > CBH_Player::GetInstance()->m_id;
}

bool CItem::CheckFilter(int filter)
{
    return true; // Original 1208f8 really always returns 1.
}

bool CItem::IsSaleWithTimer() const
{
    return GetSaleEndMS() != 0;
}

int CItem::GetSaleTimeLeft() const
{
    unsigned int end = GetSaleEndMS();
    return (int)(end - (unsigned int)CStdUtil_Cocoa::GetTimeSeconds());
}

bool CItem::IsOnSale() const
{
    if (IsSale() != true) {
        return false;
    }
    if (GetSaleEndMS() == 0) {
        return true;
    }
    unsigned int end = GetSaleEndMS();
    unsigned int firstTime = CStdUtil_Cocoa::GetTimeSeconds();
    unsigned int secondTime = CStdUtil_Cocoa::GetTimeSeconds();
    unsigned int start = GetSaleStartMS();
    if (secondTime < start) {
        return false;
    }
    return (int)(end - firstTime) > 0;
}

void CItem::ParseOverridable(TiXmlNode* node, bool keepAsOriginal)
{
    m_hidden = CXmlHelper::GetBoolAttributeValueOrUseDefault(node, "hidden", m_origHidden);
    m_priceCredits.Set(CXmlHelper::GetIntAttributeValueOrUseDefault(node, "priceCredits", m_origPriceCredits.Get()));
    m_pricePoints.Set(CXmlHelper::GetIntAttributeValueOrUseDefault(node, "pricePoints", m_origPricePoints.Get()));
    m_isSale = CXmlHelper::GetBoolAttributeValueOrUseDefault(node, "isSale", m_origIsSale);
    m_isHot = CXmlHelper::GetBoolAttributeValueOrUseDefault(node, "isHot", m_origIsHot);
    m_isNew = CXmlHelper::GetBoolAttributeValueOrUseDefault(node, "isNew", m_origIsNew);
    m_saleStartMS = CXmlHelper::GetIntAttributeValueOrUseDefault(node, "saleStartMS", m_origSaleStartMS);
    m_saleEndMS = CXmlHelper::GetIntAttributeValueOrUseDefault(node, "saleEndMS", m_origSaleEndMS);
    m_salePriceCredits.Set(CXmlHelper::GetIntAttributeValueOrUseDefault(node, "salePriceCredits", m_origSalePriceCredits.Get()));
    m_salePricePoints.Set(CXmlHelper::GetIntAttributeValueOrUseDefault(node, "salePricePoints", m_origSalePricePoints.Get()));
    if (keepAsOriginal == true) {
        m_origHidden = m_hidden;
        m_origPriceCredits = m_priceCredits;
        m_origPricePoints = m_pricePoints;
        m_origIsSale = m_isSale;
        m_origIsHot = m_isHot;
        m_origIsNew = m_isNew;
        m_origSaleStartMS = m_saleStartMS;
        m_origSaleEndMS = m_saleEndMS;
        m_origSalePriceCredits = m_salePriceCredits;
        m_origSalePricePoints = m_salePricePoints;
    }
}

int CItem::getPriceCredits() const
{
    if (IsOnSale() == true) {
        if (m_isDailyDeal) {
            return m_dailyDealCredits.Get();
        }
        return m_salePriceCredits.Get();
    }
    return m_priceCredits.Get();
}

int CItem::getPricePoints() const
{
    if (IsOnSale() == true) {
        if (m_isDailyDeal) {
            return m_dailyDealPoints.Get();
        }
        return m_salePricePoints.Get();
    }
    return m_pricePoints.Get();
}

void CItem::SetDailyDealParams(int discount, unsigned int startMS, unsigned int endMS)
{
    int points = m_pricePoints.Get();
    if (points > 0) {
        points = discount;
    }
    m_dailyDealPoints.Set(points);
    int credits = m_priceCredits.Get();
    if (credits > 0) {
        credits = discount;
    }
    m_dailyDealCredits.Set(credits);
    m_dailyDealStartMS = startMS;
    m_dailyDealEndMS = endMS;
    m_isDailyDeal = startMS != 0 && endMS != 0;
}

bool CItem::IsSale() const
{
    return m_isSale || m_isDailyDeal;
}

int CItem::GetSaleStartMS() const
{
    if (m_isDailyDeal) {
        return m_dailyDealStartMS;
    }
    return m_saleStartMS;
}

int CItem::GetSaleEndMS() const
{
    if (m_isDailyDeal) {
        return m_dailyDealEndMS;
    }
    return m_saleEndMS;
}

int CItem::GetOriginalPriceCredits() const
{
    return m_priceCredits.Get();
}

int CItem::GetOriginalPricePoints() const
{
    return m_pricePoints.Get();
}
