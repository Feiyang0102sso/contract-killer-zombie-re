// CMission.cpp - mission configuration, rewards, conditions and UI icon selection.
#include "CMission.h"
#include "CXmlHelper.h"
#include "App.h"
#include "WindowBase.h"
#include "DGHelper.h"
#include "CLocationManager.h"
#include "CSceneSetupsManager.h"
#include "CBH_Player.h"
#include "CHuntingInfo.h"
#include "CProgressSystem.h"
#include "CZombieDialogWindow.h"

// Original __DATA,__data at 0x4ff3f0; the short codes are not expanded guesses.
const char* MISSION_TYPE_MAP[7] = { "", "KA", "IC", "CB", "ST", "QU", "EM" };

CMission::CMission(TiXmlNode* node)
{
    Parse(node);
}

void CMission::Parse(TiXmlNode* node)
{
    TiXmlNode* info = node->FirstChild("info");
    if (info != NULL) {
        ParseInfo(info);
    } else {
        ParseInfo(node);
    }
}

void CMission::ParseInfo(TiXmlNode* node)
{
    m_name = CXmlHelper::GetAttributeValue(node, "name");
    m_sceneSetupName = CXmlHelper::GetAttributeValue(node, "sceneSetupName");
    m_zombieMissionName = CXmlHelper::GetAttributeValue(node, "zombieMissionName");
    m_locationName = CXmlHelper::GetAttributeValue(node, "locationName").Trim(true, true);
    m_screenName = Window::ResStringSafe(CXmlHelper::GetAttributeValue(node, "screenName"));
    m_description = Window::ResStringSafe(CXmlHelper::GetAttributeValue(node, "description"));
    m_descriptionSound = CXmlHelper::GetAttributeValueOrUseDefault(node, "descriptionSound", XString(L""));
    m_successSound = CXmlHelper::GetAttributeValueOrUseDefault(node, "successSound", XString(L""));
    m_type = StringToMissionType(CXmlHelper::GetAttributeValue(node, "type"));
    m_difficulty = (EMissionDifficulty)CXmlHelper::GetAttributeValue(node, "difficulty").ToInt();
    m_autopickable = CXmlHelper::GetAttributeValue(node, "isAutopickable").ToBool();
    m_withFriend = node->ToElement()->Attribute("withFriend") != NULL;

    XString level = CXmlHelper::GetAttributeValue(node, "level");
    Vector<XString> limits;
    DGHelper::SplitStringAndTrim(limits, level, XString("-"));
    m_minLevel = limits[0].ToInt();
    m_maxLevel = m_minLevel;
    if (limits.size() > 1) {
        m_maxLevel = limits[1].ToInt();
    }
    m_reward.Load(node, XString("reward_"));
    m_minPeople = CXmlHelper::GetAttributeValueOrUseDefault(node, "min_people", XString(L"0")).ToInt();
    m_minProvision = CXmlHelper::GetAttributeValueOrUseDefault(node, "min_provision", XString(L"0")).ToInt();
    m_scriptName = CXmlHelper::GetAttributeValue(node, "scriptName");
    XString scriptParams = CXmlHelper::GetAttributeValue(node, "scriptParams");
    CScriptManager::ParseScriptParams(m_scriptParams, scriptParams);
    // Original branches construct one temporary and share its Assign/Release at 0x12c512.
    m_descriptionWithFriend = node->ToElement()->Attribute("descriptionWithFriend") != NULL
        ? CXmlHelper::GetAttributeValue(node, "descriptionWithFriend")
        : XString(L"");
    m_numWaves = CXmlHelper::GetAttributeValueOrUseDefault(node, "numWaves", XString(L"0")).ToInt();
    m_waveFilter = CXmlHelper::GetAttributeValueOrUseDefault(node, "waveFilter", XString(L""));
    m_tutorialMultiplier = 1.0f;
    if (m_name == XString("TUTORIAL_MISSION_2") || m_name == XString("TUTORIAL_MISSION_3") ||
        m_name == XString("TUTORIAL_MISSION_4")) {
        m_tutorialMultiplier = 2.0f;
    }
    ((App*)WindowApp::m_instance)->m_sceneSetupsManager->GetSceneSetupByName(m_sceneSetupName);
    ((App*)WindowApp::m_instance)->m_locationManager->GetLocation(m_locationName);
}

CMission::EMissionType CMission::StringToMissionType(const XString& name)
{
    for (int index = 0; index < 7; index++) {
        if (XString(MISSION_TYPE_MAP[index]) == name) {
            return (EMissionType)index;
        }
    }
    return MISSION_ANY;
}

int CMission::GetRewardPeople() const
{
    return m_reward.m_values[8].Get();
}

int CMission::GetRewardProvision() const
{
    return m_reward.m_values[6].Get();
}

bool CMission::IsAutopickable() const
{
    return m_autopickable;
}

bool CMission::IsSuitableForLevel(int level) const
{
    return m_minLevel <= level && level <= m_maxLevel;
}

