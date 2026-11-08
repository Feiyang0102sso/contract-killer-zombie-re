// CShopPack.cpp - configured bundles, graphics, purchases and inventory counts.
#include "CShopPack.h"
#include "CXmlHelper.h"
#include "App.h"
#include "ResMap.h"
#include "WindowBase.h"
#include "CBH_SurfaceManager.h"
#include "CBH_Player.h"
#include "CGameAnalytics.h"
#include "CGrenadeManager.h"
#include "CPowerUpManager.h"
#include "CPowerUp.h"
#include "BaseDialog.h"

CShopPack::CShopPack(TiXmlNode* node)
    : m_resourcesLoaded(false), m_packImage(NULL), m_shopImage(NULL)
{
    ParseNode(node);
}

void CShopPack::ParseNode(TiXmlNode* node)
{
    m_id = CXmlHelper::GetAttributeValue(node, "id").ToInt();
    m_unlockLevel = CXmlHelper::GetAttributeValue(node, "levelUnlock").ToInt();
    XString shopImage = CXmlHelper::GetAttributeValue(node, "shopImage");
    m_shopImageResource = ((App*)WindowApp::m_instance)->m_resMap->getResourceID(shopImage);
    XString packImage = CXmlHelper::GetAttributeValueOptional(node, "packImage");
    if (packImage.ByteSize() != 0) {
        m_packImageResource = XString(((App*)WindowApp::m_instance)->m_resMap->getResourceID(packImage));
    }
    m_packCount = CXmlHelper::GetAttributeValue(node, "count").ToInt();
    m_name = Window::ResStringSafe(CXmlHelper::GetAttributeValue(node, "name"));
    ParseOverridable(node, true);
}

void CShopPack::loadResources()
{
    if (!m_resourcesLoaded) {
        m_shopImage = ((App*)WindowApp::m_instance)->m_surfaceManager->CreateSurface(m_shopImageResource, true, false);
        if (m_packImageResource.ByteSize() != 0) {
            m_packImage = ((App*)WindowApp::m_instance)->m_surfaceManager->CreateSurface(XString::AnsiString(m_packImageResource.m_data), true, false);
        }
        m_resourcesLoaded = true;
    }
}

void CShopPack::releaseResources()
{
    if (m_resourcesLoaded) {
        ((App*)WindowApp::m_instance)->m_surfaceManager->ReleaseSurface(m_shopImage);
        if (m_packImage != NULL) {
            ((App*)WindowApp::m_instance)->m_surfaceManager->ReleaseSurface(m_packImage);
        }
        m_resourcesLoaded = false;
    }
}

com::glu::platform::graphics::ICRenderSurface* CShopPack::GetShopImage()
{
    if (!m_resourcesLoaded) {
        loadResources();
    }
    return m_shopImage;
}

int CShopPack::Buy()
{
    int result = CItem::Buy();
    if (result != 1) {
        return result;
    }
    XString name = GetName();
    int pricePoints = getPricePoints();
    int priceCredits = getPriceCredits();
    if (pricePoints != 0) {
        // The original reads statistic 2 for both currencies.
        CGameAnalytics::logCurrencyChange(name, CBH_Player::GetInstance()->GetStats(PLAYER_STATS_SOFT_CURRENCY),
                                          -pricePoints, CGameAnalytics::CURRENCY_SOFT);
        CGameAnalytics::logItemPurchased(name, pricePoints, CGameAnalytics::CURRENCY_SOFT);
    } else if (priceCredits != 0) {
        CGameAnalytics::logCurrencyChange(name, CBH_Player::GetInstance()->GetStats(PLAYER_STATS_SOFT_CURRENCY),
                                          -priceCredits, CGameAnalytics::CURRENCY_HARD);
        CGameAnalytics::logItemPurchased(name, priceCredits, CGameAnalytics::CURRENCY_HARD);
    }
    setAvailable(true);
    CBH_Player::GetInstance()->Save(true);
    return result;
}

bool CShopPack::CheckBuyConditions(CZombieDialogWindow** dialog)
{
    // Original @0x13a1c8 returns true without touching the output pointer.
    return true;
}

CShopPack::~CShopPack()
{
    releaseResources();
}

CGrenadePack::CGrenadePack(TiXmlNode* node) : CShopPack(node)
{
    int id = CXmlHelper::GetAttributeValue(node, "id").ToInt();
    m_grenade = static_cast<CGrenade*>(((App*)WindowApp::m_instance)->m_grenadeManager->getItemById(id));
}

void CGrenadePack::setAvailable(bool available)
{
    if (available == true) {
        m_grenade->m_count.Set(m_grenade->GetCount() + m_packCount);
    }
}

bool CGrenadePack::CheckFilter(int filter)
{
    return filter == 0;
}

bool CGrenadePack::IsEquiped() const
{
    return ((App*)WindowApp::m_instance)->m_grenadeManager->GetActiveGrenade() == m_grenade && GetCount() > 0;
}

bool CGrenadePack::IsOwned() const
{
    return m_grenade->GetCount() > 0;
}

ImageWindow* CGrenadePack::CreatePackImage()
{
    ImageWindow* image = new ImageWindow();
    if (!m_resourcesLoaded) {
        loadResources();
    }
    // Original @0x13a368 leaks the allocated window when no pack image exists.
    if (m_packImage == NULL) {
        return NULL;
    }
    image->SetImage(m_packImage);
    return image;
}

CPowerUpPack::CPowerUpPack(TiXmlNode* node) : CShopPack(node)
{
    int id = CXmlHelper::GetAttributeValue(node, "id").ToInt();
    m_powerUp = static_cast<CPowerUp*>(((App*)WindowApp::m_instance)->m_powerUpManager->getItemById(id));
}

void CPowerUpPack::setAvailable(bool available)
{
    if (available == true) {
        m_powerUp->m_count.Set(m_powerUp->GetCount() + m_packCount);
    }
}

bool CPowerUpPack::CheckFilter(int filter)
{
    return filter == 1;
}

bool CPowerUpPack::IsEquiped() const
{
    return ((App*)WindowApp::m_instance)->m_powerUpManager->GetActivePowerUp() == m_powerUp && GetCount() > 0;
}

bool CPowerUpPack::IsOwned() const
{
    return m_powerUp->GetCount() > 0;
}

int CPowerUpPack::GetCount() const
{
    return m_powerUp->GetCount();
}
