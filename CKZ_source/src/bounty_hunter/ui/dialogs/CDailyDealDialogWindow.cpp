// u535 full daily deal; keeps the original behavior of continuing to update after the button/time expires.
#include "CDailyDealDialogWindow.h"
#include "CNotEnoughCurrencyDialogWindow.h"
#include "COffersManager.h"
#include "CShopItem.h"
#include "CFontMgr.h"
#include "CGameAnalytics.h"
#include "CDH_SoundManager.h"
#include "SGImageWindow.h"
#include "SGButton.h"
#include "ServiceWindows.h"
#include "SScreenAdaptingConstant.h"
#include "CStdUtil_Cocoa.h"
#include "ICRenderSurface.h"
#include "TimeValue.h"

CDailyDealDialogWindow::CDailyDealDialogWindow(SDailyDealOffer* offer)
    : CZombieDialogWindow(3), m_offer(offer), m_timer(NULL), m_lastTime(0)
{
    m_item = (CItem*)App::GetItemByStringUId(offer->m_itemUid);
    m_background->SetAnimation(2, 1);
    SetDesiredWidth(m_background->m_imageWidth);
    SetDesiredHeight(m_background->m_imageHeight);
    ShowClosingCross((unsigned int)-1);
    SetTitle(ResString("IDS_DAILY_DEAL_DIALOG_TITLE"));
    TextWindow* title = m_titleText;
    title->SetFont(CFontMgr::GetInstance()->GetFont(30));
    SetupLayout();
}

Window* CreatePriceWindow(bool credits, int price, bool strikeThrough, bool large)
{
    Window* row = new WindowTransparent(false);
    row->SetLayoutType(Window::LAYOUT_SIMPLE);
    int right = SScreenAdaptingConstant(5)();
    int left = SScreenAdaptingConstant(5)();
    row->SetOutsetSpacing(0, right, 0, left);
    row->SetWidthByContent(0, 0);
    row->SetHeightByContent(0, 0);
    com::glu::platform::graphics::CFont* font = CFontMgr::GetInstance()->GetFont(29);
    if (large == 1) {
        font = CFontMgr::GetInstance()->GetFont(30);
    }
    SGImageWindow* currency = new SGImageWindow;
    currency->SetArchetypeCharacter(1, 0);
    int animation = 6;
    if (credits) {
        animation = 5;
    }
    currency->SetAnimation(animation, 1);
    currency->SetAlign(16);
    currency->SetOutsetSpacing(0, 0, SScreenAdaptingConstant(2)(), 0);
    row->AddToFront(createTD(currency, 3), 0, 0);
    TextWindow* text = new TextWindow(font);
    text->SetText(XString::Format(L"%d", price));
    text->SetWidthByContent(0, 0);
    row->AddToFront(text, 1, 0);
    if (strikeThrough == 1) {
        ImageWindow* line = new ImageWindow(ImageWindow::FILL_HORIZONTAL, ImageRes("SUR_IDB__DISCOUNT_LINE_PNG"));
        line->SetPercentWidth(100, 0, 0);
        line->SetAlign(36);
        row->AddToFront(line);
    }
    return createTD(row, 3);
}

// b145: SetupLayout reads these values through small call-free inline accessors. Inlining
// them reorders LLVM's inliner worklist (swap-with-last), which is what gives the five inlined
// CFontMgr::GetInstance `found` locals their original stack slots.

/** The dialog's content container. */
static inline Window* ContentOf(CDailyDealDialogWindow* dialog)
{
    return dialog->m_contentContainer;
}

/** Top of the image frame of an SGImageWindow. */
static inline short FrameTop(SGImageWindow* window)
{
    return window->m_imageY;
}

/** Height of the image frame of an SGImageWindow. */
static inline short FrameHeight(SGImageWindow* window)
{
    return window->m_imageHeight;
}

/** The surface an ImageWindow draws. */
static inline com::glu::platform::graphics::ICRenderSurface* FrameSurface(ImageWindow* window)
{
    return window->m_surface;
}

