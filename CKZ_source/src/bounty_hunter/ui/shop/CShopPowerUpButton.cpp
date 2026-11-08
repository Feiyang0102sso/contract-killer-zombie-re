// k: complete PowerUp button/info window; CItem and the window tree each own their own resources.
#include "CShopPowerUpButton.h"
#include "CShopPack.h"
#include "CPowerUp.h"
#include "BaseDialog.h"
#include "CFontMgr.h"
#include "SScreenAdaptingConstant.h"
#include "ServiceWindows.h"
#include "CGameAnalytics.h"

CShopPowerUpButton::CShopPowerUpButton(CPowerUpPack* pack) : CShopRectItemButton(pack)
{
    m_powerUpPack = pack;
    m_imageContainer = new ImageWindow();
    m_imageContainer->SetImage(m_powerUpPack->GetShopImage());
    m_imageContainer->SetLayoutType(LAYOUT_SIMPLE);
    m_imageContainer->SetAlign(0x21);
    AddToFront(m_imageContainer);
    TextWindow* count = new TextWindow();
    XString formatted = XString::Format(ResString("IDS_SHOP_GRENADE_PACK_COUNT").m_data,
                                         pack->GetInPackCount());
    count->SetText(formatted, CFontMgr::GetInstance()->GetFont(26));
    count->SetAlign(0x21);
    count->SetOutsetSpacing(0, 0, 0, SScreenAdaptingConstant(70)());
    AddToFront(count);
    AddInInventoryCountPanel();
    AddStrips();
    AddMarkers();
}

Window* CShopPowerUpButton::CreateInfoDialog()
{
    InfoWindow* dialog = new InfoWindow(m_powerUpPack);
    CPowerUp* powerUp = m_powerUpPack->m_powerUp;
    if (!m_powerUpPack->IsLocked()) {
        dialog->AddButton((CZombieDialogWindow::EDialogButton)5, 0x3b7082f1,
                          (int)this, 0, true, 20);
    }
    if (powerUp->GetCount() >= 1) {
        dialog->AddButton((CZombieDialogWindow::EDialogButton)7, 0xe503ba9c,
                          (int)powerUp, 0, true, 20);
    }
    dialog->AddButton((CZombieDialogWindow::EDialogButton)4, 0x62e7ea20, 0, 0, true, 20);
    return dialog;
}

void CShopPowerUpButton::Update()
{
    CShopRectItemButton::Update();
}

CShopPowerUpButton::~CShopPowerUpButton()
{
}

CShopPowerUpButton::InfoWindow::InfoWindow(CPowerUpPack* pack) : CZombieDialogWindow(2)
{
    m_pack = pack;
    m_flag11c = false;
    SetOutsetSpacing(SScreenAdaptingConstant(-20)(), 0, 0, 0);
    m_contentContainer->SetHeightByContent(0, 0);
    m_contentContainer->SetAlign(0xc);
    m_contentContainer->SetOutsetSpacing(SScreenAdaptingConstant(50)(), 0, 0, 0);
    ImageWindow* image = new ImageWindow();
    image->SetImage(pack->GetShopImage());
    image->SetCellPos(0, 0, 1, 1);
    m_contentContainer->AddToFront(image);
    CPowerUp* powerUp = pack->m_powerUp;
    SetTitle(powerUp->GetName());
    Window* inventory = CShopRectItemButton::CreateInInventoryCountPanel(powerUp);
    inventory->SetOutsetSpacing(SScreenAdaptingConstant(16)(), 0, 0, 0);
    m_contentContainer->AddToFront(createTD(inventory, 3), 1, 0);
    Window* column = new WindowTransparent(false);
    column->SetPercentWidth(100, 0, 0);
    m_contentContainer->AddToFront(column, 2, 0);
    TextWindow* description = new TextWindow();
    XString& descriptionText = powerUp->GetDescription();
    description->SetText(descriptionText, CFontMgr::GetInstance()->GetFont(24));
    description->SetPercentWidth(90, 0, 0);
    description->SetCellPos(0, 1, 3, 1);
    m_contentContainer->AddToFront(description);
    int credits = m_pack->getPriceCredits();
    XString& name = m_pack->GetName();
    if (credits != 0) {
        CGameAnalytics::logItemViewed(XString(name), m_pack->getPriceCredits(), CGameAnalytics::CURRENCY_HARD);
    } else {
        CGameAnalytics::logItemViewed(XString(name), m_pack->getPricePoints(), CGameAnalytics::CURRENCY_SOFT);
    }
}

CShopPowerUpButton::InfoWindow::~InfoWindow()
{
    if (!m_pack->IsLocked()) {
        int credits = m_pack->getPriceCredits();
        XString& name = m_pack->GetName();
        if (credits != 0) {
            CGameAnalytics::logItemPurchaseCancelled(XString(name), m_pack->getPriceCredits(), CGameAnalytics::CURRENCY_HARD);
        } else {
            CGameAnalytics::logItemPurchaseCancelled(XString(name), m_pack->getPricePoints(), CGameAnalytics::CURRENCY_SOFT);
        }
    }
}
