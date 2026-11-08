// CBalanceManager.cpp - balance setups of res/common/configs/balance.xml.
#include "CBalanceManager.h"
#include <wchar.h>
#include "App.h"
#include "ResMap.h"
#include "tinyxml.h"
#include "CXmlHelper.h"

CBalanceManager::CBalanceManager()
    : m_currentSetup(0)
{
    ResMap* resMap = ((App*)WindowApp::m_instance)->m_resMap;
    char* text = resMap->loadText(XString(L"res/common/configs/balance.xml"));
    TiXmlDocument document;
    document.Parse(text);
    if (text != NULL) {
        delete[] text;
    }
    Parse(document.FirstChild("balance_setups"));
    ChooseCurrentBalanceSetup();
}

void CBalanceManager::Parse(TiXmlNode* node)
{
    for (TiXmlNode* child = node->FirstChild("balance_setup"); child != NULL; child = child->NextSibling("balance_setup")) {
        SBalanceSetup setup;
        setup.m_name = CXmlHelper::GetAttributeValue(child, "name");
        setup.m_maxSceneUnitsCount = CXmlHelper::GetAttributeValue(child, "maxSceneUnitsCount").ToInt();
        setup.m_waveSizeMultiplier = CXmlHelper::GetAttributeValue(child, "waveSizeMultiplier").ToFloat();
        setup.m_zombieHPMultiplier = CXmlHelper::GetAttributeValue(child, "zombieHPMultiplier").ToFloat();
        setup.m_zombieDamageMultiplier = CXmlHelper::GetAttributeValue(child, "zombieDamageMultiplier").ToFloat();
        setup.m_civilianHPMultiplier = CXmlHelper::GetAttributeValue(child, "civilianHPMultiplier").ToFloat();
        setup.m_civilianDamageMultiplier = CXmlHelper::GetAttributeValue(child, "civilianDamageMultiplier").ToFloat();
        m_setups.addElement(setup);
    }
}

void CBalanceManager::ChooseCurrentBalanceSetup()
{
    XString setupName;
    if (App::m_platformType == 0) {
        setupName = L"low_performance";
    } else {
        setupName = L"high_performance";
    }
    m_currentSetup = -1;
    for (int index = 0; index < m_setups.size(); index++) {
        if (m_setups[index].m_name == setupName) {
            m_currentSetup = index;
            break;
        }
    }
    if (m_currentSetup < 0) {
        m_currentSetup = 0;
    }
}

CBalanceManager::~CBalanceManager()
{
}
