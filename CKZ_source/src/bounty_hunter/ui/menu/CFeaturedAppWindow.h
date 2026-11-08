// PROVISIONAL (created by b079): owner src/bounty_hunter/ui/menu/CFeaturedAppWindow.cpp not written yet
// Allocation size and setter signatures from CTjManager::Update @0x13bbdc..0x13bc5a.
#ifndef CFEATUREDAPPWINDOW_H
#define CFEATUREDAPPWINDOW_H

#include "WindowBase.h"
#include "xString.h"
#include "BaseDialog.h"
#include "CArrayInputStream.h"
class WebUtil;
class ScaledURLImageWindow : public ImageWindow {
public:
    virtual ~ScaledURLImageWindow() {}
    virtual void Paint(com::glu::platform::graphics::ICGraphics2d& graphics);
    void DrawScaledImage(com::glu::platform::graphics::ICGraphics2d& graphics,
        com::glu::platform::graphics::ICRenderSurface* surface, int x, int y,
        int align, int transform, float scaleX, float scaleY);
    float m_scale;
};

class CFeaturedAppWindow : public Window {
public:
    CFeaturedAppWindow();
    virtual ~CFeaturedAppWindow();
    virtual void Update();
    virtual void OnCommand(Event& event);
    void SetDescriptionText(const XString& text);
    void SetImageUrl(const XString& url);
    void SetRedirectUrl(XString url);

    // Historical thin header: unsigned char m_unresolved[0xc0 - 0xac];
    com::glu::platform::graphics::ICRenderSurface* CreateSurfaceFromPNGStream(
        const com::glu::platform::components::CArrayInputStream& stream);
    TextWindow* m_description;                  // ac
    ScaledURLImageWindow* m_imageWindow;         // b0
    WebUtil* m_web;                             // b4, not initialized by the original ctor and not freed by the destructor.
    com::glu::platform::graphics::ICRenderSurface* m_downloadedSurface; // b8
    XString m_redirectUrl;                      // bc
};

#endif
