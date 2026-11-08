// CBH_PersonTypeManager.cpp - loads res/common/configs/persons.xml into a table of person types.
#include "CBH_PersonTypeManager.h"
#include <wchar.h>
#include "App.h"
#include "ResMap.h"
#include "tinyxml.h"
#include "CBH_PersonType.h"

CBH_PersonTypeManager::CBH_PersonTypeManager()
{
    ResMap* resMap = ((App*)WindowApp::m_instance)->m_resMap;
    char* text = resMap->loadText(XString(L"res/common/configs/persons.xml"));
    TiXmlDocument document;
    document.Parse(text);
    if (text != NULL) {
        delete[] text;
    }
    TiXmlNode* node = document.FirstChild("persons")->FirstChild("person");
    while (node != NULL) {
        CBH_PersonType* type = new CBH_PersonType(node);
        m_types.Set(type->m_name, type);
        m_typeNames.addElement(type->m_name);
        node = node->NextSibling("person");
    }
}

CBH_PersonTypeManager::~CBH_PersonTypeManager()
{
    m_types.DeleteContents();
}

CBH_PersonType* CBH_PersonTypeManager::GetType(const XString& name) const
{
    return *m_types.Get(name);
}

bool CBH_PersonTypeManager::TypeIsExists(const XString& name) const
{
    return m_types.Get(name) != NULL;
}
