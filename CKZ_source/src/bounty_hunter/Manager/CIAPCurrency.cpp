// CIAPCurrency.cpp - a currency pack of the shop bought with an in-app purchase.
#include "CIAPCurrency.h"
#include "AdBannerSizes.h"
#include <wchar.h>
#include "CStrWChar.h"
#include "ICInAppPurchase.h"
#include "App.h"
#include "ResMap.h"
#include "MathLib.h"
#include "tinyxml.h"
#include "CXmlHelper.h"
#include "CFontMgr.h"
#include "CDH_ResourceManager.h"
#include "CBH_Player.h"
#include "CIAPCurrencyManager.h"
#include "COffersManager.h"
#include "CWaitPurchaseWindow.h"
#include "CGameAnalytics.h"

using namespace com::glu::platform::components;

CIAPCurrency::CIAPCurrency(TiXmlNode* node)
{
    m_price = 0.0f;
    m_shopImage = NULL;
    m_field8c = 0;
    m_available = false;
    m_hiddenInShop = false;
    m_fromOneTimeOffer = false;
    m_id = CXmlHelper::GetAttributeValue(node, "id").ToInt();
    parseNode(node);
    setAvailable(false);
}

void CIAPCurrency::parseNode(TiXmlNode* node)
{
    m_type = CXmlHelper::GetAttributeValue(node->ToElement(), "type");
    parseTexts(node->FirstChild("texts")->ToElement());
    parseStats(node->FirstChild("stats")->ToElement());
    parseGraphics(node->FirstChild("graphics")->ToElement());
}

void CIAPCurrency::parseTexts(TiXmlElement* element)
{
    m_name = Window::ResStringSafe(CXmlHelper::GetAttributeValue(element, "name"));
    m_description = Window::ResStringSafe(CXmlHelper::GetAttributeValue(element, "desc"));
    m_appleId = CXmlHelper::GetAttributeValue(element, "appleid");
}

void CIAPCurrency::parseGraphics(TiXmlElement* element)
{
    XString buttonName = CXmlHelper::GetAttributeValue(element, "shop_button");
    m_shopButton = ((App*)WindowApp::m_instance)->m_resMap->getResourceID(buttonName);
    m_field88 = 0;
}

void CIAPCurrency::parseStats(TiXmlElement* element)
{
    m_creditsGain = CXmlHelper::GetAttributeValue(element, "credits_gain").ToInt();
    m_cashGain = CXmlHelper::GetAttributeValue(element, "cash_gain").ToInt();
    m_badgeText = CXmlHelper::GetAttributeValueOrUseDefault(element, "badge_text", XString(L""));
    m_hiddenInShop = CXmlHelper::GetAttributeValueOrUseDefault(element, "hidden", "0").ToBool();
    ParseOverridable(element, true);
}

void CIAPCurrency::loadResources()
{
    if (m_shopImage != NULL) {
        return;
    }
    m_shopImage = ((App*)WindowApp::m_instance)->m_resourceManager->getResourceData(m_shopButton, false);
}

void CIAPCurrency::releaseResources()
{
    if (m_shopImage != NULL) {
        ((App*)WindowApp::m_instance)->m_resourceManager->releaseResource(m_shopButton);
    }
    m_shopImage = NULL;
}

com::glu::platform::graphics::ICRenderSurface* CIAPCurrency::GetShopImage()
{
    if (m_shopImage == NULL) {
        loadResources();
    }
    return (com::glu::platform::graphics::ICRenderSurface*)m_shopImage;
}

CIAPCurrency::~CIAPCurrency()
{
    releaseResources();
}

void CIAPCurrency::setAvailable(bool available)
{
    m_available = available;
}

int CIAPCurrency::Buy()
{
    // b147: original lays out the available branch first (cmp #1; bne to the refresh path).
    if (isAvailable() == true) {
        ICInAppPurchase* store = ICInAppPurchase::GetInstance();
        CWaitPurchaseWindow::ShowModal();
        XString item = XString::Format(L"%02d_%s", m_id, GetName().c_str());
        if (m_cashGain != 0) {
            CGameAnalytics::logIAPBuyClicked(item, m_cashGain, CGameAnalytics::CURRENCY_SOFT);
        } else if (m_creditsGain != 0) {
            CGameAnalytics::logIAPBuyClicked(item, m_creditsGain, CGameAnalytics::CURRENCY_HARD);
        }
        XString productId = m_appleId;
        store->BuyProduct(CStrWChar(productId.c_str()));
        return 1;
    }
    // k: original 12a72c explicitly sets r1=1 and calls at 12a73c; the real argument is kept even though the current callee ignores force.
    ((App*)WindowApp::m_instance)->m_iapCurrencyManager->UpdateIAPInfo(true);
    return 0;
}

void CIAPCurrency::BuyFromOneTimeOffer()
{
    m_fromOneTimeOffer = true;
    Buy();
}

void CIAPCurrency::OnIAPBought()
{
    int bonus = 0;
    if (m_fromOneTimeOffer) {
        bonus = ((App*)WindowApp::m_instance)->m_offersManager->GetOneTimeOfferBonus();
        m_fromOneTimeOffer = false;
    }
    XString item = XString::Format(L"%02d_iap", m_id);
    if (m_creditsGain > 0) {
        CBH_Player::GetInstance()->AddCredits(m_creditsGain + bonus, item.c_str());
        CGameAnalytics::logCurrencyChange(item, CBH_Player::GetInstance()->GetStats(PLAYER_STATS_HARD_CURRENCY),
                                          m_creditsGain + bonus, CGameAnalytics::CURRENCY_HARD);
    }
    if (m_cashGain > 0) {
        CBH_Player::GetInstance()->AddMoney(m_cashGain + bonus);
        CGameAnalytics::logCurrencyChange(item, CBH_Player::GetInstance()->GetStats(PLAYER_STATS_SOFT_CURRENCY),
                                          m_cashGain + bonus, CGameAnalytics::CURRENCY_SOFT);
    }
}

void CIAPCurrency::FillPriceText(TextWindow* text)
{
    XString price;
    price.Append(m_currencySymbol);
    price.Append(L" ", wcslen(L" ") * sizeof(wchar_t));
    float value = m_price;
    // whole prices without decimals
    if (value - MathLib::Floor(value) > 0.001f) {
        price.Append(XString(value, 2));
    } else {
        price.Append(XString(value, 0));
    }
    text->SetText(price, CFontMgr::GetInstance()->GetFont(0x18));
}
