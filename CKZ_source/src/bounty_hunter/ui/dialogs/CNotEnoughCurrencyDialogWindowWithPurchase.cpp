// Full u540/u541 recovery; keeps the real methods of the two original independent owners.
#include "CNotEnoughCurrencyDialogWindowWithPurchase.h"
#include "App.h"
#include "CFontMgr.h"
#include "CBH_Player.h"
#include "CIAPCurrency.h"
#include "CIAPCurrencyManager.h"
#include "CShopIAPButton.h"
#include "SGImageWindow.h"
#include "ServiceWindows.h"
#include "SScreenAdaptingConstant.h"
#include "CWaitPurchaseWindow.h"
#include <string.h>

CNotEnoughCurrencyDialogWindowWithPurchase::CNotEnoughCurrencyDialogWindowWithPurchase(int requiredCredits, unsigned int completionCommand)
    : CZombieDialogWindow(2), m_iapManager(NULL), m_requiredCredits(requiredCredits),
      m_buttonsContainer(NULL), m_availableCount(-1), m_best(NULL), m_next(NULL),
      m_completionCommand(completionCommand)
{
    ShowClosingCross(0x0f6a94ea);
    SetOutsetSpacing(SScreenAdaptingConstant(-20)(), 0, 0, 0);
    m_buttonOuter->DeleteChilds();
    Remove(m_buttonOuter);
    memset(m_buttons, 0, sizeof(m_buttons));
}

void CNotEnoughCurrencyDialogWindowWithPurchase::InitIAP()
{
    m_iapManager = ((App*)WindowApp::m_instance)->m_iapCurrencyManager;
    SetTitle(ResString("IDS_DIALOG_TITLE_NO_CREDITS"));
    m_titleContainer->Remove(m_titleText);
    TextWindow* titleText = m_titleText;
    titleText->SetFont(CFontMgr::GetInstance()->GetFont(29));
    SGImageWindow* titleBackground = new SGImageWindow;
    titleBackground->SetArchetypeCharacter(2, 0);
    titleBackground->SetAnimation(14, 1);
    titleBackground->SetAlign(36);
    titleBackground->SetLayoutType(LAYOUT_SIMPLE);
    titleBackground->AddToFront(m_titleText);
    m_titleContainer->AddToFront(titleBackground);
    Window* row = new WindowTransparent(false);
    row->SetLayoutType(LAYOUT_SIMPLE);
    row->SetAlign(12);
    row->SetWidthByContent(0, 0);
    row->SetHeightByContent(0, 0);
    Window* content = m_contentContainer;
    content->AddToFront(createTD(row, 3), 0, 0);
    m_contentContainer->SetPercentHeight(80, 0, 0);
    m_contentContainer->SetPercentWidth(100, 0, 0);
    m_contentContainer->SetAlign(20);
    content = m_contentContainer;
    content->SetOutsetSpacing(0, 0, SScreenAdaptingConstant(20)(), 0);
    TextWindow* part1 = new TextWindow(CFontMgr::GetInstance()->GetFont(24));
    part1->SetText(ResString("IDS_INSUFFICIENT_MONEY_PART1"));
    part1->SetWidthByContent(0, 0);
    part1->SetAlign(16);
    row->AddToFront(part1, 0, 0);
    SGImageWindow* firstCurrency = new SGImageWindow;
    firstCurrency->SetArchetypeCharacter(1, 0);
    firstCurrency->SetAnimation(5, 1);
    firstCurrency->SetOutsetSpacing(0, 0, 0, SScreenAdaptingConstant(2)());
    row->AddToFront(firstCurrency, 1, 0);
    TextWindow* current = new TextWindow(CFontMgr::GetInstance()->GetFont(28));
    current->SetText(XString::Format(L"%d", CBH_Player::GetInstance()->GetStats(PLAYER_STATS_HARD_CURRENCY)));
    current->SetWidthByContent(0, 0);
    current->SetOutsetSpacing(0, SScreenAdaptingConstant(2)(), 0, 0);
    row->AddToFront(current, 2, 0);
    TextWindow* part2 = new TextWindow(CFontMgr::GetInstance()->GetFont(24));
    part2->SetText(ResString("IDS_INSUFFICIENT_MONEY_PART2"));
    part2->SetWidthByContent(0, 0);
    part2->SetAlign(16);
    part2->SetOutsetSpacing(0, SScreenAdaptingConstant(2)(), 0, 0);
    row->AddToFront(part2, 3, 0);
    SGImageWindow* secondCurrency = new SGImageWindow;
    secondCurrency->SetArchetypeCharacter(1, 0);
    secondCurrency->SetAnimation(5, 1);
    secondCurrency->SetOutsetSpacing(0, 0, 0, SScreenAdaptingConstant(2)());
    row->AddToFront(secondCurrency, 4, 0);
    TextWindow* target = new TextWindow(CFontMgr::GetInstance()->GetFont(28));
    target->SetText(XString::Format(L"%d", m_requiredCredits));
    target->SetWidthByContent(0, 0);
    target->SetOutsetSpacing(0, SScreenAdaptingConstant(2)(), 0, 0);
    row->AddToFront(target, 5, 0);
    TextWindow* part3 = new TextWindow(CFontMgr::GetInstance()->GetFont(24));
    part3->SetText(ResString("IDS_INSUFFICIENT_MONEY_PART3"));
    part3->SetWidthByContent(0, 0);
    part3->SetAlign(16);
    row->AddToFront(part3, 6, 0);
    UpdateIAPs();
}

