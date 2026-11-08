// CStatsBonusItem.cpp - encrypted player-stat XML values and configured inventory bonuses.
#include "CStatsBonusItem.h"
#include "CXmlHelper.h"
#include "tinyxml.h"
#include "WindowBase.h"
#include "App.h"
#include "ResMap.h"
#include "CDH_ResourceManager.h"
#include "CBH_Player.h"
#include "CGameAnalytics.h"
#include "CZombieDialogWindow.h"

// Original data @0x4de4f0. Odd string addresses are read without clearing their low bit.
static const char* const PlayerStatsName[] = {
    "energy", "energy_max", "cash", "xp", "credits", "medkits",
    "provision", "provision_capacity", "people", "people_capacity"
};

/** Use a resource string when present; otherwise keep the literal config attribute. */
XString TryLoadStringFromResources(TiXmlNode* node, const char* attribute)
{
    XString value = CXmlHelper::GetAttributeValue(node, attribute);
    XString resource = Window::ResString(XString::AnsiString(value.c_str()));
    if (resource == XString("")) {
        return value;
    }
    return resource;
}

void InitPlayerStats(PlayerStats& stats)
{
    for (int index = 0; index < 10; index++) {
        stats.m_values[index].Set(0);
    }
}

void LoadPlayerStats(PlayerStats& stats, TiXmlNode* node, const XString& prefix)
{
    for (int index = 0; index < 10; index++) {
        stats.m_values[index].Set(CXmlHelper::GetAttributeValueOrUseDefault(
            node, XString::AnsiString((prefix + XString(PlayerStatsName[index])).c_str()), "0").ToInt());
    }
}

void SavePlayerStats(const PlayerStats& stats, TiXmlElement* element, const XString& prefix)
{
    for (int index = 0; index < 10; index++) {
        element->SetAttribute(XString::AnsiString((prefix + XString(PlayerStatsName[index])).c_str()), stats.m_values[index].Get());
    }
}

PlayerStats::PlayerStats()
{
    InitPlayerStats(*this);
}

void PlayerStats::Load(TiXmlNode* node, const XString& prefix)
{
    LoadPlayerStats(*this, node, prefix);
}

CStatsBonusItem::CStatsBonusItem(TiXmlNode* node)
{
    InitPlayerStats(m_stats);
    m_shopImage = NULL;
    m_bigImage = NULL;
    m_fieldD8 = false;
    parseNode(node);
}

void CStatsBonusItem::parseNode(TiXmlNode* node)
{
    m_id = CXmlHelper::GetAttributeValue(node, "id").ToInt();
    parseTexts(node->FirstChild("texts"));
    parseStats(node->FirstChild("stats"));
    parseGraphics(node->FirstChild("graphics"));
}

void CStatsBonusItem::parseTexts(TiXmlNode* node)
{
    m_name = TryLoadStringFromResources(node, "name");
    m_description = TryLoadStringFromResources(node, "desc");
}

void CStatsBonusItem::parseStats(TiXmlNode* node)
{
    m_iapPrice = CXmlHelper::GetAttributeValueOrUseDefault(node, "price_iap", "0").ToInt();
    LoadPlayerStats(m_stats, node, XString(L""));
    ParseOverridable(node, true);
}

void CStatsBonusItem::parseGraphics(TiXmlNode* node)
{
    XString shopImage = CXmlHelper::GetAttributeValue(node, "shop_button");
    m_shopImageResource = ((App*)WindowApp::m_instance)->m_resMap->getResourceID(shopImage);
    XString bigImage = CXmlHelper::GetAttributeValue(node, "big_image");
    m_bigImageResource = ((App*)WindowApp::m_instance)->m_resMap->getResourceID(bigImage);
}

void CStatsBonusItem::loadResources()
{
    m_bigImage = (com::glu::platform::graphics::ICRenderSurface*)((App*)WindowApp::m_instance)->m_resourceManager->getResourceData(m_bigImageResource, false);
    m_shopImage = (com::glu::platform::graphics::ICRenderSurface*)((App*)WindowApp::m_instance)->m_resourceManager->getResourceData(m_shopImageResource, false);
}

void CStatsBonusItem::releaseResources()
{
    ((App*)WindowApp::m_instance)->m_resourceManager->releaseResource(m_bigImageResource);
    ((App*)WindowApp::m_instance)->m_resourceManager->releaseResource(m_shopImageResource);
}

int CStatsBonusItem::Buy()
{
    int result = CItem::Buy();
    if (result == 1) {
        setAvailable(true);
        WindowApp::HandleTunnelCommand(0x4dc05475, m_id, (int)this, 0);
    }
    return result;
}

