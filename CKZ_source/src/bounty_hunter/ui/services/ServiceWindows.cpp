// b081j: original 15b358..15be60 complete service windows; borrowed resources and animation state kept as in the original.
#include "ServiceWindows.h"
#include "App.h"
#include "BaseDialog.h"
#include "CBH_SurfaceManager.h"
#include "CFontMgr.h"
#include "CFont.h"
#include "ICGraphics2d.h"
#include "MathLib.h"
#include "CStdUtil_Cocoa.h"

using namespace com::glu::platform::graphics;

ViewWindow::ViewWindow(const swerve::World<CSwerveAssertingErrorHandler>& world)
    : WindowTransparent(false), m_world(world)
{
    SetFlags(0x1000);
}

ViewWindow::~ViewWindow()
{
    // Original 15b41c clears the handle before the Window destructor; the automatic wrapper destructor sees NULL afterwards.
    m_world.Clear();
}

void ViewWindow::Paint3D(swerve::Graphics3D<CSwerveAssertingErrorHandler>& graphics)
{
    if (m_world != swerve::World<CSwerveAssertingErrorHandler>()) {
        graphics.render(m_world.m_interface);
    }
}

void SetSizesByContent(Window* window, int flags)
{
    if (window == NULL) {
        return;
    }
    if (flags & 2) {
        window->SetWidthByContent(0, 0);
    }
    if (flags & 1) {
        window->SetHeightByContent(0, 0);
    }
}

Window* createTD(Window* window, int flags)
{
    Window* container = new Window(false);
    if (window != NULL) {
        container->AddToFront(window);
    }
    SetSizesByContent(container, flags);
    return container;
}

Window* createTD(Window* window, int column, int row, int flags)
{
    Window* container = createTD(window, flags);
    container->SetCellPos(column, row, 1, 1);
    return container;
}

SeparateLineRed::SeparateLineRed() : Window(false)
{
    m_center.Create("SUR_BOUNTY_MENU_SEPARATE_LINE_CENTER");
    m_right.Create("SUR_BOUNTY_MENU_SEPARATE_LINE_RIGHT");
    m_left.Create("SUR_BOUNTY_MENU_SEPARATE_LINE_LEFT");
}

void SeparateLineRed::Paint(ICGraphics2d& graphics)
{
    ICRenderSurface* left = m_left.GetSurface();
    ICRenderSurface* center = m_center.GetSurface();
    ICRenderSurface* right = m_right.GetSurface();
    DrawRectImageHoriz(graphics, left, center, right, 0, 0, m_width, 1);
}

StretchImageButton::StretchImageButton(XString text, const char* left,
                                     const char* center, const char* right)
    : m_forcedWidth(0)
{
    m_left = ((App*)WindowApp::m_instance)->m_surfaceManager->CreateSurface(left, true, false);
    m_right = ((App*)WindowApp::m_instance)->m_surfaceManager->CreateSurface(right, true, false);
    m_center = ((App*)WindowApp::m_instance)->m_surfaceManager->CreateSurface(center, true, false);
    m_text.Assign(text);
    SetLayoutType(LAYOUT_SIMPLE);
    SetAlign(0xc);
    m_font = CFontMgr::GetInstance()->GetFont(15);
    int textWidth = m_font->MeasureTextWidth(m_text.m_data, -1, -1, NULL);
    int width = textWidth + ImageWidth(m_left);
    TextWindow* label = new TextWindow(m_text, m_font);
    label->SetLayoutType(LAYOUT_SIMPLE);
    label->SetAlign(0xc);
    label->SetOutsetSpacing(App::Scale() * 4, 0, 0, 0);
    label->SetSize(width, ImageHeight(m_center));
    AddToFront(label);
    SetSize(width, ImageHeight(m_center));
}

StretchImageButton::~StretchImageButton()
{
    ((App*)WindowApp::m_instance)->m_surfaceManager->ReleaseSurface(m_left);
    ((App*)WindowApp::m_instance)->m_surfaceManager->ReleaseSurface(m_right);
    ((App*)WindowApp::m_instance)->m_surfaceManager->ReleaseSurface(m_center);
}

void StretchImageButton::Paint(ICGraphics2d& graphics)
{
    if (m_pressed) {
        BeginBlend(graphics, 0.7f, 0.0f, 0.0f, 0.0f, false, false);
    }
    DrawRectImageHoriz(graphics, m_left, m_center, m_right, 0, 0, m_width, m_height);
    // The original re-reads pressed after the draw callback instead of caching the first check.
    if (m_pressed) {
        EndBlend(graphics);
    }
    Window::Paint(graphics);
}

void StretchImageButton::SetRect(int x, int y, int width, int height, bool force)
{
    int textWidth = m_font->MeasureTextWidth(m_text.m_data, -1, -1, NULL);
    int leftWidth = ImageWidth(m_left);
    int forcedWidth = m_forcedWidth;
    int imageHeight = ImageHeight(m_center);
    int finalWidth = textWidth + leftWidth * 4;
    if (forcedWidth > 0) {
        finalWidth = forcedWidth;
    }
    Window::SetRect(x, y, finalWidth, imageHeight, force);
}

void StretchImageButton::ForceSetWidth(int width)
{
    m_forcedWidth = width;
    SetRect(m_x, m_y, m_width, m_height, true);
}

LabelWithIcon::LabelWithIcon(const char* image, XString text, bool flag, CFont* font, int spacing)
    : Window(false)
{
    Create(image, text, flag, font, spacing);
}

