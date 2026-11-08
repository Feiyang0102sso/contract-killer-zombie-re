// Original u471 complete owner; keeps the missing levels/uninitialized reward columns and the repeated attribute reads.
#include "CProgressSystem.h"
#include "App.h"
#include "CXmlHelper.h"
#include "ResMap.h"
#include "DGHelper.h"
#include "CBH_Player.h"
#include "CDH_WeaponsManager.h"
#include <stdlib.h>

CProgressSystem::CProgressSystem()
{
    char* text = ((App*)WindowApp::m_instance)->m_resMap->loadText(XString(L"res/common/configs/progress.xml"));
    TiXmlDocument document;
    document.Parse(text, NULL, TIXML_ENCODING_UNKNOWN);
    if (text != NULL) free(text);
    ParseLevels(document.FirstChild("progress"));
    m_field10 = 0.5f;
}

void CProgressSystem::ParseLevels(TiXmlNode* root)
{
    TiXmlNode* node = root->FirstChild("level");
    while (node != NULL) {
        int number = CXmlHelper::GetAttributeValue(node, "num").ToInt();
        int xp = CXmlHelper::GetAttributeValue(node, "xp").ToInt();
        int energy = CXmlHelper::GetAttributeValue(node, "energy").ToInt();
        while (m_levels.size() <= number) {
            SLevelInfo missing;
            // The real default constructor only writes present; the remaining fields of the 836B are not zeroed.
            m_levels.addElement(missing);
        }
        SLevelInfo& level = m_levels[number];
        level.m_present = true;
        level.m_xpThreshold = xp;
        level.m_maxEnergy = energy;
        for (int type = 0; type < 7; ++type) {
            for (int difficulty = 0; difficulty < 4; ++difficulty) {
                level.m_xpBase[type][difficulty] = 0;
                level.m_moneyBase[type][difficulty] = 0;
                level.m_xpPerCivilian[type][difficulty] = 0;
                level.m_moneyPerCivilian[type][difficulty] = 0;
                level.m_energy[type][difficulty] = 0;
            }
            // Original 11e9bc and 11e9c8/cc: the special heli entry is rewritten for every type.
            level.m_recommendedWeapons[type] = 0;
            level.m_recommendedICHeli = 0;
            level.m_recommendedKAHeli = 0;
        }
        // The original has no perProvision table clearing xp/money; only the three heli difficulties below are assigned.
        level.m_xpBase[CMission::MISSION_QU][0] = CXmlHelper::GetAttributeValue(node, "quest_XP").ToInt();
        level.m_xpBase[CMission::MISSION_KA][1] = CXmlHelper::GetAttributeValue(node, "killall_E_XP").ToInt();
        level.m_xpBase[CMission::MISSION_KA][2] = CXmlHelper::GetAttributeValue(node, "killall_M_XP").ToInt();
        level.m_xpBase[CMission::MISSION_KA][3] = CXmlHelper::GetAttributeValue(node, "killall_H_XP").ToInt();
        level.m_xpBase[CMission::MISSION_CB][1] = CXmlHelper::GetAttributeValue(node, "provision_E_XP").ToInt();
        level.m_xpBase[CMission::MISSION_CB][2] = CXmlHelper::GetAttributeValue(node, "provision_M_XP").ToInt();
        level.m_xpBase[CMission::MISSION_CB][3] = CXmlHelper::GetAttributeValue(node, "provision_H_XP").ToInt();
        level.m_xpBase[CMission::MISSION_IC][1] = CXmlHelper::GetAttributeValue(node, "people_E_XP").ToInt();
        level.m_xpBase[CMission::MISSION_IC][2] = CXmlHelper::GetAttributeValue(node, "people_M_XP").ToInt();
        level.m_xpBase[CMission::MISSION_IC][3] = CXmlHelper::GetAttributeValue(node, "people_H_XP").ToInt();
        level.m_xpBase[CMission::MISSION_ST][1] = CXmlHelper::GetAttributeValue(node, "heli_base_E_XP").ToInt();
        level.m_xpBase[CMission::MISSION_ST][2] = CXmlHelper::GetAttributeValue(node, "heli_base_M_XP").ToInt();
        level.m_xpBase[CMission::MISSION_ST][3] = CXmlHelper::GetAttributeValue(node, "heli_base_H_XP").ToInt();
        level.m_xpPerCivilian[CMission::MISSION_ST][1] = CXmlHelper::GetAttributeValue(node, "heli_per_civ_E_XP").ToInt();
        level.m_xpPerCivilian[CMission::MISSION_ST][2] = CXmlHelper::GetAttributeValue(node, "heli_per_civ_M_XP").ToInt();
        level.m_xpPerCivilian[CMission::MISSION_ST][3] = CXmlHelper::GetAttributeValue(node, "heli_per_civ_H_XP").ToInt();
        level.m_xpPerProvision[CMission::MISSION_ST][1] = CXmlHelper::GetAttributeValue(node, "heli_per_box_E_XP").ToInt();
        level.m_xpPerProvision[CMission::MISSION_ST][2] = CXmlHelper::GetAttributeValue(node, "heli_per_box_M_XP").ToInt();
        level.m_xpPerProvision[CMission::MISSION_ST][3] = CXmlHelper::GetAttributeValue(node, "heli_per_box_H_XP").ToInt();
        level.m_moneyBase[CMission::MISSION_QU][0] = CXmlHelper::GetAttributeValue(node, "quest_Money").ToInt();
        level.m_moneyBase[CMission::MISSION_KA][1] = CXmlHelper::GetAttributeValue(node, "killall_E_Money").ToInt();
        level.m_moneyBase[CMission::MISSION_KA][2] = CXmlHelper::GetAttributeValue(node, "killall_M_Money").ToInt();
        level.m_moneyBase[CMission::MISSION_KA][3] = CXmlHelper::GetAttributeValue(node, "killall_H_Money").ToInt();
        level.m_moneyBase[CMission::MISSION_CB][1] = CXmlHelper::GetAttributeValue(node, "provision_E_Money").ToInt();
        level.m_moneyBase[CMission::MISSION_CB][2] = CXmlHelper::GetAttributeValue(node, "provision_M_Money").ToInt();
        level.m_moneyBase[CMission::MISSION_CB][3] = CXmlHelper::GetAttributeValue(node, "provision_H_Money").ToInt();
        level.m_moneyBase[CMission::MISSION_IC][1] = CXmlHelper::GetAttributeValue(node, "people_E_Money").ToInt();
        level.m_moneyBase[CMission::MISSION_IC][2] = CXmlHelper::GetAttributeValue(node, "people_M_Money").ToInt();
        level.m_moneyBase[CMission::MISSION_IC][3] = CXmlHelper::GetAttributeValue(node, "people_H_Money").ToInt();
        level.m_moneyBase[CMission::MISSION_ST][1] = CXmlHelper::GetAttributeValue(node, "heli_base_E_Money").ToInt();
        level.m_moneyBase[CMission::MISSION_ST][2] = CXmlHelper::GetAttributeValue(node, "heli_base_M_Money").ToInt();
        level.m_moneyBase[CMission::MISSION_ST][3] = CXmlHelper::GetAttributeValue(node, "heli_base_H_Money").ToInt();
        level.m_moneyPerCivilian[CMission::MISSION_ST][1] = CXmlHelper::GetAttributeValue(node, "heli_per_civ_E_Money").ToInt();
        level.m_moneyPerCivilian[CMission::MISSION_ST][2] = CXmlHelper::GetAttributeValue(node, "heli_per_civ_M_Money").ToInt();
        level.m_moneyPerCivilian[CMission::MISSION_ST][3] = CXmlHelper::GetAttributeValue(node, "heli_per_civ_H_Money").ToInt();
        level.m_moneyPerProvision[CMission::MISSION_ST][1] = CXmlHelper::GetAttributeValue(node, "heli_per_box_E_Money").ToInt();
        level.m_moneyPerProvision[CMission::MISSION_ST][2] = CXmlHelper::GetAttributeValue(node, "heli_per_box_M_Money").ToInt();
        level.m_moneyPerProvision[CMission::MISSION_ST][3] = CXmlHelper::GetAttributeValue(node, "heli_per_box_H_Money").ToInt();
        // Original 11ef76..11efe4 really reads the three XP keys to fill energy; the odd original config logic is kept.
        level.m_energy[CMission::MISSION_KA][1] = CXmlHelper::GetAttributeValue(node, "killall_E_XP").ToInt();
        level.m_energy[CMission::MISSION_KA][2] = CXmlHelper::GetAttributeValue(node, "killall_M_XP").ToInt();
        level.m_energy[CMission::MISSION_KA][3] = CXmlHelper::GetAttributeValue(node, "killall_H_XP").ToInt();
        level.m_energy[CMission::MISSION_CB][1] = CXmlHelper::GetAttributeValue(node, "provision_E_Energy").ToInt();
        level.m_energy[CMission::MISSION_CB][2] = CXmlHelper::GetAttributeValue(node, "provision_M_Energy").ToInt();
        level.m_energy[CMission::MISSION_CB][3] = CXmlHelper::GetAttributeValue(node, "provision_H_Energy").ToInt();
        level.m_energy[CMission::MISSION_IC][1] = CXmlHelper::GetAttributeValue(node, "people_E_Energy").ToInt();
        level.m_energy[CMission::MISSION_IC][2] = CXmlHelper::GetAttributeValue(node, "people_M_Energy").ToInt();
        level.m_energy[CMission::MISSION_IC][3] = CXmlHelper::GetAttributeValue(node, "people_H_Energy").ToInt();
        level.m_energy[CMission::MISSION_ST][1] = CXmlHelper::GetAttributeValue(node, "heli_E_Energy").ToInt();
        level.m_energy[CMission::MISSION_ST][2] = CXmlHelper::GetAttributeValue(node, "heli_M_Energy").ToInt();
        level.m_energy[CMission::MISSION_ST][3] = CXmlHelper::GetAttributeValue(node, "heli_H_Energy").ToInt();
        level.m_recommendedWeapons[CMission::MISSION_IC] = CXmlHelper::GetAttributeValue(node, "rec_wid_ic").ToInt();
        level.m_recommendedWeapons[CMission::MISSION_CB] = CXmlHelper::GetAttributeValue(node, "rec_wid_cb").ToInt();
        level.m_recommendedWeapons[CMission::MISSION_ST] = CXmlHelper::GetAttributeValue(node, "rec_wid_st").ToInt();
        level.m_recommendedWeapons[CMission::MISSION_KA] = CXmlHelper::GetAttributeValue(node, "rec_wid_ka").ToInt();
        level.m_recommendedWeapons[CMission::MISSION_QU] = CXmlHelper::GetAttributeValue(node, "rec_wid_qu").ToInt();
        level.m_recommendedICHeli = CXmlHelper::GetAttributeValue(node, "rec_wid_ic_heli").ToInt();
        level.m_recommendedKAHeli = CXmlHelper::GetAttributeValue(node, "rec_wid_ka_heli").ToInt();
        for (int type = 1; type < 7; ++type) {
            // Original 11f248 rereads App and manager every time; although the return value is unused, the virtual call cannot be omitted.
            ((App*)WindowApp::m_instance)->m_weaponsManager->getItemById(level.m_recommendedWeapons[type]);
        }
        node = node->NextSibling("level");
    }
}

