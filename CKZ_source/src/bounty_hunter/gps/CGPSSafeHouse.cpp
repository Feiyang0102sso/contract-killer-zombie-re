// b082: complete Safehouse owner; tutorial update intentionally does not call base Update.
#include "CGPSSafehouse.h"
#include "App.h"
#include "CTutorialManager.h"

CGPSSafehouse::CGPSSafehouse() : CGPSItem()
{
    m_listener = NULL;
    SetSGImage(6, -1);
    SetScreenName(Window::ResString("IDS_MAP_SAFEHOUSE"));
}

CGPSSafehouse::~CGPSSafehouse()
{
}

void CGPSSafehouse::Update(long)
{
    if (((App*)WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial() == 1) {
        SetVisible(((App*)WindowApp::m_instance)->m_tutorialManager->ShowSafehouse());
    }
}

void CGPSSafehouse::Save(TiXmlElement*)
{
    // Original return-only method.
}

void CGPSSafehouse::OnClick()
{
    if (m_listener != NULL) {
        m_listener->OnSafehouseItemSelected(this);
    }
}

void CGPSSafehouse::Paint(com::glu::platform::graphics::ICGraphics2d& graphics, int x, int y)
{
    CGPSItem::Paint(graphics, x, y);
}

void CGPSSafehouse::Load(TiXmlElement*)
{
    // Original return-only method.
}

void CGPSSafehouse::SetListener(CGPSSafehouseListener* listener)
{
    m_listener = listener;
}

void CGPSSafehouse::SetBlinking(bool blinking)
{
    if (m_blinking != blinking) {
        m_blinking = blinking;
    }
}
