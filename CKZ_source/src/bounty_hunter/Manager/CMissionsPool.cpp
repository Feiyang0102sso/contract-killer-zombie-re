// CMissionsPool.cpp - mission type/difficulty pool persistence.
#include "CMissionsPool.h"
#include "CMission.h"
#include "CXmlHelper.h"

void CMissionsPool::Load(TiXmlNode* node)
{
    for (TiXmlNode* child = node->FirstChild("m"); child != NULL; child = child->NextSibling("m")) {
        int type = CMission::StringToMissionType(CXmlHelper::GetAttributeValue(child, "type"));
        int difficulty = CXmlHelper::GetAttributeValue(child, "diff").ToInt();
        SPoolElement element;
        element.m_type = type;
        element.m_difficulty = difficulty;
        m_elements.addElement(element);
    }
}

void CMissionsPool::Save(TiXmlElement* root)
{
    TiXmlElement* pool = CXmlHelper::ObtainElement(root, XString("pool"));
    pool->Clear();
    for (int index = 0; index < m_elements.size(); index++) {
        TiXmlElement* element = new TiXmlElement("m");
        element->SetStringAttribute("type", XString::AnsiString(CMission::MissionTypeToString((CMission::EMissionType)m_elements[index].m_type).c_str()));
        element->SetAttribute("diff", m_elements[index].m_difficulty);
        pool->InsertEndChild(*element);
        delete element;
    }
}
