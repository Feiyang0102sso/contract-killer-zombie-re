// b074: complete stats panel/HUD owner. Shared window constructors remain in their real owners.
#include "CBH_GPSHud.h"
#include "CPlayerGameResourceWindow.h"
#include "CPlayerXPBar.h"
#include "CGameStagesWindow.h"
#include "SScreenAdaptingConstant.h"
#include "BaseDialog.h"
#include "CFontMgr.h"
#include "CTutorialManager.h"
#include "CSettingsWindow.h"
#include "App.h"

CHudStatsPanel::CHudStatsPanel() : Window(false)
{
    SScreenAdaptingConstant resourceSpacing(80);
    SGImageWindowCommand* button = CreateAddButton(29);
    button->SetAlign(0x21);
    button->SetOutsetSpacing(0, 0, 0, 0);
    AddToFront(button);
    int offset = button->m_imageWidth;
    CPlayerGameResourceWindow* resource = new CPlayerGameResourceWindow((CGameResourceWindow::EType)0, false);
    resource->SetLayoutType(LAYOUT_SIMPLE);
    resource->SetAlign(0x21);
    resource->m_command = 0x1e11f5b7;
    resource->m_commandParameter1 = 0;
    resource->m_commandParameter2 = 0;
    resource->m_commandParameter3 = 0;
    resource->SetOutsetSpacing(0, 0, 0, offset);
    AddToFront(resource);
    offset += resourceSpacing();
    button = CreateAddButton(28);
    button->SetAlign(0x21);
    button->SetOutsetSpacing(0, 0, 0, offset);
    AddToFront(button);
    offset += button->m_imageWidth;
    resource = new CPlayerGameResourceWindow((CGameResourceWindow::EType)1, false);
    resource->SetLayoutType(LAYOUT_SIMPLE);
    resource->SetAlign(0x21);
    resource->m_command = 0x1e11f5b7;
    resource->m_commandParameter1 = 0;
    resource->m_commandParameter2 = 0;
    resource->m_commandParameter3 = 0;
    resource->SetOutsetSpacing(0, 0, 0, offset);
    AddToFront(resource);
    offset += resourceSpacing();
    CPlayerXPBar* xp = new CPlayerXPBar();
    xp->SetAlign(0x21);
    xp->SetOutsetSpacing(0, 0, 0, offset);
    AddToFront(xp);
    resourceSpacing(); // Original keeps this unused platform query.
    SetWidthByContent(0, 0);
    SetHeightByContent(0, 0);
    SetOutsetSpacing(SScreenAdaptingConstant(5)(), 0);
}

SGImageWindowCommand* CHudStatsPanel::CreateAddButton(int font)
{
    SGImageWindowCommand* button = new SGImageWindowCommand();
    button->SetArchetypeCharacter(1, 0);
    button->SetAnimation(87, 1);
    button->SetAlign(0x24);
    TextWindow* text = new TextWindow();
    text->SetAlign(0x24);
    {
        XString caption = Window::ResString("IDS_SHOP_ADD");
        text->SetText(caption, CFontMgr::GetInstance()->GetFont(font));
    }
    button->AddToFront(text);
    button->m_command = 0x1e11f5b7;
    button->m_commandParameter1 = 0;
    button->m_commandParameter2 = 0;
    button->m_commandParameter3 = 0;
    return button;
}

CBH_GPSHud::CBH_GPSHud() : CBaseScreen(false, false, true), m_top(), m_bottom()
{
    CHudStatsPanel* stats = new CHudStatsPanel();
    stats->SetAlign(0xc);
    AddToFront(stats);
    CGameStagesWindow* stages = new CGameStagesWindow();
    stages->SetAlign(0x14);
    AddToFront(stages);
    m_shop = new CShopBlinkingButton();
    m_shop->SetArchetypeCharacter(1, 0);
    m_shop->SetAnimation(77, 1);
    m_shop->m_command = 0xc4794df3;
    m_shop->m_commandParameter1 = -1;
    m_shop->m_commandParameter2 = 0;
    m_shop->m_commandParameter3 = 0;
    m_shop->SetAlign(0x11);
    TextWindow* caption = new TextWindow();
    {
        XString text = Window::ResString("IDS_BUTTON_SHOP");
        caption->SetText(text, CFontMgr::GetInstance()->GetFont(26));
    }
    caption->SetAlign(0x14);
    caption->SetOutsetSpacing(0, 0, SScreenAdaptingConstant(3)(), 0);
    m_shop->AddToFront(caption);
    AddToFront(m_shop);
    m_top.SetArchetypeCharacter(1, 0);
    m_top.SetAnimation(37);
    m_bottom.SetArchetypeCharacter(1, 0);
    m_bottom.SetAnimation(38);
}

void CBH_GPSHud::Paint(com::glu::platform::graphics::ICGraphics2d&)
{
    m_top.Draw();
    m_bottom.Draw(0, WindowApp::m_instance->m_screenHeight);
}

void CBH_GPSHud::Update()
{
    if (((App*)WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial() == 1) {
        CShopBlinkingButton* shop = m_shop;
        shop->SetBlink(((App*)WindowApp::m_instance)->m_tutorialManager->IsMapShopBlink());
    }
}

void CBH_GPSHud::OnCommand(Event& event)
{
    if ((unsigned int)event.m_id == 0x5f523cd9) {
        event.Clear();
    } else if ((unsigned int)event.m_id == 0x9720b34a) {
        CSettingsWindow* settings = new CSettingsWindow(false, false);
        settings->SetLayoutType(LAYOUT_SIMPLE);
        settings->SetAlign(0x24);
        ((App*)WindowApp::m_instance)->m_mainLayer->AddModal(settings);
        event.Clear();
    } else {
        CBaseScreen::OnCommand(event);
    }
}

void CBH_GPSHud::OnGetFocus()
{
    // Original return-only method.
}

void CBH_GPSHud::OnLostFocus()
{
    // Original return-only method.
}
