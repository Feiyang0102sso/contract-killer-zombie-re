// b074i: complete original owner 11a844..11c984; keeps the original side effects, temporaries and post-callback rereads.
#include "CGPSMapGame.h"
#include "CBH_GPSHud.h"
#include "CBH_NetworkWindow.h"
#include "CBH_Player.h"
#include "CLevelUpWindow.h"
#include "CIncentivizedWindow.h"
#include "CMissionInfoZombieDialogWindow.h"
#include "CQuestZombieDialogWindow.h"
#include "COneTimeOfferDialogWindow.h"
#include "CDialogQueueWindow.h"
#include "CLoadingScreen.h"
#include "CXmlHelper.h"
#include "COffersManager.h"
#include "CMissionManager.h"
#include "CTutorialManager.h"
#include "CGCLeaderboardsManager.h"
#include "CGameAnalytics.h"
#include "CNewsFeedManager.h"
#include "CShopPackManager.h"
#include "CDH_WeaponsManager.h"
#include "CDH_SoundManager.h"
#include "CShopItem.h"
#include "CHuntingInfo.h"
#include "CBH_SurfaceManager.h"
#include "SScreenAdaptingConstant.h"
#include "ResMap.h"
#include "tinyxml.h"
#include "CRandGen.h"
#include "CPHInterface.h"
#include "TapjoyInterface.h"
#include "DGHelper.h"
#include "App.h"
#include "ICRenderSurface.h"

using namespace com::glu::platform::core;
using namespace com::glu::platform::graphics;

// At original 45dd5c only the first 4 bytes belong to this variable; an independent UTF32 path follows, do not copy 152B.
const int EventId_StartTargettingMission = 0x4cb40874;

CGPSMapGame::CGPSMapGame() : m_availableQuest(NULL), m_tapjoyUpdater(this)
{
    m_selectedQuestId = -1;
    m_selectedFriendId = -1;
    m_selectedFriendName = L"";
    m_selectedLocation = NULL;
    m_targetMessage = NULL;
    m_targetMessageIndex = -1;
    m_hud = new CBH_GPSHud;
    m_hud->SetLayoutType(Window::LAYOUT_SIMPLE);
    m_hud->SetAlign(9);
    AddToFront(m_hud);
    m_dialogQueue = new CDialogQueueWindow(((App*)WindowApp::m_instance)->m_mainLayer);
    AddToFront(m_dialogQueue);
    CGameAnalytics::flushDeferredEvents();
}

CGPSMapGame::~CGPSMapGame()
{
    // Members are destroyed in order: SharedPtr, timer, strings and three non-owning Vectors; the entities belong to the Window tree.
}

void CGPSMapGame::AddLevelUpDialogInQueue(int unused)
{
    CDialogQueueWindow::AddDialogInQueue(new CLevelUpWindow);
    COffersManager* offers = ((App*)WindowApp::m_instance)->m_offersManager;
    offers->m_offersChanged = true;
    offers->m_activeDealIndex = -1;
}

