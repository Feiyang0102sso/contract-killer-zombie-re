// CImageButton.cpp: keeps the ignored image coordinate arguments, the repeated size queries and the resource release order.
#include "CImageButton.h"
#include "BaseDialog.h"
#include "App.h"
#include "CBH_SurfaceManager.h"
#include "CFontMgr.h"

void CImageButton::AddImage(const char* name, int x, int y)
{
    m_imageNames.addElement(XString(name));
    com::glu::platform::graphics::ICRenderSurface* surface =
        ((App*)WindowApp::m_instance)->m_surfaceManager->CreateSurface(name, true, false);
    AddImage(surface, x, y);
}

void CImageButton::AddImage(com::glu::platform::graphics::ICRenderSurface* surface, int x, int y)
{
    ImageWindow* image = new ImageWindow();
    image->SetFillType(ImageWindow::FILL_NONE);
    image->SetImage(surface);
    if (m_width < Window::ImageWidth(surface)) {
        SetSize(Window::ImageWidth(surface), m_height);
    }
    if (m_height < Window::ImageHeight(surface)) {
        SetSize(m_width, Window::ImageHeight(surface));
    }
    AddToFront(image);
    m_imageWindow = image;
}

void CImageButton::SetImage(com::glu::platform::graphics::ICRenderSurface* surface, int x, int y)
{
    if (m_imageWindow != NULL) {
        m_imageWindow->SetImage(surface);
    } else {
        AddImage(surface, x, y);
    }
}

CImageButton::CImageButton()
    : CBH_Button(), m_textWindow(NULL), m_imageWindow(NULL)
{
    SetSize(0, 0);
    SetHeightByContent(0, 0);
    SetWidthByContent(0, 0);
}

void CImageButton::SetText(XString text, int alignment, com::glu::platform::graphics::CFont* font,
                           int leftSpacing, int percentHeight)
{
    if (m_textWindow != NULL) {
        m_textWindow->Close();
    }
    TextWindow* window = new TextWindow();
    if (font == NULL) {
        font = CFontMgr::GetInstance()->GetFont(15);
    }
    window->SetText(text, font);
    window->SetAlign(alignment);
    window->SetHeightByContent(0, 0);
    if (percentHeight > 0) {
        window->SetPercentHeight(percentHeight, 0, 0);
    }
    window->m_debugFrame = false;
    window->SetWidthByContent(0, 0);
    window->SetOutsetSpacing(leftSpacing, 0, 0, 0);
    AddToFront(window);
    m_textWindow = window;
}

void CImageButton::Update()
{
    float opacity = 1.0f;
    if (m_pressed && m_command != 0) {
        opacity = 0.7f;
    }
    SetOpacity(opacity);
}

CImageButton::~CImageButton()
{
    for (int index = 0; index < m_imageNames.size(); ++index) {
        CBH_SurfaceManager* manager = ((App*)WindowApp::m_instance)->m_surfaceManager;
        XString::AnsiString name(m_imageNames[index].c_str());
        manager->ReleaseSurface(name);
    }
    // The original clears explicitly first, then the member's natural destruction still calls free_all_items; not omitted.
    m_imageNames.removeAllElements();
}
