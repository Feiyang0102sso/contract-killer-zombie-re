// k: complete ShopWindow owner; local/inline creators and the PlayHaven badge keep the original ownership relations.
#include "CShopWindow.h"
#include "CShopTabWindow.h"
#include "CShopItemButton.h"
#include "SScreenAdaptingConstant.h"
// Two groups of adapting constants the original GLOBAL initializes before the IAP ClassId call; the original variable names were not retained.
static SScreenAdaptingConstant SHOP_ITEM_WIDTH(320, 640, 768, 512);
static SScreenAdaptingConstant SHOP_ITEM_HEIGHT(50, 100, 90, 80);
#include "ICInAppPurchase.h"
static SScreenAdaptingConstant SCROLL_PAGE_WIDTH(320);
SScreenAdaptingConstant SCROLL_PAGE_HEIGHT(170, -1, -1, 250);
#include "App.h"
#include "CUpdateManager.h"
#include "CIAPCurrencyManager.h"
#include "CShopPackManager.h"
#include "CDH_WeaponsManager.h"
#include "CTutorialManager.h"
#include "CHuntingInfo.h"
#include "CEffectsWindow.h"
#include "CBH_GPSHud.h"
#include "SGImageWindow.h"
#include "SGImageWindowCommand.h"
#include "CSettingsWindow.h"
#include "BaseDialog.h"
#include "ServiceWindows.h"
#include "CFontMgr.h"
#include "CGameAnalytics.h"
#include "CDH_SoundManager.h"
#include "ICAdManager.h"
#include "TapjoyInterface.h"
#include "CPHInterface.h"
#include "CNotEnoughCurrencyDialogWindow.h"
#include "CWeaponEquipZombieDialogWindow.h"
#include "CWaitPurchaseWindow.h"

class CustomBankButtonCreator : public CShopTabContentWindow::ItemButtonCreator {
public:
    explicit CustomBankButtonCreator(int type) : m_type(type) {}
    virtual ~CustomBankButtonCreator() {}
    virtual CShopItemButton* create()
    {
        if (m_type == 2) {
            if (((App*)WindowApp::m_instance)->m_iapCurrencyManager->GetAvailableItemsCount() > 0) {
                return NULL;
            }
        }
        CShopItemButton* button = new CShopItemButton(NULL);
        SGImageWindow* background = new SGImageWindow();
        background->SetArchetypeCharacter(1, 0);
        background->SetAnimation(27, 1);
        button->AddToFront(background);
        button->SetSize(background->m_imageWidth, background->m_imageHeight);
        SGImageWindow* icon = new SGImageWindow();
        icon->SetArchetypeCharacter(1, 0);
        switch (m_type) {
        case 0:
            icon->SetAnimation(69, 1);
            icon->SetOutsetSpacing(0, 0, SScreenAdaptingConstant(10)(), 0);
            button->m_command = 0x822663d7;
            button->m_commandParameter1 = 0;
            button->m_commandParameter2 = 0;
            button->m_commandParameter3 = 0;
            break;
        case 1:
            icon->SetAnimation(64, 1);
            icon->SetOutsetSpacing(0, 0, SScreenAdaptingConstant(20)(), 0);
            button->m_command = 0x71f7f6e4;
            button->m_commandParameter1 = 0;
            button->m_commandParameter2 = 0;
            button->m_commandParameter3 = 0;
            break;
        case 2:
            icon->SetAnimation(69, 1);
            icon->SetOutsetSpacing(0, 0, SScreenAdaptingConstant(10)(), 0);
            break;
        }
        icon->SetAlign(0x14);
        button->AddToFront(icon);
        BlinkingContainer* caption = new BlinkingContainer();
        caption->SetAlign(0xc);
        caption->SetOutsetSpacing(SScreenAdaptingConstant(4)(), 0, 0, 0);
        caption->SetPercentWidth(90, 0, 0);
        caption->SetPercentHeight(30, 0, 0);
        TextWindow* text = new TextWindow();
        switch (m_type) {
        case 0:
            text->SetText(Window::ResString("IDS_SHOP_CAPTION_TAPJOY_BANK"), CFontMgr::GetInstance()->GetFont(26));
            break;
        case 1:
            text->SetText(Window::ResString("IDS_SHOP_CAPTION_PLAYHAVEN"), CFontMgr::GetInstance()->GetFont(26));
            break;
        case 2:
            caption->m_blinking = true;
            text->SetText(Window::ResString("IDS_SHOP_IAP_LOADING_ITEM_TEXT"), CFontMgr::GetInstance()->GetFont(26));
            break;
        }
        text->SetAlign(0x24);
        text->SetPercentWidth(100, 0, 0);
        text->SetHeightByContent(0, 0);
        caption->AddToFront(text);
        button->AddToFront(caption);
        return button;
    }
    virtual bool filter(int value) { return false; }
    virtual bool show() const { return true; }
    virtual int getId() const { return -666; }
    int m_type;
};

