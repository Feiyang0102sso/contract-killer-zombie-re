// CLocationManager.cpp - owns the location descriptions indexed by their names.
#include "CLocationManager.h"
#include "CLocation.h"
#include "App.h"
#include "ResMap.h"
#include "tinyxml.h"

CLocationManager::CLocationManager()
{
    ResMap* resources = ((App*)WindowApp::m_instance)->m_resMap;
    char* text = resources->loadText(XString(L"res/common/configs/location.xml"));
    TiXmlDocument document;
    document.Parse(text);
    if (text != NULL) {
        delete[] text;
    }
    for (TiXmlNode* node = document.FirstChild("location"); node != NULL; node = node->NextSibling("location")) {
        CLocation* location = new CLocation(node);
        m_locations.Set(location->m_name, location);
    }
}

CLocationManager::~CLocationManager()
{
    m_locations.DeleteContents();
}

CLocation* CLocationManager::GetLocation(const XString& name) const
{
    return *m_locations.Get(name);
}
