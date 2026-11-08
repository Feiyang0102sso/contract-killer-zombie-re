// b074i: external factory with the complete mangled symbol at original 15b48c; the body belongs to ServiceWindows.cpp and is not counted complete by declaration.
#ifndef SERVICEWINDOWS_H
#define SERVICEWINDOWS_H
#include "WindowBase.h"
#include "CBH_Button.h"
#include "tws.h"
class Window;
class TextWindow;
void SetSizesByContent(Window* window, int flags);
Window* createTD(Window* window, int index); // original 15b45c
Window* createTD(Window* window, int index, int parameter, int flags);

// Below are the real external dependencies of Popup; the complete body still belongs to ServiceWindows.cpp and is not counted complete by declaration.
class AnimatedWindow : public WindowTransparent {
public:
    enum EMode { MODE_0 = 0 };
    struct SEasyParams {
        int m_x;                  // 00
        int m_y;                  // 04
        float m_firstOpacity;     // 08
        float m_lastOpacity;      // 0c
        float m_firstScale;       // 10
        float m_lastScale;        // 14
        float m_time;             // 18
        float m_delay;            // 1c
        bool m_flag20;            // 20
        EasyLib::EMethod m_method; // 24
        bool m_enabled;           // 28, total 2c
    };
    explicit AnimatedWindow(EMode mode = MODE_0);
    virtual ~AnimatedWindow() {}
    virtual void OnLayoutEasy(int x, int y, int width, int height);
    virtual void OnStartupEasy();
    virtual void OnEasyFinished();
    virtual void Close(); // new in original vtable: bc; Window::Close itself is not virtual.
    void StartCustomEasy(SEasyParams& parameters);
    void SetStartEasy(int x, int y, float time, float firstOpacity, float lastOpacity,
                      float delay, EasyLib::EMethod method);
    void SetCloseEasy(int x, int y, float time, float firstOpacity, float lastOpacity,
                      float delay, EasyLib::EMethod method);
    void SetStartEasyScale(float first, float last);
    void SetCloseEasyScale(float first, float last);
    SEasyParams m_start;           // ac
    SEasyParams m_close;           // d8
    EMode m_mode;                 // 104
    // Historical bool m_closing; original 15bc94/15bcc2/15bd20 store the 0/1/2 state as a full word.
    int m_closing;                // 108, total 10c
};

class ViewWindow : public WindowTransparent {
public:
    ViewWindow(const swerve::World<CSwerveAssertingErrorHandler>& world);
    virtual ~ViewWindow();
    virtual void Paint3D(swerve::Graphics3D<CSwerveAssertingErrorHandler>& graphics);
    swerve::World<CSwerveAssertingErrorHandler> m_world; // ac, total b0
};

class SeparateLineRed : public Window {
public:
    SeparateLineRed();
    virtual inline ~SeparateLineRed() {}
    virtual void Paint(com::glu::platform::graphics::ICGraphics2d& graphics);
    ImageRes m_center;  // ac
    ImageRes m_left;    // b4
    ImageRes m_right;   // bc, total c4
};

class StretchImageButton : public CBH_Button {
public:
    StretchImageButton(XString text, const char* left, const char* center, const char* right);
    virtual ~StretchImageButton();
    virtual void Paint(com::glu::platform::graphics::ICGraphics2d& graphics);
    virtual void SetRect(int x, int y, int width, int height, bool force);
    void ForceSetWidth(int width);
    com::glu::platform::graphics::ICRenderSurface* m_left;   // e4
    com::glu::platform::graphics::ICRenderSurface* m_right;  // e8
    com::glu::platform::graphics::ICRenderSurface* m_center; // ec
    XString m_text;                                        // f0
    com::glu::platform::graphics::CFont* m_font;             // f4
    int m_forcedWidth;                                     // f8, total fc
};

class BlinkingContainer : public WindowTransparent {
public:
    BlinkingContainer() : WindowTransparent(false), m_blinking(false) {}
    virtual ~BlinkingContainer() {}
    virtual void Update();
    bool m_blinking; // ac, total b0
};

class LabelWithIcon : public Window {
public:
    LabelWithIcon(const char* image, XString text, bool flag,
                  com::glu::platform::graphics::CFont* font, int spacing);
    virtual ~LabelWithIcon();
    void Create(const char* image, XString text, bool flag,
                com::glu::platform::graphics::CFont* font, int spacing);
    TextWindow* m_text;            // ac, owned by the Window tree
    const char* m_imageName;       // b0, the original Create stores the borrowed name / destructor calls ReleaseSurface
};
#endif
