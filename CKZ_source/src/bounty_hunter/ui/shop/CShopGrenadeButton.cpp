// k: complete implementation of the original Grenade button and nested InfoWindow, keeping both images and the repeated price query.
#include "CShopGrenadeButton.h"
#include "CShopPack.h"
#include "CGrenade.h"
#include "BaseDialog.h"
#include "CFontMgr.h"
#include "SScreenAdaptingConstant.h"
#include "ServiceWindows.h"
#include "CGameAnalytics.h"

CShopGrenadeButton::CShopGrenadeButton(CGrenadePack* pack) : CShopRectItemButton(pack)
{
    m_grenadePack = pack;
    CGrenade* grenade = pack->m_grenade;
    TextWindow* count = new TextWindow();
    XString formatted = XString::Format(ResString("IDS_SHOP_GRENADE_PACK_COUNT").m_data,
                                         pack->GetInPackCount());
    count->SetText(formatted, CFontMgr::GetInstance()->GetFont(26));
    count->SetAlign(0x21);
    count->SetOutsetSpacing(0, 0, 0, SScreenAdaptingConstant(106)());
    AddToFront(count);
    Window* packImage = pack->CreatePackImage();
    packImage->SetLayoutType(LAYOUT_SIMPLE);
    packImage->SetAlign(0x21);
    packImage->SetOutsetSpacing(0, 0, 0, SScreenAdaptingConstant(35)());
    AddToFront(packImage);
    m_imageContainer = new ImageWindow();
    m_imageContainer->SetImage(m_grenadePack->GetShopImage());
    m_imageContainer->SetLayoutType(LAYOUT_SIMPLE);
    m_imageContainer->SetAlign(0x21);
    AddToFront(m_imageContainer);
    AddInInventoryCountPanel();
    AddStrips();
    AddMarkers();
    Window* statistics = new WindowTransparent(false);
    Window* damage = CreateStatTextWindow(ResString("IDS_GRENADE_STAT_DMG"), grenade->m_damageDescription);
    damage->SetCellPos(0, 0, 1, 1);
    statistics->AddToFront(damage);
    Window* radius = CreateStatTextWindow(ResString("IDS_GRENADE_STAT_RADIUS"), grenade->m_radiusDescription);
    radius->SetCellPos(0, 1, 1, 1);
    statistics->AddToFront(radius);
    Window* delay = CreateStatTextWindow(ResString("IDS_GRENADE_STAT_DELAY"), grenade->m_delayDescription);
    delay->SetCellPos(0, 2, 1, 1);
    statistics->AddToFront(delay);
    statistics->SetWidthByContent(0, 0);
    statistics->SetHeightByContent(0, 0);
    Window* table = createTD(statistics, 3);
    int first = SScreenAdaptingConstant(10)();
    int second = SScreenAdaptingConstant(10)();
    table->SetOutsetSpacing(first, second, 0, 0);
    table->SetAlign(0x22);
    AddToFront(table);
}

Window* CShopGrenadeButton::CreateInfoDialog()
{
    InfoWindow* dialog = new InfoWindow(m_grenadePack);
    CGrenade* grenade = m_grenadePack->m_grenade;
    if (!m_grenadePack->IsLocked()) {
        dialog->AddButton((CZombieDialogWindow::EDialogButton)5, 0x3b7082f1,
                          (int)this, 0, true, 20);
    }
    if (grenade->GetCount() >= 1) {
        dialog->AddButton((CZombieDialogWindow::EDialogButton)7, 0xcf03ebf0,
                          (int)grenade, 0, true, 20);
    }
    dialog->AddButton((CZombieDialogWindow::EDialogButton)4, 0x62e7ea20, 0, 0, true, 20);
    return dialog;
}

void CShopGrenadeButton::Update()
{
    CShopRectItemButton::Update();
}

CShopGrenadeButton::~CShopGrenadeButton()
{
}

CShopGrenadeButton::InfoWindow::InfoWindow(CGrenadePack* pack) : CZombieDialogWindow(2)
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
    CGrenade* grenade = pack->m_grenade;
    SetTitle(grenade->GetName());
    Window* inventory = CShopRectItemButton::CreateInInventoryCountPanel(grenade);
    inventory->SetOutsetSpacing(SScreenAdaptingConstant(16)(), 0, 0, 0);
    m_contentContainer->AddToFront(createTD(inventory, 3), 1, 0);
    if (pack->IsLocked() == 1) {
        TextWindow* requirement = new TextWindow();
        XString text = XString::Format(ResString("IDS_SHOP_WEAPON_REQUIRES_LEVEL").m_data,
                                       pack->getUnlockLvl());
        requirement->SetAlign(0xc);
        requirement->SetText(text, CFontMgr::GetInstance()->GetFont(27));
        requirement->SetPercentWidth(100, 0, 0);
        image->AddToFront(requirement);
    }
    Window* column = new WindowTransparent(false);
    column->SetPercentWidth(100, 0, 0);
    m_contentContainer->AddToFront(column, 2, 0);
    Window* statistics = new WindowTransparent(false);
    Window* damage = CShopRectItemButton::CreateStatTextWindow(ResString("IDS_GRENADE_STAT_DMG"), grenade->m_damageDescription);
    damage->SetCellPos(0, 0, 1, 1);
    statistics->AddToFront(damage);
    Window* radius = CShopRectItemButton::CreateStatTextWindow(ResString("IDS_GRENADE_STAT_RADIUS"), grenade->m_radiusDescription);
    radius->SetCellPos(0, 1, 1, 1);
    statistics->AddToFront(radius);
    Window* delay = CShopRectItemButton::CreateStatTextWindow(ResString("IDS_GRENADE_STAT_DELAY"), grenade->m_delayDescription);
    delay->SetCellPos(0, 2, 1, 1);
    statistics->AddToFront(delay);
    statistics->SetWidthByContent(0, 0);
    Window* table = createTD(statistics, 3);
    table->SetOutsetSpacing(0, SScreenAdaptingConstant(30)(), 0, 0);
    table->SetAlign(2);
    m_contentContainer->AddToFront(table, 3, 0);
    TextWindow* description = new TextWindow();
    XString& descriptionText = grenade->GetDescription();
    description->SetText(descriptionText, CFontMgr::GetInstance()->GetFont(24));
    description->SetPercentWidth(90, 0, 0);
    description->SetCellPos(0, 1, 4, 1);
    m_contentContainer->AddToFront(description);
    int credits = m_pack->getPriceCredits();
    XString& name = m_pack->GetName();
    if (credits != 0) {
        CGameAnalytics::logItemViewed(XString(name), m_pack->getPriceCredits(), CGameAnalytics::CURRENCY_HARD);
    } else {
        CGameAnalytics::logItemViewed(XString(name), m_pack->getPricePoints(), CGameAnalytics::CURRENCY_SOFT);
    }
}

CShopGrenadeButton::InfoWindow::~InfoWindow()
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
