// k: original 1527e4..152d30, complete 13 ranges; HUD actually allocates bc, nested Ammo 11c.
#ifndef CGAMEPLAYWEAPONINFOWINDOW_H
#define CGAMEPLAYWEAPONINFOWINDOW_H
#include "WindowBase.h"
#include "BaseDialog.h"
#include "SGImageWindow.h"
class CDH_Weapon;

class CGameplayWeaponInfoWindow : public Window {
public:
    class CAmmoWin : public SGImageWindow {
    public:
        CAmmoWin();
        virtual ~CAmmoWin() {} // Original 152d0c only calls SGImageWindow.
        virtual void Paint(com::glu::platform::graphics::ICGraphics2d& graphics);
        virtual void Update();
        bool IsLowAmmo() const
        {
            // Original 152a18/152cec clearly share byte 109 inside the SG base class padding; not misplaced in the derived tail.
            return *reinterpret_cast<const bool*>(reinterpret_cast<const unsigned char*>(this) + 0x109);
        }
        SGImageWindow* m_warning; // 10c
        int m_currentAmmo;        // 110
        int m_clipSize;           // 114
        TextWindow* m_text;       // 118, total 11c.
    };

    CGameplayWeaponInfoWindow();
    virtual ~CGameplayWeaponInfoWindow();
    virtual void Init();
    virtual void Update();
    virtual void Paint(com::glu::platform::graphics::ICGraphics2d& graphics);
    bool CheckCollision(int x, int y);
    CDH_Weapon* m_weapon;    // ac
    ImageWindow* m_image;    // b0
    CAmmoWin* m_ammo;        // b4
    TextWindow* m_reloadText;// b8, total bc.
};
#endif
