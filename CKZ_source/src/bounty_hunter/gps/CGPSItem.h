// b082: complete base ABI from 0x146be4 and vtable 0x4de920; size 0xd0.
#ifndef CGPSITEM_H
#define CGPSITEM_H
#include "WindowBase.h"
#include "SG_Presenter.h"
class TiXmlElement;
class CGPSItem {
public:
    CGPSItem();
    virtual ~CGPSItem();
    virtual void SetBlinking(bool blinking) { m_blinking = blinking; }
    virtual void SetBlinkingCmd(unsigned int command) { m_blinkCommand = command; }
    virtual unsigned int GetBlinkingCmd() const { return m_blinkCommand; }
    virtual void SetPressed(bool pressed);
    virtual void Paint(com::glu::platform::graphics::ICGraphics2d& graphics, int x, int y);
    virtual void Update(long elapsed);
    virtual void Save(TiXmlElement*) {} // Original weak return-only method 1473f4.
    virtual void Load(TiXmlElement*) {} // Original weak return-only method 1473f8.
    virtual void OnClick() = 0;
    virtual com::glu::platform::components::CRectangle<short> GetRect();
    virtual bool CheckHit(int x, int y, CGPSItem** hit);
    virtual void SetPos(float x, float y);
    virtual void SetSize(int width, int height);
    virtual float GetX();
    virtual float GetY();
    virtual int GetWidth() { return m_width; }
    virtual int GetHeight() { return m_height; }
    virtual void SetImage(const char* name);
    virtual void SetSGImage(int animation, int secondAnimation);
    virtual void SetBlinkingImage(const char* name);
    virtual void SetPressImage(const char* name);
    virtual com::glu::platform::graphics::ICRenderSurface* GetImage() { return m_image; }
    virtual float AttractSpeed() const { return m_attractSpeed; }
    virtual void SetAttractSpeed(float speed) { m_attractSpeed = speed; }
    virtual void UpdateState() {} // Original weak return-only method 146bd0.
    virtual void SetVisible(bool visible) { m_visible = visible; }
    virtual void OnCommand(Event& event);
    virtual int GetScreenX() { return m_screenX; }
    virtual int GetScreenY() { return m_screenY; }
    void SetScreenName(const XString& name);
    void UpdateRects();

    float m_x;                         // 04
    float m_y;                         // 08
    int m_width;                       // 0c
    int m_height;                      // 10
    int m_screenX;                     // 14
    int m_screenY;                     // 18
    float m_attractSpeed;              // 1c
    bool m_blinking;                   // 20
    bool m_pressed;                    // 21
    bool m_onScreen;                   // 22
    unsigned int m_pressedTime;        // 24
    bool m_visible;                    // 28
    com::glu::platform::graphics::CFont* m_font; // 2c
    int m_nameWidth;                   // 30
    XString m_screenName;              // 34
    float m_parallax;                  // 38
    SG_Presenter m_presenter;          // 3c
    SG_Presenter m_secondaryPresenter; // 78
    unsigned int m_blinkCommand;       // b4, original ctor leaves uninitialized
    com::glu::platform::graphics::ICRenderSurface* m_image;      // b8
    com::glu::platform::graphics::ICRenderSurface* m_blinkImage; // bc
    com::glu::platform::graphics::ICRenderSurface* m_pressImage; // c0
    float m_scaleX;                    // c4
    float m_scaleY;                    // c8
    float m_opacity;                   // cc
};
#endif