void CProgressSystem::RegisterGameEvent(EGameEvent event, int, int)
{
    switch (event) {
    case EVENT_ENEMY_KILLED:
        CBH_Player::GetInstance()->OnEnemyKilled();
        break;
    case EVENT_ENEMY_TRANQUILLIZED:
        CBH_Player::GetInstance()->OnEnemyTranquillized();
        break;
    case EVENT_ENEMY_ESCAPED:
        CBH_Player::GetInstance()->OnEnemyEscaped();
        break;
    }
}
int CProgressSystem::GetMaxLevel() const { return m_levels.size() - 1; }
int CProgressSystem::GetMaxXPForLevel(int level) const
{
    if (level < 1 || level >= m_levels.size()) return 0;
    return m_levels[level - 1].m_xpThreshold;
}
int CProgressSystem::GetMaxXP() const
{
    int experience = 0;
    if (m_levels.size() > 0) {
        experience = GetMaxXPForLevel(GetMaxLevel());
    }
    return experience;
}
int CProgressSystem::GetMaxEnergyForLevel(int level) const
{
    if (level < 0 || level >= m_levels.size()) return 0;
    return m_levels[level].m_maxEnergy;
}
int CProgressSystem::GetLevel(int xp) const
{
    if (xp < 1) return 0;
    for (int level = 0; level < m_levels.size(); ++level) {
        if (m_levels[level].m_xpThreshold > xp) return level;
    }
    return m_levels.size() - 1;
}
int CProgressSystem::CalcXpForKilledUnit(CUnit*) { return 0; } // original 11f35c movs r0,#0 /bx lr, a truly empty original algorithm.
int CProgressSystem::GetTaggingTimeMS()
{
    float time = ((App*)WindowApp::m_instance)->m_helper->m_config.GetFloatValueByToken("TIME_TAGGING_LIMIT_MS", true);
    CBH_Player* player = CBH_Player::GetInstance();
    int count = m_levels.size();
    if (count < 1) return 0;
    return (int)time * (player->m_id - 1) / count;
}
int CProgressSystem::GetMissionRewardXP(CMission::EMissionType type, CMission::EMissionDifficulty difficulty, int people, int provision)
{
    int number = CBH_Player::GetInstance()->m_id;
    if (number < 1 || number >= m_levels.size()) return 0;
    const SLevelInfo& level = m_levels[number];
    return level.m_xpBase[type][difficulty] + level.m_xpPerCivilian[type][difficulty] * people + level.m_xpPerProvision[type][difficulty] * provision;
}
int CProgressSystem::GetMissionRewardMoney(CMission::EMissionType type, CMission::EMissionDifficulty difficulty, int people, int provision)
{
    int number = CBH_Player::GetInstance()->m_id;
    if (number < 1 || number >= m_levels.size()) return 0;
    const SLevelInfo& level = m_levels[number];
    return level.m_moneyBase[type][difficulty] + level.m_moneyPerCivilian[type][difficulty] * people + level.m_moneyPerProvision[type][difficulty] * provision;
}
int CProgressSystem::GetRecommendedWeaponId(CMission::EMissionType type, XString mapName)
{
    int number = CBH_Player::GetInstance()->m_id;
    if (number < 1 || number >= m_levels.size()) return 0;
    // A single full-expression keeps the original multiple Lower/Find temporaries, destroyed together in reverse order at the end of the OR short-circuit.
    bool helicopter = mapName.Lower().Find(XString("rt1h"), 0) != -1 ||
                      mapName.Lower().Find(XString("rt2h"), 0) != -1 ||
                      mapName.Lower().Find(XString("rt3h"), 0) != -1;
    bool rt = mapName.Lower().Find(XString("rt"), 0) != -1;
    if (type == CMission::MISSION_IC && helicopter) return m_levels[number].m_recommendedICHeli;
    if (type == CMission::MISSION_KA && rt) return m_levels[number].m_recommendedKAHeli;
    return m_levels[number].m_recommendedWeapons[type];
}