class CustomWeaponButtonCreator : public CShopTabContentWindow::ItemButtonCreator {
public:
    class PlayHavenBadge : public Window {
    public:
        PlayHavenBadge(Window* view, Window* icon) : Window(false)
        {
            m_view = view;
            m_icon = icon;
            m_frames = 0;
            m_anchorX = -1;
            m_anchorY = -1;
            m_shown = false;
            m_lastTime = 0;
        }
        void Update(bool enabled)
        {
            if (m_frames <= 14) {
                m_frames++;
                return;
            }
            int globalX = m_icon->LocalToGlobalX(0);
            int width = m_icon->m_width;
            int globalY = m_icon->LocalToGlobalY(0);
            int height = m_icon->m_height;
            int x = (int)((double)globalX + (double)width * 0.95);
            int y = (int)((double)globalY + (double)height * 0.3);
            if (App::IsVGA() == 1) {
                y /= 2;
                x /= 2;
            }
            if (m_frames == 15) {
                m_frames++;
                m_anchorX = x;
                m_anchorY = y;
            }
            if (x == m_anchorX && y == m_anchorY && enabled) {
                if (!m_shown) {
                    unsigned int time = (unsigned int)CStdUtil_Cocoa::GetUpTimeMS();
                    if (time - m_lastTime > 500) {
                        CPHInterface::showNotificationView("more_games", false, x, y);
                        m_shown = true;
                        m_lastTime = (unsigned int)CStdUtil_Cocoa::GetUpTimeMS();
                    }
                }
            } else if (m_shown) {
                CPHInterface::hideNotificationView("more_games");
                m_shown = false;
            }
        }
        virtual ~PlayHavenBadge()
        {
            if (m_shown) {
                CPHInterface::hideNotificationView("more_games");
                m_shown = false;
            }
        }
        Window* m_view; // ac
        Window* m_icon; // b0
        int m_frames; // b4
        int m_anchorX; // b8, cached position independent of Window.
        int m_anchorY; // bc
        bool m_shown; // c0
        unsigned int m_lastTime; // c4
    };
    CustomWeaponButtonCreator() : m_badge(NULL) {}
    virtual ~CustomWeaponButtonCreator()
    {
        if (m_badge != NULL) {
            delete m_badge;
            m_badge = NULL;
        }
    }
    virtual CShopItemButton* create()
    {
        class CSimpleButton : public CShopItemButton {
        public:
            CSimpleButton() : CShopItemButton(NULL) {}
            virtual ~CSimpleButton() {}
            virtual void OnPointerEvent(Event& event) {}
            virtual void Paint(com::glu::platform::graphics::ICGraphics2d& graphics) {}
        };
        CShopItemButton* button = new CSimpleButton();
        SGImageWindow* background = new SGImageWindow();
        background->SetArchetypeCharacter(1, 0);
        background->SetAnimation(26, 1);
        button->AddToFront(background);
        button->SetSize(background->m_imageWidth, background->m_imageHeight);
        background->ClearFlags(1);
        Window* container = new WindowTransparent(false);
        Window* icon;
        Window* first = CreateItem(Window::ResString("IDS_SHOP_CAPTION_PLAYHAVEN"), 64, 0x71f7f6e4, 0, &icon);
        container->AddToFront(first, 0, 0);
        if (m_badge != NULL) {
            delete m_badge;
            m_badge = NULL;
        }
        m_badge = new PlayHavenBadge(first, icon);
        container->AddToFront(CreateItem(Window::ResString("IDS_SHOP_CAPTION_TAPJOY"), 69, 0x3a3412b7, 2, NULL), 1, 0);
        button->AddToFront(container);
        return button;
    }
    virtual bool filter(int value) { return false; }
    virtual bool show() const { return true; }
    virtual int getId() const { return -666; }
    Window* CreateItem(XString caption, int animation, unsigned int command, int parameter, Window** icon)
    {
        CustomButton* button = new CustomButton();
        SGImageWindow* background = new SGImageWindow();
        background->SetArchetypeCharacter(1, 0);
        background->SetAnimation(68, 1);
        background->SetAlign(0x24);
        button->AddToFront(background);
        SGImageWindow* image = new SGImageWindow();
        if (icon != NULL) {
            *icon = image;
        }
        image->SetArchetypeCharacter(1, 0);
        image->SetAnimation(animation, 1);
        image->SetAlign(0x24);
        button->AddToFront(image);
        Window* container = new WindowTransparent(false);
        container->SetAlign(0x14);
        int first = SScreenAdaptingConstant(8)();
        int third = SScreenAdaptingConstant(10)();
        container->SetOutsetSpacing(first, 0, third, 0);
        container->SetPercentWidth(100, 0, 0);
        container->SetHeightByContent(0, 0);
        TextWindow* text = new TextWindow();
        text->SetText(caption, CFontMgr::GetInstance()->GetFont(26));
        text->SetAlign(0x24);
        text->SetPercentWidth(100, 0, 0);
        text->SetHeightByContent(0, 0);
        container->AddToFront(text);
        button->AddToFront(container);
        button->m_command = command;
        button->m_commandParameter1 = parameter;
        button->m_commandParameter2 = 0;
        button->m_commandParameter3 = 0;
        return button;
    }
    PlayHavenBadge* m_badge; // 4, owned, not attached to the window tree.
};

