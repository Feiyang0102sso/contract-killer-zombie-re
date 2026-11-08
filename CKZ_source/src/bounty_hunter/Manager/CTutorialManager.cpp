// CTutorialManager.cpp - tutorial state gates, messages, and command-driven progression.
#include "CTutorialManager.h"
#include "App.h"
#include "BaseDialog.h"
#include "CFontMgr.h"
#include "CBH_Player.h"
#include "CGameAnalytics.h"
#include "DGHelper.h"
#include "CSwerveGame.h"
#include "CScriptedSceneController.h"
#include "CDH_BasicGameData.h"
#include "CQuestManager.h"
#include "CQuestZombieDialogWindow.h"
#include "CDialogQueueWindow.h"
#include "SScreenAdaptingConstant.h"

// Original pointer table @0x4ff450; pointed-to strings read directly from their file ranges.
const char* TutorialMissions[4] = {
    "TUTORIAL_MISSION_1", "TUTORIAL_MISSION_2", "TUTORIAL_MISSION_3", "TUTORIAL_MISSION_4"
};

CTutorialFloatingWindow::CTutorialFloatingWindow()
{
    SetArchetypeCharacter(7, 0);
    SetAnimation(0, 1);
    m_text = new TextWindow();
    m_text->SetPercentWidth(95, 0, 0);
    m_text->SetHeightByContent(0, 0);
    m_text->SetAlign(0x24);
    AddToFront(m_text);
    SetLayoutType(LAYOUT_SIMPLE);
    SetAlign(0x0c);
    ClearFlags(0x40);
}

void CTutorialFloatingWindow::SetText(const char* text, EPlace place)
{
    if (place == PLACE_TOP) {
        SetOutsetSpacing(SScreenAdaptingConstant(5)(), 0, 0, 0);
        SetAlign(0x0c);
    } else if (place == PLACE_CENTER) {
        SetOutsetSpacing(SScreenAdaptingConstant(45)(), 0, 0, 0);
        SetAlign(0x24);
    }
    m_text->SetText(Window::ResString(text), CFontMgr::GetInstance()->GetFont(0x1b));
}

CTutorialManager::CTutorialManager()
    : WindowTransparent(false)
{
    m_messagesSuppressed = false;
    m_floating = NULL;
    if (CBH_Player::GetInstance()->GetTutorialState() < 12) {
        m_floating = new CTutorialFloatingWindow();
        m_floating->ClearFlags(1);
        ((App*)WindowApp::m_instance)->m_mainLayer->AddToFront(m_floating);
    }
}

bool CTutorialManager::IsPlayerInTutorial()
{
    return CBH_Player::GetInstance()->GetTutorialState() < 12;
}

bool CTutorialManager::FirstMissionCompleted() const
{
    return CBH_Player::GetInstance()->GetTutorialState() > 3;
}

int CTutorialManager::GetState() const
{
    return CBH_Player::GetInstance()->GetTutorialState();
}

void CTutorialManager::NextTutorialStep(int steps, bool skipMessage)
{
    int state = CBH_Player::GetInstance()->GetTutorialState();
    CGameAnalytics::logTutorialStepCompleted(state);
    if (state == 12) {
        return;
    }
    CGameAnalytics::logTutorialStepCompleted(state); // Both original analytics calls are retained.
    int next = state + steps;
    if (next == 5) {
        if (CBH_Player::GetInstance()->FieldAt(0x13c) > 1) {
            next++;
        }
    }
    CBH_Player::GetInstance()->SetTutorialState(next);
    if (!skipMessage) {
        ShowStateInitMessage();
    }
    CSwerveGame* game = ((App*)WindowApp::m_instance)->m_helper->m_swerveGame;
    if (game != NULL) {
        game->GetScriptController()->OnTutorialStateUpdate();
    }
    WindowApp::HandleTunnelCommand((int)0xa4d0d121, 0, 0, 0);
}