void CNotEnoughCurrencyDialogWindowWithPurchase::UpdateIAPs()
{
    int count = m_iapManager->GetAvailableItemsCount();
    if (count == m_availableCount) {
        return;
    }
    m_availableCount = count;
    Vector<CIAPCurrency*> items;
    m_iapManager->GetAvailableCurrencies(items);
    CIAPCurrency* best = NULL;
    int bestDistance = 0x7fffffff;
    for (int index = 0; index < items.size(); ++index) {
        CIAPCurrency* item = items[index];
        int amount = item->m_creditsGain;
        if (amount < 1) {
            continue;
        }
        int required = m_requiredCredits;
        CBH_Player* player = CBH_Player::GetInstance();
        int distance = (int)((unsigned int)player->GetStats(PLAYER_STATS_HARD_CURRENCY) +
                             ((unsigned int)amount - (unsigned int)required));
        if (distance >= 0 && distance < bestDistance) {
            bestDistance = distance;
            best = item;
        } else if (best == NULL) {
            best = item;
        }
    }
    CIAPCurrency* next = NULL;
    int nextDistance = 0x7fffffff;
    for (int index = 0; index < items.size(); ++index) {
        CIAPCurrency* item = items[index];
        int amount = item->m_creditsGain;
        if (amount < 1) {
            continue;
        }
        if (next == NULL) {
            next = best;
        }
        int distance = (int)((unsigned int)amount - (unsigned int)best->m_creditsGain);
        if (distance > 0 && distance < nextDistance) {
            nextDistance = distance;
            next = item;
        }
    }
    m_best = best;
    m_next = next;
    UpdateButtons();
}