CShopWindow* CShopWindow::m_instance = NULL;

CShopWindow::CShopWindow() : CBaseScreen(false, true, false)
{
    m_adVisible = false;
    m_purchasePending = 0;
    m_unresolvedD8 = 0;
    if (((App*)WindowApp::m_instance)->m_updateManager->hasCompleteUpdates() == 1) {
        ((App*)WindowApp::m_instance)->m_updateManager->installUpdates(false);
    }
    ((App*)WindowApp::m_instance)->m_iapCurrencyManager->UpdateIAPInfo(true);
    ((App*)WindowApp::m_instance)->m_iapCurrencyManager->loadResources();
    CreateEffects();
    m_tabs = new CShopTabWindow(1, 0);
    m_tabs->SetAlign(0xc);
    AddToFront(m_tabs);
    int offset;
    if (App::IsKindle() == 1) {
        offset = SScreenAdaptingConstant(25)();
    } else {
        offset = SScreenAdaptingConstant(29)();
    }
    m_tabs->SetOutsetSpacing(offset, 0, 0, 0);
    m_tabs->SetPercentHeight(100, 0, 0);
    m_tabs->SetPercentWidth(100, 0, 0);
    CShopTabContentWindow* weapons = new CShopTabContentWindow(SCROLL_PAGE_WIDTH(), SCROLL_PAGE_HEIGHT());
    m_weaponCreator = new CustomWeaponButtonCreator();
    weapons->AddButton(m_weaponCreator, true);
    weapons->AddManagerContents(((App*)WindowApp::m_instance)->m_weaponsManager);
    m_tabIds[0] = m_tabs->AddTab(weapons, 8, 9);
    Vector<CShopTabWindow::SFilter> weaponFilters;
    CShopTabWindow::SFilter filter1(16, 17, 1);
    weaponFilters.addElement(filter1);
    CShopTabWindow::SFilter filter2(18, 19, 2);
    weaponFilters.addElement(filter2);
    CShopTabWindow::SFilter filter3(20, 21, 3);
    weaponFilters.addElement(filter3);
    CShopTabWindow::SFilter filter4(22, 23, 4);
    weaponFilters.addElement(filter4);
    CShopTabWindow::SFilter filter5(24, 25, 5);
    weaponFilters.addElement(filter5);
    m_tabs->AddFiltersForTab(m_tabIds[0], weaponFilters);
    CShopTabContentWindow* consumables = new CShopTabContentWindow(SCROLL_PAGE_WIDTH(), SCROLL_PAGE_HEIGHT());
    m_consumableCreator = new CustomWeaponButtonCreator();
    consumables->AddButton(m_consumableCreator, true);
    consumables->AddManagerContents(((App*)WindowApp::m_instance)->m_grenadePackManager);
    consumables->AddManagerContents(((App*)WindowApp::m_instance)->m_powerUpPackManager);
    m_tabIds[1] = m_tabs->AddTab(consumables, 10, 11);
    Vector<CShopTabWindow::SFilter> consumableFilters;
    CShopTabWindow::SFilter filter6(73, 74, 0);
    consumableFilters.addElement(filter6);
    CShopTabWindow::SFilter filter7(75, 76, 1);
    consumableFilters.addElement(filter7);
    m_tabs->AddFiltersForTab(m_tabIds[1], consumableFilters);
    CShopTabContentWindow* bank = new CShopTabContentWindow(SCROLL_PAGE_WIDTH(), SCROLL_PAGE_HEIGHT());
    bank->AddButton(new CustomBankButtonCreator(0), true);
    bank->AddButton(new CustomBankButtonCreator(2), true);
    bank->AddManagerContents(((App*)WindowApp::m_instance)->m_iapCurrencyManager);
    m_tabIds[2] = m_tabs->AddTab(bank, 14, 15);
    CreateClipingBorders();
    CreateInfoPanel();
    AddBackButton();
    AddSettingsButton();
}

