// CTjManager.cpp - persists Tapjoy presentation state and opens featured-app offers.
#include "CTjManager.h"
#include "App.h"
#include "CSaveManager.h"
#include "CXmlHelper.h"
#include "tinyxml.h"
#include "TapjoyInterface.h"
#include "CUtility.h"
#include "CFeaturedAppWindow.h"

CTjManager::CTjManager()
    : CSingleton(SingletonId)
{
    m_backgroundEntries = 0;
    m_elapsed = 0;
    m_shownAtLevelFour = false;
    Load();
}

void CTjManager::Load()
{
    CSaveManager* save = ((App*)WindowApp::m_instance)->m_saveManager;
    if (save == NULL) {
        return;
    }
    TiXmlNode* tapjoy = save->GetRootElement()->FirstChild("tapjoy");
    if (tapjoy == NULL) {
        return;
    }
    TiXmlNode* entries = tapjoy->FirstChild("bgEntriesCount");
    if (entries != NULL) {
        m_backgroundEntries = CXmlHelper::GetAttributeValue(entries, "count").ToInt();
    }
    TiXmlNode* shown = tapjoy->FirstChild("showed_on_4lvl");
    if (shown != NULL) {
        m_shownAtLevelFour = CXmlHelper::GetAttributeValue(shown, "showed").ToInt() != 0;
    }
}

void CTjManager::OnAppEnterBackground()
{
    int entries = m_backgroundEntries + 1;
    if (entries > 2) {
        entries = 0;
    }
    m_backgroundEntries = entries;
    Save();
}

void CTjManager::Save()
{
    TiXmlElement* tapjoy = CXmlHelper::ObtainElement(
        ((App*)WindowApp::m_instance)->m_saveManager->GetRootElement(), XString("tapjoy"));
    TiXmlElement* shown = CXmlHelper::ObtainElement(tapjoy, XString("showed_on_4lvl"));
    // A select preserves the original "1"/"0" address order and IT-ne store source.
    const char* value = m_shownAtLevelFour ? "1" : "0";
    shown->SetAttribute("showed", value);
    TiXmlElement* entries = CXmlHelper::ObtainElement(tapjoy, XString("bgEntriesCount"));
    entries->SetAttribute("count", m_backgroundEntries);
    ((App*)WindowApp::m_instance)->m_saveManager->SaveOnNextUpdate(0);
}

void CTjManager::CheckFeaturedAppCondition()
{
    // Original @0x13ba88 is empty; do not invent eligibility rules.
}

CTjManager::FeaturedAppData& CTjManager::GetFeaturedAppInfo()
{
    if (TapjoyInterface::getFeaturedApp() != NULL) {
        m_featured.m_name = TapjoyInterface::getFeaturedApp()->m_name.GetWCharPtr();
        m_featured.m_description = TapjoyInterface::getFeaturedApp()->m_description.GetWCharPtr();
        m_featured.m_imageUrl = TapjoyInterface::getFeaturedApp()->m_iconURL.GetWCharPtr();
        m_featured.m_redirectUrl = TapjoyInterface::getFeaturedApp()->m_redirectURL.GetWCharPtr();
        m_featured.m_amount = TapjoyInterface::getFeaturedApp()->m_amount;
    }
    return m_featured;
}

void CTjManager::Update(int elapsed)
{
    if (m_queryState == 1) {
        if (!TapjoyInterface::getFeaturedAppQueryState()) {
            if (TapjoyInterface::getFeaturedApp() != NULL) {
                m_queryState = 2;
            } else {
                m_queryState = 4;
            }
        }
    }
    if (m_queryState == 2) {
        m_shownAtLevelFour = true;
        Save();
        FeaturedAppData featured;
        m_queryState = 0;
        featured = GetFeaturedAppInfo();
        CFeaturedAppWindow* window = new CFeaturedAppWindow();
        XString format(CUtility::GetString("IDS_FI_DIALOG_TEXT").GetWCharPtr());
        XString upperName = featured.m_name.Upper();
        XString description = XString::Format(format.c_str(), featured.m_amount, upperName.c_str());
        window->SetDescriptionText(description);
        window->SetImageUrl(featured.m_imageUrl);
        window->SetRedirectUrl(featured.m_redirectUrl);
        window->SetAlign(0x24);
        ((App*)WindowApp::m_instance)->m_mainLayer->AddModal(window);
    }
    m_elapsed = m_elapsed + elapsed;
}
