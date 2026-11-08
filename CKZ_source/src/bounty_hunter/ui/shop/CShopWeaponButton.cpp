// k: complete Weapon button/info window; tutorial blinking and the original cancel-purchase condition are unchanged.
#include "CShopWeaponButton.h"
#include "CDH_Weapon.h"
#include "BaseDialog.h"
#include "CFontMgr.h"
#include "SScreenAdaptingConstant.h"
#include "ServiceWindows.h"
#include "CGameAnalytics.h"
#include "CTutorialManager.h"
#include "SGButton.h"
#include "App.h"

CShopWeaponButton::CShopWeaponButton(CDH_Weapon* weapon) : CShopRectItemButton(weapon)
{
    m_weapon = weapon;
    m_imageContainer = new ImageWindow();
    m_imageContainer->SetImage(m_weapon->GetShopImage());
    m_imageContainer->SetLayoutType(LAYOUT_SIMPLE);
    m_imageContainer->SetAlign(0x21);
    AddToFront(m_imageContainer);
    AddStrips();
    AddMarkers();
    Window* statistics = new WindowTransparent(false);
    const char* damageLabel = "IDS_WEAPON_STAT_DMG";
    if (weapon->m_projectileType == 2) {
        damageLabel = "IDS_WEAPON_STAT_HP";
    }
    Window* damage = CreateStatTextWindow(ResString(damageLabel), weapon->m_damageDesc);
    damage->SetCellPos(0, 0, 1, 1);
    statistics->AddToFront(damage);
    Window* accuracy = CreateStatTextWindow(ResString("IDS_WEAPON_STAT_ACC"), weapon->m_accuracyDesc);
    accuracy->SetCellPos(0, 1, 1, 1);
    statistics->AddToFront(accuracy);
    Window* capacity = CreateStatTextWindow(ResString("IDS_WEAPON_STAT_CAP"), XString(weapon->m_clipSize));
    capacity->SetCellPos(0, 2, 1, 1);
    statistics->AddToFront(capacity);
    statistics->SetWidthByContent(0, 0);
    statistics->SetHeightByContent(0, 0);
    Window* table = createTD(statistics, 3);
    int first = SScreenAdaptingConstant(10)();
    int second = SScreenAdaptingConstant(10)();
    table->SetOutsetSpacing(first, second, 0, 0);
    table->SetAlign(0x22);
    AddToFront(table);
    if (((App*)WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial() == 1) {
        CTutorialManager* tutorial = ((App*)WindowApp::m_instance)->m_tutorialManager;
        if (tutorial->IsWeaponButtonBlink(m_weapon->getId()) == 1) {
            m_flagEc = true;
        }
    }
}

bool CShopWeaponButton::IsAfterBuyDialogShown() const
{
    return m_weapon->isAvailable();
}

Window* CShopWeaponButton::CreateInfoDialog()
{
    InfoWindow* dialog = new InfoWindow(m_weapon);
    dialog->SetTitle(m_item->GetName());
    if (m_weapon->isAvailable() == 1) {
        SGButton* equip = dialog->AddButton((CZombieDialogWindow::EDialogButton)6, 0xd34330d5,
                                            (int)m_weapon, 0, true, 20);
        dialog->AddButton((CZombieDialogWindow::EDialogButton)4, 0x62e7ea20, 0, 0, true, 20);
        if (((App*)WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial() == 1) {
            CTutorialManager* tutorial = ((App*)WindowApp::m_instance)->m_tutorialManager;
            if (tutorial->IsEquipButtonBlink(m_weapon->getId()) == 1) {
                equip->m_blinking = true;
            }
        }
    } else if (m_weapon->IsLocked() == 1) {
        dialog->AddButton((CZombieDialogWindow::EDialogButton)1, -1, 0, 0, true, 20);
    } else {
        SGButton* buy = dialog->AddButton((CZombieDialogWindow::EDialogButton)5, 0x3b7082f1,
                                          (int)this, 0, true, 20);
        dialog->AddButton((CZombieDialogWindow::EDialogButton)4, 0x62e7ea20, 0, 0, true, 20);
        if (((App*)WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial() == 1) {
            if (((App*)WindowApp::m_instance)->m_tutorialManager->IsBuyButtonBlink() == 1) {
                buy->m_blinking = true;
            }
        }
    }
    return dialog;
}

void CShopWeaponButton::Update()
{
    if (((App*)WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial() == 1) {
        CTutorialManager* tutorial = ((App*)WindowApp::m_instance)->m_tutorialManager;
        m_flagEc = tutorial->IsWeaponButtonBlink(m_weapon->getId()) == 1;
    }
    CShopRectItemButton::Update();
}

CShopWeaponButton::~CShopWeaponButton()
{
}

CShopWeaponButton::InfoWindow::InfoWindow(CDH_Weapon* weapon) : CZombieDialogWindow(2)
{
    m_weapon = weapon;
    m_availableAtOpen = weapon->isAvailable();
    SetOutsetSpacing(SScreenAdaptingConstant(-20)(), 0, 0, 0);
    m_contentContainer->SetHeightByContent(0, 0);
    m_contentContainer->SetAlign(0xc);
    m_contentContainer->SetOutsetSpacing(SScreenAdaptingConstant(50)(), 0, 0, 0);
    ImageWindow* image = new ImageWindow();
    image->SetImage(weapon->GetShopImage());
    image->SetCellPos(0, 0, 1, 1);
    m_contentContainer->AddToFront(image);
    if (m_weapon->IsLocked() == 1) {
        TextWindow* requirement = new TextWindow();
        XString text = XString::Format(ResString("IDS_SHOP_WEAPON_REQUIRES_LEVEL").m_data,
                                       weapon->getUnlockLvl());
        requirement->SetAlign(0xc);
        requirement->SetText(text, CFontMgr::GetInstance()->GetFont(27));
        requirement->SetPercentWidth(100, 0, 0);
        image->AddToFront(requirement);
    }
    Window* column = new WindowTransparent(false);
    column->SetPercentWidth(100, 0, 0);
    m_contentContainer->AddToFront(column, 1, 0);
    Window* statistics = new WindowTransparent(false);
    const char* damageLabel = "IDS_WEAPON_STAT_DMG";
    if (weapon->m_projectileType == 2) {
        damageLabel = "IDS_WEAPON_STAT_HP";
    }
    Window* damage = CShopRectItemButton::CreateStatTextWindow(ResString(damageLabel), weapon->m_damageDesc);
    damage->SetCellPos(0, 0, 1, 1);
    statistics->AddToFront(damage);
    Window* accuracy = CShopRectItemButton::CreateStatTextWindow(ResString("IDS_WEAPON_STAT_ACC"), weapon->m_accuracyDesc);
    accuracy->SetCellPos(0, 1, 1, 1);
    statistics->AddToFront(accuracy);
    Window* capacity = CShopRectItemButton::CreateStatTextWindow(ResString("IDS_WEAPON_STAT_CAP"), XString(weapon->m_clipSize));
    capacity->SetCellPos(0, 2, 1, 1);
    statistics->AddToFront(capacity);
    statistics->SetWidthByContent(0, 0);
    Window* table = createTD(statistics, 3);
    table->SetOutsetSpacing(0, SScreenAdaptingConstant(30)(), 0, 0);
    table->SetAlign(2);
    m_contentContainer->AddToFront(table, 2, 0);
    TextWindow* description = new TextWindow();
    XString& descriptionText = weapon->GetDescription();
    description->SetText(descriptionText, CFontMgr::GetInstance()->GetFont(24));
    description->SetPercentWidth(90, 0, 0);
    description->SetCellPos(0, 1, 3, 1);
    m_contentContainer->AddToFront(description);
    int credits = m_weapon->getPriceCredits();
    XString& name = m_weapon->GetName();
    if (credits != 0) {
        CGameAnalytics::logItemViewed(XString(name), m_weapon->getPriceCredits(), CGameAnalytics::CURRENCY_HARD);
    } else {
        CGameAnalytics::logItemViewed(XString(name), m_weapon->getPricePoints(), CGameAnalytics::CURRENCY_SOFT);
    }
}

CShopWeaponButton::InfoWindow::~InfoWindow()
{
    bool locked = m_weapon->IsLocked();
    bool available = m_weapon->isAvailable(); // The original calls it unconditionally even when locked.
    if (!locked && !available && available == m_availableAtOpen) {
        int credits = m_weapon->getPriceCredits();
        XString& name = m_weapon->GetName();
        if (credits != 0) {
            CGameAnalytics::logItemPurchaseCancelled(XString(name), m_weapon->getPriceCredits(), CGameAnalytics::CURRENCY_HARD);
        } else {
            CGameAnalytics::logItemPurchaseCancelled(XString(name), m_weapon->getPricePoints(), CGameAnalytics::CURRENCY_SOFT);
        }
    }
}