void CShopWindow::CreateEffects()
{
    CEffectsWindow* effects = new CEffectsWindow(3, 0, WindowApp::m_instance->m_screenWidth,
                                                WindowApp::m_instance->m_screenHeight);
    effects->CreateEffect(0, 0, 0, 0, false);
    effects->CreateEffect(1, 0, 0, 0, false);
    AddToFront(effects);
}

void CShopWindow::CreateClipingBorders()
{
    SGImageWindow* first = new SGImageWindow();
    first->SetArchetypeCharacter(1, 0);
    first->SetAnimation(32, 1);
    first->SetAlign(0x21);
    AddToFront(first);
    SGImageWindow* second = new SGImageWindow();
    second->SetArchetypeCharacter(1, 0);
    second->SetAnimation(33, 1);
    second->SetAlign(0x22);
    AddToFront(second);
}

void CShopWindow::CreateInfoPanel()
{
    CHudStatsPanel* panel = new CHudStatsPanel();
    panel->SetAlign(0xc);
    AddToFront(panel);
}

void CShopWindow::Init()
{
    if (((App*)WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial() == 1) {
        WindowApp::HandleTunnelCommand(0x8486376a, 0, 0, 0);
        int filter = ((App*)WindowApp::m_instance)->m_tutorialManager->GetCurrentShopTabFilter();
        if (filter >= 0) {
            m_tabs->SetCurrentFilter(filter);
        }
    } else {
        ICAdManager::GetInstance()->ShowAd(ICAdManager::Type_AdMarvel,
                                           ICAdManager::Position_BottomCenter, -1, -1, NULL, NULL);
        CGameAnalytics::logAdShown(0);
    }
}

void CShopWindow::OnCommand(Event& event)
{
    switch ((unsigned int)event.m_id) {
    case 0x822663d7:
        if (TapjoyInterface::canDisplayInterface() != 0) {
            TapjoyInterface::openTapjoyInterface();
        } else {
            CZombieDialogWindow* dialog = new CZombieDialogWindow(3);
            dialog->SetTitle(Window::ResString("IDS_DIALOG_TITLE_WARNING"));
            dialog->SetText(Window::ResString("IDS_DIALOG_CAN_NOT_OPEN_TAPJOY"), false, NULL);
            dialog->AddButton((CZombieDialogWindow::EDialogButton)1, -1, 0, 0, true, 20);
            AddModal(dialog);
        }
        break;
    case 0x910d9f06:
    case 0x62e7ea20:
        m_tabs->UpdateTab(-1);
        event.Clear();
        break;
    case 0xc24364a4:
    case 0xff403571:
        m_tabs->UpdateTab(m_tabIds[2]);
        event.Clear();
        break;
    case 0xbe344666:
    case 0x5f9d3f54:
        m_purchasePending = 0;
        break;
    case 0x97204784:
    case 0x5f523cd9:
        WindowApp::HandleTunnelCommand(0x4d5de318, 0, 0, 0);
        if (((App*)WindowApp::m_instance)->m_tutorialManager->CanCloseShop() != 1) {
            return;
        }
        if ((unsigned int)event.m_id == 0x97204784) {
            ReverseEasy(512);
            event.Clear();
        }
        break;
    case 0x9720b34a:
        {
            CSettingsWindow* settings = new CSettingsWindow(false, false);
            settings->SetLayoutType(LAYOUT_SIMPLE);
            settings->SetAlign(0x24);
            ((App*)WindowApp::m_instance)->m_mainLayer->AddModal(settings);
            event.Clear();
        }
        break;
    case 0x71f7f6e4:
        if (!CPHInterface::isPublisherContentRequestActive("more_games")) {
            CPHInterface::startPublisherContentRequest("more_games", true, true);
        }
        event.Clear();
        break;
    case 0x072e8955:
        event.Clear();
        break;
    case 0x30471317:
        {
            ((App*)WindowApp::m_instance)->m_soundManager->Play((CDH_SoundManager::eSound)34, false, false);
            CShopItemButton* button = (CShopItemButton*)event.m_param1;
            Window* dialog = button->CreateInfoDialog();
            if (dialog != NULL) {
                WindowApp::HandleTunnelCommand(0x8f641692, 0, 0, 0);
                AddModal(dialog);
            } else if (m_purchasePending == 0) {
                BuyItem(button);
            }
            event.Clear();
        }
        break;
    case 0x1e11f5b7:
    case 0xd7a888c3:
        m_tabs->SetCurrentTab(m_tabIds[2], -1);
        event.Clear();
        break;
    case 0x3b7082f1:
        {
            CShopItemButton* button = (CShopItemButton*)event.m_param1;
            if (button->m_item != NULL && !button->m_item->isAvailable()
                && button->m_item != NULL && button->m_item->getPriceCredits() != 0) {
                // The full expression keeps the by-value construction of GetItemName ahead of the price query, with only one argument copy.
                CGameAnalytics::logBuyItemClicked(button->GetItemName(),
                    button->m_item != NULL ? button->m_item->getPriceCredits() : 0,
                    CGameAnalytics::CURRENCY_HARD);
            } else if (button->m_item != NULL && !button->m_item->isAvailable()
                       && button->m_item != NULL && button->m_item->getPricePoints() != 0) {
                CGameAnalytics::logBuyItemClicked(button->GetItemName(),
                    button->m_item != NULL ? button->m_item->getPricePoints() : 0,
                    CGameAnalytics::CURRENCY_SOFT);
            }
            BuyItem(button);
            event.Clear();
        }
        break;
    case 0x3a3412b7:
        SetTab((ETab)event.m_param1, -1);
        event.Clear();
        break;
    case 0xd34330d5:
        AddModal(new CWeaponEquipZombieDialogWindow((CDH_Weapon*)event.m_param1, NULL, NULL));
        event.Clear();
        break;
    case 0xcf03ebf0:
        AddModal(new CWeaponEquipZombieDialogWindow(NULL, (CGrenade*)event.m_param1, NULL));
        event.Clear();
        break;
    case 0xe503ba9c:
        AddModal(new CWeaponEquipZombieDialogWindow(NULL, NULL, (CPowerUp*)event.m_param1));
        event.Clear();
        break;
    case 0x6eb90413:
        if (event.m_param1 == m_tabIds[2] && !CPHInterface::isPublisherContentRequestActive("bank_launch")) {
            CPHInterface::startPublisherContentRequest("bank_launch", false, false);
        }
        event.Clear();
        break;
    case 0x659570dc:
        {
            CZombieDialogWindow* dialog = new CZombieDialogWindow(3);
            dialog->SetTitle(Window::ResString("IDS_SHOP_IAP_ERROR_TITLE"));
            dialog->SetText(Window::ResString("IDS_SHOP_IAP_ERROR"), false, NULL);
            dialog->AddButton((CZombieDialogWindow::EDialogButton)1, -1, 0, 0, true, 20);
            ((App*)WindowApp::m_instance)->m_mainLayer->AddModal(dialog);
            CWaitPurchaseWindow::SetClose();
        }
        break;
    }
    CBaseScreen::OnCommand(event);
}

void CShopWindow::BuyItem(CShopItemButton* button)
{
    CZombieDialogWindow* rejected;
    if (!button->CheckBuyConditions(&rejected)) {
        if (rejected != NULL) {
            AddModal(rejected);
        }
        return;
    }
    if (button->Buy() == 1) {
        ((App*)WindowApp::m_instance)->m_soundManager->Play((CDH_SoundManager::eSound)35, false, false);
        if (button->IsAfterBuyDialogShown() == 1) {
            CZombieDialogWindow* dialog = (CZombieDialogWindow*)button->CreateInfoDialog();
            // Original 160a1c writes d0 before the NULL comparison; no guard is added that would change the original path.
            dialog->m_disableEasy = true;
            if (dialog != NULL) {
                AddModal(dialog);
                return;
            }
        }
    } else if (!button->IsIAP()) {
        ((App*)WindowApp::m_instance)->m_soundManager->Play((CDH_SoundManager::eSound)38, false, false);
        CNotEnoughCurrencyDialogWindow* dialog = new CNotEnoughCurrencyDialogWindow(button);
        dialog->InitIAP();
        AddModal(dialog);
    }
    m_tabs->UpdateTab(-1);
}

void CShopWindow::SetTab(ETab tab, int itemId)
{
    // Original 160a8c maps the enum index directly to the consecutive c8/cc/d0, with no range clamping.
    m_tabs->SetCurrentTab(m_tabIds[(int)tab], itemId);
}

void CShopWindow::TryOpenTapJoy()
{
    // Original 160aa0 is only bx lr.
}

void CShopWindow::Update()
{
    bool badgeAllowed = false;
    bool weaponBadge = false;
    if (!HasModalChild()) {
        if (((App*)WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial() == 0
            && (m_flags & 0x200) == 0 && (m_flags & 0x20000000) != 0) {
            weaponBadge = m_tabs->m_currentTab == m_tabIds[0];
            badgeAllowed = true;
        }
    }
    if (m_weaponCreator->m_badge != NULL) {
        m_weaponCreator->m_badge->Update(weaponBadge);
    }
    bool consumableBadge = false;
    if (badgeAllowed && m_tabs->m_currentTab == m_tabIds[1]) {
        consumableBadge = true;
    }
    if (m_consumableCreator->m_badge != NULL) {
        m_consumableCreator->m_badge->Update(consumableBadge);
    }
    if (((App*)WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial() == 1) {
        Window* back = m_back;
        if (((App*)WindowApp::m_instance)->m_tutorialManager->IsShopBackButtonHide() == 0) {
            back->SetFlags(1);
        } else {
            back->ClearFlags(1);
        }
    }
    if ((HasModalChild() == 1 && m_adVisible) || (m_flags & 0x20000000) == 0
        || m_tabs->m_currentTab == m_tabIds[2]) {
        ICAdManager::GetInstance()->HideAd();
        m_adVisible = false;
    } else if (!HasModalChild() && !m_adVisible && (m_flags & 0x20000000) != 0
               && m_tabs->m_currentTab != m_tabIds[2]) {
        if (((App*)WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial() == 0) {
            ICAdManager::GetInstance()->ShowAd(ICAdManager::Type_AdMarvel,
                                               ICAdManager::Position_BottomCenter, -1, -1, NULL, NULL);
            CGameAnalytics::logAdShown(0);
        }
        m_adVisible = true;
    }
    Window::Update();
}

void CShopWindow::OnClosed()
{
    ICAdManager::GetInstance()->HideAd();
    m_adVisible = false;
    if (((App*)WindowApp::m_instance)->m_huntingInfo->m_field00) {
        WindowApp::HandleTunnelCommand(0x5e099a77, 0, 0, 0);
    }
}

CShopWindow* CShopWindow::GetInstance()
{
    if (m_instance == NULL) {
        m_instance = new CShopWindow();
    }
    return m_instance;
}

CShopWindow::~CShopWindow()
{
    m_instance = NULL;
}

void CShopWindow::Paint(com::glu::platform::graphics::ICGraphics2d& graphics)
{
    CBaseScreen::Paint(graphics);
}

void CShopWindow::AddModal(Window* window)
{
    Window::AddModal(window);
}

void CShopWindow::OnGetFocus()
{
}

void CShopWindow::OnLostFocus()
{
}

CShopWindow::ETab CShopWindow::GetTabForItem(CItem* item)
{
    XString type(item->GetItemTypeName());
    if (type == XString("weapon")) {
        return TAB_0;
    }
    if (type == XString("iapcurrency")) {
        return TAB_2;
    }
    return TAB_1;
}

void CShopWindow::OnEnterForeground()
{
    ((App*)WindowApp::m_instance)->m_iapCurrencyManager->UpdateIAPInfo(true);
}