void CTutorialManager::ShowStateInitMessage()
{
    switch (CBH_Player::GetInstance()->GetTutorialState()) {
    case 2:
        ShowFloatingMessage("IDS_TUTORIAL_FLOATING_HOW_TO_SWITCH", CTutorialFloatingWindow::PLACE_TOP);
        break;
    case 3:
    case 8:
        if (m_floating != NULL) {
            m_floating->ClearFlags(1);
        }
        break;
    case 4:
        ShowMessage(NULL, "IDS_TUTORIAL_FIRST_CITY_MAP", false);
        break;
    case 5: {
        DGHelper* helper = ((App*)WindowApp::m_instance)->m_helper;
        helper->m_gamePlayManager.m_playerData.GetGameData(helper->m_gamePlayManager.m_gameType)->SetActiveWeapon(1);
        helper->m_gamePlayManager.SetWeapon(0, -1);
        break;
    }
    case 7:
        WindowApp::HandleTunnelCommand(0x2dd14cf3, 1, 0, 0);
        break;
    case 9:
        if (m_floating != NULL) {
            m_floating->ClearFlags(1);
        }
        ShowMessage(NULL, "IDS_TUTORIAL_MISSION3_CITY_MAP", false);
        break;
    case 10:
        if (m_floating != NULL) {
            m_floating->ClearFlags(1);
        }
        ShowMessage(NULL, "IDS_TUTORIAL_MISSION4_CITY_MAP", false);
        break;
    case 11:
        ShowMessage(NULL, "IDS_TUTORIAL_FINAL", false);
        WindowApp::HandleTunnelCommand(0x3da61159, 0, 0, 0);
        break;
    }
}

void CTutorialManager::ShowFloatingMessage(const char* text, CTutorialFloatingWindow::EPlace place)
{
    if (m_floating != NULL) {
        m_floating->SetText(text, place);
        m_floating->SetFlags(1);
    }
}

void CTutorialManager::ShowMessage(const char* unused, const char* text, bool once)
{
    if (m_messagesSuppressed || (once && m_lastMessage == text)) {
        return;
    }
    m_lastMessage = text;
    CQuestManager::SDialog dialog;
    dialog.m_image = "IDB_EVELIN_1_PNG";
    dialog.m_text = Window::ResString(text);
    CQuestZombieDialogWindow* window = new CQuestZombieDialogWindow(dialog);
    CDialogQueueWindow::AddDialogInQueue(window);
}

