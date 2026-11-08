// CSceneSetupsManager.cpp - named scene spawn layouts and optional scene parts.
#include "CSceneSetupsManager.h"
#include "CXmlHelper.h"
#include "App.h"
#include "ResMap.h"

CSceneSetupsManager::CSceneSetupsManager()
{
    ResMap* resources = ((App*)WindowApp::m_instance)->m_resMap;
    char* text = resources->loadText(XString(L"res/common/configs/scene_setups.xml"));
    TiXmlDocument document;
    document.Parse(text);
    delete[] text;
    Parse(document.FirstChild("scene_setups"));
}

void CSceneSetupsManager::Parse(TiXmlNode* node)
{
    for (TiXmlNode* child = node->FirstChild("scene_setup"); child != NULL; child = child->NextSibling("scene_setup")) {
        SSceneSetup setup;
        setup.m_name = CXmlHelper::GetAttributeValue(child, "name");
        TiXmlNode* objects = child->FirstChild("objects");
        if (objects != NULL) {
            for (TiXmlNode* object = objects->FirstChild("o"); object != NULL; object = object->NextSibling("o")) {
                SSceneSpawnInfo spawn;
                spawn.m_name = CXmlHelper::GetAttributeValue(object, "name");
                spawn.m_point = CXmlHelper::GetAttributeValue(object, "point");
                spawn.m_probability = CXmlHelper::GetAttributeValue(object, "prob").ToInt();
                setup.m_objects.addElement(spawn);
            }
        }
        TiXmlNode* persons = child->FirstChild("persons");
        if (persons != NULL) {
            for (TiXmlNode* person = persons->FirstChild("p"); person != NULL; person = person->NextSibling("p")) {
                SSceneSpawnInfo spawn;
                spawn.m_name = CXmlHelper::GetAttributeValue(person, "name");
                spawn.m_point = CXmlHelper::GetAttributeValue(person, "point");
                spawn.m_probability = CXmlHelper::GetAttributeValue(person, "prob").ToInt();
                setup.m_persons.addElement(spawn);
            }
        }
        TiXmlNode* optionalParts = child->FirstChild("optional_parts");
        if (optionalParts != NULL) {
            for (TiXmlNode* part = optionalParts->FirstChild("p"); part != NULL; part = part->NextSibling("p")) {
                setup.m_optionalParts.addElement(CXmlHelper::GetAttributeValue(part, "name"));
            }
        }
        setup.m_helicopterHidden = false;
        TiXmlNode* special = child->FirstChild("special");
        if (special != NULL) {
            setup.m_helicopterModelId = CXmlHelper::GetAttributeValueOptional(special, "helicopter_model_id");
            setup.m_helicopterHidden = CXmlHelper::GetAttributeValueOrUseDefault(special, "helicopter_hidden", "0").ToBool();
        }
        m_setups.addElement(setup);
    }
}

CSceneSetupsManager::SSceneSetup* CSceneSetupsManager::GetSceneSetupByName(const XString& name)
{
    for (int index = 0; index < m_setups.size(); index++) {
        if (m_setups[index].m_name == name) {
            return &m_setups[index];
        }
    }
    XString diagnostic = XString::Format(L"Can't scene setup with name \"%s\"", name.c_str());
    // The shipped lookup returns the first element after formatting the diagnostic.
    return m_setups.m_items;
}

CSceneSetupsManager::~CSceneSetupsManager()
{
    // Member destruction frees the configured records, as in the original D1.
}
