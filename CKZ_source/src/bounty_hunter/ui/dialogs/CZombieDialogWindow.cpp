// CZombieDialogWindow.cpp: full u546; the window tree owns the child windows, member pointers are not freed separately.
#include "CZombieDialogWindow.h"
#include "App.h"
#include "BaseDialog.h"
#include "SGImageWindow.h"
#include "SGImageWindowCommand.h"
#include "SGButton.h"
#include "CImageButton.h"
#include "CFontMgr.h"
#include "SScreenAdaptingConstant.h"
#include "MathLib.h"
#include "CBH_Player.h"
#include "CShopItem.h"
#include "CShopItemButton.h"
#include "CShopPackManager.h"
#include "CDH_WeaponsManager.h"
#include "ServiceWindows.h"
#include "DGHelper.h"
#include "CSwerveGame.h"

CZombieDialogWindow::CZombieDialogWindow(int style) : Window(false),
    m_buttonCount(0), m_disableEasy(false), m_hasIgnoreButton(false)
{
    m_background = new SGImageWindow();
    m_background->SetArchetypeCharacter(2, 0);
    m_background->SetAnimation(style, 1);
    m_background->SetLayoutType(LAYOUT_NONE);
    AddToFront(m_background);
    SetDesiredWidth(m_background->m_imageWidth);
    SetDesiredHeight(m_background->m_imageHeight);
    int verticalInset = SScreenAdaptingConstant(30)();
    int horizontalInset = SScreenAdaptingConstant(60)();
    SetInsetSpacing(verticalInset, horizontalInset);
    SetAlign(0x24);

    m_titleContainer = new WindowTransparent(false);
    m_titleContainer->SetOutsetSpacing(SScreenAdaptingConstant(20)(), 0, 0, 0);
    m_titleContainer->SetAlign(0x0c);
    m_titleContainer->SetPercentWidth(100, 0, 0);
    m_titleContainer->SetHeightByContent(0, 0);
    AddToFront(m_titleContainer);

    m_contentContainer = new WindowTransparent(false);
    m_contentContainer->SetAlign(0x24);
    m_contentContainer->SetPercentWidth(100, 0, 0);
    m_contentContainer->SetPercentHeight(0, 0, 0);
    AddToFront(m_contentContainer);

    m_messageContainer = new WindowTransparent(false);
    m_messageContainer->SetAlign(0x24);
    m_messageContainer->SetPercentWidth(100, 0, 0);
    m_messageContainer->SetHeightByContent(0, 0);
    AddToFront(m_messageContainer);

    m_buttonOuter = new WindowTransparent(false);
    m_buttonOuter->SetOutsetSpacing(0, 0, SScreenAdaptingConstant(16)(), 0);
    m_buttonOuter->SetAlign(0x14);
    m_buttonOuter->SetPercentWidth(100, 0, 0);
    m_buttonOuter->SetHeightByContent(0, 0);
    AddToFront(m_buttonOuter);

    m_buttonGrid = new WindowTransparent(false);
    m_buttonGrid->SetLayoutType(LAYOUT_SIMPLE);
    m_buttonGrid->SetAlign(0x24);
    m_buttonGrid->SetHeightByContent(0, 0);
    m_buttonGrid->SetWidthByContent(0, 0);
    m_buttonOuter->AddToFront(m_buttonGrid);

    m_bodyText = new TextWindow();
    m_bodyText->SetHeightByContent(0, 0);
    m_bodyText->SetPercentWidth(100, 0, 0);
    m_bodyText->SetAlign(9);
    m_messageContainer->AddToFront(m_bodyText);

    m_titleText = new TextWindow();
    m_titleText->SetHeightByContent(0, 0);
    m_titleText->SetPercentWidth(100, 0, 0);
    m_titleText->SetAlign(0x24);
    m_titleContainer->AddToFront(m_titleText);
}

CZombieDialogWindow::~CZombieDialogWindow()
{
    App* app = (App*)WindowApp::m_instance;
    // Original 14f806 is CMP #1, reading the object representation; keeps the exact comparison for non-canonical bytes.
    if (*(const unsigned char*)&app->m_field3c9 == 1) {
        app->m_field3c9 = false;
    }
}

