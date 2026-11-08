// CEnemyWeaponTypeManager.cpp - loads res/common/configs/enemy_weapons.xml into a table of weapons.
#include "CEnemyWeaponTypeManager.h"
#include <wchar.h>
#include "App.h"
#include "ResMap.h"
#include "tinyxml.h"
#include "CEnemyWeaponType.h"

CEnemyWeaponTypeManager::CEnemyWeaponTypeManager()
{
    ResMap* resMap = ((App*)WindowApp::m_instance)->m_resMap;
    char* text = resMap->loadText(XString(L"res/common/configs/enemy_weapons.xml"));
    TiXmlDocument document;
    document.Parse(text);
    if (text != NULL) {
        delete[] text;
    }
    for (TiXmlNode* node = document.FirstChild("enemyWeapon"); node != NULL; node = node->NextSibling("enemyWeapon")) {
        CEnemyWeaponType* type = new CEnemyWeaponType(node);
        m_types.Set(type->m_name, type);
    }
}

CEnemyWeaponTypeManager::~CEnemyWeaponTypeManager()
{
    m_types.DeleteContents();
}

CEnemyWeaponType* CEnemyWeaponTypeManager::GetType(const XString& name) const
{
    return *m_types.Get(name);
}