void CMission::OnMissionCompleted(int money, int xp) const
{
    CBH_Player* player = CBH_Player::GetInstance();
    CHuntingInfo* hunt = ((App*)WindowApp::m_instance)->m_huntingInfo;
    int boxesSent = 0;
    int peopleSent = 0;
    if (m_type == MISSION_ST) {
        boxesSent = hunt->ProvisionToSend();
        peopleSent = hunt->PeopleSent();
        player->m_gameStats.AddBoxesSent(boxesSent);
        player->m_gameStats.AddPeopleSent(hunt->PeopleToSend());
        player->ReducePeople(player->GetStats(PLAYER_STATS_PEOPLE));
        player->ReduceProvision(player->GetStats(PLAYER_STATS_PROVISION));
    }
    if (money < 0) {
        money = ((App*)WindowApp::m_instance)->m_progressSystem->GetMissionRewardMoney(m_type, m_difficulty, peopleSent, boxesSent);
    }
    if (xp < 0) {
        xp = ((App*)WindowApp::m_instance)->m_progressSystem->GetMissionRewardXP(m_type, m_difficulty, peopleSent, boxesSent);
    }
    player->AddMoney(money);
    player->AddXp(xp);
    int peopleEarned = hunt->PeopleEarned();
    int provisionEarned = hunt->ProvisionEarned();
    player->AddPeople(peopleEarned);
    player->AddProvision(provisionEarned);
    player->OnMissionCompleted(this);
    player->m_gameStats.RecordMissionRewards(provisionEarned, peopleEarned, peopleSent, boxesSent, money, xp);
}

void CMission::OnMissionStarted() const
{
    CBH_Player* player = CBH_Player::GetInstance();
    if (m_type == MISSION_ST) {
        int people = player->GetStats(PLAYER_STATS_PEOPLE);
        ((App*)WindowApp::m_instance)->m_huntingInfo->PeopleToSend() = people;
        int provision = player->GetStats(PLAYER_STATS_PROVISION);
        ((App*)WindowApp::m_instance)->m_huntingInfo->ProvisionToSend() = provision;
    }
}

int CMission::GetBaseRewardMoney() const
{
    return ((App*)WindowApp::m_instance)->m_progressSystem->GetMissionRewardMoney(m_type, m_difficulty, 0, 0);
}

int CMission::GetBaseRewardXP() const
{
    return ((App*)WindowApp::m_instance)->m_progressSystem->GetMissionRewardXP(m_type, m_difficulty, 0, 0);
}

int CMission::GetMapIconAnimation() const
{
    bool special = m_name.Lower().Find(XString("rt1h"), 0) != -1 ||
                   m_name.Lower().Find(XString("rt2h"), 0) != -1 ||
                   m_name.Lower().Find(XString("rt3h"), 0) != -1;
    bool repeat = m_name.Lower().Find(XString("rt"), 0) != -1;
    switch (m_type) {
    case MISSION_KA:
        if (repeat) {
            return 0x0d;
        }
        return 0;
    case MISSION_IC:
        if (special) {
            return 0x0e;
        }
        return 3;
    case MISSION_CB: return 2;
    case MISSION_ST: return 1;
    case MISSION_EM: return 0x0c;
    default: return 0;
    }
}

int CMission::GetDialogDifficultyIconAnimation() const
{
    switch (m_difficulty) {
    case DIFFICULTY_EASY: return 0x3b;
    case DIFFICULTY_MEDIUM: return 0x3c;
    case DIFFICULTY_HARD: return 0x3d;
    default: return -1;
    }
}

XString CMission::GetDifficultyString() const
{
    switch (m_difficulty) {
    case DIFFICULTY_EASY: return Window::ResString("IDS_MISSION_DIFFICULTY_EASY");
    case DIFFICULTY_MEDIUM: return Window::ResString("IDS_MISSION_DIFFICULTY_MEDIUM");
    case DIFFICULTY_HARD: return Window::ResString("IDS_MISSION_DIFFICULTY_HARD");
    default: return XString();
    }
}

int CMission::GetMapIconDifficultyAnimation() const
{
    switch (m_difficulty) {
    case DIFFICULTY_EASY: return 9;
    case DIFFICULTY_MEDIUM: return 10;
    case DIFFICULTY_HARD: return 11;
    default: return -1;
    }
}

XString CMission::MissionTypeToString(EMissionType type)
{
    if ((unsigned int)type >= 7) {
        return XString();
    }
    return XString(MISSION_TYPE_MAP[type]);
}

bool CMission::CheckMissionConditions(CZombieDialogWindow** dialog) const
{
    CBH_Player* player = CBH_Player::GetInstance();
    XString text;
    if (m_minProvision > player->GetStats(PLAYER_STATS_PROVISION) ||
        m_minPeople > player->GetStats(PLAYER_STATS_PEOPLE)) {
        {
            // Original 0x12cbe0 releases the format string before constructing the dialog.
            XString format = Window::ResString("IDS_NOT_ENOUGH_PEOPLE_OR_PROVISION");
            text.Append(XString::Format(format.c_str(), player->GetStats(PLAYER_STATS_PROVISION), m_minProvision,
                                        player->GetStats(PLAYER_STATS_PEOPLE), m_minPeople));
        }
        *dialog = new CZombieDialogWindow(3);
        (*dialog)->SetTitle(Window::ResString("IDS_MISSION_CONDITIONS_FAILED_TITLE"));
        (*dialog)->SetText(text, true, NULL);
        (*dialog)->AddButton(CZombieDialogWindow::BUTTON_OK, 0xffffffff, 0, 0, true, 0x14);
        return false;
    }
    // The successful path leaves the caller's dialog output untouched.
    return true;
}
