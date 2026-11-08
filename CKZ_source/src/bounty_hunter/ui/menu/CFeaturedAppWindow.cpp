// j: real download state and the original scale/translate algorithm; the original WebUtil overwrite leak is not patched on our own.
#include "CFeaturedAppWindow.h"
#include "WebUtil.h"
#include "CBH_SurfaceManager.h"
#include "CImageButton.h"
#include "SScreenAdaptingConstant.h"
#include "CFontMgr.h"
#include "App.h"
#include "CApplet.h"
#include "ICCore.h"
#include "ICGraphics.h"
#include "ICGraphics2d.h"
#include "ICRasterizerState.h"
#include "ICRenderSurface.h"
#include <math.h>
using namespace com::glu::platform::graphics;
using namespace com::glu::platform::components;

static SScreenAdaptingConstant TITLE_TEXT_Y(30, 50);

CFeaturedAppWindow::CFeaturedAppWindow() : Window(false), m_downloadedSurface(NULL)
{
    SetOutsetSpacing(SScreenAdaptingConstant(38, 76)(), 0, 0, 0);
    SetWidthByContent(0, 0);
    SetHeightByContent(0, 0);
    ImageWindow* background = new ImageWindow;
    background->SetFillType(ImageWindow::FILL_NONE);
    background->SetImage(((App*)WindowApp::m_instance)->m_surfaceManager->CreateSurface("IDB_DIALOG_WINDOW_BG", true, false));
    AddToFront(background);
    WindowTransparent* body = new WindowTransparent(false);
    body->SetLayoutType((ELayoutType)2);
    ImageWindow* logo = new ImageWindow;
    logo->SetFillType(ImageWindow::FILL_NONE);
    logo->SetImage(((App*)WindowApp::m_instance)->m_surfaceManager->CreateSurface("IDB_CK_LOGO", true, false));
    logo->SetCellPos(0, 0, 2, 1);
    logo->SetOutsetSpacing(TITLE_TEXT_Y(), 0, 0, 0);
    logo->SetAlign(0x0c);
    logo->SetFlags(0x800000);
    logo->SetPercentHeight(30, 0, 0);
    body->AddToFront(logo);
    WindowTransparent* imageContainer = new WindowTransparent(false);
    imageContainer->SetPercentWidth(25, 0, 0);
    imageContainer->SetHeightByContent(0, 0);
    // Original 1555ce..1555da clears the whole dc then calls the ImageWindow base constructor, matching value initialization with no user ctor.
    m_imageWindow = new ScaledURLImageWindow();
    m_imageWindow->m_scale = 1.0f;
    m_imageWindow->SetFillType(ImageWindow::FILL_NONE);
    m_imageWindow->SetImage(((App*)WindowApp::m_instance)->m_surfaceManager->CreateSurface("IDB_USER_PIC", true, false));
    m_imageWindow->SetAlign(0x22);
    imageContainer->SetCellPos(0, 1, 1, 1);
    imageContainer->AddToFront(m_imageWindow);
    body->AddToFront(imageContainer);
    WindowTransparent* descriptionContainer = new WindowTransparent(false);
    m_description = new TextWindow(XString("TESSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSST"), CFontMgr::GetInstance()->GetFont(15));
    m_description->SetPercentWidth(80, 0, 0);
    m_description->SetPercentHeight(100, 0, 0);
    m_description->SetAlign(0x21);
    descriptionContainer->SetPercentHeight(35, 0, 0);
    descriptionContainer->SetPercentWidth(60, 0, 0);
    descriptionContainer->AddToFront(m_description);
    descriptionContainer->SetCellPos(1, 1, 1, 1);
    body->AddToFront(descriptionContainer);
    Window* buttons = new Window(false);
    WindowTransparent* skipContainer = new WindowTransparent(false);
    CImageButton* skip = new CImageButton;
    skip->AddImage("IDB_SHOP_CARD_BUTTON", 0, 0);
    skip->SetText(XString("SKIP"), 0x0c, NULL, 0, 0);
    skip->SetLayoutType((ELayoutType)1);
    skip->SetAlign(0x12);
    skip->m_command = 0x00292395;
    skip->m_commandParameter1 = 0;
    skip->m_commandParameter2 = 0;
    skip->m_commandParameter3 = 0;
    skip->SetFlags(0x800000);
    skipContainer->SetPercentWidth(50, 0, 0);
    skipContainer->SetCellPos(0, 0, 1, 1);
    skipContainer->AddToFront(skip);
    int rightSpacing = SScreenAdaptingConstant(65, 130)();
    int bottomSpacing = SScreenAdaptingConstant(40, 80, 80)();
    skipContainer->SetOutsetSpacing(0, 0, rightSpacing, bottomSpacing);
    buttons->AddToFront(skipContainer);
    WindowTransparent* getContainer = new WindowTransparent(false);
    CImageButton* get = new CImageButton;
    get->AddImage("IDB_SHOP_CARD_BUTTON", 0, 0);
    get->SetText(XString("GET IT!"), 0x0c, NULL, 0, 0);
    get->SetLayoutType((ELayoutType)1);
    get->SetAlign(0x11);
    get->m_command = 0x02358586;
    get->m_commandParameter1 = 0;
    get->m_commandParameter2 = 0;
    get->m_commandParameter3 = 0;
    get->SetFlags(0x800000);
    getContainer->SetPercentWidth(50, 0, 0);
    getContainer->SetCellPos(1, 0, 1, 1);
    getContainer->AddToFront(get);
    int leftSpacing = SScreenAdaptingConstant(40, 80, 80)();
    rightSpacing = SScreenAdaptingConstant(65, 130)();
    getContainer->SetOutsetSpacing(0, leftSpacing, rightSpacing, 0);
    buttons->AddToFront(getContainer);
    buttons->SetCellPos(0, 2, 2, 1);
    body->AddToFront(buttons);
    AddToFront(body);
}

