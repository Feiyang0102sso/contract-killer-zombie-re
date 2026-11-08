// u545 real Zombie derivative; original 14ea4c and consumer 160850 confirm the full 148.
#ifndef CWEAPONEQUIPZOMBIEDIALOGWINDOW_H
#define CWEAPONEQUIPZOMBIEDIALOGWINDOW_H
#include "CZombieDialogWindow.h"
class CDH_Weapon;
class CGrenade;
class CPowerUp;
class ImageWindow;

class CWeaponEquipZombieDialogWindow : public CZombieDialogWindow {
public:
    CWeaponEquipZombieDialogWindow(CDH_Weapon* weapon, CGrenade* grenade, CPowerUp* powerUp);
    virtual ~CWeaponEquipZombieDialogWindow() {}
    virtual void Init();
    virtual void OnCommand(Event& event);
    Window* CreateWeaponSlotWindow(int slot);
    void UpdateSlots();
    Window* CreateSlotWindow(const char* text, bool occupied, unsigned int command,
                            int slot, ImageWindow** image, bool blink);
    CDH_Weapon* GetSlotWeapon(int slot);
    void WarnForbiddenWeaponConfiguration();

    ImageWindow* m_weaponImages[2]; // 118/11c
    ImageWindow* m_grenadeImage;    // 120
    ImageWindow* m_powerUpImage;    // 124
    int m_selectedSlot;            // 128
    bool m_swap;                   // 12c
    CDH_Weapon* m_offeredWeapon;    // 130
    bool m_selectGrenade;          // 134
    bool m_selectPowerUp;          // 135
    CGrenade* m_offeredGrenade;     // 138
    CPowerUp* m_offeredPowerUp;     // 13c
    SGButton* m_tutorialBlink;      // 140
    SGButton* m_okButton;           // 144
};
#endif
