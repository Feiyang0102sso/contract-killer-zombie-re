// b082: complete shop item, 0x147508..0x147634.
#include "CGPSShop.h"
#include "App.h"
#include "CTutorialManager.h"

CGPSShop::CGPSShop() : CGPSItem()
{
    m_listener = NULL;
    SetSGImage(5, -1);
    SetScreenName(Window::ResString("IDS_MENU_STORE"));
    SetBlinkingCmd(0xe84cf49e);
}

CGPSShop::~CGPSShop()
{
}

void CGPSShop::Update(long elapsed)
{
    if (((App*)WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial() == 1) {
        bool blink = ((App*)WindowApp::m_instance)->m_tutorialManager->IsMapShopBlink() == 1;
        SetBlinking(blink);
    }
    CGPSItem::Update(elapsed);
}

void CGPSShop::Save(TiXmlElement*)
{
    // Original return-only method.
}

void CGPSShop::OnClick()
{
    if (m_listener != NULL) {
        m_listener->OnShopItemSelected(this);
    }
}

void CGPSShop::Paint(com::glu::platform::graphics::ICGraphics2d& graphics, int x, int y)
{
    CGPSItem::Paint(graphics, x, y);
}

void CGPSShop::Load(TiXmlElement*)
{
    // Original return-only method.
}

void CGPSShop::SetListener(CGPSShopListener* listener)
{
    m_listener = listener;
}

void CGPSShop::SetBlinking(bool blinking)
{
    if (m_blinking != blinking) {
        m_blinking = blinking;
    }
}
