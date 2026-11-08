// CTipManager.cpp - owns the loading tips read from tips.xml.
#include "CTipManager.h"
#include "CTip.h"
#include "App.h"
#include "ResMap.h"
#include "tinyxml.h"
#include "CRandGen.h"

CTipManager::CTipManager()
{
    ResMap* resources = ((App*)WindowApp::m_instance)->m_resMap;
    char* text = resources->loadText(XString(L"res/common/configs/tips.xml"));
    TiXmlDocument document;
    document.Parse(text);
    if (text != NULL) {
        delete[] text;
    }
    for (TiXmlNode* node = document.FirstChild("tip"); node != NULL; node = node->NextSibling("tip")) {
        CTip* tip = new CTip(node);
        m_tips.addElement(tip);
    }
}

CTipManager::~CTipManager()
{
    SAFE_DELETE_VECTOR(m_tips);
}

CTip* CTipManager::GetRandomTip() const
{
    int count = m_tips.size();
    if (count < 1) {
        return NULL;
    }
    int index = com::glu::platform::core::CRandGen::GetInstance()->GetRandRange(0, count - 1);
    if (index >= 0 && index < count) {
        return m_tips[index];
    }
    if (index < 0) {
        index = 0;
    } else if (index > count - 1) {
        index = count - 1;
    }
    return m_tips[index];
}