void CGPSMapGame::Init()
{
    CBH_Player* player = CBH_Player::GetInstance();
    CreateMapLocations();
    CBH_GPSMap::Init();
    if (((App*)WindowApp::m_instance)->m_huntingInfo->m_field00) {
        XString itemName(((App*)WindowApp::m_instance)->m_huntingInfo->m_text04);
        // App's current external thin declaration still calls it CShopItem; the virtual getId of the real returned item is CItem+50.
        CItem* item = (CItem*)App::GetItemByStringUId(itemName);
        CShopWindow::ETab tab = CShopWindow::GetTabForItem(item);
        int itemId = item->getId();
        ShowShop(tab, itemId, false);
        return;
    }
    if (((App*)WindowApp::m_instance)->m_tutorialManager->IsShowShop() == 1) {
        int tab = ((App*)WindowApp::m_instance)->m_tutorialManager->GetCurrentShopTab();
        ShowShop((CShopWindow::ETab)tab, -1, false);
    } else if (((App*)WindowApp::m_instance)->m_tutorialManager->IsShowHome() == 1) {
        CBH_NetworkWindow* network = new CBH_NetworkWindow;
        m_parent->AddModal(network);
    }
    CMapLocation* completedLocation = NULL;
    for (int index = 0; index < m_locations.size(); index++) {
        if (m_locations[index]->m_id == player->FieldAt(0x60)) {
            completedLocation = m_locations[index];
        }
    }
    if (player->FieldAt(0x5c) != 0) {
        if (player->FieldAt(0x5c) == 2) {
            if (player->FieldAt(0x64) >= 0) {
                ShowQuestFinalDialogs(player->FieldAt(0x64));
            }
            if (completedLocation != NULL) {
                CMissionManager* manager = ((App*)WindowApp::m_instance)->m_missionManager;
                const CMission* mission = manager->GetMissionByName(completedLocation->GetCurrentMission());
                if (mission != NULL) {
                    completedLocation->CompleteMission();
                    if (mission->m_type == CMission::MISSION_ST) {
                        StartNewCycle();
                    }
                }
            }
        }
        player->FieldAt(0x5c) = 0;
        player->Save(true);
    }
    PopulateMapLocationsFromPool();
    PopulateMapLocationsWithEndlessModeMission();
    CTutorialManager* tutorial = ((App*)WindowApp::m_instance)->m_tutorialManager;
    if (tutorial->IsPlayerInTutorial() == 1) {
        PopulateTutorialLocations();
    }
    tutorial->ShowStateInitMessage();
    if (tutorial->OnlyTutorialMissions() == 0) {
        SearchForAvailableQuests();
    }
    CBH_GPSMap::Save();
    if (((App*)WindowApp::m_instance)->m_huntingInfo->m_levelIncreased) {
        AddLevelUpDialogInQueue(0); // The original argument is not read by the callee.
        if (((App*)WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial() == 0) {
            AddRecommendedWeaponDialogInQueue();
        }
    }
    m_timer.add(&m_tapjoyUpdater, 5000);
    UpdateTutorialAttractions();
    if (((App*)WindowApp::m_instance)->m_offersManager->ShouldBeShown() == 1) {
        COneTimeOfferDialogWindow* dialog = ((App*)WindowApp::m_instance)->m_offersManager->ShowOTODialog();
        if (dialog != NULL) {
            CDialogQueueWindow::AddDialogInQueue(dialog);
        }
    }
    ((App*)WindowApp::m_instance)->m_leaderboardsManager->ReloadLeaderboards();
}

void CGPSMapGame::CreateMapLocations()
{
    TiXmlDocument document = CXmlHelper::Parse(L"res/common/configs/map_locations.xml");
    const TiXmlNode* node = document.FirstChild("map_locations")->FirstChild("map_location");
    Vector<int> locationIds;
    while (node != NULL) {
        CMapLocation* location = new CMapLocation((TiXmlNode*)node);
        int locationId = location->m_id;
        locationIds.addElement(locationId);
        location->SetMapLocationListener(this);
        if (location->m_withFriend) {
            // SAFE_DELETE style: the NULL store is skipped when location is NULL (original beq past it).
            if (location != NULL) {
                delete location;
                location = NULL;
            }
        } else {
            AddGPSItem(location);
            m_locations.addElement(location);
            if (location->m_hasQuestImage) {
                CQuestMapLocation* quest = new CQuestMapLocation(location);
                quest->SetQuestMapLocationListener(this);
                AddGPSItem(quest);
                m_questLocations.addElement(quest);
            }
        }
        node = node->NextSiblingElement("map_location");
    }
}

void CGPSMapGame::ShowShop(CShopWindow::ETab tab, int itemId, bool tapjoy)
{
    CShopWindow* shop = CShopWindow::GetInstance();
    shop->SetTab(tab, itemId);
    m_parent->AddModal(shop);
    if (tapjoy) {
        shop->TryOpenTapJoy();
    }
    // Original tests the non-bank tab first (beq to the bank block).
    if (tab != CShopWindow::TAB_2) {
        if (!CPHInterface::isPublisherContentRequestActive("store_launch")) {
            CPHInterface::startPublisherContentRequest("store_launch", false, false);
        }
    } else if (!CPHInterface::isPublisherContentRequestActive("bank_launch")) {
        CPHInterface::startPublisherContentRequest("bank_launch", false, false);
    }
}

void CGPSMapGame::ShowQuestFinalDialogs(int questId)
{
    CQuestManager::SQuest* quest = ((App*)WindowApp::m_instance)->m_questManager->GetQuestById(questId);
    const CMission* mission = ((App*)WindowApp::m_instance)->m_missionManager->GetMissionByName(quest->m_missionName);
    if (mission != NULL && mission->m_successSound.ByteSize() != 0) {
        ResetBriefingSound();
        CDH_SoundManager* manager = ((App*)WindowApp::m_instance)->m_soundManager;
        m_briefingTrack = manager->PlayTrack(XString::AnsiString(mission->m_successSound.c_str()), true);
    }
    for (int index = 0; index < quest->m_gameDialogs.size(); index++) {
        if (quest->m_gameDialogs[index].m_condition == CQuestManager::SDialog::CONDITION_WIN) {
            CDialogQueueWindow::AddDialogInQueue(new CQuestZombieDialogWindow(quest->m_gameDialogs[index]));
        }
    }
}

void CGPSMapGame::StartNewCycle()
{
    if (((App*)WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial() != 0) {
        return;
    }
    CBH_Player* player = CBH_Player::GetInstance();
    int level = player->GetLevel();
    int score = player->m_gameStats.GetThisCycleScore();
    int value = player->FieldAt(0x188) + player->FieldAt(0x18c) + player->FieldAt(0x190);
    CGameAnalytics::logCycleScoreAchieved(level, score, value);
    player->m_gameStats.NotifyNewCycleBegin();
    player->FieldAt(0x90)++;
    if (!player->RateMeShown() && player->FieldAt(0x90) == 2) {
        CZombieDialogWindow* dialog = new CZombieDialogWindow(3);
        dialog->SetTitle(Window::ResString("IDS_RATE_ME_HEADER"));
        dialog->SetText(Window::ResString("IDS_RATE_ME_TEXT"), false, NULL);
        dialog->AddButton((CZombieDialogWindow::EDialogButton)10, 0x7e145385, 0, 0, true, 20);
        dialog->AddButton((CZombieDialogWindow::EDialogButton)4, (unsigned int)-1, 0, 0, true, 20);
        CDialogQueueWindow::AddDialogInQueue(dialog);
        player->RateMeShown() = true;
        player->Save(true);
    }
    player->RefreshMissionsPool();
    ClearLocations();
    PopulateMapLocationsFromPool();
    PopulateMapLocationsWithKillAllMission();
    PopulateMapLocationsWithEndlessModeMission();
}

void CGPSMapGame::PopulateMapLocationsFromPool()
{
    if (((App*)WindowApp::m_instance)->m_tutorialManager->OnlyTutorialMissions() == 1) {
        PopulateTutorialLocations();
    } else {
        CBH_Player* player = CBH_Player::GetInstance();
        Vector<CMissionsPool::SPoolElement>& pool = player->MissionsPool().m_elements;
        for (int index = 0; index < pool.size(); index++) {
            int type = pool[index].m_type;
            int difficulty = pool[index].m_difficulty;
            Vector<CMapLocation*> locations;
            for (int locationIndex = 0; locationIndex < m_locations.size(); locationIndex++) {
                if (!m_locations[locationIndex]->HasMission()
                    && m_locations[locationIndex]->m_missionType == type) {
                    locations.addElement(m_locations[locationIndex]);
                }
            }
            while (locations.size() > 0) {
                int selectedIndex = CRandGen::GetInstance()->GetRand(locations.size());
                CMapLocation* location = locations[selectedIndex];
                Vector<const CMission*> missions;
                ((App*)WindowApp::m_instance)->m_missionManager->GetSuitableMissionsForLocation(
                    missions, location, (CMission::EMissionDifficulty)difficulty, (CMission::EMissionType)type);
                if (missions.size() == 0) {
                    locations.removeElementAt(selectedIndex);
                } else {
                    unsigned int missionIndex = CRandGen::GetInstance()->GetRand(missions.size());
                    location->SetMission(missions[missionIndex]->m_name, false);
                    pool.removeElementAt(index);
                    index--;
                    break;
                }
            }
        }
        CBH_Player::GetInstance()->Save(true);
    }
    CBH_GPSMap::Save();
}

void CGPSMapGame::PopulateMapLocationsWithEndlessModeMission()
{
    Vector<CMapLocation*> locations;
    for (int index = 0; index < m_locations.size(); index++) {
        if (m_locations[index]->m_missionType == CMission::MISSION_EM) {
            locations.addElement(m_locations[index]);
        }
    }
    if (locations.size() != 0) {
        for (int attempt = 0; attempt < 20; attempt++) {
            CMapLocation* location = locations[CRandGen::GetInstance()->GetRand(locations.size())];
            Vector<const CMission*> missions;
            ((App*)WindowApp::m_instance)->m_missionManager->GetSuitableMissionsForLocation(
                missions, location, CMission::DIFFICULTY_ANY, CMission::MISSION_EM);
            if (missions.size() != 0) {
                unsigned int index = CRandGen::GetInstance()->GetRand(missions.size());
                location->SetMission(missions[index]->m_name, true);
                break;
            }
        }
        CBH_GPSMap::Save();
    }
}

void CGPSMapGame::PopulateTutorialLocations()
{
    CTutorialManager* tutorial = ((App*)WindowApp::m_instance)->m_tutorialManager;
    if (tutorial->IsPlayerInTutorial() == 1) {
        ClearLocations();
        const char* name = tutorial->GetActiveTutorialMissionName();
        if (name != NULL) {
            CMissionManager* manager = ((App*)WindowApp::m_instance)->m_missionManager;
            const CMission* mission = manager->GetMissionByName(XString(name));
            CMapLocation* location = LocationByName(mission->m_locationName);
            location->SetMission(XString(name), false);
            location->SetVisible(true);
            location->SetBlinking(true);
        }
    }
}

void CGPSMapGame::SearchForAvailableQuests()
{
    for (int index = 0; index < m_questLocations.size(); index++) {
        if (m_questLocations[index]->CheckForQuests() == 1) {
            m_availableQuest = m_questLocations[index];
            SetMapLocationsVisible(false, CMission::MISSION_ST);
        }
    }
}

void CGPSMapGame::AddRecommendedWeaponDialogInQueue()
{
    Vector<int> weapons;
    CDH_WeaponsManager* weaponManager = ((App*)WindowApp::m_instance)->m_weaponsManager;
    weaponManager->GetUnlockedWeaponsIdOnLevel(CBH_Player::GetInstance()->GetLevel(), weapons);
    Vector<int> packs;
    CGrenadePackManager* packManager = ((App*)WindowApp::m_instance)->m_grenadePackManager;
    packManager->GetUnlockedGrenadePackIdsOnLevel(CBH_Player::GetInstance()->GetLevel(), packs);
    int grenade = -1;
    if (packs.size() > 0) {
        grenade = packs[0];
    }
    int weapon = -1;
    if (weapons.size() > 0) {
        weapon = weapons[0];
    }
    // The original tests the sign bit after the AND, which is equivalent to creating when either valid non-negative id exists; both are not required.
    if ((weapon & grenade) >= 0) {
        CDialogQueueWindow::AddDialogInQueue(new CRecommendedWeaponDialog(weapon, grenade));
    }
}

void CGPSMapGame::UpdateTutorialAttractions()
{
    if (((App*)WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial() == 1
        && ((App*)WindowApp::m_instance)->m_tutorialManager->GetActiveTutorialMissionName() != NULL) {
        for (int index = 0; index < m_locations.size(); index++) {
            CMapLocation* location = m_locations[index];
            if (location->HasMission() == 1) {
                bool matches = location->GetCurrentMission() == XString(
                    ((App*)WindowApp::m_instance)->m_tutorialManager->GetActiveTutorialMissionName());
                if (matches) {
                    location->SetAttractSpeed(0.4f);
                }
            }
        }
    }
}

void CGPSMapGame::OnMapLocationSelected(CMapLocation* location)
{
    m_selectedQuestId = -1;
    m_selectedFriendId = -1;
    m_selectedFriendName = L"";
    m_targetMessage = NULL;
    m_targetMessageIndex = -1;
    m_selectedLocation = location;
    if (location->HasMission() == 1) {
        CMissionManager* manager = ((App*)WindowApp::m_instance)->m_missionManager;
        const CMission* mission = manager->GetMissionByName(location->GetCurrentMission());
        if (location->m_withFriend) {
            ShowSelectFriendDialog();
        } else if (mission->m_type == CMission::MISSION_ST && m_availableQuest != NULL) {
            ShowCompleteQuestFirstDialog();
        } else {
            // As in the original, first obtain the by-value string, then query description; the callback can change the field.
            ShowMissionStartDialog(location->GetCurrentMission(), location->GetCurrentMissionDescriptionId());
        }
    }
}

void CGPSMapGame::ShowCompleteQuestFirstDialog()
{
    CZombieDialogWindow* dialog = new CZombieDialogWindow(3);
    dialog->SetTitle(Window::ResString("IDS_MAP_DIALOG_TITLE"));
    dialog->SetText(Window::ResString("IDS_MAP_DIALOG_COMPLETE_QUEST"), false, NULL);
    dialog->AddButton(CZombieDialogWindow::BUTTON_OK, (unsigned int)-1, 0, 0, true, 20);
    CDialogQueueWindow::AddDialogInQueue(dialog);
    m_availableQuest->SetAttractSpeed(0.4f);
}

void CGPSMapGame::ShowMissionStartDialog(XString missionName, int descriptionId)
{
    const CMission* mission = ((App*)WindowApp::m_instance)->m_missionManager->GetMissionByName(missionName);
    if (mission->m_descriptionSound.ByteSize() != 0) {
        ResetBriefingSound();
        CDH_SoundManager* manager = ((App*)WindowApp::m_instance)->m_soundManager;
        m_briefingTrack = manager->PlayTrack(XString::AnsiString(mission->m_descriptionSound.c_str()), true);
    }
    if (((App*)WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial() != 0) {
        descriptionId = -1;
    }
    Window* dialog;
    if (mission->m_type == CMission::MISSION_EM) {
        dialog = new CEndlessModeInfoZombieDialogWindow(mission, descriptionId);
    } else {
        dialog = new CMissionInfoZombieDialogWindow(mission, m_briefingTrack,
            descriptionId, m_selectedFriendId, m_selectedFriendName);
    }
    AddModal(dialog);
}

void CGPSMapGame::OnQuestMapLocationSelected(CQuestMapLocation* location)
{
    m_selectedQuestId = -1;
    m_selectedFriendId = -1;
    m_selectedFriendName = L"";
    m_selectedLocation = NULL;
    m_targetMessage = NULL;
    m_targetMessageIndex = -1;
    int questId = location->m_questId;
    if (questId > 0) {
        m_selectedQuestId = questId;
        CQuestManager::SQuest* quest = ((App*)WindowApp::m_instance)->m_questManager->GetQuestById(questId);
        for (int index = 0; index < quest->m_dialogs.size(); index++) {
            CDialogQueueWindow::AddDialogInQueue(new CQuestZombieDialogWindow(quest->m_dialogs[index]));
        }
        CDialogQueueWindow* queue = m_dialogQueue;
        queue->m_finishCommand = 0x33e56be7;
        queue->m_finishParameters[0] = 0;
        queue->m_finishParameters[1] = 0;
        queue->m_finishParameters[2] = 0;
    }
}

void CGPSMapGame::OnSafehouseItemSelected(CGPSSafehouse* safehouse)
{
    CBH_NetworkWindow* network = new CBH_NetworkWindow;
    m_parent->AddModal(network);
}

void CGPSMapGame::OnShopItemSelected(CGPSShop* shop)
{
    ShowShop(CShopWindow::TAB_0, -1, false);
}

void CGPSMapGame::SetMapLocationsVisible(bool visible, CMission::EMissionType type)
{
    for (int index = 0; index < m_locations.size(); index++) {
        if (m_locations[index]->HasMission() == 1) {
            CMissionManager* manager = ((App*)WindowApp::m_instance)->m_missionManager;
            const CMission* mission = manager->GetMissionByName(m_locations[index]->GetCurrentMission());
            if (mission->m_type == type) {
                m_locations[index]->SetVisible(visible);
            }
        }
    }
}

void CGPSMapGame::ClearLocations()
{
    for (int index = 0; index < m_locations.size(); index++) {
        CMapLocation* location = m_locations[index];
        location->ClearMissions();
        location->SetVisible(false);
    }
    for (int index = 0; index < m_extraLocations.size(); index++) {
        CMapLocation* location = m_extraLocations[index];
        location->ClearMissions();
        location->SetVisible(false);
    }
}

void CGPSMapGame::PopulateMapLocationsWithKillAllMission()
{
    Vector<CMapLocation*> locations;
    for (int index = 0; index < m_locations.size(); index++) {
        if (m_locations[index]->HasMission() == 1) {
            locations.addElement(m_locations[index]);
        }
    }
    if (locations.size() != 0) {
        for (int attempt = 0; attempt < 20; attempt++) {
            CMapLocation* location = locations[CRandGen::GetInstance()->GetRand(locations.size())];
            CMissionManager* manager = ((App*)WindowApp::m_instance)->m_missionManager;
            const CMission* current = manager->GetMissionByName(location->GetCurrentMission());
            Vector<const CMission*> missions;
            ((App*)WindowApp::m_instance)->m_missionManager->GetSuitableMissionsForLocation(
                missions, location, current->m_difficulty, CMission::MISSION_KA);
            if (missions.size() != 0) {
                unsigned int index = CRandGen::GetInstance()->GetRand(missions.size());
                location->SetMission(missions[index]->m_name, false);
                break;
            }
        }
        CBH_GPSMap::Save();
    }
}

CMapLocation* CGPSMapGame::LocationByName(const XString& name)
{
    for (int index = 0; index < m_locations.size(); index++) {
        CMapLocation* location = m_locations[index];
        if (location->m_locations.indexOf(name, 0) != -1) {
            return location;
        }
    }
    return NULL;
}

void CGPSMapGame::ShowZombieDialog(XString title, XString text)
{
    CZombieDialogWindow* dialog = new CZombieDialogWindow(3);
    dialog->SetTitle(title);
    dialog->SetText(text, false, NULL);
    dialog->AddButton(CZombieDialogWindow::BUTTON_OK, (unsigned int)-1, 0, 0, true, 20);
    CDialogQueueWindow::AddDialogInQueue(dialog);
}

void CGPSMapGame::ResetBriefingSound()
{
    if (m_briefingTrack.Get() != NULL) {
        m_briefingTrack->fadeOut(1500);
        // The original clears the member first, then destroys the old reference; assigning null directly would call operator= in addition.
        CSharedPtr<TrackHandler> old;
        old.m_pointer = m_briefingTrack.m_pointer;
        m_briefingTrack.m_pointer = NULL;
        old.m_count = m_briefingTrack.m_count;
        m_briefingTrack.m_count = NULL;
    }
}

void CGPSMapGame::StartHunt(const CMission* mission)
{
    CHuntingInfo::SAssignedData data;
    data.m_missionName = mission->m_name;
    data.m_questId = m_selectedQuestId;
    data.m_field08 = m_selectedFriendId;
    data.m_text10 = m_selectedFriendName;
    if (((App*)WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial() == 1) {
        data.m_questId = ((App*)WindowApp::m_instance)->m_tutorialManager->GetActiveQuestId();
    }
    CBH_Player* player = CBH_Player::GetInstance();
    player->FieldAt(0x5c) = 0;
    player->Save(true);
    CMapLocation* location = m_selectedLocation;
    player = CBH_Player::GetInstance();
    int locationId = -1;
    if (location != NULL) {
        // The original rereads the member pointer after the GetInstance callback; keep this timing.
        locationId = m_selectedLocation->m_id;
    }
    player->FieldAt(0x60) = locationId;
    player = CBH_Player::GetInstance();
    player->FieldAt(0x64) = data.m_questId;
    data.m_flag20 = data.m_field08 != -1 || data.m_text10.ByteSize() != 0;
    data.m_flag21 = m_targetMessage != NULL;
    if (m_targetMessage != NULL) {
        ((App*)WindowApp::m_instance)->m_newsFeedManager->RemoveMessage(m_targetMessage, m_targetMessageIndex);
    }
    ((App*)WindowApp::m_instance)->m_huntingInfo->SetHuntingData(data);
    mission->OnMissionStarted();
    CBH_Player::GetInstance()->FieldAt(0xe0)++;
    m_parent->SetInterrupt(1);
    m_selectedQuestId = -1;
    m_selectedFriendId = -1;
    m_selectedFriendName = L"";
    m_selectedLocation = NULL;
    m_targetMessage = NULL;
    m_targetMessageIndex = -1;
}

void CGPSMapGame::StartMission(const CMission* mission)
{
    CZombieDialogWindow* dialog = NULL;
    if (!mission->CheckMissionConditions(&dialog)) {
        CDialogQueueWindow::AddDialogInQueue(dialog);
    } else {
        StartHunt(mission);
    }
}

void CGPSMapGame::OnCommand(Event& event)
{
    switch (event.m_id) {
    case (int)0xa4d0d121:
        if (((App*)WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial() == 1) {
            PopulateTutorialLocations();
            UpdateTutorialAttractions();
        } else {
            StartNewCycle();
            SearchForAvailableQuests();
        }
        break;
    case 0x7e145385:
        DGHelper::LaunchRateMe();
        event.Clear();
        break;
    case 0x0bed1dfb:
        StartMission((const CMission*)event.m_param1);
        event.Clear();
        break;
    case 0x33e56be7:
        ShowMissionStartDialog(((App*)WindowApp::m_instance)->m_questManager->GetQuestById(m_selectedQuestId)->m_missionName, -1);
        event.Clear();
        break;
    case (int)0xd9305e1e:
        if (!m_dialogQueue->HasModalChild()) {
            // The original does title first, then text; the arguments are by value and both are destroyed only after the ShowZombieDialog call.
            ShowZombieDialog(Window::ResString("IDS_CYCLES_HELP_TITLE"), Window::ResString("IDS_CYCLES_HELP_TEXT"));
        }
        event.Clear();
        break;
    case 0x1e11f5b7:
        ShowShop(CShopWindow::TAB_2, -1, false);
        event.Clear();
        break;
    case (int)0xc4794df3:
        ShowShop(CShopWindow::TAB_0, event.m_param1, false);
        event.Clear();
        break;
    case (int)0xc9cb1c42:
        ShowShop(CShopWindow::TAB_1, event.m_param1, false);
        event.Clear();
        break;
    case 0x5e099a77:
        m_parent->SetInterrupt(1);
        break;
    case (int)0x8785e4c8:
        ShowShop(CShopWindow::TAB_2, event.m_param1, false);
        event.Clear();
        break;
    case (int)0x8b7127cd:
        break; // The original forwards directly to the base class OnCommand; must not clear the event.
    default:
        if (event.m_id == EventId_TargettingMissionFriendSelected) {
            CIncentivizedWindow* dialog = (CIncentivizedWindow*)event.m_param1;
            Vector<int> selected = dialog->GetSelectedIds();
            if (selected.size() > 0 && selected[0] >= 0) {
                m_selectedFriendId = selected[0];
                ShowMissionStartDialog(m_selectedLocation->GetCurrentMission(), m_selectedLocation->GetCurrentMissionDescriptionId());
            } else {
                m_selectedQuestId = -1;
                m_selectedFriendId = -1;
                m_selectedFriendName = L"";
                m_selectedLocation = NULL;
                m_targetMessage = NULL;
                m_targetMessageIndex = -1;
            }
            // Original 11c112 calls Window::Close with the same unadjusted address as dialog.
            ((Window*)dialog)->Close();
        } else if (event.m_id == EventId_StartTargettingMission) {
            m_selectedQuestId = -1;
            m_selectedFriendId = -1;
            m_selectedFriendName = L"";
            m_selectedLocation = NULL;
            m_targetMessage = NULL;
            m_targetMessageIndex = -1;
            m_selectedFriendId = event.m_param1;
            if (event.m_param2 == 0) {
                m_targetMessage = NULL;
                m_targetMessageIndex = -1;
            } else {
                m_targetMessage = (const SMessage*)event.m_param2;
                m_targetMessageIndex = event.m_param3;
            }
            CMapLocation* location = NULL;
            for (int index = 0; index < m_locations.size(); index++) {
                if (m_locations[index]->m_withFriend) {
                    location = m_locations[index];
                    m_selectedLocation = location;
                }
            }
            if (location != NULL) {
                ShowMissionStartDialog(location->GetCurrentMission(), m_selectedLocation->GetCurrentMissionDescriptionId());
            }
        }
        break;
    }
    CBH_GPSMap::OnCommand(event);
}

void CGPSMapGame::OnGetFocus()
{
    WindowApp::HandleTunnelCommand(0x40067282, 0, 0, 0);
}

void CGPSMapGame::Update()
{
    if (((App*)WindowApp::m_instance)->m_huntingInfo->m_field00) {
        return;
    }
    CBH_GPSMap::Update();
    ((App*)WindowApp::m_instance)->m_leaderboardsManager->Update();
    if (m_fogY > 0.0f && m_fogY < (float)m_mapHeight) {
        for (int index = 0; index < m_locations.size(); index++) {
            // Although the original discards both return values, both external calls must be kept.
            SScreenAdaptingConstant(50)();
            m_locations[index]->HasMission();
        }
    }
    COffersManager* offers = ((App*)WindowApp::m_instance)->m_offersManager;
    offers->Tick(CStdUtil_Cocoa::GetTimeSeconds());
    m_timer.update(WindowApp::m_instance->m_updateFrameTime);
}

void CGPSMapGame::AddDialogInQueue(Window* dialog)
{
    CDialogQueueWindow::AddDialogInQueue(dialog);
}

void FillMapLoadingTasks(CLoadingScreen* screen, CGPSMapGame* game)
{
    class TileLoader : public CLoadingScreen::IStep {
    public:
        explicit TileLoader(CGPSMapGame* game) : m_game(game)
        {
            XString filename = ((App*)WindowApp::m_instance)->m_resMap->getFilename("GPS_MAP_DESCRIPTION");
            char* buffer = ((App*)WindowApp::m_instance)->m_resMap->loadText(filename);
            m_document.Parse(buffer, NULL, TIXML_ENCODING_UNKNOWN);
            if (buffer != NULL) {
                free(buffer);
            }
            m_node = m_document.FirstChild("maps")->FirstChild("map")->FirstChild("tile");
            m_total = m_game->m_tileCountX * m_game->m_tileCountY;
            if (m_total == 0) {
                m_total = 1;
            }
            m_step = 0;
        }
        virtual ~TileLoader() {}
        virtual bool Load(CLoadingScreen* screen)
        {
            if (m_node != NULL) {
                XString path = CXmlHelper::GetAttributeValue(m_node, "path");
                const char* resourceId = ((App*)WindowApp::m_instance)->m_resMap->getResourceID(path);
                ICRenderSurface* surface = ((App*)WindowApp::m_instance)->m_surfaceManager->CreateSurface(resourceId, false, true);
                ICRenderSurface::Description description;
                surface->GetDescription(description);
                m_game->AddMapPatch(surface);
                m_node = m_node->NextSibling("tile");
                m_step++;
                return false;
            }
            ((App*)WindowApp::m_instance)->m_soundManager->PrecacheSound(CDH_SoundManager::SOUND_MUSIC_FIRST);
            return true;
        }
        virtual int GetTotal() const { return m_total; }
        virtual int GetStep() const { return m_step; }
        CGPSMapGame* m_game;       // 04
        const TiXmlNode* m_node;  // 08
        int m_uninitialized0c;    // Not written by the original constructor.
        int m_total;             // 10
        int m_step;              // 14
        TiXmlDocument m_document;// 18, total 60.
    };
    screen->AddLoadingStep(new TileLoader(game), "TileLoader");
    CTutorialManager* tutorial = ((App*)WindowApp::m_instance)->m_tutorialManager;
    if (tutorial != NULL && !tutorial->FirstMissionCompleted()) {
        screen->SetRange(20, 50, false);
    } else {
        screen->SetRange(20, 100, false);
    }
}

void CGPSMapGame::TapjoyUpdater::onReceiveTapjoyPoints(int points)
{
    CZombieDialogWindow* dialog = new CZombieDialogWindow(3);
    dialog->SetTitle(Window::ResString("IDS_TJ_DIALOG_TITLE"));
    dialog->SetText(XString::Format(Window::ResString("IDS_TJ_DIALOG_MESSAGE").c_str(), points), false, NULL);
    dialog->AddButton(CZombieDialogWindow::BUTTON_OK, (unsigned int)-1, 0, 0, true, 20);
    ((App*)WindowApp::m_instance)->m_mainLayer->AddModal(dialog);
}

void CGPSMapGame::TapjoyUpdater::Execute(TimeManager* manager)
{
    if (TapjoyInterface::getRemainingTapjoyPoints() != 0) {
        // The original second read compares as a signed int against 1; must not be folded into the first read.
        int points = (int)TapjoyInterface::getRemainingTapjoyPoints();
        if (points >= 1) {
            int credits = TapjoyInterface::convertPointsToGameCurrency(points);
            CBH_Player::GetInstance()->AddCredits(credits, L"");
            XString source("00_tapjoy");
            // The original copies source before getting Player/GetStats, and releases the original temporary after the notification.
            CGameAnalytics::logCurrencyChange(source,
                CBH_Player::GetInstance()->GetStats(PLAYER_STATS_HARD_CURRENCY),
                credits, (CGameAnalytics::ECurrencyType)1);
            onReceiveTapjoyPoints(credits);
        }
        CGameAnalytics::logAdBonus(0, TapjoyInterface::getRemainingTapjoyPoints());
        TapjoyInterface::consumeTapjoyPoints(TapjoyInterface::getRemainingTapjoyPoints());
    }
    manager->add(this, 5000);
}
