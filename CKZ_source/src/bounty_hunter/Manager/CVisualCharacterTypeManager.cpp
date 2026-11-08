// CVisualCharacterTypeManager.cpp - visual_characters.xml and the common shadow model.
#include "CVisualCharacterTypeManager.h"
#include "CVisualCharacterType.h"
#include "App.h"
#include "ResMap.h"
#include "SwerveHelper.h"
#include "tinyxml.h"

CVisualCharacterTypeManager::CVisualCharacterTypeManager()
    : m_types(8)
{
    char* text = ((App*)WindowApp::m_instance)->m_resMap->loadText(XString(L"res/common/configs/visual_characters.xml"));
    TiXmlDocument document;
    document.Parse(text);
    delete[] text;
    TiXmlNode* common = document.FirstChild("commonSection");
    CVisualCharacterType::LoadCommonInfo(common);
    CVisualCharacterType::LoadCommonAnimation(common);
    for (TiXmlNode* shared = document.FirstChild("sharedSection"); shared != NULL; shared = shared->NextSibling("sharedSection")) {
        CVisualCharacterType::LoadSharedInfo(shared);
    }
    swerve::World<CSwerveAssertingErrorHandler> shadow(App::LoadObject3D("BIN_M3G_EFFECT_SHADOW"));
    m_commonGroup = swerve::Group<CSwerveAssertingErrorHandler>(shadow.find(SwerveHelper::Id("RootNode")));
    for (TiXmlNode* node = document.FirstChild("visualCharacter"); node != NULL; node = node->NextSibling("visualCharacter")) {
        CVisualCharacterType* type = new CVisualCharacterType(node);
        m_types.Set(type->m_name, type);
    }
}

CVisualCharacterTypeManager::~CVisualCharacterTypeManager()
{
    for (int index = 0; index < m_types.m_count; index++) {
        CVisualCharacterType* type = *m_types.GetAt(index, NULL);
        if (type != NULL) {
            // Original 0x13fd26 calls this static cleanup once for every non-null entry.
            type->DestroyCommonAnimation();
        }
    }
    m_types.DeleteContents();
}

CVisualCharacterType* CVisualCharacterTypeManager::GetType(const XString& name) const
{
    return *m_types.Get(name);
}

void CVisualCharacterTypeManager::CleanupResources() const
{
    for (int index = 0; index < m_types.m_count; index++) {
        (*m_types.GetAt(index, NULL))->CleanupResources();
    }
}
