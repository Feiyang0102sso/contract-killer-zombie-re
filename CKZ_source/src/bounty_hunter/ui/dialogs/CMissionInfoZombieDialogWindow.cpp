// MissionInfo full owner: the normal/Endless variants share the real window tree; XString local lifetimes are kept per the original call sites.
#include "CMissionInfoZombieDialogWindow.h"
#include "App.h"
#include "CMission.h"
#include "CMissionManager.h"
#include "CLocation.h"
#include "CLocationManager.h"
#include "CProgressSystem.h"
#include "CTutorialManager.h"
#include "CBH_Player.h"
#include "CGCLeaderboardsManager.h"
#include "CFriendAvatarWindow.h"
#include "CGameResourceWindow.h"
#include "CDH_SoundManager.h"
#include "CDH_WeaponsManager.h"
#include "CDH_Weapon.h"
#include "DGHelper.h"
#include "BaseDialog.h"
#include "ServiceWindows.h"
#include "CFontMgr.h"
#include "SGImageWindow.h"
#include "SGImageWindowCommand.h"
#include "SGButton.h"
#include "CImageButton.h"
#include "SScreenAdaptingConstant.h"

// The original ref was mismapped to 4ff55c via the same-named local symbol; actually the 14b10a addPC points to 4ff56c, an independent 16B static.
static SScreenAdaptingConstant RESOURCE_WIN_SPACING_X(10);