void CZombieDialogWindow::SetTitle(const XString& title)
{
    TextWindow* window = m_titleText;
    com::glu::platform::graphics::CFont* font = CFontMgr::GetInstance()->GetFont(26);
    window->SetText(title, font);
}

void CZombieDialogWindow::SetText(const XString& text, bool center,
    com::glu::platform::graphics::CFont* font)
{
    if (center == true) {
        m_bodyText->SetAlign(0x0c);
    }
    TextWindow* window = m_bodyText;
    if (font == NULL) {
        font = CFontMgr::GetInstance()->GetFont(24);
    }
    window->SetText(text, font);
}

void CZombieDialogWindow::InitOkMessage(const XString& title, const XString& text,
    bool center, com::glu::platform::graphics::CFont* font)
{
    SetTitle(title);
    SetText(text, center, font);
    AddButton(BUTTON_OK, (unsigned int)-1, 0, 0, true, 20);
}

SGButton* CZombieDialogWindow::AddButton(EDialogButton buttonType, unsigned int command,
    int parameter1, int parameter2, bool closes, int)
{
    SGButton* button = CreateButton(buttonType);
    unsigned int wrapper = 0xe87c119b;
    if (closes == true) {
        wrapper = 0x0c9b0cea;
    }
    button->m_command = wrapper;
    button->m_commandParameter1 = command;
    button->m_commandParameter2 = parameter1;
    button->m_commandParameter3 = parameter2;
    button->SetCellPos(m_buttonCount, 0, 1, 1);
    button->SetOutsetSpacing(0, SScreenAdaptingConstant(20)());
    m_buttons[m_buttonCount] = button;
    ++m_buttonCount;
    m_buttonGrid->AddToFront(button);
    if (m_buttonCount >= 3) {
        for (int index = 0; index < m_buttonCount; ++index) {
            SGButton* current = m_buttons[index];
            current->SetOutsetSpacing(0, SScreenAdaptingConstant(5)());
        }
    }
    return button;
}

void CZombieDialogWindow::OnStartupEasy()
{
    if (m_disableEasy) {
        return;
    }
    EasyParams params(this, (EasyLib::EMethod)26);
    switch (WindowApp::m_instance->GetRandomValue().Int(4)) {
    case 0: params.SetMoving(m_x, m_parent->m_height, m_x, m_y); break;
    case 1: params.SetMoving(m_x, -m_height, m_x, m_y); break;
    case 2: params.SetMoving(-m_width, m_y, m_x, m_y); break;
    case 3: params.SetMoving(m_parent->m_width, m_y, m_x, m_y); break;
    }
    params.SetFullTime(0.4f);
    params.SetDelayTime(0.1f);
    params.SetOpacity(0.0f, 1.0f);
    params.SetScale(0.3f, 1.0f);
    StartEasy(params, 0x100);
}