bool CTutorialManager::IsAutoStartMission() { return false; } // Original @0x13c1dc.
bool CTutorialManager::ShowSafehouse() const { return CBH_Player::GetInstance()->GetTutorialState() > 7; }
bool CTutorialManager::IsShowShop() { return (unsigned int)(CBH_Player::GetInstance()->GetTutorialState() - 5) < 2; }
bool CTutorialManager::IsShowHome() { return false; } // Original @0x13c210.
int CTutorialManager::GetCurrentShopTabFilter()
{
    if (CBH_Player::GetInstance()->GetTutorialState() == 5) {
        return 0;
    }
    return -1;
}
int CTutorialManager::GetCurrentShopTab() { return 0; } // Original @0x13c230.
bool CTutorialManager::CanCloseShop() { return (unsigned int)(CBH_Player::GetInstance()->GetTutorialState() - 5) > 1; }
bool CTutorialManager::CanCloseNetwork() { return true; } // Original @0x13c24c.
void CTutorialManager::Update() { CBH_Player::GetInstance()->GetTutorialState(); }
bool CTutorialManager::IsShopAttraction() { return CBH_Player::GetInstance()->GetTutorialState() == 4; }
bool CTutorialManager::IsStartButtonBlink() { return (unsigned int)(CBH_Player::GetInstance()->GetTutorialState() - 8) < 3; }
bool CTutorialManager::IsAchievementsForbidden() { return true; } // Original @0x13c294.
bool CTutorialManager::IsEquipLocked(int slot)
{
    bool firstSlot = slot == 0;
    return (CBH_Player::GetInstance()->GetTutorialState() < 7) & firstSlot;
}
int CTutorialManager::IsEquipButtonBlink(int slot)
{
    if (CBH_Player::GetInstance()->GetTutorialState() == 6) {
        return slot != 0;
    }
    return false;
}
bool CTutorialManager::IsBuyButtonBlink() { return CBH_Player::GetInstance()->GetTutorialState() == 5; }
bool CTutorialManager::OnlyTutorialMissions() const { return CBH_Player::GetInstance()->GetTutorialState() < 12; }
bool CTutorialManager::IsShopBackButtonHide() { return CBH_Player::GetInstance()->GetTutorialState() < 7; }
bool CTutorialManager::IsMapShopBlink() const { return CBH_Player::GetInstance()->GetTutorialState() == 4; }
const char* CTutorialManager::GetActiveTutorialMissionName() const
{
    switch (CBH_Player::GetInstance()->GetTutorialState()) {
    case 8: return TutorialMissions[1];
    case 9: return TutorialMissions[2];
    case 10: return TutorialMissions[3];
    }
    return NULL;
}
int CTutorialManager::GetActiveQuestId() const
{
    if (CBH_Player::GetInstance()->GetTutorialState() == 10) {
        return 1;
    }
    return -1;
}
const char* CTutorialManager::GetFirstTutorialMission() const { return TutorialMissions[0]; }
int CTutorialManager::IsMapLocationBlink()
{
    if (CBH_Player::GetInstance()->GetTutorialState() == 8) {
        return true;
    }
    if (CBH_Player::GetInstance()->GetTutorialState() == 9) {
        return true;
    }
    int result = 1;
    if (CBH_Player::GetInstance()->GetTutorialState() != 10) {
        result = 0;
    }
    return result;
}
bool CTutorialManager::IsSwitchWeaponButtonBlink() { return CBH_Player::GetInstance()->GetTutorialState() == 2; }
bool CTutorialManager::IsShowSwitchWeaponButton() { return CBH_Player::GetInstance()->GetTutorialState() > 1; }
int CTutorialManager::IsWeaponButtonBlink(int slot)
{
    if ((unsigned int)(CBH_Player::GetInstance()->GetTutorialState() - 5) < 2) {
        return slot == 1;
    }
    return false;
}
bool CTutorialManager::IsMenuBarBlocked() { return false; } // Original @0x13c440.

void CTutorialManager::OnCommand(Event& event)
{
    int state = CBH_Player::GetInstance()->GetTutorialState();
    switch (event.m_id) {
    case (int)0x8f641692:
        if (m_floating != NULL) {
            m_floating->ClearFlags(1);
        }
        event.Clear();
        break;
    case 0x45f9bf40:
        if ((unsigned int)(state - 1) < 3) {
            NextTutorialStep(4 - state, true);
        } else if ((unsigned int)(state - 8) < 3) {
            NextTutorialStep(1, true);
        }
        event.Clear();
        break;
    case (int)0x9a412292:
        if (state == 5) {
            NextTutorialStep(1, false);
        }
        break;
    case 0x0df6b618:
        if (state == 6 && event.m_param1 >= 0) {
            NextTutorialStep(1, false);
        }
        event.Clear();
        break;
    case (int)0x807f0f43:
        if (state == 2) {
            NextTutorialStep(1, false);
        }
        event.Clear();
        break;
    case (int)0x8486376a:
        if (state == 4) {
            NextTutorialStep(1, false);
        }
        break;
    case 0x40067282:
        if (state == 7 || state == 11) {
            NextTutorialStep(1, false);
        }
        event.Clear();
        break;
    case (int)0xd17ff5d4:
        m_lastMessage = NULL;
        CBH_Player::GetInstance()->GetTutorialState();
        break;
    }
}

void ShowFloatingMessage(const char* text)
{
    ((App*)WindowApp::m_instance)->m_tutorialManager->ShowFloatingMessage(text, CTutorialFloatingWindow::PLACE_TOP);
}

void ResetFloatingMessage()
{
    CTutorialFloatingWindow* window = ((App*)WindowApp::m_instance)->m_tutorialManager->m_floating;
    if (window != NULL) {
        window->ClearFlags(1);
    }
}