CMissionInfoZombieDialogWindow::CMissionInfoZombieDialogWindow(const CMission* mission,
    com::glu::platform::core::CSharedPtr<TrackHandler> track, int description,
    int friendId, XString friendName)
    : CZombieDialogWindow(2), m_avatarWindow(NULL), m_track(track)
{
    if (App::IsKindle() == 1) {
        m_background->SetLayoutType(LAYOUT_SIMPLE);
        m_background->SetAlign(9);
        m_background->SetOutsetSpacing(-40, 0, 0, -100);
    }
    m_titleContainer->SetOutsetSpacing(SScreenAdaptingConstant(20, 45, -1, 30)(), 0, 0, 0);
    m_messageContainer->SetAlign(0x0c);
    m_messageContainer->SetOutsetSpacing(SScreenAdaptingConstant(40, 110, -1, 65)(), 0, 0, 0);
    XString title = mission->m_screenName.Upper();
    XString text = mission->m_description;
    if (description >= 0) {
        CMissionManager::SMissionDescription* entry = ((App*)WindowApp::m_instance)->m_missionManager->GetMissionDescriptionById(description);
        title = entry->m_title;
        text = entry->m_text;
    }
    SetTitle(title);
    Window* avatarFrame = createTD(NULL, 0);
    avatarFrame->SetDesiredWidth(SScreenAdaptingConstant(50)());
    avatarFrame->SetDesiredHeight(SScreenAdaptingConstant(50)());
    int avatarTop = SScreenAdaptingConstant(20)();
    int avatarLeft = SScreenAdaptingConstant(20)();
    avatarFrame->SetOutsetSpacing(avatarTop, 0, 0, avatarLeft);
    m_background->AddToFront(avatarFrame);
    App::LoadSGArchetype(5, 0);
    if (friendId >= 0 || friendName.ByteSize() != 0) {
        CFriendAvatarWindow* avatar;
        if (friendId == 0) {
            avatar = new CFriendAvatarWindow(com::glu::platform::components::CStrWChar(friendName.m_data));
        } else {
            avatar = new CFriendAvatarWindow(friendId);
        }
        avatarFrame->AddToFront(avatar);
    }
    XString resolvedName = DGHelper::GetFriendName(friendId, friendName);
    if (resolvedName.ByteSize() != 0 &&
        XString::CmpData(resolvedName.m_data, resolvedName.ByteSize(), L"unknown", wcslen(L"unknown") * sizeof(wchar_t), 0x7fffffff) != 0 &&
        mission->m_descriptionWithFriend.ByteSize() != 0) {
        text = XString::Format(mission->m_descriptionWithFriend.m_data, resolvedName.m_data);
    }
    text = ParametrizeTextWithParams(text, mission);
    SetText(text, false, NULL);
    m_contentContainer->SetHeightByContent(0, 0);
    m_contentContainer->SetAlign(0x14);
    m_contentContainer->SetOutsetSpacing(0, 0, SScreenAdaptingConstant(45)(), 0);

    Window* costs = new WindowTransparent(false);
    costs->SetWidthByContent(0, 0);
    costs->SetHeightByContent(0, 0);
    costs->SetOutsetSpacing(0, 0, SScreenAdaptingConstant(5, 10)(), 0);
    TextWindow* costLabel = new TextWindow(CFontMgr::GetInstance()->GetFont(26));
    costLabel->SetText(ResString("IDS_MISSION_INFO_COST"));
    costLabel->SetAlign(0x10);
    costs->AddToFront(costLabel, 0, 0);
    int costColumn = 1;
    if (mission->m_minPeople != 0) {
        CGameResourceWindow* resource = new CGameResourceWindow((CGameResourceWindow::EType)4, false);
        resource->SetOutsetSpacing(0, RESOURCE_WIN_SPACING_X());
        resource->SetValues(mission->m_minPeople, 0);
        costs->AddToFront(resource, 1, 0);
        costColumn = 2;
    }
    if (mission->m_minProvision != 0) {
        CGameResourceWindow* resource = new CGameResourceWindow((CGameResourceWindow::EType)6, false);
        resource->SetOutsetSpacing(0, RESOURCE_WIN_SPACING_X());
        resource->SetValues(mission->m_minProvision, 0);
        costs->AddToFront(resource, costColumn, 0);
    } else if (costColumn == 1) {
        costs->ClearFlags(1);
    }
    m_contentContainer->AddToFront(createTD(costs, 3), 0, 0);

    Window* rewards = new WindowTransparent(false);
    rewards->SetAlign(1);
    rewards->SetWidthByContent(0, 0);
    rewards->SetHeightByContent(0, 0);
    TextWindow* rewardLabel = new TextWindow(CFontMgr::GetInstance()->GetFont(26));
    rewardLabel->SetText(ResString("IDS_MISSION_INFO_REWARD"));
    rewardLabel->SetAlign(0x10);
    rewards->AddToFront(rewardLabel, 0, 0);
    int rewardColumn = 1;
    // The original checks the getter first, then after constructing/querying spacing calls the getter again to get the display value.
    if (mission->GetRewardPeople() != 0) {
        CGameResourceWindow* resource = new CGameResourceWindow((CGameResourceWindow::EType)4, false);
        resource->SetOutsetSpacing(0, RESOURCE_WIN_SPACING_X());
        resource->SetValues(mission->GetRewardPeople(), 0);
        rewards->AddToFront(resource, rewardColumn, 0);
        rewardColumn++;
    }
    if (mission->GetRewardProvision() != 0) {
        CGameResourceWindow* resource = new CGameResourceWindow((CGameResourceWindow::EType)6, false);
        resource->SetOutsetSpacing(0, RESOURCE_WIN_SPACING_X());
        resource->SetValues(mission->GetRewardProvision(), 0);
        rewards->AddToFront(resource, rewardColumn, 0);
        rewardColumn++;
    }
    if (mission->GetBaseRewardMoney() != 0) {
        CGameResourceWindow* resource = new CGameResourceWindow((CGameResourceWindow::EType)0, false);
        resource->SetOutsetSpacing(0, RESOURCE_WIN_SPACING_X());
        resource->SetValues(mission->GetBaseRewardMoney(), 0);
        rewards->AddToFront(resource, rewardColumn, 0);
        rewardColumn++;
    }
    if (mission->GetBaseRewardXP() != 0) {
        CGameResourceWindow* resource = new CGameResourceWindow((CGameResourceWindow::EType)3, false);
        resource->SetOutsetSpacing(0, RESOURCE_WIN_SPACING_X());
        resource->SetValues(mission->GetBaseRewardXP(), 0);
        rewards->AddToFront(resource, rewardColumn, 0);
    }
    m_contentContainer->AddToFront(createTD(rewards, 3), 0, 1);
    int weaponId = ((App*)WindowApp::m_instance)->m_progressSystem->GetRecommendedWeaponId(mission->m_type, mission->m_name);
    Window* recommended = CreateRecommendedWeaponWindow(weaponId);
    recommended->SetOutsetSpacing(SScreenAdaptingConstant(0, 5)(), 0, 0, 0);
    m_contentContainer->AddToFront(recommended, 0, 2);
    int difficultyIcon = mission->GetDialogDifficultyIconAnimation();
    if (difficultyIcon >= 0) {
        SGImageWindow* icon = new SGImageWindow();
        icon->SetArchetypeCharacter(1, 0);
        icon->SetAnimation(difficultyIcon, 1);
        int top = SScreenAdaptingConstant(41)();
        int right = SScreenAdaptingConstant(44)();
        icon->SetOutsetSpacing(top, right, 0, 0);
        icon->SetAlign(10);
        TextWindow* label = new TextWindow();
        {
            XString difficulty = mission->GetDifficultyString();
            label->SetText(difficulty, CFontMgr::GetInstance()->GetFont(29));
        }
        int textTop = SScreenAdaptingConstant(4)();
        int textRight = SScreenAdaptingConstant(5)();
        label->SetOutsetSpacing(textTop, textRight, 0, 0);
        label->SetAlign(10);
        icon->AddToFront(label);
        m_background->AddToFront(icon);
    }
    SGButton* start = AddButton((EDialogButton)8, 0x0bed1dfb, (int)mission, 0, true, 20);
    AddButton((EDialogButton)4, 0x8b7127cd, 0, 0, true, 20);
    if (((App*)WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial() == 1 &&
        ((App*)WindowApp::m_instance)->m_tutorialManager->IsStartButtonBlink() == 1) {
        start->m_blinking = true;
    }
}

XString CMissionInfoZombieDialogWindow::ParametrizeTextWithParams(const XString& text, const CMission* mission)
{
    CLocation* location = ((App*)WindowApp::m_instance)->m_locationManager->GetLocation(mission->m_locationName);
    if (location == NULL) {
        return text;
    }
    XString replaced = text.Replace(L"[LocationName]", location->m_title.m_data, true);
    return replaced;
}

Window* CMissionInfoZombieDialogWindow::CreateRecommendedWeaponWindow(int weaponId)
{
    Window* content = new WindowTransparent(false);
    content->SetAlign(1);
    content->SetWidthByContent(0, 0);
    content->SetHeightByContent(0, 0);
    TextWindow* title = new TextWindow(CFontMgr::GetInstance()->GetFont(26));
    title->SetText(ResString("IDS_MISSION_INFO_RECOMMENDED_WEAPON"));
    title->SetAlign(0x21);
    content->AddToFront(title, 0, 0);
    CDH_Weapon* weapon = (CDH_Weapon*)((App*)WindowApp::m_instance)->m_weaponsManager->getItemById(weaponId);
    CImageButton* image = new CImageButton();
    image->SetOutsetSpacing(0, 0, 0, SScreenAdaptingConstant(0, 10)());
    if (weapon != NULL) {
        image->SetImage(weapon->recommendedImage(), 0);
    }
    image->m_command = 0xc4794df3;
    image->m_commandParameter1 = weaponId;
    image->m_commandParameter2 = 0;
    image->m_commandParameter3 = 0;
    content->AddToFront(image, 1, 0);
    SGImageWindowCommand* buy = new SGImageWindowCommand();
    int animation;
    // The original has a NULL check only for the image setting; the real calls below rely on a valid weapon, no early return is added.
    if (weapon->isAvailable() == 1) {
        buy->SetArchetypeCharacter(1, 0);
        if (!weapon->IsEquiped()) {
            animation = 66;
        } else {
            animation = 71;
        }
    } else {
        buy->SetArchetypeCharacter(7, 0);
        animation = 2;
    }
    buy->SetAnimation(animation, 1);
    buy->m_command = 0xc4794df3;
    buy->m_commandParameter1 = weaponId;
    buy->m_commandParameter2 = 0;
    buy->m_commandParameter3 = 0;
    buy->SetOutsetSpacing(0, 0, 0, SScreenAdaptingConstant(0, 10)());
    buy->SetAlign(0x24);
    buy->SetFlags(0x800000);
    Window* buyFrame = createTD(buy, 3);
    content->AddToFront(buyFrame, 2, 0);
    if (weapon->IsEquiped() == 1) {
        buy->SetAlign(0x0c);
        TextWindow* label = new TextWindow(CFontMgr::GetInstance()->GetFont(26));
        label->SetText(ResString("IDS_EQUIPED"));
        label->SetAlign(0x24);
        label->SetOutsetSpacing(SScreenAdaptingConstant(15)(), 0, 0, 0);
        buyFrame->SetOutsetSpacing(SScreenAdaptingConstant(7)(), 0, 0, 0);
        buyFrame->AddToFront(label);
    } else {
        TextWindow* label = new TextWindow(CFontMgr::GetInstance()->GetFont(26));
        const char* text = "IDS_BUY_NOW_BUTTON";
        if (weapon->isAvailable() != 0) {
            text = "IDS_EQUIP_BUTTON";
        }
        label->SetText(ResString(text));
        label->SetAlign(0x24);
        int top = SScreenAdaptingConstant(2)();
        int left = SScreenAdaptingConstant(15)();
        label->SetOutsetSpacing(top, 0, 0, left);
        buy->AddToFront(label);
    }
    return createTD(content, 3);
}

CMissionInfoZombieDialogWindow::~CMissionInfoZombieDialogWindow()
{
    if (m_track.Get() != NULL) {
        m_track->fadeOut(1500);
        // Original 14ba18 transfers the two words to a local and clears the member; the local immediately releases the old track.
        com::glu::platform::core::CSharedPtr<TrackHandler> released;
        released.m_pointer = m_track.m_pointer;
        m_track.m_pointer = NULL;
        released.m_count = m_track.m_count;
        m_track.m_count = NULL;
    }
}

void CMissionInfoZombieDialogWindow::Init()
{
    SetOutsetSpacing(0, 0, SScreenAdaptingConstant(20, 40)(), 0);
    SetAlign(0x14);
}

void CMissionInfoZombieDialogWindow::OnCommand(Event& event)
{
    if (EventId_FriendAvatarDone == event.m_id) {
        if (m_avatarWindow != NULL) {
            m_avatarWindow->Close();
            m_avatarWindow = NULL;
        }
        event.Clear();
    } else if (event.m_id == (int)0xc4794df3) {
        Close();
    }
    CZombieDialogWindow::OnCommand(event);
}

CEndlessModeInfoZombieDialogWindow::CEndlessModeInfoZombieDialogWindow(const CMission* mission, int description)
    : CZombieDialogWindow(2)
{
    m_titleContainer->SetOutsetSpacing(SScreenAdaptingConstant(20, 45, -1, 30)(), 0, 0, 0);
    m_messageContainer->SetAlign(0x0c);
    m_messageContainer->SetOutsetSpacing(SScreenAdaptingConstant(40, 110, -1, 65)(), 0, 0, 0);
    XString title = mission->m_screenName.Upper();
    XString text = mission->m_description;
    if (description >= 0) {
        CMissionManager::SMissionDescription* entry = ((App*)WindowApp::m_instance)->m_missionManager->GetMissionDescriptionById(description);
        title = entry->m_title;
        text = entry->m_text;
    }
    SetTitle(title);
    Window* avatarFrame = createTD(NULL, 0);
    avatarFrame->SetDesiredWidth(SScreenAdaptingConstant(50)());
    avatarFrame->SetDesiredHeight(SScreenAdaptingConstant(50)());
    int avatarTop = SScreenAdaptingConstant(20)();
    int avatarLeft = SScreenAdaptingConstant(20)();
    avatarFrame->SetOutsetSpacing(avatarTop, 0, 0, avatarLeft);
    m_background->AddToFront(avatarFrame);
    App::LoadSGArchetype(5, 0);
    com::glu::platform::graphics::CFont* playerFont = CFontMgr::GetInstance()->GetFont(28);
    com::glu::platform::graphics::CFont* normalFont = CFontMgr::GetInstance()->GetFont(24);
    com::glu::platform::graphics::CFont* scoreFont = CFontMgr::GetInstance()->GetFont(29);
    text = CMissionInfoZombieDialogWindow::ParametrizeTextWithParams(text, mission);
    SetText(text, true, normalFont);
    m_contentContainer->SetHeightByContent(0, 0);
    m_contentContainer->SetAlign(0x24);
    int score = CBH_Player::GetInstance()->m_gameStats.m_survivalModeScore;
    CGCLeaderboardsManager* manager = ((App*)WindowApp::m_instance)->m_leaderboardsManager;
    int state = manager->m_state;
    Window* board = new WindowTransparent(false);
    if (state == 2) {
        board->SetPercentWidth(80, 0, 0);
        board->SetHeightByContent(0, 0);
        board->SetAlign(0x24);
        m_contentContainer->AddToFront(board);
        Window* heading = new WindowTransparent(false);
        heading->SetCellPos(0, 0, 1, 1);
        heading->SetAlign(0x24);
        heading->SetWidthByContent(0, 0);
        heading->SetHeightByContent(0, 0);
        heading->SetOutsetSpacing(8);
        {
            // b146: the original builds the "\n" temporary first and reads the flag inside the same
            // full expression (select between the two keys right before ResString).
            XString headingText = XString("\n") + ResString(((App*)WindowApp::m_instance)->m_leaderboardsManager->m_isFriendsBoard
                                                            ? "IDS_ENDLESS_MODE_LEADERBOARD_FRIENDS"
                                                            : "IDS_ENDLESS_MODE_LEADERBOARD_SCORES");
            TextWindow* label = new TextWindow(headingText, playerFont);
            label->SetCellPos(0, 0, 2, 1);
            label->SetAlign(0x24);
            heading->AddToFront(label);
        }
        board->AddToFront(heading);
        Window* rows = new WindowTransparent(false);
        rows->SetCellPos(0, 1, 1, 1);
        rows->SetAlign(0x24);
        rows->SetWidthByContent(0, 0);
        rows->SetHeightByContent(0, 0);
        rows->SetOutsetSpacing(8);
        for (int index = 0; index < manager->m_leaderboards.size(); index++) {
            XString rank;
            com::glu::platform::graphics::CFont* font = normalFont;
            if (manager->m_leaderboards[index]->m_isPlayer) {
                font = playerFont;
            }
            int position = manager->m_leaderboards[index]->m_rank;
            if (position < 1) {
                rank = L"   ";
            } else {
                rank = XString(position) + XString(". ");
            }
            XString name = rank + manager->m_leaderboards[index]->m_name;
            TextWindow* label = new TextWindow(name, font);
            label->SetCellPos(0, index, 1, 1);
            label->SetAlign(0x21);
            rows->AddToFront(label);
            TextWindow* points = new TextWindow(XString(manager->m_leaderboards[index]->m_score), scoreFont);
            points->SetCellPos(1, index, 1, 1);
            points->SetAlign(0x22);
            rows->AddToFront(points);
        }
        board->AddToFront(rows);
    } else {
        board->SetAlign(0x24);
        board->SetWidthByContent(0, 0);
        board->SetHeightByContent(0, 0);
        {
            XString yourScore = XString::Format(ResString("IDS_ENDLESS_MODE_LEADERBOARD_YOUR_SCORE").m_data, score);
            TextWindow* value = new TextWindow(yourScore, playerFont);
            value->SetAlign(0x24);
            board->AddToFront(value, 0, 0);
            TextWindow* pending = new TextWindow(ResString("IDS_ENDLESS_MODE_LEADERBOARD_NOT_READY"), normalFont);
            pending->SetOutsetSpacing(SScreenAdaptingConstant(10)(), 0, 0, 0);
            pending->SetAlign(0x24);
            board->AddToFront(pending, 0, 1);
        }
        m_contentContainer->SetWidthByContent(0, 0);
        m_contentContainer->AddToFront(board);
    }
    AddButton((EDialogButton)9, 0x0bed1dfb, (int)mission, 0, true, 20);
    AddButton((EDialogButton)4, 0x8b7127cd, 0, 0, true, 20);
}