CFeaturedAppWindow::~CFeaturedAppWindow()
{
    ((App*)WindowApp::m_instance)->m_surfaceManager->ReleaseSurface("IDB_DIALOG_WINDOW_BG");
    ((App*)WindowApp::m_instance)->m_surfaceManager->ReleaseSurface("IDB_CK_LOGO");
    ((App*)WindowApp::m_instance)->m_surfaceManager->ReleaseSurface("IDB_SHOP_CARD_BUTTON");
    delete m_downloadedSurface;
}

void CFeaturedAppWindow::SetDescriptionText(const XString& text)
{
    TextWindow* description = m_description;
    description->SetText(text, CFontMgr::GetInstance()->GetFont(15));
}

void CFeaturedAppWindow::SetImageUrl(const XString& url)
{
    WebUtil* web = new WebUtil;
    m_web = web;
    web->httpGet(XString(url.m_data));
}

void CFeaturedAppWindow::Update()
{
    if (m_web == NULL) return;
    m_web->update();
    if (m_web->m_state != 0 || m_downloadedSurface != NULL) return;
    const unsigned char* data = m_web->m_response;
    if (data == NULL) return;
    int length = m_web->m_responseLength;
    if (length == 0) return;
    CArrayInputStream stream;
    stream.Open(data, length);
    m_downloadedSurface = CreateSurfaceFromPNGStream(stream);
    stream.Close();
    if (m_downloadedSurface != NULL) {
        unsigned int width;
        unsigned int height;
        m_downloadedSurface->GetWidthAndHeight(width, height);
        ICRenderSurface* surface = m_downloadedSurface;
        float scale = 1.0f;
        if (App::IsHD()) scale = 2.0f;
        m_imageWindow->m_scale = scale;
        m_imageWindow->SetImage(surface);
    }
}

ICRenderSurface* CFeaturedAppWindow::CreateSurfaceFromPNGStream(const CArrayInputStream& stream)
{
    ICGraphics::GetInstance()->GetAbstraction();
    ICRenderSurface* surface = ICRenderSurface::CreateInstance(ICRenderSurface::Type_Default, ICRenderSurface::Targetability_0);
    ICRenderSurface::InitParam params[] = { {6, 0}, {15, 0xb7178678u}, {14, (unsigned int)&stream}, {0, 0} };
    if (surface->Initialize(params) != 0 || surface == NULL) return surface;
    delete surface;
    return NULL;
}

void CFeaturedAppWindow::OnCommand(Event& event)
{
    if ((unsigned int)event.m_id == 0x02358586u) {
        Close();
        CApplet::m_pCore->LaunchURL(m_redirectUrl.m_data, false);
        event.Clear();
    } else if ((unsigned int)event.m_id == 0x97204784u || (unsigned int)event.m_id == 0x00292395u) {
        Close();
        event.Clear();
    }
}

void CFeaturedAppWindow::SetRedirectUrl(XString url) { m_redirectUrl = url; }

void ScaledURLImageWindow::Paint(ICGraphics2d& graphics)
{
    int align = m_align;
    int x = 0;
    int y = 0;
    if (!(align & 1)) {
        if (align & 2) x = m_width;
        else { x = m_width / 2; align |= 4; }
    }
    if (!(align & 8)) {
        if (align & 16) y = m_height;
        else { y = m_height / 2; align |= 32; }
    }
    if (m_surface != NULL) DrawScaledImage(graphics, m_surface, x, y, align, 0, m_scale, m_scale);
}

void ScaledURLImageWindow::DrawScaledImage(ICGraphics2d& graphics, ICRenderSurface* surface,
    int x, int y, int align, int transform, float scaleX, float scaleY)
{
    if (surface == NULL) return;
    if (!(align & 0x40) && align != 0 && align != 9) {
        unsigned int width;
        unsigned int height;
        surface->GetWidthAndHeight(width, height);
        width = (unsigned int)((float)width * scaleX);
        height = (unsigned int)((float)height * scaleY);
        if (align & 4) x -= (int)width / 2;
        else if ((align & 2) && !(align & 1) && (align & 3) != 3) x -= (int)width;
        if (align & 32) y -= (int)height / 2;
        else if ((align & 16) && !(align & 8) && (align & 24) != 24) y -= (int)height;
    }
    graphics.PushTransform();
    ICGraphics::GetInstance()->GetRasterizerState()->SetScissor(0, 0, 10000, 10000);
    ICGraphics2d::Flip flip = ICGraphics2d::Flip_None;
    switch (transform) {
    case 1: flip = ICGraphics2d::Flip_Vertical; break;
    case 2: flip = ICGraphics2d::Flip_Horizontal; break;
    case 3: flip = ICGraphics2d::Flip_Both; break;
    case 4: graphics.Rotate(270.0f); flip = ICGraphics2d::Flip_Horizontal; break;
    case 5: graphics.Rotate(90.0f); break;
    case 6: graphics.Rotate(270.0f); break;
    case 7: graphics.Rotate(90.0f); flip = ICGraphics2d::Flip_Horizontal; break;
    }
    graphics.Scale(scaleX, scaleY);
    int translatedX = x + (int)(fabsf((float)x) / scaleX);
    int translatedY = y + (int)(fabsf((float)y) / scaleY);
    graphics.Translate((float)translatedX, (float)translatedY);
    graphics.Draw(*surface, NULL, &WindowApp::m_instance->Color(), flip);
    graphics.PopTransform();
}
