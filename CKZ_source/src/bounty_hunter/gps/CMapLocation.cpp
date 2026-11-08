// b082: complete location/mission queue owner; XML cloning and temporary lifetimes follow the original.
#include "CMapLocation.h"
#include "CXmlHelper.h"
#include "CMissionManager.h"
#include "CLocationManager.h"
#include "CTutorialManager.h"
#include "CDH_SoundManager.h"
#include "DGHelper.h"
#include "tinyxml.h"
#include "App.h"

CMapLocation::CMapLocation(TiXmlNode* node)
    : CGPSItem(), m_locations(), m_listener(NULL), m_missions(), m_unresolvedF8(-1),
      m_hasQuestImage(false), m_questImage(), m_withFriend(false)
{
    ParseNode(node);
    SetVisible(false);
    App::LoadSGArchetype(5, 0);
}

void CMapLocation::ParseNode(TiXmlNode* node)
{
    m_id = CXmlHelper::GetAttributeValue(node, "id").ToInt();
    XString image = CXmlHelper::GetAttributeValue(node, "image"); // Original keeps this otherwise unused value.
    int x = CXmlHelper::GetAttributeValue(node, "posX").ToInt();
    int y = CXmlHelper::GetAttributeValue(node, "posY").ToInt();
    XString names = CXmlHelper::GetAttributeValue(node, "locationNames");
    {
        XString separator(",");
        DGHelper::SplitStringAndTrim(m_locations, names, separator);
        SetPos((float)x, (float)y);
    }
    m_missionType = CMission::StringToMissionType(CXmlHelper::GetAttributeValue(node, "missionType"));
    m_questImage.Assign(CXmlHelper::GetAttributeValueOptional(node, "questImage"));
    if (m_questImage.ByteSize() != 0) {
        m_questX = CXmlHelper::GetAttributeValue(node, "questPosX").ToInt() + x;
        m_questY = CXmlHelper::GetAttributeValue(node, "questPosY").ToInt() + y;
        m_hasQuestImage = true;
    }
    m_withFriend = node->ToElement()->Attribute("withFriend") != NULL;
    for (int index = 0; index < m_locations.m_size; ++index) {
        if (m_locations[index].ByteSize() != 0) {
            ((App*)WindowApp::m_instance)->m_locationManager->GetLocation(m_locations[index]);
        }
    }
}

