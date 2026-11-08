// CPowerUp.cpp - configured shop power ups and their visual effects.
#include "CPowerUp.h"
#include "CXmlHelper.h"
#include "WindowBase.h"
#include "App.h"
#include "ResMap.h"
#include "CBH_SurfaceManager.h"
#include "DGHelper.h"
#include "CSwerveGame.h"
#include "CGameplayHUD.h"

CPowerUp::CPowerUp(TiXmlNode* node)
    : m_resourcesLoaded(false), m_shopImage(NULL), m_count(0)
{
    m_id = CXmlHelper::GetAttributeValue(node, "id").ToInt();
    m_name = Window::ResStringSafe(CXmlHelper::GetAttributeValue(node, "name"));
    m_description = Window::ResStringSafe(CXmlHelper::GetAttributeValue(node, "desc"));
    XString shopImage = CXmlHelper::GetAttributeValue(node, "shopImage");
    m_shopImageResource = ((App*)WindowApp::m_instance)->m_resMap->getResourceID(shopImage);
    XString hudImage = CXmlHelper::GetAttributeValue(node, "hudImage");
    m_hudImageResource = ((App*)WindowApp::m_instance)->m_resMap->getResourceID(hudImage);
    m_effectTime = CXmlHelper::GetAttributeValue(node, "effect_time").ToInt();
    m_damageMultiplier = CXmlHelper::GetAttributeValue(node, "damage_mult").ToFloat();
    m_reloadSpeed = CXmlHelper::GetAttributeValue(node, "reload_speed_mult").ToFloat();
    m_fireSpeed = CXmlHelper::GetAttributeValue(node, "fire_speed_mult").ToFloat();
    m_switchSpeed = CXmlHelper::GetAttributeValue(node, "switch_speed_mult").ToFloat();
    m_reloadSoundSpeed = CXmlHelper::GetAttributeValue(node, "time_slow").ToFloat();
    m_blurEffect = 0;
    XString blurEffect = CXmlHelper::GetAttributeValue(node, "blurEffect");
    if (blurEffect.ByteSize() == 0) {
        m_blurEffect = 0;
    } else if (blurEffect == L"fovBlurIn") {
        m_blurEffect = 1;
    } else if (blurEffect == L"fovBlurOut") {
        m_blurEffect = 2;
    }
    m_effectImage = -1;
    int effectImage = CXmlHelper::GetAttributeValue(node, "effectImageNum").ToInt();
    switch (effectImage) {
    case 1: m_effectImage = 0x18; break;
    case 2: m_effectImage = 0x16; break;
    case 3: m_effectImage = 0x17; break;
    }
    ParseOverridable(node, true);
}

void CPowerUp::loadResources()
{
    if (!m_resourcesLoaded) {
        m_shopImage = ((App*)WindowApp::m_instance)->m_surfaceManager->CreateSurface(m_shopImageResource, true, false);
        m_resourcesLoaded = true;
    }
}

void CPowerUp::releaseResources()
{
    if (m_resourcesLoaded) {
        ((App*)WindowApp::m_instance)->m_surfaceManager->ReleaseSurface(m_shopImage);
        m_resourcesLoaded = false;
    }
}

com::glu::platform::graphics::ICRenderSurface* CPowerUp::GetShopImage()
{
    if (!m_resourcesLoaded) {
        loadResources();
    }
    return m_shopImage;
}

int CPowerUp::Buy()
{
    int result = CItem::Buy();
    if (result == 1) {
        setAvailable(true);
    }
    return result;
}

bool CPowerUp::CheckBuyConditions(CZombieDialogWindow** dialog)
{
    // Original @0x12fe10 returns true without writing the output pointer.
    return true;
}

CPowerUp::~CPowerUp()
{
    releaseResources();
}

void CPowerUp::setAvailable(bool available)
{
    // Original @0x12fe8c is bx lr; availability is not stored for this item type.
}

void CPowerUp::RunVisualEffect()
{
    CSwerveGame* game = ((App*)WindowApp::m_instance)->m_helper->m_swerveGame;
    if (game == NULL) {
        return;
    }
    switch (m_blurEffect) {
    case 1: game->RunFovBlur(1.2f, 2000, false); break;
    case 2: game->RunFovBlur(0.8f, 2000, false); break;
    }
    if (m_effectImage >= 0) {
        game->GetHUD()->StartFlashingEffect(m_effectImage, (int)(800.0f / m_reloadSpeed));
    }
}

void CPowerUp::StopVisualEffect()
{
    CSwerveGame* game = ((App*)WindowApp::m_instance)->m_helper->m_swerveGame;
    if (game != NULL) {
        game->GetHUD()->StopFlashingEffect();
    }
}