SGButton* CZombieDialogWindow::CreateButton(EDialogButton type)
{
    // b147: the button is created first (compared 20, then 7), then the caption switch;
    // type 7 shares the IDS_EQUIP_BUTTON caption block with type 6.
    SGButton* button;
    switch (type) {
    case 7:
        button = new SGButton(2, 0, 10, 11);
        break;
    case 20: {
        button = new SGButton(2, 0, 16, 17);
        TextWindow* text = button->SetText(ResString("IDS_DIALOG_BUTTON_REVIVE"), (eGameFont)29);
        text->SetAlign(0x21);
        text->SetOutsetSpacing(0, 0, 0, SScreenAdaptingConstant(15)());
        return button;
    }
    default:
        button = new SGButton(2, 0, 0, 1);
        break;
    }
    // b147: case order follows the original block layout.
    switch (type) {
    case 6:
    case 7: button->SetText(ResString("IDS_EQUIP_BUTTON"), (eGameFont)26); break;
    case 5: button->SetText(ResString("IDS_DIALOG_BUTTON_BUY"), (eGameFont)26); break;
    case 17: button->SetText(ResString("IDS_DIALOG_BUTTON_BUY"), (eGameFont)26); break;
    case 10: button->SetText(ResString("IDS_DIALOG_BUTTON_ACCEPT"), (eGameFont)26); break;
    case 4:
        m_hasIgnoreButton = true;
        button->SetText(ResString("IDS_DIALOG_BUTTON_IGNORE"), (eGameFont)26);
        break;
    case 2: button->SetText(ResString("IDS_DIALOG_BUTTON_YES"), (eGameFont)26); break;
    case 3: button->SetText(ResString("IDS_DIALOG_BUTTON_NO"), (eGameFont)26); break;
    case 1: button->SetText(ResString("IDS_SK_OK"), (eGameFont)26); break;
    case 8: button->SetText(ResString("IDS_MISSION_INFO_BUTTON_START"), (eGameFont)26); break;
    case 9: button->SetText(ResString("IDS_MISSION_INFO_BUTTON_PLAY"), (eGameFont)26); break;
    case 11: button->SetText(ResString("IDS_SAVE_CONFLICT_LOCAL_BUTTON"), (eGameFont)26); break;
    case 12: button->SetText(ResString("IDS_SAVE_CONFLICT_SERVER_BUTTON"), (eGameFont)26); break;
    case 13: button->SetText(ResString("IDS_DIALOG_BUTTON_LATER"), (eGameFont)26); break;
    case 14: button->SetText(ResString("IDS_RECONNECT"), (eGameFont)26); break;
    case 15: button->SetText(ResString("IDS_PLAY_OFFLINE_GSERVE"), (eGameFont)26); break;
    case 16: button->SetText(ResString("IDS_RECOMMEND_WEAPON_BUTTON"), (eGameFont)26); break;
    case 18: button->SetText(ResString("IDS_DIALOG_BUTTON_SHOP_NOW"), (eGameFont)28); break;
    case 19: button->SetText(ResString("IDS_DIALOG_BUTTON_NEXT_WAVE"), (eGameFont)26); break;
    }
    return button;
}

void CZombieDialogWindow::OnPointerEvent(Event& event) { event.Clear(); }

void CZombieDialogWindow::OnCommand(Event& event)
{
    if ((unsigned int)event.m_id != 0xe87c119b && (unsigned int)event.m_id != 0x0c9b0cea) {
        return;
    }
    if (event.m_param1 != 0) {
        WindowApp::HandleTunnelCommand(event.m_param1, event.m_param2, event.m_param3, 0);
    }
    if ((unsigned int)event.m_id == 0x0c9b0cea) {
        if (m_disableEasy) {
            Close();
            ClearFlags(0x2001);
        } else {
            ReverseEasy(0x200);
        }
    }
    event.Clear();
}

void CZombieDialogWindow::ShowClosingCross(unsigned int command)
{
    SGImageWindow* image = new SGImageWindow();
    image->SetArchetypeCharacter(1, 0);
    image->SetAnimation(84, 1);
    CustomButton* button = new CustomButton();
    button->AddToFront(image);
    button->SetSize(image->m_imageWidth, image->m_imageHeight);
    button->SetLayoutType(LAYOUT_SIMPLE);
    button->SetAlign(10);
    button->SetOutsetSpacing(0, SScreenAdaptingConstant(-30)(), 0, 0);
    button->m_command = 0x0c9b0cea;
    button->m_commandParameter1 = command;
    button->m_commandParameter2 = 0;
    button->m_commandParameter3 = 0;
    AddToFront(button);
}

CServerGiftDialog::CServerGiftDialog(int money, int credits, int third, int fourth)
    : CZombieDialogWindow(3), m_money(money), m_credits(credits), m_third(third), m_fourth(fourth)
{
}

void CServerGiftDialog::OnCommand(Event& event)
{
    if ((unsigned int)event.m_id == 0x0c9b0cea) {
        CBH_Player::GetInstance()->AddMoney(m_money);
        // The PC-relative UTF32 literal 451510 at original 14ffca is an empty string.
        CBH_Player::GetInstance()->AddCredits(m_credits, L"");
    }
    CZombieDialogWindow::OnCommand(event);
}

