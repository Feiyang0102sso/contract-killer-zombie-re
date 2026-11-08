// CSceneObjectManager.cpp - load scene-object configs and construct the matching runtime type.
#include "CSceneObjectManager.h"
#include "CSpawnSceneObject.h"
#include "CExplosiveSceneObject.h"
#include "CXmlHelper.h"
#include "App.h"
#include "ResMap.h"

CSceneObjectManager::CSceneObjectManager()
{
    ResMap* resources = ((App*)WindowApp::m_instance)->m_resMap;
    char* text = resources->loadText(XString(L"res/common/configs/scene_objects.xml"));
    TiXmlDocument document;
    document.Parse(text);
    if (text != NULL) {
        delete[] text;
    }
    Parse(document.FirstChild("scene_objects"));
}

void CSceneObjectManager::Parse(TiXmlNode* node)
{
    for (TiXmlNode* child = node->FirstChild("explosive_scene_object"); child != NULL; child = child->NextSibling("explosive_scene_object")) {
        ParseExplosiveObject(child);
    }
    for (TiXmlNode* child = node->FirstChild("spawn_scene_object"); child != NULL; child = child->NextSibling("spawn_scene_object")) {
        SSpawnSceneObjectInfo* info = new SSpawnSceneObjectInfo();
        ParseObject(child, info);
    }
}

void CSceneObjectManager::ParseExplosiveObject(TiXmlNode* node)
{
    SExplosiveSceneObjectInfo* info = new SExplosiveSceneObjectInfo();
    info->m_damage = CXmlHelper::GetAttributeValue(node, "damage").ToInt();
    info->m_damageRadius = CXmlHelper::GetAttributeValue(node, "damageRadius").ToFloat();
    info->m_hp = CXmlHelper::GetAttributeValue(node, "hp").ToInt();
    ParseObject(node, info);
}

void CSceneObjectManager::ParseObject(TiXmlNode* node, SSceneObjectInfo* info)
{
    info->m_name = CXmlHelper::GetAttributeValue(node, "name");
    info->m_objectTransform = CXmlHelper::GetAttributeValue(node, "objectTransform");
    info->m_meshTransform = CXmlHelper::GetAttributeValue(node, "meshTransform");
    info->m_lightNodeName = CXmlHelper::GetAttributeValueOrUseDefault(node, "lightNodeName", XString(L""));
    XString path = CXmlHelper::GetAttributeValue(node, "modelPath");
    info->m_modelResource = ((App*)WindowApp::m_instance)->m_resMap->getResourceID(path);
    m_objects.addElement(info);
}

CSceneObjectManager::SSceneObjectInfo* CSceneObjectManager::GetSceneObjectInfoByName(const XString& name)
{
    for (int index = 0; index < m_objects.size(); index++) {
        if (m_objects[index]->m_name == name) {
            return m_objects[index];
        }
    }
    return NULL;
}

void CSceneObjectManager::CleanupResources()
{
    for (int index = 0; index < m_objects.size(); index++) {
        m_objects[index]->m_model.Clear();
    }
}

CSceneObjectManager::~CSceneObjectManager()
{
    SAFE_DELETE_VECTOR(m_objects);
}

CSceneObject* CSceneObjectManager::CreateSceneObject(const SSceneObjectInfo* info)
{
    switch (info->m_type) {
    case 0:
        return new CExplosiveSceneObject(static_cast<const SExplosiveSceneObjectInfo*>(info));
    case 1:
        return new CSpawnSceneObject(static_cast<const SSpawnSceneObjectInfo*>(info));
    }
    return NULL;
}
