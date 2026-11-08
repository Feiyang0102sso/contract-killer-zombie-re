// b082: complete GPS item owner, including repeated virtual rectangle/width queries.
#include "CGPSItem.h"
#include "App.h"
#include "CFontMgr.h"
#include "CFont.h"
#include "CBH_SurfaceManager.h"
#include "DGHelper.h"
#include "CStdUtil_Cocoa.h"
#include "ICRenderSurface.h"
#include "MathLib.h"

using namespace com::glu::platform::graphics;
using namespace com::glu::platform::components;

CGPSItem::CGPSItem()
    : m_attractSpeed(0.0f), m_blinking(false), m_pressed(false), m_visible(true),
      m_screenName(), m_parallax(0.05f), m_presenter(), m_secondaryPresenter()
{
    m_opacity = 1.0f;
    m_image = NULL;
    m_blinkImage = NULL;
    m_pressImage = NULL;
    if (App::IsHD() == 1) {
        m_scaleX = 2.0f;
        m_scaleY = 2.0f;
    } else {
        m_scaleX = 1.0f;
        m_scaleY = 1.0f;
    }
    m_font = CFontMgr::GetInstance()->GetFont(26);
    App::LoadSGArchetype(5, 0);
    m_presenter.SetArchetypeCharacter(5, 0);
    m_secondaryPresenter.SetArchetypeCharacter(5, 0);
}

CRectangle<short> CGPSItem::GetRect()
{
    float x = GetX();
    int width = m_width;
    float y = GetY();
    CRectangle<short> rect;
    rect.m_x = (int)(x - (float)width * 0.5f);
    rect.m_y = (int)(y - (float)m_height * 0.5f);
    rect.m_width = m_width;
    rect.m_height = m_height;
    return rect;
}

void CGPSItem::SetPos(float x, float y)
{
    m_x = x;
    m_y = y;
}

float CGPSItem::GetX()
{
    return m_x * m_scaleX;
}

float CGPSItem::GetY()
{
    return m_y * m_scaleY;
}

void CGPSItem::SetSize(int width, int height)
{
    m_width = width;
    m_height = height;
}

void CGPSItem::SetScreenName(const XString& name)
{
    m_screenName.Assign(name);
    DGHelper* helper = ((App*)WindowApp::m_instance)->m_helper;
    XString::AnsiString text(m_screenName.m_data);
    m_nameWidth = helper->GetTextWidth(text.m_text, m_font);
}

void CGPSItem::Paint(ICGraphics2d&, int x, int y)
{
    if (!m_visible) {
        return;
    }
    int left = GetRect().m_x + x;
    int top = GetRect().m_y + y;
    int width = m_width;
    int height = m_height;
    m_screenX = left;
    m_screenY = top;
    float distanceX = ((float)WindowApp::m_instance->m_screenWidth * 0.5f - (float)(left + width / 2));
    float parallaxX = m_parallax;
    int scaleX = App::Scale();
    float distanceY = (float)WindowApp::m_instance->m_screenHeight * 0.5f - (float)(top + height / 2);
    float offsetX = distanceX * parallaxX;
    float parallaxY = m_parallax;
    int scaleY = App::Scale();
    int drawX = (int)((float)left - offsetX * (float)scaleX);
    int drawY = (int)((float)top - (distanceY * parallaxY) * (float)scaleY);
    if (drawX <= -m_width || drawX >= WindowApp::m_instance->m_screenWidth ||
        drawY <= -m_height || drawY >= WindowApp::m_instance->m_screenHeight) {
        m_onScreen = false;
        return;
    }
    m_onScreen = true;
    ICGraphics::GetInstance()->GetRasterizerState()->PushState(ICRasterizerState::State_Blending);
    ICGraphics::GetInstance()->GetRasterizerState()->EnableBlending(1);
    ICGraphics::GetInstance()->GetRasterizerState()->SetBlendFactors(ICRasterizerState::BlendFactor_SrcAlpha, ICRasterizerState::BlendFactor_OneMinusSrcAlpha);
    WindowApp::m_instance->PushOpacity(m_opacity);
    if (m_image != NULL) {
        ((App*)WindowApp::m_instance)->m_helper->DrawImage(m_image, drawX, drawY, 1.0f, 0, 0, 0.0f, 0);
    }
    if (m_presenter.IsViable()) {
        m_presenter.Draw(drawX, drawY);
    }
    if (m_secondaryPresenter.IsViable() && !m_blinking) {
        m_secondaryPresenter.Draw(drawX, drawY);
    }
    if (m_blinking && m_blinkImage != NULL) {
        float opacity = MathLib::Fabs((float)((int)((unsigned int)CStdUtil_Cocoa::GetUpTimeMS() % 800) - 400)) / 800.0f + 0.5f;
        ((App*)WindowApp::m_instance)->m_helper->DrawImage(m_blinkImage, drawX + m_width / 2, drawY + m_height / 2, opacity, 0, 0, 0.0f, 0x30);
    }
    CFont* font = m_font;
    const wchar_t* text = m_screenName.m_data;
    int textAreaWidth = GetWidth();
    int textWidth = m_nameWidth;
    int textY = GetWidth() + drawY;
    const Color_RGBA* color = &WindowApp::m_instance->Color();
    font->PaintText(text, -1, drawX + textAreaWidth / 2 - textWidth / 2, textY, -1, -1, color);
    WindowApp::m_instance->PopColorStack();
    ICGraphics::GetInstance()->GetRasterizerState()->PopState(ICRasterizerState::State_Blending);
}

