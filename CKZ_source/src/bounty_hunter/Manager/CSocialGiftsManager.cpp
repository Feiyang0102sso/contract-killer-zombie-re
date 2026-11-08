// CSocialGiftsManager.cpp - gift configuration and the shop managers that grant it.
#include "CSocialGiftsManager.h"
#include "App.h"
#include "ResMap.h"
#include "tinyxml.h"
#include "CXmlHelper.h"
#include "CDH_WeaponsManager.h"
#include "CEnergyManager.h"

CSocialGiftManager::CSocialGiftManager()
{
    ParseConfig();
}

void CSocialGiftManager::ParseConfig()
{
    char* text = ((App*)WindowApp::m_instance)->m_resMap->loadText(XString(L"res/common/configs/gserve_gifts.xml"));
    TiXmlDocument document;
    document.Parse(text, NULL, TIXML_ENCODING_UNKNOWN);
    if (text != NULL) {
        free(text);
    }
    const char* giftTag = "gift";
    TiXmlNode* node = document.FirstChild("gifts")->FirstChild(giftTag);
    while (node != NULL) {
        XString type = CXmlHelper::GetAttributeValue(node, "type");
        CItemManager* manager = GetItemManager(type);
        if (manager != NULL) {
            SGift* gift = new SGift;
            gift->m_itemManager = manager;
            gift->m_itemId = CXmlHelper::GetAttributeValue(node, "item_id").ToInt();
            m_gifts.addElement(gift);
        }
        node = node->NextSibling(giftTag);
    }
}

CItemManager* CSocialGiftManager::GetItemManager(XString& type)
{
    if (type == XString("weapon")) {
        return ((App*)WindowApp::m_instance)->m_weaponsManager;
    }
    if (type == XString("energy")) {
        return ((App*)WindowApp::m_instance)->m_energyManager;
    }
    return NULL;
}

CSocialGiftManager::~CSocialGiftManager()
{
    SAFE_DELETE_VECTOR(m_gifts);
}

int CSocialGiftManager::GetGiftsCount()
{
    return m_gifts.size();
}

SGift* CSocialGiftManager::GetGift(int index)
{
    if (index < m_gifts.size()) {
        return m_gifts[index];
    }
    return NULL;
}
