// CCommonGameConfig.cpp - res/common/configs/common.xml: kill scores, ranks, mission pools, provision boxes.
#include "CCommonGameConfig.h"
#include <wchar.h>
#include "App.h"
#include "ResMap.h"
#include "MathLib.h"
#include "tinyxml.h"
#include "CXmlHelper.h"
#include "DGHelper.h"

CCommonGameConfig::CCommonGameConfig()
{
    ResMap* resMap = ((App*)WindowApp::m_instance)->m_resMap;
    char* text = resMap->loadText(XString(L"res/common/configs/common.xml"));
    TiXmlDocument document;
    document.Parse(text);
    if (text != NULL) {
        delete[] text;
    }
    TiXmlNode* common = document.FirstChild("common");
    ParseScore(common->FirstChild("score"));
    ParseRanks(common->FirstChild("ranks"));
    ParseMissionTypesPools(common->FirstChild("missions_pools"));
    ParseBoxes(common->FirstChild("provision_boxes"));
}

void CCommonGameConfig::ParseScore(TiXmlNode* node)
{
    m_headshotScore = CXmlHelper::GetAttributeValue(node, "headshot").ToInt();
    m_multikillScore = CXmlHelper::GetAttributeValue(node, "multikill").ToInt();
    m_megakillScore = CXmlHelper::GetAttributeValue(node, "megakill").ToInt();
    m_unstoppableScore = CXmlHelper::GetAttributeValue(node, "unstoppable").ToInt();
    m_dominatingScore = CXmlHelper::GetAttributeValue(node, "dominating").ToInt();
    m_multiheadshotScore = CXmlHelper::GetAttributeValue(node, "multiheadshot").ToInt();
    m_decapitatorScore = CXmlHelper::GetAttributeValue(node, "decapitator").ToInt();
    m_deadlyPrecisionScore = CXmlHelper::GetAttributeValue(node, "deadlyprecision").ToInt();
    m_kaboomScore = CXmlHelper::GetAttributeValue(node, "kaboom").ToInt();
}

void CCommonGameConfig::ParseRanks(TiXmlNode* node)
{
    for (TiXmlNode* child = node->FirstChild("rank"); child != NULL; child = child->NextSibling("rank")) {
        SRank rank;
        rank.m_score = CXmlHelper::GetAttributeValue(child, "score").ToInt();
        rank.m_title = CXmlHelper::GetAttributeValue(child, "title");
        rank.m_achievement = CXmlHelper::GetAttributeValue(child, "achievement");
        // keep the ranks sorted by score
        int index = 0;
        while (index < m_ranks.size() && m_ranks[index].m_score <= rank.m_score) {
            index++;
        }
        m_ranks.insertElementAt(rank, index);
    }
}

void CCommonGameConfig::ParseMissionTypesPools(TiXmlNode* node)
{
    for (TiXmlNode* child = node->FirstChild("pool"); child != NULL; child = child->NextSibling("pool")) {
        SPoolInfo info;
        // "min-max" or a single level
        XString levelText = CXmlHelper::GetAttributeValue(child, "level");
        Vector<XString> levelParts;
        DGHelper::SplitStringAndTrim(levelParts, levelText, XString("-"));
        info.m_minLevel = levelParts[0].ToInt();
        int maxLevel = info.m_minLevel;
        if (levelParts.size() > 1) {
            maxLevel = levelParts[1].ToInt();
        }
        info.m_maxLevel = maxLevel;
        info.m_pool.Load(child);
        m_pools.addElement(info);
    }
}

void CCommonGameConfig::ParseBoxes(TiXmlNode* node)
{
    for (TiXmlNode* child = node->FirstChild("b"); child != NULL; child = child->NextSibling("b")) {
        SProvisionBox box;
        box.m_name = CXmlHelper::GetAttributeValue(child, "name");
        box.m_group = CXmlHelper::GetAttributeValue(child, "group");
        m_boxes.addElement(box);
    }
}

XString& CCommonGameConfig::GetRankTitleForScore(int score)
{
    int index = 0;
    while (index < m_ranks.size() && m_ranks[index].m_score <= score) {
        index++;
    }
    if (index > 0) {
        return m_ranks[index - 1].m_title;
    }
    return m_ranks[0].m_title;
}

void CCommonGameConfig::GetAllRankAchievementsForScore(int score, Vector<int>& ranks)
{
    for (int index = 0; index < m_ranks.size() && m_ranks[index].m_score <= score; index++) {
        ranks.addElement(index);
    }
}

CMissionsPool CCommonGameConfig::GetMissionsPoolForLevel(int level)
{
    for (int index = 0; index < m_pools.size(); index++) {
        if (level >= m_pools[index].m_minLevel && level <= m_pools[index].m_maxLevel) {
            return m_pools[index].m_pool;
        }
    }
    return m_pools[0].m_pool;
}

XString& CCommonGameConfig::GetRandomBoxGroupName()
{
    RandomValue* random = (RandomValue*)&WindowApp::m_instance->m_randomSeed;   // WindowApp 0x1c4 is a RandomValue
    return m_boxes[random->Int(m_boxes.size())].m_group;
}

XString& CCommonGameConfig::GetBoxGroupName(const XString& name)
{
    for (int index = 0; index < m_boxes.size(); index++) {
        if (m_boxes[index].m_name == name) {
            return m_boxes[index].m_group;
        }
    }
    return m_boxes[0].m_group;
}

CCommonGameConfig::~CCommonGameConfig()
{
}
