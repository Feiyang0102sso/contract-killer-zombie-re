// CGrenade.cpp - a grenade type read from a <grenade> node of grenades.xml.
#include "CGrenade.h"
#include "App.h"
#include "ResMap.h"
#include "CXmlHelper.h"
#include "CBH_SurfaceManager.h"

/** `value` limited to [0, 1]. */
static inline float ClampToUnit(float value)
{
    if (value < 0.0f) {
        return 0.0f;
    }
    // `?:` with the constant first: the original uses vmin.f32 here
    return (1.0f < value) ? 1.0f : value;
}

/** `value`, or 0 when negative. */
static inline float ClampToPositive(float value)
{
    // `?:` with the constant first: the original uses vmax.f32 here
    return (0.0f > value) ? 0.0f : value;
}

CGrenade::CGrenade(TiXmlNode* node)
    : m_count(0), m_resourcesLoaded(false), m_shopSurface(NULL)
{
    m_id = CXmlHelper::GetAttributeValue(node, "id").ToInt();
    XString modelName = CXmlHelper::GetAttributeValue(node, "model");
    m_model = ((App*)WindowApp::m_instance)->m_resMap->getResourceID(modelName);
    m_damageCenter = CXmlHelper::GetAttributeValue(node, "damage_center").ToInt();
    m_damagePeripheral = CXmlHelper::GetAttributeValue(node, "damage_periferal").ToInt();
    m_explosionRadius = CXmlHelper::GetAttributeValue(node, "explo_radius").ToInt();
    m_delayAfterContact = CXmlHelper::GetAttributeValue(node, "delay_after_contact").ToInt();
    m_isBeacon = CXmlHelper::GetAttributeValue(node, "is_beacon").ToBool();
    m_throwTime = CXmlHelper::GetAttributeValue(node, "cooldown").ToInt();
    m_bounce = CXmlHelper::GetAttributeValue(node, "bounce").ToFloat();
    m_bounce = ClampToUnit(m_bounce);
    m_friction = CXmlHelper::GetAttributeValue(node, "friction").ToFloat();
    m_friction = ClampToUnit(m_friction);
    m_drag = CXmlHelper::GetAttributeValue(node, "drag").ToFloat();
    m_drag = ClampToUnit(m_drag);
    m_shardBounce = CXmlHelper::GetAttributeValue(node, "shard_bounce").ToFloat();
    m_shardBounce = ClampToUnit(m_shardBounce);
    m_shardFriction = CXmlHelper::GetAttributeValue(node, "shard_friction").ToFloat();
    m_shardFriction = ClampToUnit(m_shardFriction);
    m_shardDrag = CXmlHelper::GetAttributeValue(node, "shard_drag").ToFloat();
    m_shardDrag = ClampToUnit(m_shardDrag);
    m_shardCount = CXmlHelper::GetAttributeValue(node, "shard_count").ToFloat();
    m_shardCount = ClampToPositive(m_shardCount);
    m_shardSpeed = CXmlHelper::GetAttributeValue(node, "shard_speed").ToFloat();
    m_shardSpeed = ClampToPositive(m_shardSpeed);
    m_shardMulti = CXmlHelper::GetAttributeValue(node, "shard_multi").ToFloat();
    m_shardMulti = ClampToPositive(m_shardMulti);
    m_shardAngle = CXmlHelper::GetAttributeValue(node, "shard_angle").ToFloat();
    m_shardAngle = ClampToPositive(m_shardAngle);
    m_damageDescription = CXmlHelper::GetAttributeValue(node, "damage_desc");
    m_radiusDescription = CXmlHelper::GetAttributeValue(node, "radius_desc");
    m_delayDescription = CXmlHelper::GetAttributeValue(node, "delay_desc");
    m_name = Window::ResStringSafe(CXmlHelper::GetAttributeValue(node, "name"));
    m_description = Window::ResStringSafe(CXmlHelper::GetAttributeValue(node, "desc"));
    XString shopImageName = CXmlHelper::GetAttributeValue(node, "shopImage");
    m_shopImage = ((App*)WindowApp::m_instance)->m_resMap->getResourceID(shopImageName);
    XString hudImageName = CXmlHelper::GetAttributeValue(node, "hudImage");
    m_hudImage = ((App*)WindowApp::m_instance)->m_resMap->getResourceID(hudImageName);
    ParseOverridable(node, true);
}

void CGrenade::loadResources()
{
    if (m_resourcesLoaded) {
        return;
    }
    m_shopSurface = ((App*)WindowApp::m_instance)->m_surfaceManager->CreateSurface(m_shopImage, true, false);
    m_resourcesLoaded = true;
}

void CGrenade::releaseResources()
{
    if (!m_resourcesLoaded) {
        return;
    }
    ((App*)WindowApp::m_instance)->m_surfaceManager->ReleaseSurface(m_shopSurface);
    m_resourcesLoaded = false;
}

com::glu::platform::graphics::ICRenderSurface* CGrenade::GetShopImage()
{
    if (!m_resourcesLoaded) {
        loadResources();
    }
    return m_shopSurface;
}

int CGrenade::Buy()
{
    int result = CItem::Buy();
    if (result == 1) {
        setAvailable(true);
    }
    return result;
}

bool CGrenade::CheckBuyConditions(CZombieDialogWindow** dialog)
{
    return true;
}

CGrenade::~CGrenade()
{
    releaseResources();
}

void CGrenade::setAvailable(bool available)
{
}