LabelWithIcon::~LabelWithIcon()
{
    ((App*)WindowApp::m_instance)->m_surfaceManager->ReleaseSurface(m_imageName);
}

void LabelWithIcon::Create(const char* image, XString text, bool flag, CFont* font, int spacing)
{
    m_imageName = image;
    if (font == NULL) {
        font = CFontMgr::GetInstance()->GetFont(8);
    }
    m_text = new TextWindow(text, font);
    m_text->SetAlign(0x24);
    m_text->SetOutsetSpacing(spacing, 0, 0, 0);
    if (image != NULL) {
        ImageWindow* icon = new ImageWindow;
        icon->SetImage(((App*)WindowApp::m_instance)->m_surfaceManager->CreateSurface(m_imageName, true, false));
        icon->SetAlign(0x24);
        if (flag == 1) {
            AddToFront(m_text, 0, 0);
            AddToFront(icon, 1, 0);
        } else {
            AddToFront(icon, 0, 0);
            AddToFront(m_text, 1, 0);
        }
    } else {
        AddToFront(m_text, 0, 0);
    }
    SetWidthByContent(0, 0);
    SetHeightByContent(0, 0);
}

AnimatedWindow::AnimatedWindow(EMode mode) : WindowTransparent(false)
{
    // The original writes only these six groups of fields; the remaining parameters stay uninitialized until SetStart/CloseEasy.
    m_start.m_enabled = false;
    m_start.m_flag20 = false;
    m_start.m_lastScale = 1.0f;
    m_start.m_firstScale = 1.0f;
    m_close.m_enabled = false;
    m_close.m_flag20 = false;
    m_close.m_lastScale = 1.0f;
    m_close.m_firstScale = 1.0f;
    m_mode = mode; m_closing = 0;  // assigned after the easy parameters (original store order)
    SetWidthByContent(0, 0);
    SetHeightByContent(0, 0);
}

void AnimatedWindow::StartCustomEasy(SEasyParams& parameters)
{
    EasyParams easy(this, parameters.m_method);
    easy.SetFullTime(parameters.m_time);
    easy.SetOpacity(parameters.m_firstOpacity, parameters.m_lastOpacity);
    easy.SetScale(parameters.m_firstScale, parameters.m_lastScale);
    easy.SetDelayTime(parameters.m_delay);
    int x = m_x;
    int y = m_y;
    int newX = x + parameters.m_x;
    int newY = y + parameters.m_y;
    if (parameters.m_flag20) {
        easy.SetMoving(newX, newY, x, y);
    } else {
        easy.SetMoving(x, y, newX, newY);
    }
    StartEasy(easy, 0x100);
}

void AnimatedWindow::SetStartEasy(int x, int y, float time, float firstOpacity,
    float lastOpacity, float delay, EasyLib::EMethod method)
{
    m_start.m_x = x;
    m_start.m_y = y;
    m_start.m_firstOpacity = firstOpacity;
    m_start.m_lastOpacity = lastOpacity;
    m_start.m_time = time;
    m_start.m_delay = delay;
    m_start.m_enabled = true;
    m_start.m_method = method;
    m_start.m_flag20 = true;
}

void AnimatedWindow::SetCloseEasy(int x, int y, float time, float firstOpacity,
    float lastOpacity, float delay, EasyLib::EMethod method)
{
    m_close.m_x = x;
    m_close.m_y = y;
    m_close.m_firstOpacity = firstOpacity;
    m_close.m_lastOpacity = lastOpacity;
    m_close.m_time = time;
    m_close.m_delay = delay;
    m_close.m_enabled = true;
    m_close.m_method = method;
}

void AnimatedWindow::SetStartEasyScale(float first, float last)
{
    m_start.m_firstScale = first;
    m_start.m_lastScale = last;
}

void AnimatedWindow::SetCloseEasyScale(float first, float last)
{
    m_close.m_firstScale = first;
    m_close.m_lastScale = last;
}

void AnimatedWindow::OnStartupEasy()
{
    if (m_closing != 0) {
        return;
    }
    if (m_start.m_enabled) {
        StartCustomEasy(m_start);
    } else {
        OnEasyFinished();
    }
}

void AnimatedWindow::OnEasyFinished()
{
    // b147: original compares closing 2 before 0 and mode 2 before 1, i.e. the
    // "0" and "mode 1" branches are written first.
    if (m_closing == 0) {
        m_closing = 1;
        if ((int)m_mode == 1) {
            Window::Close();
        } else if ((int)m_mode == 2) {
            if (m_close.m_enabled) {
                StartCustomEasy(m_close);
                m_closing = 2;
            } else {
                Window::Close();
            }
        }
    } else if (m_closing == 2) {
        Window::Close();
    }
}

void AnimatedWindow::Close()
{
    if (m_close.m_enabled) {
        StartCustomEasy(m_close);
        m_closing = 2;
    } else {
        Window::Close();
    }
}

void AnimatedWindow::OnLayoutEasy(int x, int y, int width, int height) {} // Original 15bd30 is really empty.

void BlinkingContainer::Update()
{
    if (m_blinking) {
        unsigned int time = CStdUtil_Cocoa::GetUpTimeMS();
        float distance = (int)(time % 600 - 300);
        SetOpacity(MathLib::Fabs(distance) / 360.0f + 0.2f);
    } else {
        SetOpacity(1.0f);
    }
}