void CGPSItem::OnCommand(Event& event)
{
    if ((unsigned int)event.m_id == m_blinkCommand) {
        SetBlinking(event.m_param1 != 0);
    }
}

bool CGPSItem::CheckHit(int x, int y, CGPSItem** hit)
{
    if (!m_visible) {
        return false; // Original does not clear *hit on this path.
    }
    CRectangle<short> rect = GetRect();
    if (x > rect.m_x && x < rect.m_x + rect.m_width && y > rect.m_y && y < rect.m_y + rect.m_height) {
        *hit = this;
        return true;
    }
    *hit = NULL;
    return false;
}

void CGPSItem::SetSGImage(int animation, int secondAnimation)
{
    m_presenter.SetAnimation(animation);
    if (secondAnimation >= 1) {
        m_secondaryPresenter.SetAnimation(secondAnimation);
    }
    UpdateRects();
}

void CGPSItem::UpdateRects()
{
    if (m_presenter.IsViable()) {
        CRectangle<short> rect = {};
        m_presenter.Bounds(rect);
        SetSize(rect.m_width, rect.m_height);
    }
}

void CGPSItem::SetImage(const char* name)
{
    if (m_image != NULL) {
        ((App*)WindowApp::m_instance)->m_surfaceManager->ReleaseSurface(m_image);
    }
    m_image = ((App*)WindowApp::m_instance)->m_surfaceManager->CreateSurface(name, true, false);
    unsigned int width = 0;
    unsigned int height = 0;
    m_image->GetWidthAndHeight(width, height);
    SetSize(width, height);
}

void CGPSItem::SetBlinkingImage(const char* name)
{
    if (m_blinkImage != NULL) {
        ((App*)WindowApp::m_instance)->m_surfaceManager->ReleaseSurface(m_blinkImage);
    }
    m_blinkImage = ((App*)WindowApp::m_instance)->m_surfaceManager->CreateSurface(name, true, false);
}

void CGPSItem::SetPressImage(const char* name)
{
    if (m_pressImage != NULL) {
        ((App*)WindowApp::m_instance)->m_surfaceManager->ReleaseSurface(m_pressImage);
    }
    m_pressImage = ((App*)WindowApp::m_instance)->m_surfaceManager->CreateSurface(name, true, false);
}

void CGPSItem::SetPressed(bool pressed)
{
    if (m_pressed != pressed) {
        m_opacity = 1.0f;
        if (pressed) {
            m_opacity = 0.5f;
        }
        UpdateRects();
    }
    m_pressed = pressed;
    m_pressedTime = CStdUtil_Cocoa::GetUpTimeMS();
}

void CGPSItem::Update(long)
{
    if (m_pressed) {
        return;
    }
    if (m_blinking) {
        m_opacity = MathLib::Fabs((float)((int)((unsigned int)CStdUtil_Cocoa::GetUpTimeMS() % 800) - 400)) / 800.0f + 0.5f;
    } else {
        m_opacity = 1.0f;
    }
}

CGPSItem::~CGPSItem()
{
    if (m_image != NULL) {
        ((App*)WindowApp::m_instance)->m_surfaceManager->ReleaseSurface(m_image);
    }
    if (m_blinkImage != NULL) {
        ((App*)WindowApp::m_instance)->m_surfaceManager->ReleaseSurface(m_blinkImage);
    }
    if (m_pressImage != NULL) {
        ((App*)WindowApp::m_instance)->m_surfaceManager->ReleaseSurface(m_pressImage);
    }
}