void CDailyDealDialogWindow::SetupLayout()
{
    m_contentContainer->SetPercentHeight(60, 0, 0);
    m_contentContainer->SetPercentWidth(100, 0, 0);
    int credits = m_item->GetOriginalPriceCredits();
    Window* table = new WindowTransparent(false);
    table->SetLayoutType(LAYOUT_SIMPLE);
    table->SetAlign(12);
    table->SetWidthByContent(0, 0);
    table->SetHeightByContent(0, 0);
    Window* content = ContentOf(this);
    content->AddToFront(createTD(table, 3), 0, 0);
    TextWindow* name = new TextWindow(CFontMgr::GetInstance()->GetFont(26));
    name->SetText(m_item->GetName());
    name->SetAlign(12);
    table->AddToFront(name, 0, 0);
    SGImageWindow* itemBackground = new SGImageWindow;
    itemBackground->SetArchetypeCharacter(2, 0);
    itemBackground->SetAnimation(8, 1);
    itemBackground->SetAlign(12);
    table->AddToFront(itemBackground, 0, 1);
    short top = FrameTop(itemBackground);
    unsigned short height = (unsigned short)FrameHeight(itemBackground);
    ImageWindow* itemImage = new ImageWindow;
    itemImage->SetImage(m_item->GetShopImage());
    itemImage->SetAlign(36);
    itemBackground->AddToFront(itemImage);
    unsigned int imageWidth;
    unsigned int imageHeight;
    FrameSurface(itemImage)->GetWidthAndHeight(imageWidth, imageHeight);
    // Original 149818..sxth followed by unsigned lsr; negative differences also keep the wraparound.
    unsigned int remaining = (unsigned int)((int)(short)(height + top) - (int)top) - imageHeight;
    itemImage->SetOutsetSpacing((int)(remaining >> 1), 0, 0, 0);
    Window* prices = new WindowTransparent(false);
    prices->SetLayoutType(LAYOUT_SIMPLE);
    prices->SetAlign(36);
    prices->SetWidthByContent(0, 0);
    prices->SetHeightByContent(0, 0);
    table->AddToFront(createTD(prices, 3), 0, 2);
    TextWindow* oldTitle = new TextWindow(CFontMgr::GetInstance()->GetFont(29));
    oldTitle->SetText(ResString("IDS_DAILY_DEAL_OLD_PRICE"));
    oldTitle->SetAlign(18);
    prices->AddToFront(oldTitle, 0, 0);
    int oldPrice;
    if (credits != 0) {
        oldPrice = m_item->GetOriginalPriceCredits();
    } else {
        oldPrice = m_item->GetOriginalPricePoints();
    }
    Window* old = CreatePriceWindow(credits != 0, oldPrice, true, false);
    old->SetAlign(1);
    prices->AddToFront(old, 1, 0);
    TextWindow* newTitle = new TextWindow(CFontMgr::GetInstance()->GetFont(29));
    newTitle->SetText(ResString("IDS_DAILY_DEAL_NEW_PRICE"));
    newTitle->SetAlign(18);
    prices->AddToFront(newTitle, 0, 1);
    Window* discounted = CreatePriceWindow(credits != 0, m_offer->m_newPrice, false, true);
    discounted->SetAlign(1);
    prices->AddToFront(discounted, 1, 1);
    SGImageWindow* badge = new SGImageWindow;
    badge->SetArchetypeCharacter(2, 0);
    badge->SetAnimation(15, 1);
    prices->AddToFront(badge, 2, 1);
    TextWindow* discount = new TextWindow(CFontMgr::GetInstance()->GetFont(29));
    discount->SetText(m_offer->m_discount);
    discount->SetWidthByContent(0, 0);
    discount->SetAlign(36);
    badge->AddToFront(discount);
    Window* timerRow = new WindowTransparent(false);
    timerRow->SetAlign(20);
    timerRow->SetWidthByContent(0, 0);
    timerRow->SetHeightByContent(0, 0);
    content = ContentOf(this);
    content->AddToFront(createTD(timerRow, 3), 0, 1);
    if (m_timer != NULL) {
        m_timer->Close();
        m_timer = NULL;
    }
    m_timer = new TextWindow(CFontMgr::GetInstance()->GetFont(24));
    m_timer->SetAlign(36);
    timerRow->AddToFront(m_timer, 0, 0);
    SGButton* buy = AddButton((EDialogButton)5, (unsigned int)-1, 0, 0, true, 20);
    buy->SetText(ResString("IDS_DAILY_DEAL_BUY_BUTTON"), (eGameFont)28);
    buy->m_command = 0xf77a7e09;
    buy->m_commandParameter1 = 0;
    buy->m_commandParameter2 = 0;
    buy->m_commandParameter3 = 0;
}

