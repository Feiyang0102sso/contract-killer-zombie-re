// CEnemyWeaponType.cpp - one enemy weapon read from an <enemyWeapon> node of enemy_weapons.xml.
#include "CEnemyWeaponType.h"
#include "App.h"
#include "ResMap.h"
#include "MathLib.h"
#include "CXmlHelper.h"
#include "DGHelper.h"
#include "SwerveHelper.h"
#include "CDH_SoundManager.h"

CEnemyWeaponType::CEnemyWeaponType(TiXmlNode* node)
    : m_baseDamage(0), m_modelCopies(0)
{
    m_model.Clear();
    Parse(node);
}

void CEnemyWeaponType::Parse(TiXmlNode* node)
{
    m_name = CXmlHelper::GetAttributeValue(node, "name");
    ParseInfo(node->FirstChild("info"));
    ParseModel(node->FirstChild("model"));
    ParseSounds(node->FirstChild("sounds"));
}

CEnemyWeaponType::~CEnemyWeaponType()
{
    m_model.Clear();
}

swerve::Group<CSwerveAssertingErrorHandler> CEnemyWeaponType::GetModelCopy() const
{
    if (m_model == NULL) {
        LoadModel();
    }
    m_modelCopies++;
    return SwerveHelper::FullObjectDuplicate(swerve::Group<CSwerveAssertingErrorHandler>(m_model), 0);
}

void CEnemyWeaponType::LoadModel() const
{
    swerve::Object3D<CSwerveAssertingErrorHandler> root = App::LoadObject3D(m_modelResource);
    if (root == NULL) {
        return;
    }
    {
        XString::AnsiString dummyName(m_dummyName.c_str());
        m_model = root.find(DGHelper::getSwerveID(dummyName));
    }
    if (m_model != NULL) {
        DGHelper::SetUserIDRecursively(m_model, GetWeaponHitZoneUserId());
    }
}

void CEnemyWeaponType::ReleaseModelCopy() const
{
    m_modelCopies--;
    if (m_modelCopies != 0) {
        return;
    }
    m_model.Clear();
}

void CEnemyWeaponType::ParseInfo(TiXmlNode* node)
{
    if (node == NULL) {
        return;
    }
    m_baseDamage = CXmlHelper::GetAttributeValue(node, "base_damage").ToInt();

    Vector<XString> animNames;
    XString animText = CXmlHelper::GetAttributeValue(node, "characterAnimationTypes");
    DGHelper::SplitStringAndTrim(animNames, animText, XString(","));
    for (int index = 0; index < animNames.size(); index++) {
        EPersonAnimType animType = (EPersonAnimType)animNames[index].ToInt();
        m_animTypes.addElement(animType);
    }

    m_dummyName = CXmlHelper::GetAttributeValue(node, "dummy");
    m_effectiveRange = CXmlHelper::GetAttributeValue(node, "effectiveRange").ToInt();
    m_hasTracer = CXmlHelper::GetAttributeValueOrUseDefault(node, "hasTracer", "1").ToBool();
    m_isMelee = CXmlHelper::GetAttributeValueOrUseDefault(node, "isMelee", "0").ToBool();
    m_attackDelayMS = CXmlHelper::GetAttributeValueOrUseDefault(node, "attackDelayMS", "0").ToInt();
    m_hp = CXmlHelper::GetAttributeValueOrUseDefault(node, "hp", "0").ToInt();
    if (m_hp < 1) {
        m_isDestructible = false;
        m_hp = 1;
    } else {
        m_isDestructible = true;
    }
}

void CEnemyWeaponType::ParseModel(TiXmlNode* node)
{
    if (node == NULL) {
        return;
    }
    XString modelName = CXmlHelper::GetAttributeValue(node, "m3g");
    m_modelResource = ((App*)WindowApp::m_instance)->m_resMap->getResourceID(modelName);
    m_lightNode = CXmlHelper::GetAttributeValueOrUseDefault(node, "lightNode", "light_grad");
}

void CEnemyWeaponType::ParseSounds(TiXmlNode* node)
{
    if (node == NULL) {
        return;
    }
    XString shootName = CXmlHelper::GetAttributeValue(node, "shoot");
    const char* resourceId = ((App*)WindowApp::m_instance)->m_resMap->getResourceID(shootName);
    m_shootSound = CDH_SoundManager::GetSoundFromResourceID(resourceId);
}

int CEnemyWeaponType::GetWeaponHitZoneUserId()
{
    static int s_hitZoneUserId = DGHelper::getSwerveID("WeaponHitZone");
    return s_hitZoneUserId;
}

EPersonAnimType CEnemyWeaponType::GetCharacterAnimationType() const
{
    RandomValue* random = (RandomValue*)&WindowApp::m_instance->m_randomSeed;   // WindowApp 0x1c4 is a RandomValue
    return m_animTypes[random->Int(m_animTypes.size())];
}
