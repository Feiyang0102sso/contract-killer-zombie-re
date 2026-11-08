// CQuestManager.cpp - quest prerequisites, map selection and configured dialogs.
#include "CQuestManager.h"
#include "CXmlHelper.h"
#include "App.h"
#include "ResMap.h"
#include "CMissionManager.h"
#include "CBH_Player.h"

CQuestManager::CQuestManager()
{
    ResMap* resources = ((App*)WindowApp::m_instance)->m_resMap;
    char* text = resources->loadText(XString(L"res/common/configs/quests.xml"));
    TiXmlDocument document;
    document.Parse(text);
    delete[] text;
    Parse(document.FirstChild("quests"));
}

void CQuestManager::Parse(TiXmlNode* node)
{
    for (TiXmlNode* child = node->FirstChild("quest"); child != NULL; child = child->NextSibling("quest")) {
        SQuest quest;
        quest.m_id = CXmlHelper::GetAttributeValue(child, "id").ToInt();
        quest.m_screenName = Window::ResStringSafe(CXmlHelper::GetAttributeValue(child, "screenName"));
        ParseDialog(quest.m_gameDialogs, child, "gameDialog1", quest.m_screenName);
        ParseDialog(quest.m_gameDialogs, child, "gameDialog2", quest.m_screenName);
        quest.m_minXp = CXmlHelper::GetAttributeValue(child, "xpMin").ToInt();
        quest.m_requiredQuestId = CXmlHelper::GetAttributeValue(child, "requiredQuestId").ToInt();
        quest.m_locationName = CXmlHelper::GetAttributeValue(child, "locationName");
        quest.m_missionName = CXmlHelper::GetAttributeValue(child, "missionName");
        ((App*)WindowApp::m_instance)->m_missionManager->GetMissionByName(quest.m_missionName);
        m_quests.addElement(quest);
    }
}

void CQuestManager::ParseDialog(Vector<SDialog>& dialogs, TiXmlNode* node, const char* prefix, const XString& title)
{
    SDialog dialog;
    XString base(prefix);
    dialog.m_text = Window::ResStringSafe(CXmlHelper::GetAttributeValue(node, XString::AnsiString((base + L"Text").c_str())));
    dialog.m_title = title;
    if (dialog.m_text.ByteSize() != 0) {
        XString image = CXmlHelper::GetAttributeValue(node, XString::AnsiString((base + L"Image").c_str()));
        if (image.ByteSize() != 0) {
            dialog.m_image = ((App*)WindowApp::m_instance)->m_resMap->getResourceID(image);
        }
        dialog.m_condition = SDialog::GetDialogCondition(CXmlHelper::GetAttributeValueOrUseDefault(node,
            XString::AnsiString((base + L"Condition").c_str()), ""));
        dialogs.addElement(dialog);
    }
}

CQuestManager::SDialog::ECondition CQuestManager::SDialog::GetDialogCondition(XString condition)
{
    if (condition.ByteSize() == 0) {
        return CONDITION_ANY;
    }
    if (condition == XString("start")) {
        return CONDITION_START;
    }
    if (condition == XString("win")) {
        return CONDITION_WIN;
    }
    // Nonempty unknown conditions also mean start in the shipped code.
    return CONDITION_START;
}

CQuestManager::SQuest* CQuestManager::FindAvailableQuestForLocations(const Vector<XString>& locations)
{
    for (int index = 0; index < m_quests.size(); index++) {
        SQuest& quest = m_quests[index];
        if (locations.indexOf(quest.m_locationName, 0) != -1 &&
            !CBH_Player::GetInstance()->IsQuestCompleted(quest.m_id) &&
            CBH_Player::GetInstance()->GetStats((PLAYER_STATS_TYPE)3) >= quest.m_minXp) {
            if (quest.m_requiredQuestId < 1 || CBH_Player::GetInstance()->IsQuestCompleted(quest.m_requiredQuestId)) {
                return &quest;
            }
        }
    }
    return NULL;
}

CQuestManager::SQuest* CQuestManager::GetQuestById(int id)
{
    for (int index = 0; index < m_quests.size(); index++) {
        if (m_quests[index].m_id == id) {
            return &m_quests[index];
        }
    }
    return m_quests.m_items; // Original lookup falls back to the first record.
}

CQuestManager::~CQuestManager()
{
    // Nested vectors and strings are destroyed by the owning member.
}
