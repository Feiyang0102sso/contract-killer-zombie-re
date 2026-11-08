// CBH_PersonType.cpp - one kind of person read from a <person> node of persons.xml.
#include "CBH_PersonType.h"
#include "CApplet.h"
#include "CApp.h"
#include "CHash.h"
#include "CRandGen.h"
#include "MathLib.h"
#include "App.h"
#include "CXmlHelper.h"
#include "DGHelper.h"
#include "CEnemyWeaponTypeManager.h"
#include "CVisualCharacterTypeManager.h"

using namespace com::glu::platform::core;

unsigned int CBH_PersonType::m_globalSeed;

CBH_PersonType::CBH_PersonType(TiXmlNode* node)
{
    m_weapon = NULL;
    m_baseHp = 0;
    m_damageMin = 0;
    m_damageMax = 0;
    m_critProbability = 0.0f;
    m_critMultiplier = 0.0f;
    m_damageCooldown = 0;
    m_attackRange = 1.0f;
    m_isCivilian = true;
    m_showHudInfo = false;
    m_scaleMin = 1.0f;
    m_scaleMax = 1.0f;
    Parse(node);
}

void CBH_PersonType::Parse(TiXmlNode* node)
{
    if (node == NULL) {
        return;
    }
    m_name = CXmlHelper::GetAttributeValue(node, "name");
    {
        XString visualText = CXmlHelper::GetAttributeValue(node, "visualCharacters");
        Vector<XString> visualNames;
        DGHelper::SplitStringAndTrim(visualNames, visualText, XString(","));
        m_visualCharacters = visualNames;
        // Every visual character must exist (the result is only looked up).
        for (int index = 0; index < m_visualCharacters.size(); index++) {
            ((App*)WindowApp::m_instance)->m_visualCharacterTypeManager->GetType(m_visualCharacters[index]);
        }
    }

    XString weaponName = CXmlHelper::GetAttributeValue(node, "weapon");
    if (weaponName.Length() != 0) {
        m_weapon = ((App*)WindowApp::m_instance)->m_enemyWeaponTypeManager->GetType(weaponName);
    }
    m_baseHp = CXmlHelper::GetAttributeValue(node, "base_hp").ToInt();

    // "min-max" or a single value.
    XString damageText = CXmlHelper::GetAttributeValue(node, "base_damage");
    Vector<XString> damageParts;
    DGHelper::SplitStringAndTrim(damageParts, damageText, XString("-"));
    m_damageMin = damageParts[0].ToInt();
    int damageMax = m_damageMin;
    if (damageParts.size() >= 2) {
        damageMax = damageParts[1].ToInt();
    }
    m_damageMax = damageMax;

    m_blowDamage = CXmlHelper::GetAttributeValueOrUseDefault(node, "blow_damage", XString(L"0")).ToInt();
    m_critProbability = CXmlHelper::GetAttributeValue(node, "crit_prob").ToFloat();
    m_critMultiplier = CXmlHelper::GetAttributeValue(node, "crit_mult").ToFloat();
    m_damageCooldown = CXmlHelper::GetAttributeValue(node, "damage_cooldown").ToInt();
    m_attackRange = CXmlHelper::GetAttributeValue(node, "attack_range").ToFloat();
    m_fearRange = CXmlHelper::GetAttributeValueOrUseDefault(node, "fear_range", XString(10.0f, 2)).ToFloat();
    m_dangerRange = CXmlHelper::GetAttributeValueOrUseDefault(node, "danger_range", XString(20.0f, 2)).ToFloat();
    m_specialAttackRange = CXmlHelper::GetAttributeValueOrUseDefault(node, "special_attack_range", XString(20.0f, 2)).ToFloat();
    m_isBoss = CXmlHelper::GetAttributeValueOrUseDefault(node, "is_boss", XString(L"0")).ToBool();
    m_arcadeScore = CXmlHelper::GetAttributeValueOrUseDefault(node, "arcade_score", XString(0)).ToFloat();
    m_arcadeCash = CXmlHelper::GetAttributeValueOrUseDefault(node, "arcade_cash", XString(0)).ToFloat();
    m_arcadeXp = CXmlHelper::GetAttributeValueOrUseDefault(node, "arcade_xp", XString(0)).ToFloat();
    m_scriptName = CXmlHelper::GetAttributeValue(node, "scriptName");
    m_isCivilian = CXmlHelper::GetAttributeValueOrUseDefault(node, "is_civilian", "0").ToBool();
    m_ignoreLureGrenades = CXmlHelper::GetBoolAttributeValueOrUseDefault(node, "ignore_lure_grenades", m_isCivilian);
    m_showHudInfo = CXmlHelper::GetAttributeValueOrUseDefault(node, "show_hud_info", "0").ToBool();

    // "min-max" or a single scale.
    XString scaleText = CXmlHelper::GetAttributeValueOptional(node, "scale");
    if (scaleText.ByteSize() != 0) {
        Vector<XString> scaleParts;
        DGHelper::SplitStringAndTrim(scaleParts, scaleText, XString("-"));
        m_scaleMin = scaleParts[0].ToFloat();
        if (scaleParts.size() >= 2) {
            m_scaleMax = scaleParts[1].ToFloat();
        } else {
            m_scaleMax = m_scaleMin;
        }
    }
    if (m_isCivilian) {
        m_showHudInfo = true;
    }
}

CVisualCharacterType* CBH_PersonType::GetVisualObject(unsigned int index) const
{
    const XString& name = m_visualCharacters[m_globalSeed % m_visualCharacters.size()];
    return ((App*)WindowApp::m_instance)->m_visualCharacterTypeManager->GetType(name);
}

int CBH_PersonType::GetAttackDamage() const
{
    int damage = CRandGen::GetInstance()->GetRandRange(m_damageMin, m_damageMax);
    float chance = (float)CRandGen::GetInstance()->GetRand(100000) * 0.00001f;
    if (chance < m_critProbability) {
        damage = (int)((float)damage * m_critMultiplier);
    }
    return damage;
}

float CBH_PersonType::GenerateRandScale() const
{
    RandomValue* random = (RandomValue*)&WindowApp::m_instance->m_randomSeed;   // WindowApp 0x1c4 is a RandomValue
    return m_scaleMin + (m_scaleMax - m_scaleMin) * random->Float();
}

void CBH_PersonType::GenerateGlobalSeed()
{
    m_globalSeed = CRandGen::GetInstance()->Generate();
}