CRecommendedWeaponDialog::CRecommendedWeaponDialog(int weapon, int grenadePack)
    : CZombieDialogWindow(3)
{
    SetTitle(ResString("IDS_RECOMMEND_WEAPON_TITLE"));
    CItemManager* manager;
    int item;
    if (grenadePack >= 0) {
        manager = ((App*)WindowApp::m_instance)->m_grenadePackManager;
        m_recommendCommand = 0xc9cb1c42;
        item = grenadePack;
    } else {
        manager = ((App*)WindowApp::m_instance)->m_weaponsManager;
        m_recommendCommand = 0xc4794df3;
        item = weapon;
    }
    if (manager != NULL) {
        CShopItemButton* button = manager->CreateShopItemButton(item);
        button->SetLayoutType(LAYOUT_SIMPLE);
        button->SetAlign(0x24);
        button->m_command = m_recommendCommand;
        button->m_commandParameter1 = item;
        button->m_commandParameter2 = 0;
        button->m_commandParameter3 = 0;
        AddToFront(button);
    }
    AddButton((EDialogButton)16, m_recommendCommand, item, 0, true, 20);
}

void CRecommendedWeaponDialog::OnCommand(Event& event)
{
    if ((unsigned int)event.m_id == m_recommendCommand) {
        Close();
    }
    CZombieDialogWindow::OnCommand(event);
}
CNextWaveZombieDialogWindow::CNextWaveZombieDialogWindow(CItem* item)
    : CZombieDialogWindow(2)
{
    SetTitle(ResString("IDS_NEXT_WAVE_WINDOW_TITLE"));
    m_messageContainer->SetAlign(0x0c);
    m_messageContainer->SetOutsetSpacing(SScreenAdaptingConstant(40, 110, -1, 65)(), 0, 0, 0);
    SetText(ResString("IDS_NEXT_WAVE_WINDOW_TEXT"), true, NULL);

    Window* content = new WindowTransparent(false);
    content->SetAlign(1);
    content->SetWidthByContent(0, 0);
    content->SetHeightByContent(0, 0);
    TextWindow* label = new TextWindow(CFontMgr::GetInstance()->GetFont(26));
    label->SetText(ResString("IDS_MISSION_INFO_RECOMMENDED_WEAPON"));
    label->SetAlign(0x21);
    content->AddToFront(label, 0, 0);

    CImageButton* itemButton = new CImageButton();
    itemButton->SetOutsetSpacing(0, 0, 0, SScreenAdaptingConstant(0, 10)());
    // Original 1502a0 virtual slot 28 is GetSmallImage; must not be guessed as slot 24 from image semantics.
    itemButton->SetImage(item->GetSmallImage(), 0, 0);
    itemButton->m_command = 0xa89c3a3b;
    itemButton->m_commandParameter1 = 0;
    itemButton->m_commandParameter2 = 0;
    itemButton->m_commandParameter3 = 0;
    content->AddToFront(itemButton, 1, 0);

    SGImageWindowCommand* buy = new SGImageWindowCommand();
    buy->SetArchetypeCharacter(7, 0);
    buy->SetAnimation(2, 1);
    buy->m_command = 0xa89c3a3b;
    buy->m_commandParameter1 = 0;
    buy->m_commandParameter2 = 0;
    buy->m_commandParameter3 = 0;
    buy->SetOutsetSpacing(0, 0, 0, SScreenAdaptingConstant(0, 10)());
    buy->SetAlign(0x24);
    buy->SetFlags(0x800000);
    content->AddToFront(createTD(buy, 3), 2, 0);

    TextWindow* buyLabel = new TextWindow(CFontMgr::GetInstance()->GetFont(26));
    buyLabel->SetText(ResString("IDS_BUY_NOW_BUTTON"));
    buyLabel->SetAlign(0x24);
    int top = SScreenAdaptingConstant(2)();
    int left = SScreenAdaptingConstant(15)();
    buyLabel->SetOutsetSpacing(top, 0, 0, left);
    buy->AddToFront(buyLabel);
    Window* table = createTD(content, 3);
    table->SetAlign(0x24);
    AddToFront(table);
    AddButton((EDialogButton)18, 0xa89c3a3b, 0, 0, true, 20);
    AddButton((EDialogButton)19, 0x1958a2e7, 0, 0, true, 20);
}

void CNextWaveZombieDialogWindow::Shutdown()
{
    CSwerveGame* game = ((App*)WindowApp::m_instance)->m_helper->m_swerveGame;
    if (game != NULL) {
        game->OnNextWaveZombieDialogWindowShutdown();
    }
}