void CNotEnoughCurrencyDialogWindowWithPurchase::UpdateButtons()
{
    if (m_buttonsContainer != NULL) {
        m_contentContainer->Remove(m_buttonsContainer);
        m_buttonsContainer->DeleteChilds();
        if (m_buttonsContainer != NULL) {
            delete m_buttonsContainer;
            m_buttonsContainer = NULL;
        }
    }
    m_buttonsContainer = new WindowTransparent(false);
    m_buttonsContainer->SetAlign(36);
    m_buttonsContainer->SetLayoutType(LAYOUT_SIMPLE);
    m_buttonsContainer->SetHeightByContent(0, 0);
    m_buttonsContainer->SetWidthByContent(0, 0);
    m_contentContainer->AddToFront(m_buttonsContainer, 0, 2);
    bool credits = true;
    CShopItemButton* button;
    if (m_best == NULL) {
        button = CreateLoadingButton(credits);
    } else {
        button = CreateButton(m_best);
        button->m_command = 0x51212dd9;
        button->m_commandParameter1 = (int)button;
        button->m_commandParameter2 = 0;
        button->m_commandParameter3 = 0;
    }
    if (button != NULL) {
        button->SetLayoutType(LAYOUT_SIMPLE);
        button->SetAlign(36);
        Window* container = m_buttonsContainer;
        container->AddToFront(createTD(button, 3), 0, 0);
    }
    CIAPCurrency* next = m_next;
    if (next != m_best && next != NULL) {
        button = CreateButton(next);
        // The original writes the command before the NULL check; keep that order.
        button->m_command = 0x51212dd9;
        button->m_commandParameter1 = (int)button;
        button->m_commandParameter2 = 0;
        button->m_commandParameter3 = 0;
        if (button == NULL) {
            return;
        }
        button->SetLayoutType(LAYOUT_SIMPLE);
        button->SetAlign(36);
        Window* container = m_buttonsContainer;
        container->AddToFront(createTD(button, 3), 1, 0);
    }
}

void CNotEnoughCurrencyDialogWindowWithPurchase::Update()
{
    Window::Update();
    UpdateIAPs();
}

CShopItemButton* CNotEnoughCurrencyDialogWindowWithPurchase::CreateButton(CIAPCurrency* item)
{
    if (item == NULL) {
        return NULL;
    }
    CShopIAPButton* button = new CShopIAPButton(item);
    button->DeleteChilds();
    int credits = item->m_creditsGain;
    int count = credits;
    int fontId = 28;
    if (credits == 0) {
        count = item->m_cashGain;
        fontId = 29;
    }
    com::glu::platform::graphics::CFont* font = CFontMgr::GetInstance()->GetFont(fontId);
    SGImageWindow* background = new SGImageWindow;
    background->SetArchetypeCharacter(2, 0);
    background->SetAnimation(12, 1);
    button->AddToFront(background);
    button->SetSize(background->m_imageWidth, background->m_imageHeight);
    button->SetFlags(0x800000);
    ImageWindow* image = new ImageWindow;
    image->SetImage(item->GetShopImage());
    image->SetOutsetSpacing(0, 0, SScreenAdaptingConstant(10)(), 0);
    image->SetAlign(20);
    button->AddToFront(image);
    Window* amountRow = new WindowTransparent(false);
    amountRow->SetWidthByContent(0, 0);
    amountRow->SetHeightByContent(0, 0);
    amountRow->SetLayoutType(LAYOUT_SIMPLE);
    amountRow->SetAlign(12);
    amountRow->SetOutsetSpacing(SScreenAdaptingConstant(3)(), 0, 0, 0);
    button->AddToFront(amountRow);
    SGImageWindow* currency = new SGImageWindow;
    currency->SetArchetypeCharacter(1, 0);
    int animation = 6;
    if (credits != 0) {
        animation = 5;
    }
    currency->SetAnimation(animation, 1);
    currency->SetOutsetSpacing(0, 0, 0, SScreenAdaptingConstant(2)());
    amountRow->AddToFront(currency, 0, 0);
    TextWindow* amount = new TextWindow;
    amount->SetText(XString(count), font);
    amount->SetAlign(12);
    amountRow->AddToFront(amount, 1, 0);
    Window* priceRow = new WindowTransparent(false);
    priceRow->SetPercentWidth(70, 0, 0);
    priceRow->SetHeightByContent(0, 0);
    priceRow->SetLayoutType(LAYOUT_SIMPLE);
    priceRow->SetAlign(20);
    priceRow->SetOutsetSpacing(0, 0, SScreenAdaptingConstant(5)(), 0);
    button->AddToFront(priceRow);
    TextWindow* buy = new TextWindow;
    buy->SetText(ResString("IDS_DIALOG_BUTTON_BUY"), font);
    buy->SetAlign(33);
    priceRow->AddToFront(buy);
    TextWindow* price = new TextWindow;
    item->FillPriceText(price);
    price->SetAlign(34);
    priceRow->AddToFront(price);
    if (item->m_badgeText.ByteSize() != 0) {
        SGImageWindow* badge = new SGImageWindow;
        badge->SetArchetypeCharacter(2, 0);
        badge->SetAnimation(13, 1);
        badge->SetLayoutType(LAYOUT_SIMPLE);
        badge->SetAlign(12);
        badge->SetOutsetSpacing(SScreenAdaptingConstant(-10)(), 0, 0, 0);
        button->AddToFront(badge);
        TextWindow* text = new TextWindow(CFontMgr::GetInstance()->GetFont(24));
        text->SetLayoutType(LAYOUT_SIMPLE);
        text->SetAlign(36);
        text->SetText(item->m_badgeText);
        badge->AddToFront(text);
    }
    return button;
}

