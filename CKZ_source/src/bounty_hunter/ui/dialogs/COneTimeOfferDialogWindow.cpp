// u542 full owner; keeps the original ownership behavior where, when rebuilding the layout, the old inner only clears its subtree.
#include "COneTimeOfferDialogWindow.h"
#include "App.h"
#include "COffersManager.h"
#include "CIAPCurrencyManager.h"
#include "CIAPCurrency.h"
#include "CShopItemButton.h"
#include "CFontMgr.h"
#include "CGameAnalytics.h"
#include "SGImageWindow.h"
#include "ServiceWindows.h"
#include "SScreenAdaptingConstant.h"

COneTimeOfferDialogWindow::COneTimeOfferDialogWindow(int packId)
    : CZombieDialogWindow(3), m_currency(NULL), m_inner(NULL), m_button(NULL), m_packId(packId)
{
    m_background->SetArchetypeCharacter(8, 0);
    m_background->SetAnimation(0, 1);
    SetDesiredWidth(m_background->m_imageWidth);
    SetDesiredHeight(m_background->m_imageHeight);
    ShowClosingCross((unsigned int)-1);
    SetTitle(ResString("IDS_ONE_TIME_OFFER_DIALOG_TITLE"));
    TextWindow* title = m_titleText;
    title->SetFont(CFontMgr::GetInstance()->GetFont(30));
    m_contentContainer->SetLayoutType(LAYOUT_SIMPLE);
    m_contentContainer->SetAlign(20);
    m_contentContainer->SetPercentHeight(100, 0, 0);
    m_contentContainer->SetPercentWidth(100, 0, 0);
    UpdateIAPLayout();
}

void COneTimeOfferDialogWindow::UpdateIAPLayout()
{
    if (m_button == NULL) {
        m_button = new CShopItemButton(m_currency);
        m_button->SetPercentWidth(100, 0, 0);
        m_button->SetPercentHeight(80, 0, 0);
        Window* button = m_button;
        button->SetOutsetSpacing(0, 0, SScreenAdaptingConstant(20)(), 0);
        m_button->SetLayoutType(LAYOUT_SIMPLE);
        m_button->SetAlign(20);
        m_button->m_command = 0x4e349820;
        m_button->m_commandParameter1 = 0;
        m_button->m_commandParameter2 = 0;
        m_button->m_commandParameter3 = 0;
        AddToFront(m_button);
    } else {
        m_inner->DeleteChilds();
    }
    m_inner = new Window(false);
    m_inner->SetPercentWidth(100, 0, 0);
    m_inner->SetPercentHeight(100, 0, 0);
    m_button->AddToFront(m_inner);
    if (m_currency == NULL) {
        return;
    }
    TextWindow* description = new TextWindow(CFontMgr::GetInstance()->GetFont(30));
    {
        XString text = ((App*)WindowApp::m_instance)->m_offersManager->m_text;
        description->SetText(text);
    }
    description->SetLayoutType(LAYOUT_SIMPLE);
    description->SetAlign(36);
    description->SetOutsetSpacing(SScreenAdaptingConstant(0)(), 0, 0, 0);
    m_inner->AddToFront(description, 0, 0);
    ImageWindow* image = new ImageWindow;
    image->SetImage(m_currency->GetShopImage());
    image->SetLayoutType(LAYOUT_SIMPLE);
    image->SetAlign(36);
    Window* inner = m_inner;
    inner->AddToFront(createTD(image, 3), 0, 1);
    SGImageWindow* buyBackground = new SGImageWindow;
    buyBackground->SetArchetypeCharacter(1, 0);
    buyBackground->SetAnimation(83, 1);
    buyBackground->SetLayoutType(LAYOUT_SIMPLE);
    buyBackground->SetAlign(36);
    buyBackground->SetOutsetSpacing(0, 0, SScreenAdaptingConstant(5)(), 0);
    Window* row = new WindowTransparent(false);
    row->SetHeightByContent(0, 0);
    row->SetPercentWidth(100, 0, 0);
    row->SetInsetSpacing(0, SScreenAdaptingConstant(10)());
    row->SetLayoutType(LAYOUT_SIMPLE);
    row->SetAlign(36);
    buyBackground->AddToFront(row);
    TextWindow* buy = new TextWindow(CFontMgr::GetInstance()->GetFont(29));
    buy->SetText(ResString("IDS_ONE_TIME_OFFER_DIALOG_BUY_BUTTON"));
    buy->SetLayoutType(LAYOUT_SIMPLE);
    buy->SetAlign(36);
    buy->SetOutsetSpacing(SScreenAdaptingConstant(1)(), 0);
    row->AddToFront(buy, 0, 0);
    TextWindow* price = new TextWindow;
    m_currency->FillPriceText(price);
    price->SetFont(CFontMgr::GetInstance()->GetFont(29));
    price->SetLayoutType(LAYOUT_SIMPLE);
    price->SetAlign(36);
    price->SetOutsetSpacing(SScreenAdaptingConstant(1)(), 0);
    row->AddToFront(price, 0, 1);
    inner = m_inner;
    inner->AddToFront(createTD(buyBackground, 3), 0, 2);
}

void COneTimeOfferDialogWindow::Update()
{
    CIAPCurrency* item = (CIAPCurrency*)((App*)WindowApp::m_instance)->m_iapCurrencyManager->getItemById(m_packId);
    if (item != m_currency) {
        m_currency = item;
        UpdateIAPLayout();
    }
    Window::Update();
}

void COneTimeOfferDialogWindow::OnCommand(Event& event)
{
    if (event.m_id == 0x4e349820) {
        ReverseEasy(0x200);
        const XString& name = m_currency->GetName();
        int price = (int)((App*)WindowApp::m_instance)->m_offersManager->m_probabilityRandom;
        CGameAnalytics::logOneTimeOfferResult(name, price, (CGameAnalytics::EOTOResult)1);
        m_currency->BuyFromOneTimeOffer();
        event.Clear();
        return;
    }
    if (event.m_id == 0x0c9b0cea) {
        const XString& name = m_currency->GetName();
        int price = (int)((App*)WindowApp::m_instance)->m_offersManager->m_probabilityRandom;
        CGameAnalytics::logOneTimeOfferResult(name, price, (CGameAnalytics::EOTOResult)0);
    } else if (event.m_id == (int)0x97204784) {
        ReverseEasy(0x200);
        event.Clear();
        return;
    }
    CZombieDialogWindow::OnCommand(event);
}
