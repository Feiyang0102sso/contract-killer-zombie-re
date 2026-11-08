// b074: original 0x117d34..0x118360. Repeated resource queries and ownership are intentional.
#include "CBH_MenuButton.h"
#include "CBH_SurfaceManager.h"
#include "App.h"
#include "CFontMgr.h"
#include "CFont.h"
#include "CUtility.h"
#include "CStrChar.h"
#include "CStdUtil_Cocoa.h"
#include "MathLib.h"
#include "ICRenderSurface.h"

using namespace com::glu::platform::graphics;
using namespace com::glu::platform::components;

void CBH_MenuButton::Init()
{
    // Original 0x117d34 is a return.
}

void CBH_MenuButton::Paint(ICGraphics2d& graphics)
{
    int padding = m_padding;
    int imageX = padding + (m_width - m_padding * 2 - ImageWidth(m_image)) / 2;
    DrawImage(graphics, m_image, imageX, m_padding, 0, 0);
    if (m_blinking && m_blinkImage != NULL) {
        float opacity = MathLib::Fabs((float)((int)((unsigned int)CStdUtil_Cocoa::GetUpTimeMS() % 600) - 300)) / 360.0f + 0.2f;
        BeginBlend(graphics, opacity, 1.0f, 1.0f, 0.0f, true, false);
        ICRenderSurface* image = m_blinkImage;
        int dx = ImageWidth(m_image) - ImageWidth(m_blinkImage);
        int dy = ImageHeight(m_image) - ImageHeight(m_blinkImage);
        DrawImage(graphics, image, imageX + dx / 2, padding + dy / 2, 0, 0);
        EndBlend(graphics);
    } else if (m_pressed && m_pressedImage != NULL) {
        ICRenderSurface* image = m_pressedImage;
        int dx = ImageWidth(m_image) - ImageWidth(m_pressedImage);
        int dy = ImageHeight(m_image) - ImageHeight(m_pressedImage);
        DrawImage(graphics, image, imageX + dx / 2, padding + dy / 2, 0, 0);
    }
    int fontHeight = m_font->GetFontHeight();
    if (m_caption.ByteSize() != 0) {
        const wchar_t* text = m_caption.m_data;
        int width = m_width;
        CFont* font = m_font;
        int imageHeight = ((App*)WindowApp::m_instance)->m_surfaceManager->GetSurfaceHeight(m_image);
        DrawFontText(graphics, font, text, width / 2, imageHeight - fontHeight / 2, 0x24);
    }
}

void CBH_MenuButton::ReleaseResources()
{
    if (m_image != NULL) {
        ((App*)WindowApp::m_instance)->m_surfaceManager->ReleaseSurface(m_image);
    }
    if (m_pressedImage != NULL) {
        ((App*)WindowApp::m_instance)->m_surfaceManager->ReleaseSurface(m_pressedImage);
    }
    if (m_blinkImage != NULL) {
        ((App*)WindowApp::m_instance)->m_surfaceManager->ReleaseSurface(m_blinkImage);
    }
    m_image = NULL;
    m_pressedImage = NULL;
    m_blinkImage = NULL;
}

CBH_MenuButton::CBH_MenuButton() : CBH_Button(), m_caption()
{
    m_image = NULL;
    m_pressedImage = NULL;
    m_blinkImage = NULL;
    m_padding = 0;
    m_blinking = false;
    m_blinkCommand = 0;
    m_unresolved104 = -1;
    m_font = CFontMgr::GetInstance()->GetFont(9);
}

void CBH_MenuButton::setImage(const char* name)
{
    if (m_image != NULL) {
        ((App*)WindowApp::m_instance)->m_surfaceManager->ReleaseSurface(m_image);
    }
    m_image = ((App*)WindowApp::m_instance)->m_surfaceManager->CreateSurface(name, true, false);
    UpdateSize();
    CStrChar active(name);
    active.Concatenate("_ACTIVE");
    if (((App*)WindowApp::m_instance)->m_surfaceManager->IsImageExists(active.GetCharPtr())) {
        m_pressedImage = ((App*)WindowApp::m_instance)->m_surfaceManager->CreateSurface(active.GetCharPtr(), true, false);
    }
}

void CBH_MenuButton::UpdateSize()
{
    unsigned int width;
    unsigned int height;
    m_image->GetWidthAndHeight(width, height);
    if (m_caption.ByteSize() != 0) {
        float textWidth = m_font->MeasureTextWidth(m_caption.m_data, -1, -1, NULL);
        float imageWidth = width;
        if (textWidth > imageWidth) {
            imageWidth = textWidth;
        }
        width = (unsigned int)imageWidth;
    }
    SetSize(width + m_padding * 2, height + m_padding * 2);
    SetDesiredSizes();
}

void CBH_MenuButton::setPressedImage(const char* name)
{
    if (m_pressedImage != NULL) {
        ((App*)WindowApp::m_instance)->m_surfaceManager->ReleaseSurface(m_pressedImage);
    }
    m_pressedImage = ((App*)WindowApp::m_instance)->m_surfaceManager->CreateSurface(name, true, false);
}

void CBH_MenuButton::SetCaption(const char* name)
{
    {
        CStrWChar text = CUtility::GetString(name);
        m_caption.Assign(text.GetWCharPtr(), wcslen(text.GetWCharPtr()) * sizeof(wchar_t));
    }
    UpdateSize();
}

void CBH_MenuButton::SetDesiredSizes()
{
    unsigned int width;
    unsigned int height;
    m_image->GetWidthAndHeight(width, height);
    if (m_caption.ByteSize() != 0) {
        float textWidth = m_font->MeasureTextWidth(m_caption.m_data, -1, -1, NULL);
        float imageWidth = width;
        if (textWidth > imageWidth) {
            imageWidth = textWidth;
        }
        width = (unsigned int)imageWidth;
    }
    SetDesiredWidth(width);
    SetDesiredHeight(height);
}

void CBH_MenuButton::SetCorner(int x, int y)
{
    Window::SetCorner(x - m_padding, y - m_padding);
}

void CBH_MenuButton::SetBlinkingCmd(unsigned int command)
{
    m_blinkCommand = command;
}

void CBH_MenuButton::OnCommand(Event& event)
{
    if ((unsigned int)event.m_id == m_blinkCommand) {
        m_blinking = event.m_param1 != 0;
    }
}
