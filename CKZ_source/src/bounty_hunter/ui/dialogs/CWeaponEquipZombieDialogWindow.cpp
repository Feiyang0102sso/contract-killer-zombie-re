// u545 full equip window; keeps the original control flow that still handles swap/consumables after a conflict.
#include "CWeaponEquipZombieDialogWindow.h"
#include "App.h"
#include "DGHelper.h"
#include "CDH_Weapon.h"
#include "CDH_SoundManager.h"
#include "CGameAnalytics.h"
#include "CGrenade.h"
#include "CGrenadeManager.h"
#include "CPowerUp.h"
#include "CPowerUpManager.h"
#include "CTutorialManager.h"
#include "SGImageWindow.h"
#include "SGButton.h"
#include "SScreenAdaptingConstant.h"

CWeaponEquipZombieDialogWindow::CWeaponEquipZombieDialogWindow(CDH_Weapon* weapon, CGrenade* grenade, CPowerUp* powerUp)
    : CZombieDialogWindow(2), m_grenadeImage(NULL), m_powerUpImage(NULL), m_selectedSlot(-1),
      m_swap(false), m_offeredWeapon(weapon), m_selectGrenade(false), m_selectPowerUp(false),
      m_offeredGrenade(grenade), m_offeredPowerUp(powerUp), m_tutorialBlink(NULL)
{
    SetOutsetSpacing(SScreenAdaptingConstant(-20)(), 0, 0, 0);
    Window* slots = new WindowTransparent(false);
    slots->SetWidthByContent(0, 0);
    slots->SetAlign(12);
    slots->SetOutsetSpacing(SScreenAdaptingConstant(40)(), 0, 0, 0);
    slots->SetHeightByContent(0, 0);
    slots->AddToFront(CreateWeaponSlotWindow(0), 0, 0);
    slots->AddToFront(CreateWeaponSlotWindow(1), 0, 1);
    ImageWindow* grenadeImage;
    Window* grenadeSlot = CreateSlotWindow("IDS_EQUIP_GRENADES", true, 0xb7aa6264, 0, &grenadeImage, false);
    m_grenadeImage = grenadeImage;
    slots->AddToFront(grenadeSlot, 1, 0);
    ImageWindow* powerUpImage;
    Window* powerUpSlot = CreateSlotWindow("IDS_EQUIP_ENHANCEMENTS", true, 0xb7aa6264, 1, &powerUpImage, false);
    m_powerUpImage = powerUpImage;
    slots->AddToFront(powerUpSlot, 1, 1);
    AddToFront(slots);
    UpdateSlots();
    m_okButton = AddButton(BUTTON_OK, 0x910d9f06, 0, 0, true, 20);
    AddButton((EDialogButton)4, 0x62e7ea20, 0, 0, true, 20);
}