void CDailyDealDialogWindow::Update()
{
    Window::Update();
    int now = CStdUtil_Cocoa::GetTimeSeconds();
    if ((int)((unsigned int)now - (unsigned int)m_lastTime) < 1) {
        return;
    }
    int remaining = m_item->GetSaleTimeLeft();
    if (remaining <= 0) {
        m_item->SetDailyDealParams(0, 0, 0);
        ReverseEasy(0x200);
        COffersManager* offers = ((App*)WindowApp::m_instance)->m_offersManager;
        offers->m_offersChanged = true;
        offers->m_activeDealIndex = -1;
    } else if (remaining >= 86400) {
        const char* key = "IDS_SHOP_SALE_DAY";
        if (remaining > 172799) {
            key = "IDS_SHOP_SALE_DAYS";
        }
        XString time = XString::Format(L"%d %s", remaining / 86400, ResString(key).m_data);
        TextWindow* timer = m_timer;
        {
            XString format = ResString("IDS_DAILY_DEAL_TIMEOUT");
            XString text = XString::Format(format.m_data, time.m_data);
            timer->SetText(text);
        }
        m_lastTime = now;
        return;
    }
    TimeValue unusedClock; // Original 149bfc real construction; Format is static but its clock side effect is still kept.
    XString time = TimeValue::Format(remaining, true);
    TextWindow* timer = m_timer;
    {
        XString format = ResString("IDS_DAILY_DEAL_TIMEOUT");
        XString text = XString::Format(format.m_data, time.m_data);
        timer->SetText(text);
    }
    m_lastTime = now;
}

void CDailyDealDialogWindow::OnClosed()
{
    ((App*)WindowApp::m_instance)->m_offersManager->DialogClosed();
}

void CDailyDealDialogWindow::OnCommand(Event& event)
{
    if (event.m_id == (int)0xf77a7e09) {
        CZombieDialogWindow* conditionDialog;
        if (!m_item->CheckBuyConditions(&conditionDialog)) {
            if (conditionDialog != NULL) {
                AddModal(conditionDialog);
            }
            return;
        }
        int result = m_item->Buy();
        if (result == 1) {
            {
                XString id(m_offer->m_id);
                int price = (int)((App*)WindowApp::m_instance)->m_offersManager->m_probabilityRandom;
                CGameAnalytics::logDailyDealResult(id, price, (CGameAnalytics::EOTOResult)1);
            }
            ((App*)WindowApp::m_instance)->m_soundManager->Play((CDH_SoundManager::eSound)35, false, false);
            COffersManager* offers = ((App*)WindowApp::m_instance)->m_offersManager;
            offers->OnDDPurchased(m_offer);
            ReverseEasy(0x200);
            return;
        }
        ((App*)WindowApp::m_instance)->m_soundManager->Play((CDH_SoundManager::eSound)38, false, false);
        {
            XString id(m_offer->m_id);
            int price = (int)((App*)WindowApp::m_instance)->m_offersManager->m_probabilityRandom;
            CGameAnalytics::logDailyDealResult(id, price, (CGameAnalytics::EOTOResult)2);
        }
        CNotEnoughCurrencyDialogWindow* dialog = new CNotEnoughCurrencyDialogWindow(m_item);
        WindowApp::HandleTunnelCommand(0x1e11f5b7, 0, 0, 0);
        dialog->InitIAP();
        m_parent->AddModal(dialog);
        return;
    }
    if (event.m_id == 0x0c9b0cea) {
        XString id(m_offer->m_id);
        int price = (int)((App*)WindowApp::m_instance)->m_offersManager->m_probabilityRandom;
        CGameAnalytics::logDailyDealResult(id, price, (CGameAnalytics::EOTOResult)0);
    }
    CZombieDialogWindow::OnCommand(event);
}