void CMapLocation::Update(long elapsed)
{
    SetBlinking(false);
    if (((App*)WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial() == 1) {
        if (((App*)WindowApp::m_instance)->m_tutorialManager->IsMapLocationBlink() == 1) {
            SetBlinking(true);
        }
    }
    CGPSItem::Update(elapsed);
}

void CMapLocation::Paint(com::glu::platform::graphics::ICGraphics2d& graphics, int x, int y)
{
    CGPSItem::Paint(graphics, x, y);
}

void CMapLocation::OnClick()
{
    if (m_listener != NULL) {
        m_listener->OnMapLocationSelected(this);
        ((App*)WindowApp::m_instance)->m_soundManager->Play((CDH_SoundManager::eSound)27, false, false);
    }
}

CMapLocation::~CMapLocation()
{
}

bool CMapLocation::CheckHit(int x, int y, CGPSItem** hit)
{
    return CGPSItem::CheckHit(x, y, hit);
}

void CMapLocation::SetMapLocationListener(CMapLocationListener* listener)
{
    m_listener = listener;
}

void CMapLocation::SetMission(const XString& name, bool clear)
{
    CMission* mission = ((App*)WindowApp::m_instance)->m_missionManager->GetMissionByName(name);
    SLocationMission entry;
    entry.m_name.Assign(name);
    entry.m_descriptionId = ((App*)WindowApp::m_instance)->m_missionManager->GetRandomMissionDescriptionId(mission);
    if (clear == 1) {
        m_missions.removeAllElements();
    }
    m_missions.insertElementAt(entry, 0);
    UpdateCurrentMission();
}

void CMapLocation::UpdateCurrentMission()
{
    XString current = GetCurrentMission();
    if (current.ByteSize() != 0) {
        CMission* mission = ((App*)WindowApp::m_instance)->m_missionManager->GetMissionByName(current);
        SetVisible(true);
        // Original loads the SetSGImage slot (0x50) before evaluating both arguments.
        SetSGImage(mission->GetMapIconAnimation(), mission->GetMapIconDifficultyAnimation());
        XString name = mission->m_screenName;
        if (m_missions.m_size >= 1 && m_missions.m_items != NULL) {
            int descriptionId = m_missions[0].m_descriptionId;
            if (descriptionId >= 0 && ((App*)WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial() == 0) {
                name.Assign(((App*)WindowApp::m_instance)->m_missionManager->GetMissionDescriptionById(descriptionId)->m_title);
            }
        }
        SetScreenName(name);
    } else {
        SetVisible(false);
    }
}

XString CMapLocation::GetCurrentMission()
{
    if (m_missions.m_size >= 1 && m_missions.m_items != NULL) {
        return m_missions[0].m_name;
    }
    return *XString::pEmpty;
}

int CMapLocation::GetCurrentMissionDescriptionId()
{
    if (m_missions.m_size >= 1 && m_missions.m_items != NULL) {
        return m_missions[0].m_descriptionId;
    }
    return -1;
}

bool CMapLocation::HasMission()
{
    return GetCurrentMission().ByteSize() != 0;
}

void CMapLocation::Save(TiXmlElement* root)
{
    if (m_missions.m_size == 0) {
        return;
    }
    TiXmlElement* map = new TiXmlElement("mapLoc");
    map->SetAttribute("id", m_id);
    TiXmlElement* missions = new TiXmlElement("missions");
    for (int index = 0; index < m_missions.m_size; ++index) {
        if (m_missions[index].m_name.ByteSize() != 0) {
            TiXmlElement* mission = new TiXmlElement("mission");
            mission->SetAttribute("n", index);
            {
                XString::AnsiString name(m_missions[index].m_name.m_data);
                mission->SetStringAttribute("name", name.m_text);
            }
            mission->SetAttribute("did", m_missions[index].m_descriptionId);
            missions->InsertEndChild(*mission);
            delete mission;
        }
    }
    map->InsertEndChild(*missions);
    delete missions;
    root->InsertEndChild(*map);
    delete map;
}

void CMapLocation::Load(TiXmlElement* root)
{
    for (TiXmlElement* map = root->FirstChildElement("mapLoc"); map != NULL; map = map->NextSiblingElement("mapLoc")) {
        int id = CXmlHelper::GetAttributeValue(map, "id").ToInt();
        if (id == m_id) {
            TiXmlElement* missions = map->FirstChildElement("missions");
            if (missions == NULL) {
                return;
            }
            for (TiXmlElement* mission = missions->FirstChildElement("mission"); mission != NULL; mission = mission->NextSiblingElement("mission")) {
                int index = CXmlHelper::GetAttributeValue(mission, "n").ToInt();
                while (m_missions.m_size <= index) {
                    // Value-initialized: original zero-fills the 8-byte entry, then constructs m_name.
                    SLocationMission entry = SLocationMission();
                    m_missions.addElement(entry);
                }
                m_missions[index].m_name.Assign(CXmlHelper::GetAttributeValue(mission, "name"));
                m_missions[index].m_descriptionId = CXmlHelper::GetAttributeValue(mission, "did").ToInt();
            }
        }
    }
    for (int index = 0; index < m_missions.m_size; ++index) {
        if (((App*)WindowApp::m_instance)->m_missionManager->GetMissionByName(m_missions[index].m_name) == NULL) {
            ClearMissions();
            break;
        }
    }
    // If validation cleared the queue, original intentionally calls this twice.
    UpdateCurrentMission();
}

void CMapLocation::ClearMissions()
{
    m_missions.removeAllElements();
    UpdateCurrentMission();
}

void CMapLocation::CompleteMission()
{
    if (m_missions.m_size >= 1) {
        m_missions.removeElementAt(0);
        UpdateCurrentMission();
    }
}