Window* CWeaponEquipZombieDialogWindow::CreateWeaponSlotWindow(int slot)
{
    if ((unsigned int)slot > 1) {
        return NULL;
    }
    const char* text = "IDS_SECONDARY_WEAPON";
    bool primary = slot == 0;
    if (primary) {
        text = "IDS_PRIMARY_WEAPON";
    }
    unsigned int command = 0x30826e06;
    bool blink = false;
    if (((App*)WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial() == 1) {
        if (m_offeredWeapon != NULL) {
            CTutorialManager* tutorial = ((App*)WindowApp::m_instance)->m_tutorialManager;
            if (tutorial->IsEquipLocked(m_offeredWeapon->getId())) {
                command = 0;
            } else {
                tutorial = ((App*)WindowApp::m_instance)->m_tutorialManager;
                if (tutorial->IsEquipButtonBlink(m_offeredWeapon->getId()) == 1) {
                    command = 0;
                    if (primary) {
                        command = 0x30826e06;
                    }
                    blink = primary;
                }
            }
        }
    }
    ImageWindow* image;
    Window* window = CreateSlotWindow(text, false, command, slot, &image, blink);
    m_weaponImages[slot] = image;
    return window;
}

void CWeaponEquipZombieDialogWindow::UpdateSlots()
{
    CDH_Weapon* first = GetSlotWeapon(0);
    CDH_Weapon* second = GetSlotWeapon(1);
    CGrenade* grenade = ((App*)WindowApp::m_instance)->m_grenadeManager->GetActiveGrenade();
    CPowerUp* powerUp = ((App*)WindowApp::m_instance)->m_powerUpManager->GetActivePowerUp();
    CDH_Weapon* offered = m_offeredWeapon;
    if (offered != NULL) {
        int slot = m_selectedSlot;
        if (slot == 0) {
            first = offered;
            if (offered->m_projectileType == 2 && second->m_projectileType == 2) {
                WarnForbiddenWeaponConfiguration();
                return;
            }
        } else if (slot == 1) {
            if (offered->m_projectileType == 2 && first->m_projectileType == 2) {
                WarnForbiddenWeaponConfiguration();
                return;
            }
            second = offered;
        }
    }
    if (m_selectGrenade && m_offeredGrenade != NULL) {
        grenade = m_offeredGrenade;
    }
    if (m_selectPowerUp && m_offeredPowerUp != NULL) {
        powerUp = m_offeredPowerUp;
    }
    if (first == NULL) {
        m_weaponImages[0]->ClearFlags(1);
    } else {
        m_weaponImages[0]->SetFlags(1);
    }
    if (second == NULL) {
        m_weaponImages[1]->ClearFlags(1);
    } else {
        m_weaponImages[1]->SetFlags(1);
    }
    if (first != NULL) {
        ImageWindow* image = m_weaponImages[0];
        image->SetImage(first->recommendedImage());
    }
    if (second != NULL) {
        ImageWindow* image = m_weaponImages[1];
        image->SetImage(second->recommendedImage());
    }
    if (grenade == NULL) {
        m_grenadeImage->ClearFlags(1);
    } else {
        m_grenadeImage->SetFlags(1);
        if (grenade->GetCount() > 0) {
            ImageWindow* image = m_grenadeImage;
            image->SetImage(grenade->GetShopImage());
        }
    }
    if (powerUp == NULL) {
        m_powerUpImage->ClearFlags(1);
    } else {
        m_powerUpImage->SetFlags(1);
        if (powerUp->GetCount() > 0) {
            ImageWindow* image = m_powerUpImage;
            image->SetImage(powerUp->GetShopImage());
        }
    }
}

void CWeaponEquipZombieDialogWindow::Init()
{
    if (m_offeredGrenade != NULL) {
        CGrenadeManager* manager = ((App*)WindowApp::m_instance)->m_grenadeManager;
        manager->SetActiveGrenade(m_offeredGrenade->getId());
    } else if (m_offeredPowerUp != NULL) {
        CPowerUpManager* manager = ((App*)WindowApp::m_instance)->m_powerUpManager;
        manager->SetActivePowerUp(m_offeredPowerUp->getId());
    } else {
        return;
    }
    WindowApp::HandleTunnelCommand(0x62e7ea20, 0, 0, 0);
    Close();
}

Window* CWeaponEquipZombieDialogWindow::CreateSlotWindow(const char* text, bool occupied,
    unsigned int command, int slot, ImageWindow** image, bool blink)
{
    // Original 14ef0e memsets the whole e4 block; arises naturally from value-initializing CustomButton, which has no user constructor.
    CustomButton* button = new CustomButton();
    button->SetLayoutType(LAYOUT_SIMPLE);
    button->SetWidthByContent(0, 0);
    button->SetHeightByContent(0, 0);
    button->m_command = command;
    button->m_commandParameter1 = slot;
    button->m_commandParameter2 = 0;
    button->m_commandParameter3 = 0;
    SGImageWindow* background = new SGImageWindow;
    background->SetArchetypeCharacter(2, 0);
    int animation = 8;
    if (occupied) {
        animation = 9;
    }
    background->SetAnimation(animation, 1);
    background->SetAlign(12);
    background->SetOutsetSpacing(SScreenAdaptingConstant(10)(), 0, 0, 0);
    button->AddToFront(background);
    SGButton* label = new SGButton(2, 0, 0, 1);
    label->SetLayoutType(LAYOUT_SIMPLE);
    label->SetText(ResString(text), (eGameFont)26);
    label->SetAlign(12);
    label->m_command = command;
    label->m_commandParameter1 = slot;
    label->m_commandParameter2 = 0;
    label->m_commandParameter3 = 0;
    button->AddToFront(label);
    ImageWindow* icon = new ImageWindow;
    icon->SetAlign(12);
    int top = 25;
    if (occupied) {
        top = 20;
    }
    icon->SetOutsetSpacing(SScreenAdaptingConstant(top)(), 0, 0, 0);
    icon->ClearFlags(1);
    button->AddToFront(icon);
    *image = icon;
    if (blink == 1) {
        label->m_blinking = blink;
        m_tutorialBlink = label;
    }
    return button;
}

CDH_Weapon* CWeaponEquipZombieDialogWindow::GetSlotWeapon(int slot)
{
    DGHelper* helper = ((App*)WindowApp::m_instance)->m_helper;
    if (m_swap) {
        slot = slot != 1;
    }
    return helper->m_gamePlayManager.m_playerData.GetGameData(helper->m_gamePlayManager.m_gameType)->GetWeapon(slot);
}

void CWeaponEquipZombieDialogWindow::WarnForbiddenWeaponConfiguration()
{
    CZombieDialogWindow* dialog = new CZombieDialogWindow(3);
    XString title = ResString("IDS_DIALOG_TITLE_WRONG_EQUIPMENT");
    dialog->SetTitle(title);
    XString text = ResString("IDS_DIALOG_TEXT_WRONG_EQUIPMENT");
    dialog->SetText(text, false, NULL);
    dialog->AddButton(BUTTON_OK, (unsigned int)-1, 0, 0, true, 20);
    m_parent->AddModal(dialog);
}

void CWeaponEquipZombieDialogWindow::OnCommand(Event& event)
{
    // b147: original block order is slot, toggle, OK (reverse of the old chain).
    if (event.m_id == 0x30826e06) {
        if (m_offeredWeapon != NULL) {
            // b147: one swap condition; its "selected = -1" store is tail-merged with the deselect branch.
            if ((event.m_param1 == 0 && GetSlotWeapon(1) == m_offeredWeapon) ||
                (event.m_param1 == 1 && GetSlotWeapon(0) == m_offeredWeapon)) {
                m_swap = !m_swap;
                m_selectedSlot = -1;
            } else if (GetSlotWeapon(event.m_param1) == m_offeredWeapon) {
                m_selectedSlot = -1;
            } else {
                m_selectedSlot = event.m_param1;
                if (m_tutorialBlink != NULL) {
                    m_tutorialBlink->m_blinking = false;
                    m_okButton->m_blinking = true;
                }
            }
        }
        UpdateSlots();
        event.Clear();
    } else if (event.m_id == (int)0xb7aa6264) {
        if (m_offeredGrenade != NULL && event.m_param1 == 0) {
            m_selectGrenade = !m_selectGrenade;
        }
        if (m_offeredPowerUp != NULL && event.m_param1 == 1) {
            m_selectPowerUp = !m_selectPowerUp;
        }
        UpdateSlots();
        event.Clear();
    } else if (event.m_id == (int)0x910d9f06) {
        ((App*)WindowApp::m_instance)->m_soundManager->Play((CDH_SoundManager::eSound)36, false, false);
        WindowApp::HandleTunnelCommand(0x0df6b618, m_selectedSlot, 0, 0);
        CDH_Weapon* offered = m_offeredWeapon;
        if (offered != NULL) {
            int slot = m_selectedSlot;
            if (slot >= 0) {
                // b147: one combined condition; the warning block is moved to the function end in the original.
                if (offered->m_projectileType == 2 &&
                    ((App*)WindowApp::m_instance)->m_helper->m_gamePlayManager.m_playerData.GetGameData(
                        ((App*)WindowApp::m_instance)->m_helper->m_gamePlayManager.m_gameType)->GetWeapon(slot == 0)->m_projectileType == 2) {
                    WarnForbiddenWeaponConfiguration();
                } else {
                    ((App*)WindowApp::m_instance)->m_helper->m_gamePlayManager.SetWeapon(slot, m_offeredWeapon);
                    CGameAnalytics::logItemEquipped(m_offeredWeapon->GetName());
                }
            }
            if (m_swap) {
                DGHelper* helper = ((App*)WindowApp::m_instance)->m_helper;
                CDH_Weapon* first = helper->m_gamePlayManager.m_playerData.GetGameData(helper->m_gamePlayManager.m_gameType)->GetWeapon(0);
                CDH_Weapon* second = helper->m_gamePlayManager.m_playerData.GetGameData(helper->m_gamePlayManager.m_gameType)->GetWeapon(1);
                helper->m_gamePlayManager.SetWeapon(0, second);
                helper->m_gamePlayManager.SetWeapon(1, first);
            }
        }
        if (m_offeredGrenade != NULL && m_selectGrenade) {
            CGrenadeManager* manager = ((App*)WindowApp::m_instance)->m_grenadeManager;
            manager->SetActiveGrenade(m_offeredGrenade->getId());
        }
        if (m_offeredPowerUp != NULL && m_selectPowerUp) {
            CPowerUpManager* manager = ((App*)WindowApp::m_instance)->m_powerUpManager;
            manager->SetActivePowerUp(m_offeredPowerUp->getId());
        }
    }
    CZombieDialogWindow::OnCommand(event);
}
