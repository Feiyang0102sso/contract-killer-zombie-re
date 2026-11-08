// k: original 1527e4..152d30; keeps allocation, temporary string lifetimes and the cache re-read after the callback.
#include "CGameplayWeaponInfoWindow.h"
#include "App.h"
#include "DGHelper.h"
#include "CDH_Weapon.h"
#include "CFontMgr.h"
#include "SScreenAdaptingConstant.h"
#include "ServiceWindows.h"
using namespace com::glu::platform::graphics;

CGameplayWeaponInfoWindow::CGameplayWeaponInfoWindow() : Window(false), m_weapon(NULL)
{
    m_image = new ImageWindow;
    m_image->SetCellPos(0, 0, 2, 1);
    ImageWindow* image = m_image;
    int imageBottom = SScreenAdaptingConstant(5)();
    image->SetOutsetSpacing(0, 0, imageBottom, 0);
    AddToFront(m_image);
    m_ammo = new CAmmoWin;
    m_ammo->SetLayoutType(LAYOUT_SIMPLE);
    m_ammo->SetAlign(2);
    AddToFront(createTD(m_ammo, 3), 1, 1);
    TextWindow* reload = new TextWindow;
    m_reloadText = reload;
    {
        XString text = ResString("IDS_HUD_RELOAD");
        reload->SetText(text, CFontMgr::GetInstance()->GetFont(27));
    }
    m_reloadText->SetAlign(36);
    reload = m_reloadText;
    int right = SScreenAdaptingConstant(5)();
    int left = SScreenAdaptingConstant(25, 30)();
    reload->SetOutsetSpacing(0, right, 0, left);
    AddToFront(createTD(m_reloadText, 3), 0, 1);
    SetHeightByContent(0, 0);
    SetWidthByContent(0, 0);
}

CGameplayWeaponInfoWindow::~CGameplayWeaponInfoWindow() {}
void CGameplayWeaponInfoWindow::Init() {} // Original 1529c8 bx lr+nop.
void CGameplayWeaponInfoWindow::Paint(ICGraphics2d&) {} // Original 152a34 bx lr+nop.

void CGameplayWeaponInfoWindow::Update()
{
    CDH_Weapon* weapon = ((App*)WindowApp::m_instance)->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon();
    if (m_weapon != weapon && weapon != NULL) {
        ImageWindow* image = m_image;
        image->SetImage(weapon->gameImage());
    }
    m_weapon = weapon; // The original writes it back even when NULL or unchanged.
    CAmmoWin* ammo = m_ammo;
    TextWindow* reload = m_reloadText;
    if (ammo->IsLowAmmo()) {
        reload->SetFlags(1);
    } else {
        reload->ClearFlags(1);
    }
}

bool CGameplayWeaponInfoWindow::CheckCollision(int x, int y)
{
    if (x <= m_x || x >= m_x + m_width) {
        return false;
    }
    if (y <= m_y || y >= m_y + m_height) {
        return false;
    }
    return IsVisible();
}

CGameplayWeaponInfoWindow::CAmmoWin::CAmmoWin()
{
    SetArchetypeCharacter(0, 0);
    SetAnimation(10, 1);
    m_text = new TextWindow(CFontMgr::GetInstance()->GetFont(25));
    m_text->SetLayoutType(LAYOUT_SIMPLE);
    m_text->SetAlign(36);
    TextWindow* text = m_text;
    int top = SScreenAdaptingConstant(4, 4, 4, 5)();
    int right = SScreenAdaptingConstant(9, 18, 18, 18)();
    text->SetOutsetSpacing(top, right, 0, 0);
    m_text->SetWidthByContent(0, 0);
    m_text->SetHeightByContent(0, 0);
    AddToFront(m_text);
    m_warning = new SGImageWindow;
    m_warning->SetTranslateCoords(false);
    m_warning->SetArchetypeCharacter(0, 0);
    m_warning->SetAnimation(11, 1);
    m_warning->SetLayoutType(LAYOUT_SIMPLE);
    m_warning->SetAlign(9);
    m_warning->ClearFlags(1);
    AddToFront(m_warning);
    // In the original the two caches are written only after the whole subtree is created; not moved forward into the initializer list.
    m_currentAmmo = -1;
    m_clipSize = -1;
}

void CGameplayWeaponInfoWindow::CAmmoWin::Paint(ICGraphics2d& graphics)
{
    SGImageWindow::Paint(graphics);
}

void CGameplayWeaponInfoWindow::CAmmoWin::Update()
{
    CDH_Weapon* weapon = ((App*)WindowApp::m_instance)->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon();
    if (weapon == NULL) {
        return;
    }
    int clipSize = weapon->m_clipSize;
    int ammo = weapon->m_ammo;
    int current = m_currentAmmo;
    if (ammo != current || clipSize != m_clipSize) {
        m_currentAmmo = ammo;
        m_clipSize = clipSize;
        {
            XString text;
            if (App::IsVGA() != 0 || App::IsXGA() == 1) {
                text.Assign(XString::Format(L"%d / %d", m_currentAmmo, m_clipSize));
            } else {
                text.Assign(XString::Format(L"%d/%d", m_currentAmmo, m_clipSize));
            }
            m_text->SetText(text);
        }
        current = m_currentAmmo;
        clipSize = m_clipSize;
    } else {
        clipSize = m_clipSize;
    }
    bool lowAmmo = (float)current < (float)clipSize * 0.34f;
    *reinterpret_cast<bool*>(reinterpret_cast<unsigned char*>(this) + 0x109) = lowAmmo;
    if (lowAmmo) {
        m_warning->SetFlags(1);
    } else {
        m_warning->ClearFlags(1);
    }
}