bool CStatsBonusItem::CheckBuyConditions(CZombieDialogWindow** dialog)
{
    CBH_Player* player = CBH_Player::GetInstance();
    XString message;
    bool allowed = true;
    if (m_stats.m_values[7].Get() != 0) {
        int capacity = player->GetStats((PLAYER_STATS_TYPE)7);
        if (capacity >= m_stats.m_values[7].Get()) {
            allowed = false;
            message.Append(Window::ResString("IDS_PROVISION_STORAGE_CAPACITY_DOWNGRADE") + XString(L"\n"));
        }
    }
    if (m_stats.m_values[9].Get() != 0) {
        int capacity = player->GetStats((PLAYER_STATS_TYPE)9);
        if (capacity >= m_stats.m_values[9].Get()) {
            allowed = false;
            message.Append(Window::ResString("IDS_PEOPLE_STORAGE_CAPACITY_DOWNGRADE") + XString(L"\n"));
        }
    }
    unsigned int command = 0;
    if (m_stats.m_values[6].Get() != 0) {
        int amount = player->GetStats((PLAYER_STATS_TYPE)6);
        int capacity = player->GetStats((PLAYER_STATS_TYPE)7);
        if (amount >= capacity) {
            command = 0x973fb14e;
            allowed = false;
            message.Append(Window::ResString("IDS_NO_PLACE_IN_PROVISION_STORAGE") + XString(L"\n"));
        }
    }
    if (m_stats.m_values[8].Get() != 0) {
        int amount = player->GetStats((PLAYER_STATS_TYPE)8);
        int capacity = player->GetStats((PLAYER_STATS_TYPE)9);
        if (amount >= capacity) {
            command = 0xff982709;
            allowed = false;
            message.Append(Window::ResString("IDS_NO_PLACE_IN_PEOPLE_STORAGE") + XString(L"\n"));
        }
    }
    if (allowed) {
        // Original leaves the caller's output pointer untouched on success.
        return true;
    }
    CZombieDialogWindow* window = new CZombieDialogWindow(3);
    window->SetTitle(Window::ResString("IDS_TITLE_INFO"));
    window->SetText(message, true, NULL);
    window->AddButton((CZombieDialogWindow::EDialogButton)2, command, 0, 0, true, 0x14);
    window->AddButton((CZombieDialogWindow::EDialogButton)3, 0xffffffff, 0, 0, true, 0x14);
    *dialog = window;
    return false;
}

CStatsBonusItem::~CStatsBonusItem()
{
    releaseResources();
}

void CStatsBonusItem::setAvailable(bool available)
{
    if (available == true) {
        CBH_Player* player = CBH_Player::GetInstance();
        int amount = m_stats.m_values[2].Get();
        if (amount != 0) {
            player->AddStats((PLAYER_STATS_TYPE)2, amount, false, true);
            const XString source = XString::Format(L"%02d_currency", m_id);
            CGameAnalytics::logCurrencyChange(source, player->GetStats((PLAYER_STATS_TYPE)2), amount, CGameAnalytics::CURRENCY_SOFT);
        }
        amount = m_stats.m_values[6].Get();
        if (amount != 0) {
            int capacity = player->GetStats((PLAYER_STATS_TYPE)7);
            int current = player->GetStats((PLAYER_STATS_TYPE)6);
            if (capacity >= current + amount) {
                player->AddStats((PLAYER_STATS_TYPE)6, amount, false, true);
            }
        }
        amount = m_stats.m_values[7].Get();
        if (amount != 0) {
            amount -= player->GetStats((PLAYER_STATS_TYPE)7);
            if (amount < 0) {
                amount = 0;
            }
            player->AddStats((PLAYER_STATS_TYPE)7, amount, false, true);
        }
        amount = m_stats.m_values[8].Get();
        if (amount != 0) {
            int capacity = player->GetStats((PLAYER_STATS_TYPE)9);
            int current = player->GetStats((PLAYER_STATS_TYPE)8);
            if (capacity >= current + amount) {
                player->AddStats((PLAYER_STATS_TYPE)8, amount, false, true);
            }
        }
        amount = m_stats.m_values[9].Get();
        if (amount != 0) {
            amount -= player->GetStats((PLAYER_STATS_TYPE)9);
            if (amount < 0) {
                amount = 0;
            }
            player->AddStats((PLAYER_STATS_TYPE)9, amount, false, true);
        }
    }
}