CShopItemButton* CNotEnoughCurrencyDialogWindowWithPurchase::CreateLoadingButton(bool credits)
{
    CShopItemButton* button = new CShopItemButton(NULL);
    button->DeleteChilds();
    SGImageWindow* background = new SGImageWindow;
    background->SetArchetypeCharacter(2, 0);
    background->SetAnimation(12, 1);
    button->AddToFront(background);
    button->SetSize(background->m_imageWidth, background->m_imageHeight);
    button->SetFlags(0x800000);
    SGImageWindow* currency = new SGImageWindow;
    currency->SetArchetypeCharacter(1, 0);
    int animation = 70;
    if (credits) {
        animation = 69;
    }
    currency->SetAnimation(animation, 1);
    currency->SetOutsetSpacing(0, 0, SScreenAdaptingConstant(10)(), 0);
    currency->SetAlign(20);
    button->AddToFront(currency);
    BlinkingContainer* blinking = new BlinkingContainer;
    blinking->SetAlign(12);
    blinking->SetPercentWidth(100, 0, 0);
    blinking->SetPercentHeight(25, 0, 0);
    blinking->m_blinking = true;
    TextWindow* text = new TextWindow;
    // Original 14df2e..: keeps the ResString lifetime across the font queries.
    {
        XString loading = ResString("IDS_SHOP_IAP_LOADING_ITEM_TEXT");
        text->SetText(loading, CFontMgr::GetInstance()->GetFont(26));
    }
    text->SetAlign(36);
    text->SetPercentWidth(100, 0, 0);
    text->SetHeightByContent(0, 0);
    blinking->AddToFront(text);
    button->AddToFront(blinking);
    return button;
}

void CNotEnoughCurrencyDialogWindowWithPurchase::OnCommand(Event& event)
{
    if (event.m_id == (int)0x97204784) {
        ReverseEasy(0x200);
        event.Clear();
        return;
    }
    if (event.m_id == 0x51212dd9) {
        ((CShopItemButton*)event.m_param1)->Buy();
        event.Clear();
        return;
    }
    if (event.m_id == (int)0xbe344666) {
        if (m_completionCommand != 0) {
            WindowApp::HandleTunnelCommand(m_completionCommand, 0, 0, 0);
        }
        ReverseEasy(0x200);
        return;
    }
    if (event.m_id == 0x5f9d3f54) {
        return;
    }
    if (event.m_id == 0x659570dc) {
        CZombieDialogWindow* error = new CZombieDialogWindow(3);
        error->SetTitle(ResString("IDS_SHOP_IAP_ERROR_TITLE"));
        error->SetText(ResString("IDS_SHOP_IAP_ERROR"), false, NULL);
        error->AddButton(BUTTON_OK, (unsigned int)-1, 0, 0, true, 20);
        ((App*)WindowApp::m_instance)->m_mainLayer->AddModal(error);
        CWaitPurchaseWindow::SetClose();
        return;
    }
    CZombieDialogWindow::OnCommand(event);
}
