// CMissionManager.cpp - configured missions and the cycling description table.
#include "CMissionManager.h"
#include "CMapLocation.h"
#include "CXmlHelper.h"
#include "App.h"
#include "ResMap.h"
#include "DGHelper.h"
#include "CBH_Player.h"
#include "CRandGen.h"

CMissionManager::CMissionManager()
{
    ResMap* resources = ((App*)WindowApp::m_instance)->m_resMap;
    char* text = resources->loadText(XString(L"res/common/configs/missions.xml"));
    TiXmlDocument document;
    document.Parse(text);
    delete[] text;
    TiXmlNode* root = document.FirstChild("missions");
    for (TiXmlNode* node = root->FirstChild("mission"); node != NULL; node = node->NextSibling("mission")) {
        CMission* mission = new CMission(node);
        m_missions.addElement(mission);
    }
    ParseDescriptions();
}

void CMissionManager::ParseDescriptions()
{
    ResMap* resources = ((App*)WindowApp::m_instance)->m_resMap;
    char* text = resources->loadText(XString(L"res/common/configs/missions_descriptions.xml"));
    TiXmlDocument document;
    document.Parse(text);
    delete[] text;
    Vector<int> ids;
    TiXmlNode* root = document.FirstChild("cycledesc");
    for (TiXmlNode* node = root->FirstChild("missiondesc"); node != NULL; node = node->NextSibling("missiondesc")) {
        SMissionsClassDesctiption description;
        description.m_location = CXmlHelper::GetAttributeValueOptional(node, "location");
        XString type = CXmlHelper::GetAttributeValueOptional(node, "type");
        if (type.ByteSize() == 0) {
            description.m_type = CMission::MISSION_ANY;
        } else {
            description.m_type = CMission::StringToMissionType(type.Trim(true, true));
        }
        XString difficulty = CXmlHelper::GetAttributeValueOptional(node, "diff");
        if (difficulty.ByteSize() == 0) {
            description.m_difficulty = CMission::DIFFICULTY_ANY;
        } else {
            description.m_difficulty = (CMission::EMissionDifficulty)difficulty.ToInt();
        }
        XString level = CXmlHelper::GetAttributeValue(node, "level");
        Vector<XString> limits;
        DGHelper::SplitStringAndTrim(limits, level, XString("-"));
        description.m_minLevel = limits[0].ToInt();
        int maxLevel = description.m_minLevel;
        if (limits.size() > 1) {
            maxLevel = limits[1].ToInt();
        }
        description.m_maxLevel = maxLevel;
        description.m_description.m_id = CXmlHelper::GetAttributeValue(node, "id").ToInt();
        description.m_description.m_title = Window::ResStringSafe(CXmlHelper::GetAttributeValue(node, "title"));
        description.m_description.m_text = Window::ResStringSafe(CXmlHelper::GetAttributeValue(node, "text"));
        ids.addElement(description.m_description.m_id);
        m_descriptions.addElement(description);
    }
}

CMissionManager::~CMissionManager()
{
    SAFE_DELETE_VECTOR(m_missions);
}

CMission* CMissionManager::GetMissionByName(const XString& name) const
{
    for (int index = 0; index < m_missions.size(); index++) {
        if (m_missions[index]->m_name == name) {
            return m_missions[index];
        }
    }
    XString::Format(L"Can't find mission with name \"%s\"", name.c_str());
    return NULL;
}

void CMissionManager::GetSuitableMissionsForLocation(Vector<const CMission*>& missions, const CMapLocation* location,
                                                    CMission::EMissionDifficulty difficulty, CMission::EMissionType type) const
{
    for (int index = 0; index < m_missions.size(); index++) {
        CMission* mission = m_missions[index];
        // The shipped loop keeps this copied name alive even though it is otherwise unused.
        XString missionName = mission->m_name;
        if (location->m_locations.indexOf(mission->m_locationName, 0) != -1 && mission->IsAutopickable() == true &&
            (difficulty == CMission::DIFFICULTY_ANY || mission->m_difficulty == difficulty) &&
            (type == CMission::MISSION_ANY || mission->m_type == type) &&
            mission->IsSuitableForLevel(CBH_Player::GetInstance()->GetLevel()) == true) {
            const CMission* selected = mission;
            missions.addElement(selected);
        }
    }
}

int CMissionManager::GetRandomMissionDescriptionId(const CMission* mission)
{
    Vector<SMissionDescription*> candidates;
    for (int index = 0; index < m_descriptions.size(); index++) {
        SMissionsClassDesctiption& description = m_descriptions[index];
        if (description.m_difficulty != CMission::DIFFICULTY_ANY && description.m_difficulty != mission->m_difficulty) {
            continue;
        }
        if (description.m_type != CMission::MISSION_ANY && description.m_type != mission->m_type) {
            continue;
        }
        int level = CBH_Player::GetInstance()->GetLevel();
        // Original @0x12d218/0x12d220 selects OUTSIDE the interval. Preserve that behavior.
        if (level >= description.m_minLevel && level <= description.m_maxLevel) {
            continue;
        }
        if (description.m_location.ByteSize() != 0 && !(description.m_location == mission->m_locationName)) {
            continue;
        }
        SMissionDescription* selected = &description.m_description;
        candidates.addElement(selected);
    }
    if (candidates.size() != 0) {
        int index = com::glu::platform::core::CRandGen::GetInstance()->GetRand(candidates.size());
        return candidates[index]->m_id;
    }
    return -1;
}

CMissionManager::SMissionDescription* CMissionManager::GetMissionDescriptionById(int id)
{
    for (int index = 0; index < m_descriptions.size(); index++) {
        if (m_descriptions[index].m_description.m_id == id) {
            return &m_descriptions[index].m_description;
        }
    }
    // Original @0x12d2ea falls back to the first description, even for an empty table.
    return &m_descriptions[0].m_description;
}
